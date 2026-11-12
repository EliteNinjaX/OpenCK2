import pefile,capstone
from pathlib import Path
p=pefile.PE('inputs/FreeCK2game.exe',fast_load=True);base=p.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
regions=[(0x140181b40,0x140182090),(0x1401834a0,0x1401839fd),(0x1407aef70,0x1407b0f57),(0x1407b69d0,0x1407b6f35),(0x1405fae30,0x1405fb065)]
with open('deliverables/research/focused.txt','w') as f:
 for a,b in regions:
  f.write(f'\nFUNCTION {a:#x}..{b:#x}\n')
  for i in cs.disasm(p.get_data(a-base,b-a),a):
   note=''
   for op in i.operands:
    if op.type==capstone.x86.X86_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP:
     rva=i.address+i.size+op.mem.disp-base
     try:
      raw=p.get_data(rva,180).split(b'\0')[0]
      if len(raw)>3 and all(32<=c<127 for c in raw):note+=' ; '+raw.decode()
     except:pass
   f.write(f'{i.address:#x}: {i.mnemonic} {i.op_str}{note}\n')
