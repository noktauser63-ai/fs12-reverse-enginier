#!/usr/bin/env python3
"""PNG -> .p2d (yuksek cozunurluklu doku degisimi). Kullanim: png2p2d.py <orijinal_assets> <png_klasoru> <cikis_assets>
png_klasoru/{map,objects}/ADI.png -> cikis/{map,objects}/ADI.p2d. Orijinalde RGB olan doku RGB8, alfa/RGBA olan RGBA8, L8 olan L8 yazilir.
Mip zinciri (kutu suzgeci) otomatik uretilir; bayraklar orijinalden kopyalanir. Boyutlar 2'nin kuvveti olmali."""
import sys, os, struct, glob
from PIL import Image
orig, src, out = sys.argv[1:4]
for f in sorted(glob.glob(os.path.join(src, '*', '*.png'))):
    d = os.path.basename(os.path.dirname(f)); b = os.path.basename(f)[:-4]
    o = os.path.join(orig, d, b + '.p2d')
    if not os.path.exists(o): print('orijinal yok, atlandi:', o); continue
    fmt0, w0, h0, _, flags = struct.unpack('<5I', open(o, 'rb').read(20))
    im = Image.open(f)
    w, h = im.size
    if w & (w - 1) or h & (h - 1): print('2^n degil, atlandi:', f); continue
    if fmt0 == 6: im = im.convert('L'); fmt, mode = 6, 'L'
    elif fmt0 == 12: im = im.convert('RGB'); fmt, mode = 1, 'RGB'
    else: im = im.convert('RGBA'); fmt, mode = 0, 'RGBA'
    levels = []
    cur = im
    while True:
        levels.append(cur.tobytes())
        if cur.size == (1, 1): break
        cur = cur.resize((max(cur.width // 2, 1), max(cur.height // 2, 1)), Image.BOX)
    p = os.path.join(out, d, b + '.p2d'); os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, 'wb') as g:
        g.write(struct.pack('<5I', fmt, w, h, len(levels) - 1, flags))
        for l in levels: g.write(l)
    print(f'{d}/{b}: {w0}x{h0} -> {w}x{h} fmt{fmt}')
