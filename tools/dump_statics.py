#!/usr/bin/env python3
"""emu_init.py'nin urettigi bellek goruntusunden GUI'nin statik tablolarini C++ olarak yazar.
Kullanim: dump_statics.py <libapp.so> <image.bin> <GenericGUIManager.c> <cikti_dizini>"""
import re, subprocess, struct, sys, os
so, imgp, guic, out = sys.argv[1:5]
img = open(imgp, 'rb').read()
text = open(guic, encoding='utf-8', errors='ignore').read()
syms = []
for l in subprocess.run(['readelf', '-sW', so], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) >= 8 and p[3] == 'OBJECT' and p[1] != '00000000': syms.append((p[7], int(p[1], 16), int(p[2])))
dem = subprocess.run(['c++filt'], input='\n'.join(s[0] for s in syms), capture_output=True, text=True).stdout.split('\n')
rows = []
for (n, a, sz), dn in zip(syms, dem):
    if dn.startswith('GenericGUIManager::') and 'fontData' not in dn:
        rows.append((dn.split('::')[1], a, sz))
dats = sorted(set(re.findall(r'&DAT_(001[0-9a-f]{5})', text)))
def words(a, n): return struct.unpack('<%dI' % (n // 4), img[a:a + n])
def arr(name, ws):
    return 'const unsigned %s[%d] = {%s};\n' % (name, len(ws), ','.join('0x%08x' % w for w in ws))
with open(os.path.join(out, 'GuiStatics.cpp'), 'w') as f:
    f.write('// OTOMATIK URETILDI: tools/emu_init.py + tools/dump_statics.py (orijinal statik baslaticilarin sonucu)\n#include "data/GuiStatics.h"\nnamespace fs12 {\n')
    for name, a, sz in rows: f.write(arr('g_' + name, words(a, sz)))
    for d in dats:
        a = int(d, 16) - 0x10000
        f.write(arr('g_btn_' + d, words(a, 104)))
    f.write('}\n')
with open(os.path.join(out, 'GuiStatics.h'), 'w') as f:
    f.write('// OTOMATIK URETILDI. Ham 32-bit kelimeler (float/int); yorumlayan kod src/game/Gui*.cpp\n#pragma once\nnamespace fs12 {\n')
    for name, a, sz in rows: f.write('extern const unsigned g_%s[%d];\n' % (name, sz // 4))
    for d in dats: f.write('extern const unsigned g_btn_%s[26];\n' % d)
    f.write('}\n')
print('semboller', len(rows), 'DAT butonlari', len(dats))
