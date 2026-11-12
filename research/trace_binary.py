import pefile, capstone, struct, json, hashlib
from pathlib import Path
p=Path('inputs/FreeCK2game.exe'); raw=p.read_bytes(); pe=pefile.PE(data=raw, fast_load=True); base=pe.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64); cs.detail=True; cs.skipdata=True
terms=[b'bloodline.cpp',b'government.cpp',b'governmentflavor.cpp',b'common/bloodlines',b'common/governments',b'patrilineal',b'matrilineal',b'inheritance',b'government_icon',b'bloodlines_window',b'set_government',b'validate_government']
strings={}
import re
for m in re.finditer(rb'[\x20-\x7e]{4,}\x00',raw):
 s=m.group()[:-1]
 if any(t.lower() in s.lower() for t in terms):
  try: strings[base+pe.get_rva_from_offset(m.start())]=s.decode()
  except: pass
pe.parse_data_directories(directories=[3])
bounds=[(base+e.struct.BeginAddress,base+e.struct.EndAddress) for e in pe.DIRECTORY_ENTRY_EXCEPTION]
import bisect
starts=[a for a,b in bounds]
refs=[]
text=next(s for s in pe.sections if s.Name.startswith(b'.text'))
def instructions():
 for a,b in bounds:
  if base+text.VirtualAddress <= a < base+text.VirtualAddress+text.Misc_VirtualSize:
   yield from cs.disasm(pe.get_data(a-base,b-a),a)
for ins in instructions():
 if ins.id == 0: continue
 for op in ins.operands:
  target=None
  if op.type==capstone.x86.X86_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP: target=ins.address+ins.size+op.mem.disp
  elif op.type==capstone.x86.X86_OP_IMM: target=op.imm
  if target in strings:
   i=bisect.bisect_right(starts,ins.address)-1
   bound=bounds[i] if i>=0 and ins.address<bounds[i][1] else None
   refs.append(dict(string=strings[target],string_va=hex(target),xref=hex(ins.address),instruction=f'{ins.mnemonic} {ins.op_str}',function=[hex(x) for x in bound] if bound else None))
out=Path('deliverables/research'); (out/'xrefs.json').write_text(json.dumps({'sha256':hashlib.sha256(raw).hexdigest(),'image_base':hex(base),'strings':{hex(k):v for k,v in strings.items()},'refs':refs},indent=2))
with (out/'disassembly.txt').open('w') as f:
 for a,b in sorted(set(tuple(int(x,16) for x in r['function']) for r in refs if r['function'])):
  f.write(f'\nFUNCTION {a:#x}..{b:#x}\n')
  data=pe.get_data(a-base,b-a)
  for ins in cs.disasm(data,a): f.write(f'{ins.address:#x}: {ins.mnemonic} {ins.op_str}\n')
print(len(strings),'strings;',len(refs),'references')
