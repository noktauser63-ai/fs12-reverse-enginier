#!/usr/bin/env python3
"""disasm.py ciktisini literal havuzu degerleri ve float yorumuyla zenginlestirir: dis2.py <libapp.so> <ad>"""
import re, subprocess, sys, struct
so, pat = sys.argv[1], sys.argv[2]
d = open(so, 'rb').read()
import collections
out = subprocess.run([sys.executable, __file__.replace('dis2.py', 'disasm.py'), so, pat], capture_output=True, text=True).stdout
lines = out.splitlines()
pend = {}
for l in lines:
    mm = re.match(r'\s+([0-9a-f]+):\s+add\s+(r\d+), pc', l)
    if mm and mm.group(2) in pend:
        a = int(mm.group(1), 16); v = pend[mm.group(2)]; tgt = (v + a + 4) & 0xffffffff
        if tgt < len(d):
            e = d.find(b'\0', tgt); t = d[tgt:e][:60]
            txt = t.decode('latin1') if t and all(32 <= c < 127 or c in (10,) for c in t) else '(veri @0x%x)' % tgt
            l = l + '   ; -> ' + repr(txt)
    m0 = re.match(r'\s+[0-9a-f]+:\s+ldr\s+(r\d+), \[pc.*@ 0x([0-9a-f]+)', l)
    if m0: pend[m0.group(1)] = struct.unpack_from('<I', d, int(m0.group(2), 16))[0]
    if False: pass
    m = re.search(r'@ 0x([0-9a-f]+) <', l) or re.search(r'@ 0x([0-9a-f]+)\s*$', l)
    if m and 'ldr' in l and '[pc' in l:
        a = int(m.group(1), 16); v, = struct.unpack_from('<I', d, a); f, = struct.unpack('<f', struct.pack('<I', v))
        l = l.split('@')[0].rstrip() + '   ; =0x%x (%.6g)' % (v, f)
    elif l.strip().startswith(('ldr', )) and False: pass
    print(re.sub(r'<_Z[^>]*>', lambda m: '<' + subprocess.run(['c++filt'], input=m.group(0)[1:-1].split('+')[0], capture_output=True, text=True).stdout.strip()[:60] + '>', l) if 'bl\t' in l else l)
