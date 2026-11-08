#!/usr/bin/env python3
"""Orijinal libapp.so'dan bir fonksiyonun Thumb/ARM assembly'sini yazdirir.
Kullanim: disasm.py <libapp.so> <demangled-ad-parcasi veya mangled ad> [--all]
Ornek:   disasm.py libapp.so 'RandomGenerator::getRandomMinMaxFloat'"""
import re, subprocess, sys

so, pat = sys.argv[1], sys.argv[2]
out = subprocess.run(['nm', '-D', '-S', '-C', '--defined-only', so], capture_output=True, text=True).stdout
mang = subprocess.run(['nm', '-D', '-S', '--defined-only', so], capture_output=True, text=True).stdout
mmap = {l.split()[0]: l.split()[3] for l in mang.splitlines() if len(l.split()) >= 4}
hits = []
for l in out.splitlines():
    p = l.split(None, 3)
    if len(p) == 4 and (pat in p[3] or pat == mmap.get(p[0], '')):
        hits.append((int(p[0], 16), int(p[1], 16), p[3]))
seen = set()
for addr, size, name in hits:
    if addr in seen: continue
    seen.add(addr)
    thumb = addr & 1
    a = addr & ~1
    triple = 'thumbv5te' if thumb else 'armv5te'
    print(f'; {name}  @0x{a:x} size={size} {"thumb" if thumb else "arm"}')
    r = subprocess.run(['llvm-objdump', '-d', '--no-show-raw-insn', f'--triple={triple}',
                        f'--start-address=0x{a:x}', f'--stop-address=0x{a+size:x}', so], capture_output=True, text=True)
    for line in r.stdout.splitlines():
        if re.match(r'\s+[0-9a-f]+:', line): print(line)
    print()
    if '--all' not in sys.argv: break
