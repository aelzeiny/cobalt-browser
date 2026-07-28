# Guide: Memory Profiling Cobalt with Perfetto on RDK

This guide details how to build, deploy, run, and symbolize memory profiles (`heapprofd`) for containerized Cobalt on RDK TV platforms.

---

## 1. Prerequisites

Ensure your environment variables are configured on the host:
```bash
export RDK_HOME=/usr/local/google/home/ahmedelzeiny/rdk/toolchain
export PATH=/usr/local/google/home/ahmedelzeiny/cobalt/tools/depot_tools:$PATH
```

We assume your build directory is:
`/usr/local/google/home/ahmedelzeiny/cobalt/cobalt-perfetto/src` (on branch `perfetto`).

---

## 2. Building Perfetto & Cobalt

We must build using the **QA** configuration (`evergreen-arm-hardfp-rdk_qa`). 

> [!IMPORTANT]
> Do **not** use the production `gold` configuration. The `gold` build strips Call Frame Information (CFI) and debugging symbols, which prevents the stack unwinder from traversing the call tree, leading to `INVALID_MAP` or `MEMORY_INVALID` root frames. The `qa` build preserves the necessary unwinding tables.

### A. Configure the Build
Generate the build files (if not already done):
```bash
cobalt/build/gn.py -c qa -p evergreen-arm-hardfp-rdk
```

### B. Build Cobalt, Daemons, and Preload Library
We must build the preload library using the **Clang toolchain** to bypass the RDK GCC toolchain's broken Thread-Local Storage (TLS) implementation.

Run the build commands:
```bash
# Build Cobalt and the main Perfetto daemons
autoninja -C out/evergreen-arm-hardfp-rdk_qa \
  cobalt \
  loader_app \
  native_target/traced \
  native_target/heapprofd

# Build the heapprofd preload library using Clang
autoninja -C out/evergreen-arm-hardfp-rdk_qa \
  third_party/perfetto:heapprofd_glibc_preload_clang
```

This will produce:
- `out/.../libcobalt.so` (and unstripped in `lib.unstripped/libcobalt.so`)
- `out/.../traced` (Producer/Consumer daemon)
- `out/.../heapprofd` (Profiling daemon, which we deploy as `heapprofd.real`)
- `out/.../clang_.../libheapprofd_glibc_preload.so` (Preload hook library, which we deploy as `libheapprofd.so`)

---

## 3. Deployment

We deploy the binaries to the TV. Since `/data/out_cobalt` is wiped during Cobalt deployments, ensure you deploy these files **after** deploying Cobalt.

Copy the files to the TV (replace `rdk1` with your TV SSH target):
```bash
# Deploy Cobalt (using standard deploy scripts)
# ...

# Deploy Perfetto Platform Services and Hook Library
scp out/evergreen-arm-hardfp-rdk_qa/libperfetto_platform_services.so rdk1:/data/
scp out/evergreen-arm-hardfp-rdk_qa/clang_x86_v7a/libheapprofd_glibc_preload.so rdk1:/data/out_cobalt/libheapprofd.so

# Deploy Daemons
scp out/evergreen-arm-hardfp-rdk_qa/traced rdk1:/data/
scp out/evergreen-arm-hardfp-rdk_qa/heapprofd rdk1:/data/heapprofd.real
```

---

## 4. Configuration

Create a Perfetto configuration file on the TV at `/tmp/heapprofd_cobalt.config`:
```protobuf
buffers: {
    size_kb: 65536
    fill_policy: DISCARD
}

data_sources: {
    config {
        name: "android.heapprofd"
        target_buffer: 0
        heapprofd_config {
            sampling_interval_bytes: 4096
            adaptive_sampling_shmem_threshold: 1048576
            adaptive_sampling_max_sampling_interval_bytes: 65536
            all_heaps: true
            process_cmdline: "cobalt"
            block_client: false
        }
    }
}

duration_ms: 60000
write_into_file: true
file_write_period_ms: 1000
```

---

## 5. Execution

### A. Start the Perfetto Daemon
Run the `traced` daemon on the TV. 

> [!NOTE]
> **Socket Path Workaround (`EOVERFLOW`)**: Because the Dobby container uses a 64-bit `tmpfs` filesystem for `/tmp`, 32-bit Perfetto binaries will fail with `EOVERFLOW` (Value too large for defined data type) when trying to bind to sockets in `/tmp`. We work around this by redirecting Perfetto producer/consumer sockets to `/data/out_cobalt/sockets/` (an `ext4` partition).

It must use the custom socket directory `/data/out_cobalt/sockets/` so the containerized process can reach it.

```bash
ssh rdk1 "mkdir -p /data/out_cobalt/sockets/ && \
          export LD_LIBRARY_PATH=/data && \
          export PERFETTO_PRODUCER_SOCK_NAME=/data/out_cobalt/sockets/producer && \
          export PERFETTO_CONSUMER_SOCK_NAME=/data/out_cobalt/sockets/consumer && \
          /data/traced >/tmp/traced.log 2>&1 &"
```

> [!IMPORTANT]
> The sockets created by `traced` must be readable by the Cobalt container. Fix permissions on the socket:
> ```bash
> ssh rdk1 "chown -R cobalt:dobbyapp /data/out_cobalt/sockets"
> ```

### B. Launch Cobalt with Preload
To profile malloc, we must preload `libheapprofd.so` into the Cobalt process.
On the TV, modify `/usr/bin/WPEProcess` (or the wrapper script that starts Cobalt) to inject `LD_PRELOAD`:

```bash
export LD_PRELOAD=/data/out_cobalt/libheapprofd.so
export LD_LIBRARY_PATH=/data:/data/out_cobalt
export PERFETTO_PRODUCER_SOCK_NAME=/data/out_cobalt/sockets/producer
```

Restart WPEFramework to apply changes:
```bash
ssh rdk1 "systemctl restart wpeframework"
```

> [!TIP]
> **Container Recovery**: If the container hangs or reports "already running", run the following to force reset the Dobby state:
> ```bash
> crun --root /run/rdk/crun delete --force Cobalt
> systemctl restart dobby
> systemctl restart wpeframework
> ```

---

## 6. Capturing the Trace

Run the `perfetto` client on the TV to start a trace session:
```bash
ssh rdk1 "export LD_LIBRARY_PATH=/data && \
          export PERFETTO_CONSUMER_SOCK_NAME=/data/out_cobalt/sockets/consumer && \
          /data/perfetto --txt -c /tmp/heapprofd_cobalt.config -o /tmp/trace_cobalt.perfetto"
```
Once the trace duration expires, pull the trace file to your host:
```bash
scp rdk1:/tmp/trace_cobalt.perfetto /tmp/trace_cobalt.perfetto
```

---

## 7. Symbolization

Because Cobalt loads `libcobalt.so` as an anonymous executable mapping, standard Perfetto UI cannot resolve its symbols. We must symbolize offline using the unstripped `libcobalt.so` on the host.

### A. Tools: `addr2line` vs `llvm-symbolizer`
> [!WARNING]
> Standard system `addr2line` on host machines may hang or leak memory when parsing large 32-bit ARM ELF binaries (like our 900MB unstripped `libcobalt.so`). If this happens, you should use `llvm-symbolizer` (found in Cobalt's prebuilt tools at `third_party/llvm-build/Release+Asserts/bin/llvm-symbolizer`) which is much faster. Pass it via `--addr2line`.

### B. Run Symbolizer Script
We have two scripts depending on your desired output:

1.  **Text Callstack Report**:
    Run `symbolize_perfetto_trace.py` (parses raw protobuf):
    ```bash
    python3 starboard/tools/symbolize/symbolize_perfetto_trace.py /tmp/trace_cobalt.perfetto > symbolized_profile.txt
    ```

2.  **Flamegraph Output**:
    Run `symbolize_perfetto_flame.py` (requires `trace_processor_shell` query engine):
    ```bash
    python3 starboard/tools/symbolize/symbolize_perfetto_flame.py /tmp/trace_cobalt.perfetto trace_qa.flame
    ```
    You can then convert `trace_qa.flame` to an SVG flamegraph using `flamegraph.pl`.

Both scripts automatically handle the **`-0x10000` database shift** (subtracting `0x10000` load bias from stored relative PCs to align them to the ELF virtual addresses).
