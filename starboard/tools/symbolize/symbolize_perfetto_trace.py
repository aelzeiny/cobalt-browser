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

"""Offline symbolizer for Perfetto heap profiles targeting Cobalt.

This tool parses a Perfetto trace file and uses addr2line from the toolchain
to resolve address offsets inside Cobalt's anonymous mappings and loader library.
"""

import argparse
import os
import subprocess
import sys

# Find repository root relative to this script (starboard/tools/symbolize/symbolize_perfetto_trace.py)
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))

def read_varint(data, offset):
    res = 0
    shift = 0
    while True:
        b = data[offset]
        res |= (b & 0x7f) << shift
        offset += 1
        if not (b & 0x80):
            break
        shift += 7
    return res, offset

def parse_packet(data, start, end):
    offset = start
    fields = []
    while offset < end:
        tag, offset = read_varint(data, offset)
        field_num = tag >> 3
        wire_type = tag & 0x7
        if wire_type == 0: # Varint
            val, offset = read_varint(data, offset)
            fields.append((field_num, "varint", val))
        elif wire_type == 2: # Length-delimited
            length, offset = read_varint(data, offset)
            val = data[offset:offset+length]
            offset += length
            fields.append((field_num, "bytes", val))
        elif wire_type == 1: # 64-bit
            offset += 8
            fields.append((field_num, "64bit", None))
        elif wire_type == 5: # 32-bit
            offset += 4
            fields.append((field_num, "32bit", None))
        else:
            print(f"Unknown wire type {wire_type} at offset {offset}")
            break
    return fields

def resolve_mapping_path(path_ids, global_mapping_paths):
    parts = []
    for pid in path_ids:
        if pid in global_mapping_paths:
            parts.append(global_mapping_paths[pid])
        else:
            parts.append(f"<missing str {pid}>")
    return "/".join(parts)

def parse_interned_data(data, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings, global_build_ids):
    fields = parse_packet(data, 0, len(data))
    for fnum, ftype, fval in fields:
        if fnum == 5: # function_names
            str_fields = parse_packet(fval, 0, len(fval))
            iid = None
            string_val = None
            for sfnum, sftype, sfval in str_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2: string_val = sfval.decode('utf-8', errors='replace')
            if iid is not None: global_function_names[iid] = string_val
        elif fnum == 17: # mapping_paths
            str_fields = parse_packet(fval, 0, len(fval))
            iid = None
            string_val = None
            for sfnum, sftype, sfval in str_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2: string_val = sfval.decode('utf-8', errors='replace')
            if iid is not None: global_mapping_paths[iid] = string_val
        elif fnum == 16: # build_ids
            str_fields = parse_packet(fval, 0, len(fval))
            iid = None
            string_val = None
            for sfnum, sftype, sfval in str_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2:
                     try:
                          string_val = sfval.decode('utf-8')
                     except UnicodeDecodeError:
                          string_val = sfval.hex()
            if iid is not None: global_build_ids[iid] = string_val
        elif fnum == 6: # frames
            frame_fields = parse_packet(fval, 0, len(fval))
            iid = None
            function_name_id = None
            mapping_id = None
            rel_pc = None
            for sfnum, sftype, sfval in frame_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2: function_name_id = sfval
                elif sfnum == 3: mapping_id = sfval
                elif sfnum == 4: rel_pc = sfval
            if iid is not None:
                global_frames[iid] = {
                    "name_id": function_name_id,
                    "mapping_id": mapping_id,
                    "rel_pc": rel_pc
                }
        elif fnum == 7: # callstacks
            callstack_fields = parse_packet(fval, 0, len(fval))
            iid = None
            frame_ids = []
            for sfnum, sftype, sfval in callstack_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2: frame_ids.append(sfval)
            if iid is not None: global_callstacks[iid] = frame_ids
        elif fnum == 19: # mappings
            map_fields = parse_packet(fval, 0, len(fval))
            iid = None
            build_id_id = None
            start = None
            end = None
            exact_offset = None
            start_offset = None
            for sfnum, sftype, sfval in map_fields:
                if sfnum == 1: iid = sfval
                elif sfnum == 2: build_id_id = sfval
                elif sfnum == 3: start_offset = sfval
                elif sfnum == 4: start = sfval
                elif sfnum == 5: end = sfval
                elif sfnum == 8: exact_offset = sfval
            path_string_ids = [sfval for sfnum, sftype, sfval in map_fields if sfnum == 7]
            if iid is not None:
                global_mappings[iid] = {
                    "path_ids": path_string_ids,
                    "build_id_id": build_id_id,
                    "start": start,
                    "end": end,
                    "exact_offset": exact_offset,
                    "start_offset": start_offset
                }

def run_addr2line(addr2line_path, binary_path, addresses):
    if not os.path.exists(binary_path):
        print(f"Warning: Binary not found for symbolization: {binary_path}")
        return {}
    
    cmd = [addr2line_path, "-C", "-f", "-e", binary_path] + [f"0x{addr:x}" for addr in addresses]
    try:
        res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        lines = res.stdout.splitlines()
        
        resolved = {}
        for idx, addr in enumerate(addresses):
            func = lines[idx*2]
            file_line = lines[idx*2 + 1]
            if func != "??" and file_line != "??:0":
                resolved[addr] = f"{func} ({file_line})"
            else:
                resolved[addr] = None
        return resolved
    except Exception as e:
        print(f"Error running addr2line: {e}")
        return {}

def symbolize_frames(addr2line_path, cobalt_bin, loader_bin, global_frames, global_mappings, global_mapping_paths, global_function_names):
    # Group relative PCs by binary
    cobalt_pcs = set()
    loader_pcs = set()
    
    frame_to_bin = {} # frame_id -> (bin_type, rel_pc)
    
    for fid, frame in global_frames.items():
        mapping_id = frame["mapping_id"]
        rel_pc = frame["rel_pc"]
        if rel_pc is None: continue
        
        if mapping_id in global_mappings:
            mval = global_mappings[mapping_id]
            path_str = resolve_mapping_path(mval["path_ids"], global_mapping_paths)
            start = mval["start"]
            end = mval["end"]
            
            # Identify Cobalt (anonymous, >30MB)
            is_cobalt = False
            if path_str == "" and start is not None and end is not None:
                if (end - start) > 30 * 1024 * 1024:
                    is_cobalt = True
            
            if is_cobalt:
                cobalt_pcs.add(rel_pc)
                frame_to_bin[fid] = ("cobalt", rel_pc)
            elif "libloader_app.so" in path_str:
                loader_pcs.add(rel_pc)
                frame_to_bin[fid] = ("loader", rel_pc)
                
    # Run addr2line
    resolved_cobalt = run_addr2line(addr2line_path, cobalt_bin, list(cobalt_pcs)) if cobalt_pcs and cobalt_bin else {}
    resolved_loader = run_addr2line(addr2line_path, loader_bin, list(loader_pcs)) if loader_pcs and loader_bin else {}
    
    # Update global_function_names with resolved symbols
    resolved_count = 0
    for fid, (bin_type, rel_pc) in frame_to_bin.items():
        resolved_str = None
        if bin_type == "cobalt" and rel_pc in resolved_cobalt:
            resolved_str = resolved_cobalt[rel_pc]
        elif bin_type == "loader" and rel_pc in resolved_loader:
            resolved_str = resolved_loader[rel_pc]
            
        if resolved_str:
            pseudo_id = f"resolved_{fid}"
            global_function_names[pseudo_id] = resolved_str
            global_frames[fid]["name_id"] = pseudo_id
            resolved_count += 1
            
    print(f"Symbolized {resolved_count} frames offline ({len(resolved_cobalt)} Cobalt, {len(resolved_loader)} Loader).")

def parse_profile_packet(data, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings):
    fields = parse_packet(data, 0, len(data))
    for fnum, ftype, fval in fields:
        if fnum == 5: # process_dumps
            parse_process_heap_samples(fval, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings)

def parse_process_heap_samples(data, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings):
    fields = parse_packet(data, 0, len(data))
    samples = []
    pid = None
    heap_name = None
    unwinding_errors = None
    
    for fnum, ftype, fval in fields:
        if fnum == 1: pid = fval
        elif fnum == 11: heap_name = fval.decode('utf-8', errors='replace')
        elif fnum == 2: samples.append(fval)
        elif fnum == 5:
            stats_fields = parse_packet(fval, 0, len(fval))
            for sfnum, sftype, sfval in stats_fields:
                if sfnum == 1: unwinding_errors = sfval
    
    print(f"  PID: {pid} | Heap: {heap_name} | Samples: {len(samples)} | Unwind Errors: {unwinding_errors}")
    
    # Sort samples by self_allocated to show largest allocations first
    parsed_samples = []
    for sample_data in samples:
        sample_fields = parse_packet(sample_data, 0, len(sample_data))
        callstack_id = None
        self_allocated = 0
        self_freed = 0
        alloc_count = 0
        free_count = 0
        for sfnum, sftype, sfval in sample_fields:
            if sfnum == 1: callstack_id = sfval
            elif sfnum == 2: self_allocated = sfval
            elif sfnum == 3: self_freed = sfval
            elif sfnum == 5: alloc_count = sfval
            elif sfnum == 6: free_count = sfval
        
        net_allocated = self_allocated - self_freed
        if net_allocated > 0:
            parsed_samples.append((net_allocated, callstack_id, self_allocated, self_freed, alloc_count, free_count))
            
    parsed_samples.sort(key=lambda x: x[0], reverse=True)
    
    for idx, (net, callstack_id, self_allocated, self_freed, alloc_count, free_count) in enumerate(parsed_samples[:100]): # Top 100
        callstack_str = ""
        if callstack_id in global_callstacks:
             frame_ids = global_callstacks[callstack_id]
             frame_strs = []
             for fid in frame_ids:
                  if fid in global_frames:
                       frame_info = global_frames[fid]
                       fn_id = frame_info["name_id"]
                       mapping_id = frame_info["mapping_id"]
                       rel_pc = frame_info["rel_pc"]
                       
                       fn_str = ""
                       if fn_id is not None and fn_id in global_function_names:
                            fn_str = global_function_names[fn_id]
                            
                       lib_str = ""
                       if mapping_id is not None and mapping_id in global_mappings:
                            lib_str = resolve_mapping_path(global_mappings[mapping_id]["path_ids"], global_mapping_paths)
                            if lib_str == "" and global_mappings[mapping_id]["start"] is not None:
                                # Show anonymous mapping details
                                lib_str = f"anon_0x{global_mappings[mapping_id]['start']:x}"
                            
                       pc_str = f"0x{rel_pc:x}" if rel_pc is not None else ""
                        
                       frame_str = ""
                       if fn_str:
                            frame_str = fn_str
                       elif lib_str:
                            frame_str = f"[{lib_str}+{pc_str}]"
                       else:
                            frame_str = f"[{pc_str}]"
                       frame_strs.append(frame_str)
                  else:
                       frame_strs.append(f"<unknown frame {fid}>")
             callstack_str = " -> ".join(frame_strs)
        else:
             callstack_str = f"<unknown callstack {callstack_id}>"
             
        print(f"    Allocation {idx}: Net {net/1024/1024:.2f} MB | {callstack_str}")

def main():
    parser = argparse.ArgumentParser(description="Offline symbolize Perfetto heap profiles for Cobalt.")
    parser.add_argument("trace_file", help="Path to the Perfetto trace file.")
    parser.add_argument("--addr2line", help="Path to the toolchain addr2line binary. Falls back to $RDK_HOME/sysroots/.../arm-rdk-linux-gnueabi-addr2line.")
    parser.add_argument("--cobalt-bin", help="Path to the unstripped libcobalt.so. Defaults to out/evergreen-arm-hardfp-rdk_qa/lib.unstripped/libcobalt.so.")
    parser.add_argument("--loader-bin", help="Path to the unstripped libloader_app.so. Defaults to out/evergreen-arm-hardfp-rdk_qa/starboard/lib.unstripped/libloader_app.so.")
    
    args = parser.parse_args()

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
            # Fallback to PATH search
            import shutil
            addr2line_path = shutil.which("arm-rdk-linux-gnueabi-addr2line") or shutil.which("arm-linux-gnueabi-addr2line") or "addr2line"

    # Resolve binaries
    cobalt_bin = args.cobalt_bin
    if not cobalt_bin:
        cobalt_bin = os.path.join(REPO_ROOT, "out", "evergreen-arm-hardfp-rdk_qa", "lib.unstripped", "libcobalt.so")

    loader_bin = args.loader_bin
    if not loader_bin:
        loader_bin = os.path.join(REPO_ROOT, "out", "evergreen-arm-hardfp-rdk_qa", "starboard", "lib.unstripped", "libloader_app.so")

    print(f"Using addr2line: {addr2line_path}")
    print(f"Using libcobalt: {cobalt_bin}")
    print(f"Using libloader_app: {loader_bin}")

    if not os.path.exists(args.trace_file):
        print(f"Error: Trace file not found: {args.trace_file}")
        return

    with open(args.trace_file, "rb") as f:
        data = f.read()

    global_function_names = {}
    global_mapping_paths = {}
    global_frames = {}
    global_callstacks = {}
    global_mappings = {}
    global_build_ids = {}
    
    # Pass 1: Parse interned data
    offset = 0
    while offset < len(data):
        tag, offset = read_varint(data, offset)
        field_num = tag >> 3
        wire_type = tag & 0x7
        if field_num != 1 or wire_type != 2:
            break
        length, offset = read_varint(data, offset)
        packet_data = data[offset:offset+length]
        offset += length
        
        fields = parse_packet(packet_data, 0, len(packet_data))
        for fnum, ftype, fval in fields:
            if fnum == 12: # InternedData
                parse_interned_data(fval, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings, global_build_ids)

    print(f"Parsed trace: {len(global_frames)} frames, {len(global_callstacks)} callstacks.")
    
    # Symbolize
    symbolize_frames(addr2line_path, cobalt_bin, loader_bin, global_frames, global_mappings, global_mapping_paths, global_function_names)
    
    # Pass 2: Parse and print profile packets
    offset = 0
    packet_idx = 0
    while offset < len(data):
        tag, offset = read_varint(data, offset)
        field_num = tag >> 3
        wire_type = tag & 0x7
        if field_num != 1 or wire_type != 2:
            break
        
        length, offset = read_varint(data, offset)
        packet_data = data[offset:offset+length]
        offset += length
        
        fields = parse_packet(packet_data, 0, len(packet_data))
        for fnum, ftype, fval in fields:
            if fnum == 37: # ProfilePacket
                parse_profile_packet(fval, global_function_names, global_mapping_paths, global_frames, global_callstacks, global_mappings)
        packet_idx += 1

if __name__ == "__main__":
    main()
