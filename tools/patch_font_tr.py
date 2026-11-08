#!/usr/bin/env python3
"""Turkce harfler icin font03_P.p2d atlasina eksik glifleri ekler (Ç ğ Ğ ı İ ş Ş Ü) ve src/data/FontTr.cpp uretir.
Kullanim: patch_font_tr.py <font03_P.p2d girdi> <p2d cikti> [FontTr.cpp cikti]
Atlas L8 2048x2048; orijinal glifler y<1040 arasinda, yeni glifler y=1100 satirina yerlestirilir (sabit konumlar => FontTr.cpp deterministik).
Glifler mevcut glif parcalarindan (ç cedilla, ö/ü noktalari, i noktasi) ve cizilmis bir kisa-yay (breve) ile birlestirilir."""
import re, struct, sys
import numpy as np
from PIL import Image, ImageDraw

SRC_CPP = __import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'src', 'data', 'FontData03.cpp')

def load_table():
    t = open(SRC_CPP).read()
    return {int(a): [float(x) for x in (b, c, d, e, f, h, i)]
            for a, b, c, d, e, f, h, i in re.findall(r'\{(\d+),([\d.-]+)f,([\d.-]+)f,([\d.-]+)f,([\d.-]+)f,([\d.-]+)f,([\d.-]+)f,([\d.-]+)f\}', t)}

class Canvas:
    """Kaynak orijinli koordinatlar: (ox, oy) = kalem konumuna gore sol-ust. Parcalar max ile birlesir."""
    def __init__(self): self.parts = []
    def add(self, arr, ox, oy): self.parts.append((arr, int(round(ox)), int(round(oy))))
    def render(self):
        x0 = min(p[1] for p in self.parts); y0 = min(p[2] for p in self.parts)
        x1 = max(p[1] + p[0].shape[1] for p in self.parts); y1 = max(p[2] + p[0].shape[0] for p in self.parts)
        out = np.zeros((y1 - y0, x1 - x0), np.uint8)
        for a, ox, oy in self.parts:
            r = out[oy - y0:oy - y0 + a.shape[0], ox - x0:ox - x0 + a.shape[1]]
            np.maximum(r, a, out=r)
        return out, x0, y0

def ink_runs(a):
    rows = (a > 24).any(axis=1); runs = []; s = None
    for i, v in enumerate(rows):
        if v and s is None: s = i
        if not v and s is not None: runs.append((s, i)); s = None
    if s is not None: runs.append((s, len(rows)))
    return runs

def breve(width, thick=2.6, height=6):
    S = 8; W, H = int(width * S), int(height * S)
    im = Image.new('L', (W, H), 0); d = ImageDraw.Draw(im)
    t = int(thick * S)
    d.arc([0, -H, W - 1, H - 1], 20, 160, fill=255, width=t)   # alt yarim daire: ince "u" yayi
    im = im.resize((int(width), int(height)), Image.LANCZOS)
    return np.array(im, np.uint8)

def build(mip0, tab):
    def crop(cp):
        x, y, w, h = [int(v) for v in tab[cp][:4]]
        return mip0[y:y + h, x:x + w].copy()
    def info(cp): return tab[cp]
    def center(cp): return info(cp)[4] + info(cp)[2] / 2.0   # kalem-gore yatay merkez
    new = {}

    # ı: i'nin govdesi (noktasiz)
    a = crop(0x69); runs = ink_runs(a); stem = a[runs[-1][0]:]
    new[0x131] = (stem, info(0x69)[4], info(0x69)[5] + runs[-1][0], info(0x69)[6])

    # İ: I + i'nin noktasi
    dot = a[runs[0][0]:runs[0][1]]; gap = runs[-1][0] - runs[0][1]
    c = Canvas(); c.add(crop(0x49), info(0x49)[4], info(0x49)[5])
    c.add(dot, center(0x49) - dot.shape[1] / 2.0, info(0x49)[5] - dot.shape[0] - gap)
    bm, ox, oy = c.render(); new[0x130] = (bm, ox, oy, info(0x49)[6])

    # Ü: U + Ö'nun noktalari
    o = crop(0xd6); r = ink_runs(o); dots = o[r[0][0]:r[0][1]]
    dots_oy = info(0xd6)[5] + r[0][0]; dots_ox = info(0xd6)[4] + (o.shape[1] - dots.shape[1]) / 2.0
    odc = center(0xd6)
    c = Canvas(); c.add(crop(0x55), info(0x55)[4], info(0x55)[5])
    c.add(dots, center(0x55) - dots.shape[1] / 2.0, dots_oy)
    bm, ox, oy = c.render(); new[0xdc] = (bm, ox, oy, info(0x55)[6])

    # cedilla: ç'nin taban cizgisinin altindaki parca
    cc = crop(0xe7); base = 50 - int(info(0xe7)[5])      # kaynakta satir (govde alti ~ 52)
    ced = cc[base:]; ced_oy = info(0xe7)[5] + base
    def with_cedilla(cp, key):
        c = Canvas(); c.add(crop(cp), info(cp)[4], info(cp)[5])
        c.add(ced, center(cp) - ced.shape[1] / 2.0 + 0.5, ced_oy)
        bm, ox, oy = c.render(); new[key] = (bm, ox, oy, info(cp)[6])
    with_cedilla(0x43, 0xc7); with_cedilla(0x73, 0x15f); with_cedilla(0x53, 0x15e)

    # ğ / Ğ: g/G + kisa yay
    def with_breve(cp, key, w):
        b = breve(w); c = Canvas(); c.add(crop(cp), info(cp)[4], info(cp)[5])
        c.add(b, center(cp) - b.shape[1] / 2.0, info(cp)[5] - b.shape[0] - 2)
        bm, ox, oy = c.render(); new[key] = (bm, ox, oy, info(cp)[6])
    with_breve(0x67, 0x11f, 12); with_breve(0x47, 0x11e, 16)
    return new

def main():
    if len(sys.argv) < 3: sys.exit(__doc__)
    d = bytearray(open(sys.argv[1], 'rb').read())
    fmt, w, h, extra, fl = struct.unpack('<5I', d[:20]); assert (fmt, w, h) == (6, 2048, 2048), 'beklenmeyen font atlasi'
    mip0 = np.flipud(np.frombuffer(bytes(d[20:20 + w * h]), np.uint8).reshape(h, w)).copy()   # dosyada satirlar alttan uste; FontInfo y ustten
    tab = load_table(); new = build(mip0, tab)
    x, y, rows = 8, 1100, []
    for cp in sorted(new):
        bm, ox, oy, adv = new[cp]; hh, ww = bm.shape
        mip0[y:y + hh, x:x + ww] = bm
        rows.append((cp, x, y, ww, hh, ox, oy, adv)); x += ww + 8
    # mip zinciri yeniden (kutu suzgeci)
    off = 20; lvl = np.flipud(mip0); sizes = []
    for l in range(extra + 1):
        n = lvl.shape[0]; d[off:off + n * n] = lvl.tobytes(); off += n * n
        if n > 1: lvl = ((lvl[0::2, 0::2].astype(np.uint16) + lvl[1::2, 0::2] + lvl[0::2, 1::2] + lvl[1::2, 1::2]) // 4).astype(np.uint8)
    open(sys.argv[2], 'wb').write(bytes(d))
    if len(sys.argv) > 3:
        with open(sys.argv[3], 'w') as f:
            f.write('// OTOMATIK URETILDI: tools/patch_font_tr.py (Turkce harfler: Ç ğ Ğ ı İ ş Ş Ü). Atlas yamasi CI/paketlemede ayni arac ile yapilir.\n')
            f.write('#include "data/Font.h"\nnamespace fs12 {\nconst int kFontTrCount = %d;\nconst FontInfo kFontTr[%d] = {\n' % (len(rows), len(rows)))
            for r in rows: f.write('  {%d,%.1ff,%.1ff,%.1ff,%.1ff,%.1ff,%.1ff,%.1ff},\n' % r)
            f.write('};\n}\n')
    if len(sys.argv) > 4:   # onizleme PNG
        Image.fromarray(mip0[1090:1160, 0:400]).resize((1600, 280), Image.NEAREST).save(sys.argv[4])
if __name__ == '__main__': main()
