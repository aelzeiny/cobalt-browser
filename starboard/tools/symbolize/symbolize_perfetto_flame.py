#!/usr/bin/env python3
# Copyright 2026 The Cobalt Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Offline symbolizer for Perfetto heap profiles, outputting Flamegraph format.

This tool uses trace_processor_shell to extract allocation callstacks from a Perfetto
trace database, symbolizes the addresses using addr2line, and outputs a format
compatible with Flamegraph visualization tools (e.g. flamegraph.pl).
"""

import argparse
import csv
import os
import subprocess
import sys

# Find repository root relative to this script (starboard/tools/symbolize/symbolize_perfetto_flame.py)
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))

def run_query(tp_path, trace_file, query):
    res = subprocess.run([tp_path, trace_file, '-Q', query], capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Query failed: {res.stderr}", file=sys.stderr)
        sys.exit(1)
    reader = csv.reader(res.stdout.strip().split('\n'))
    try:
        header = next(reader)
    except StopIteration:
        return []
    return [dict(zip(header, row)) for row in reader]

def main():
    parser = argparse.ArgumentParser(description="Generate symbolized flamegraph lines from Perfetto heap profile.")
    parser.add_argument("trace_file", help="Path to the Perfetto trace file.")
    parser.add_argument("out_file", help="Path to the output flamegraph text file.")
    parser.add_argument("--addr2line", help="Path to the toolchain addr2line binary. Falls back to $RDK_HOME/sysroots/.../arm-rdk-linux-gnueabi-addr2line.")
    parser.add_argument("--cobalt-bin", help="Path to the unstripped libcobalt.so. Defaults to out/evergreen-arm-hardfp-rdk_qa/lib.unstripped/libcobalt.so.")
    parser.add_argument("--trace-processor", help="Path to trace_processor_shell. Defaults to ~/.local/share/perfetto/prebuilts/trace_processor_shell.")
    
    args = parser.parse_args()

    # Resolve trace_processor_shell
    tp_path = args.trace_processor
    if not tp_path:
        tp_path = os.path.expanduser("~/.local/share/perfetto/prebuilts/trace_processor_shell")
        if not os.path.exists(tp_path):
            import shutil
            tp_path = shutil.which("trace_processor_shell") or "trace_processor_shell"

    # Resolve addr2line
    addr2line_path = args.addr2line
    if not addr2line_path:
        rdk_home = os.environ.get("RDK_HOME")
        if rdk_home:
            addr2line_path = os.path.join(
                rdk_home,
                "sysroots/x86_64-rdksdk-linux/usr/bin/arm-rdk-linux-gnueabi/arm-rdk-linux-gnueabi-addr2line"
            )
        else:
            import shutil
            addr2line_path = shutil.which("arm-rdk-linux-gnueabi-addr2line") or shutil.which("arm-linux-gnueabi-addr2line") or "addr2line"

    # Resolve Cobalt binary
    cobalt_bin = args.cobalt_bin
    if not cobalt_bin:
        cobalt_bin = os.path.join(REPO_ROOT, "out", "evergreen-arm-hardfp-rdk_qa", "lib.unstripped", "libcobalt.so")

    print(f"Using trace_processor: {tp_path}")
    print(f"Using addr2line: {addr2line_path}")
    print(f"Using libcobalt: {cobalt_bin}")

    if not os.path.exists(args.trace_file):
        print(f"Error: Trace file not found: {args.trace_file}", file=sys.stderr)
        return

    print("Loading mappings...")
    mappings_raw = run_query(tp_path, args.trace_file, "select id, name, start, end, exact_offset from stack_profile_mapping")
    
    code_mapping_id = None
    for m in mappings_raw:
        # Code segment is libcobalt.so (or anonymous mapped to it) with non-zero offset
        if "libcobalt.so" in m['name'] and int(m['exact_offset']) > 0:
            code_mapping_id = int(m['id'])
            print(f"Found Cobalt Code Segment: Mapping {code_mapping_id} (offset: {hex(int(m['exact_offset']))})")
            break

    if code_mapping_id is None:
        # Fallback: look for anonymous mapping >30MB if the patcher didn't run
        for m in mappings_raw:
            if m['name'] == "":
                start = int(m['start']) if m['start'] else 0
                end = int(m['end']) if m['end'] else 0
                if (end - start) > 30 * 1024 * 1024:
                    code_mapping_id = int(m['id'])
                    print(f"Fallback: Found Cobalt Code Segment (Anonymous): Mapping {code_mapping_id} [{hex(start)}-{hex(end)}]")
                    break

    if code_mapping_id is None:
        print("Error: Could not find Cobalt code segment mapping in trace!", file=sys.stderr)
        sys.exit(1)

    print("Loading frames...")
    frames_raw = run_query(tp_path, args.trace_file, "select id, name, mapping, rel_pc from stack_profile_frame")
    frames = {}
    for r in frames_raw:
        frames[int(r['id'])] = {
            'name': r['name'],
            'mapping': int(r['mapping']),
            'rel_pc': int(r['rel_pc'])
        }

    print("Loading allocations (cumulative)...")
    allocs_raw = run_query(tp_path, args.trace_file, """
    with recursive callstack_path as (
      select
        a.size,
        a.count,
        c.id as callsite_id,
        c.parent_id,
        c.frame_id,
        cast(c.frame_id as text) as path
      from heap_profile_allocation a
      join stack_profile_callsite c on a.callsite_id = c.id
      where a.size > 0
      
      union all
      
      select
        p.size,
        p.count,
        c.id as callsite_id,
        c.parent_id,
        c.frame_id,
        cast(c.frame_id as text) || ';' || p.path
      from callstack_path p
      join stack_profile_callsite c on p.parent_id = c.id
    )
    select
      sum(size) as size,
      sum(count) as count,
      path
    from callstack_path
    where parent_id is null
    group by path
    """)

    # Find unique PCs to symbolize in the code segment
    pcs_to_symbolize = set()
    for f_id, f in frames.items():
        if f['mapping'] == code_mapping_id and not f['name']:
            pcs_to_symbolize.add(f['rel_pc'])

    # Symbolize PCs using a single addr2line process
    resolved_symbols = {}
    import shlex
    addr2line_args = shlex.split(addr2line_path) if addr2line_path else []
    import shutil
    addr2line_bin_exists = len(addr2line_args) > 0 and (os.path.exists(addr2line_args[0]) or shutil.which(addr2line_args[0]))
    
    if pcs_to_symbolize and os.path.exists(cobalt_bin) and addr2line_bin_exists:
        print(f"Symbolizing {len(pcs_to_symbolize)} unique PCs safely...")
        cmd = addr2line_args + ['-e', cobalt_bin, '-f', '-C']
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        
        for pc in pcs_to_symbolize:
            # SUBTRACT 0x10000 offset to align with ELF virtual address!
            elf_pc = pc - 0x10000
            proc.stdin.write(hex(elf_pc) + '\n')
            proc.stdin.flush()
            
            func = proc.stdout.readline().strip()
            file_line = proc.stdout.readline().strip()
            
            if func == '??':
                func = f"sub_{hex(elf_pc)}"
                
            resolved_symbols[pc] = func
            # print(f"Resolved rel_pc {hex(pc)} (ELF: {hex(elf_pc)}) -> {func} ({file_line})")
            
        proc.stdin.close()
        proc.wait()
    else:
        if not pcs_to_symbolize:
            print("No frames need offline symbolization (all resolved by traced?).")
        else:
            print("Warning: Skipping symbolization because libcobalt or addr2line is missing.")

    # Generate symbolized flamegraph lines
    print("Generating flamegraph file...")
    out_lines = []
    for row in allocs_raw:
        size = int(row['size'])
        path = row['path']
        
        frame_ids = [int(f) for f in path.split(';')]
        names = []
        for f_id in frame_ids:
            f = frames.get(f_id)
            if not f:
                names.append("UNKNOWN_FRAME")
                continue
            
            if f['name']:
                names.append(f['name'])
            elif f['mapping'] == code_mapping_id:
                pc = f['rel_pc']
                names.append(resolved_symbols.get(pc, f"sub_{hex(pc - 0x10000)}"))
            else:
                names.append(f"mapping_{f['mapping']}_sub_{hex(f['rel_pc'])}")
        
        flame_line = ";".join(names) + f" {size}"
        out_lines.append(flame_line)
        
    with open(args.out_file, 'w') as f_out:
        f_out.write("\n".join(out_lines) + "\n")
    print(f"Flamegraph file saved to {args.out_file}")

if __name__ == '__main__':
    main()
