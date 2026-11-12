#!/usr/bin/env python3
"""Conservative, dependency-free CK2 PE string/RIP-reference probe.

Usage: python3 research/pe_probe.py /path/to/CK2game.exe --output /tmp/ck2-pe-findings.json

Does not execute the EXE. A hit only locates potential disassembly leads:
RIP-relative LEA/MOV patterns, not all machine instructions or call graphs.
"""
import argparse
import bisect
import hashlib
import json
import re
import struct
from pathlib import Path


def inspect(data):
    pe_off = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe_off:pe_off+4] != b'PE\x00\x00':
        raise ValueError('not a PE executable')
    machine, num_sections = struct.unpack_from('<HH', data, pe_off+4)
    if machine != 0x8664:
        raise ValueError('expected a PE x86-64 binary')
    optional_size = struct.unpack_from('<H', data, pe_off+20)[0]
    oh = pe_off+24
    if struct.unpack_from('<H', data, oh)[0] != 0x20b:
        raise ValueError('expected PE32+ optional header')
    base = struct.unpack_from('<Q', data, oh+24)[0]
    entry = struct.unpack_from('<I', data, oh+16)[0]
    sections = []
    for i in range(num_sections):
        off = oh+optional_size+40*i
        name = data[off:off+8].rstrip(b'\x00').decode('ascii','replace')
        vsz, rva, raw_size, raw_offset = struct.unpack_from('<IIII', data, off+8)
        sections.append(dict(name=name,va=base+rva,virtual_size=vsz,raw_offset=raw_offset,raw_size=raw_size))
    text = next(s for s in sections if s['name'] == '.text')
    pdata = next(s for s in sections if s['name'] == '.pdata')
    boundaries=[]
    for i in range(pdata['raw_offset'], pdata['raw_offset']+pdata['raw_size']-11,12):
        lo,hi,uw=struct.unpack_from('<III',data,i)
        if 0<lo<hi and uw and hi < 0x2000000:
            boundaries.append((base+lo,base+hi))
    boundaries.sort()
    starts=[b[0] for b in boundaries]
    def in_function(va):
        i=bisect.bisect_right(starts,va)-1
        return [hex(v) for v in boundaries[i]] if i>=0 and va<boundaries[i][1] else None
    def find_ascii(pattern):
        idx=data.find(pattern)
        if idx < 0: return None
        for s in sections:
            if s['raw_offset'] <= idx < s['raw_offset']+s['raw_size']:
                return s['va'] + idx-s['raw_offset']
        return None
    identifiers = [
      b'common/bloodlines',b'common/governments',b'bloodlines_window',
      b'vassal_entry',b'icon_topbar_societies',b'mapmode_governments',
      b'CURRENT_WINTER',b'PROVINCE_HOTKEY_INFO',b'TECHNOLOGY',b'CK2game.pdb',
      b'D:\\jenkins\\workspace\\ck2\\ck2\\source\\bloodline.cpp',
      b'D:\\jenkins\\workspace\\ck2\\ck2\\source\\government.cpp',
      b'D:\\jenkins\\workspace\\ck2\\ck2\\source\\governmentflavor.cpp',
      b'D:\\jenkins\\workspace\\ck2\\ck2\\source\\ConsoleCmdManager.cpp',
      b'D:\\jenkins\\workspace\\ck2\\ck2\\source\\ConsoleCmdImpl.cpp',
    ]
    selected={p: find_ascii(p) for p in identifiers}
    results=[]
    txt=data[text['raw_offset']:text['raw_offset']+text['raw_size']]
    for needle,target in selected.items():
        if target is None:continue
        xrefs=[]
        # Only simple 7-byte RIP-relative LEA and MOV operand encoding.
        for i in range(len(txt)-7):
            if txt[i] in (0x48,0x4c) and txt[i+1] in (0x8d,0x8b) and txt[i+2]&0xc7 == 0x05:
                if text['va']+i+7+struct.unpack_from('<i',txt,i+3)[0] == target:
                    va=text['va']+i
                    xrefs.append(dict(instruction_va=hex(va),unwind_range=in_function(va)))
        results.append(dict(identifier=needle.decode('ascii'),va=hex(target),xrefs=xrefs))
    return dict(sha256=hashlib.sha256(data).hexdigest(),size=len(data),image_base=hex(base),entry_rva=hex(entry),sections=sections,valid_unwind_ranges=len(boundaries),limitations='Targeted RIP-relative LEA/MOV pattern search only. Not full disassembly.',findings=results)

if __name__ == '__main__':
    cli=argparse.ArgumentParser()
    cli.add_argument('exe',type=Path)
    cli.add_argument('--output',type=Path)
    args=cli.parse_args()
    report=inspect(args.exe.read_bytes())
    j=json.dumps(report,indent=2)
    if args.output:args.output.write_text(j+'\n',encoding='utf-8')
    else:print(j)
