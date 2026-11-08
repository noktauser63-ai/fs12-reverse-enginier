#!/usr/bin/env python3
"""libapp.so (orijinal, 32-bit ARM) icinden salt-veri tablolarini cikarir ve C++ kaynagi uretir.
Kullanim: extract_game_data.py <libapp.so> <cikti_dizini>
 - LocalizationStringManager::m_strings (312 dize x 10 dil)
 - GenericGUIManager::fontData03 (1171 glif, 32 bayt)
Veri-segment adresleri: dosya ofseti = vaddr - 0x1000 (RW segment), RX segment: ofset = vaddr.
"""
import struct, subprocess, sys, os

so, out = sys.argv[1], sys.argv[2]
d = open(so, 'rb').read()
syms = {}
for l in subprocess.run(['readelf', '-sW', so], capture_output=True, text=True).stdout.splitlines():
    p = l.split()
    if len(p) >= 8 and p[3] == 'OBJECT': syms[p[7]] = (int(p[1], 16), int(p[2]))

def off(v): return v if v < 0xe7d10 else v - 0x1000

def cstr(v):
    if v == 0: return ''
    o = off(v); return d[o:d.index(b'\0', o)].decode('utf-8')

def cesc(s):
    r = []
    for ch in s.encode('utf-8'):
        if ch in (34, 92): r.append('\\' + chr(ch))
        elif ch == 10: r.append('\\n')
        elif 32 <= ch < 127: r.append(chr(ch))
        else: r.append('\\%03o' % ch)
    return '"' + ''.join(r) + '"'

# --- yerellestirme ---
addr, size = syms['_ZN25LocalizationStringManager9m_stringsE']
n = size // 4
ptrs = struct.unpack('<%dI' % n, d[off(addr):off(addr) + size])
rows = [[cstr(ptrs[i * 10 + l]) for l in range(10)] for i in range(n // 10)]
with open(os.path.join(out, 'Strings.cpp'), 'w', encoding='utf-8') as f:
    f.write('// OTOMATIK URETILDI: tools/extract_game_data.py (orijinal libapp.so m_strings)\n')
    f.write('#include "data/Strings.h"\nnamespace fs12 {\nconst int kStringCount = %d;\n' % len(rows))
    f.write('const char* const kStrings[%d][10] = {\n' % len(rows))
    for r in rows: f.write('  {' + ','.join(cesc(s) for s in r) + '},\n')
    f.write('};\n}\n')

# --- font ---
faddr, fsize = syms['_ZN17GenericGUIManager10fontData03E']
cnt = 0x493
with open(os.path.join(out, 'FontData03.cpp'), 'w') as f:
    f.write('// OTOMATIK URETILDI: tools/extract_game_data.py (orijinal GenericGUIManager::fontData03)\n')
    f.write('#include "data/Font.h"\nnamespace fs12 {\nconst int kFontData03Count = %d;\n' % cnt)
    f.write('const FontInfo kFontData03[%d] = {\n' % cnt)
    for i in range(cnt):
        cp, x, y, w, h, xo, yo, adv = struct.unpack('<I7f', d[off(faddr) + 32 * i: off(faddr) + 32 * i + 32])
        fl = lambda v: (lambda t: t if ('.' in t or 'e' in t or 'n' in t) else t + '.0')('%.9g' % v) + 'f'
        f.write('  {%d,%s,%s,%s,%s,%s,%s,%s},\n' % (cp, fl(x), fl(y), fl(w), fl(h), fl(xo), fl(yo), fl(adv)))
    f.write('};\n}\n')
print('strings', len(rows), 'glyphs', cnt, 'font symbol size', fsize, 'expected', cnt * 32)
