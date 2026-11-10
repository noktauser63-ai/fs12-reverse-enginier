#!/usr/bin/env python3
"""FS12_Entegrasyon.zip -> ext_assets/ (orijinal FS12 formatina).
p3d: [1][nv][ni] + nv*28 (pos12 nrm4 tan4 uv8) + idx  ->  [nv][ni] + nv*24 (pos12 nrm4 uv8) + idx(+pad)
Dokular (p2d) ve sesler (p1d) oldugu gibi kopyalanir. kullanim: import_ext.py <FS12_Entegrasyon dizini> <cikis dizini>"""
import struct, sys, os, glob, shutil
src, out = sys.argv[1], sys.argv[2]
def conv(b):
    ver, nv, ni = struct.unpack('<3I', b[:12])
    assert ver == 1 and len(b) >= 12 + nv * 28 + ni * 2, 'beklenmeyen p3d'
    o = bytearray(struct.pack('<2I', nv, ni))
    for i in range(nv):
        v = b[12 + i * 28: 12 + (i + 1) * 28]
        o += v[:16] + v[20:28]
    o += b[12 + nv * 28: 12 + nv * 28 + ni * 2]
    if ni & 1: o += b'\0\0'
    return bytes(o)
n = 0
for vd in sorted(glob.glob(os.path.join(src, '01_araclar', '*'))):
    name = os.path.basename(vd)
    for sub in ('model_ana', 'model_parcalar'):
        for f in glob.glob(os.path.join(vd, sub, '*.p3d')):
            d = os.path.join(out, 'objects'); os.makedirs(d, exist_ok=True)
            open(os.path.join(d, os.path.basename(f)), 'wb').write(conv(open(f, 'rb').read())); n += 1
    for sub in ('doku_govde', 'doku_tekerlek'):
        for f in glob.glob(os.path.join(vd, sub, '*diffuse.p2d')):       # normal haritalari orijinal motorda yok
            d = os.path.join(out, 'objects'); shutil.copy(f, os.path.join(d, os.path.basename(f))); n += 1
    for sd in ('ses', 'ses_alternatif_lizard'):
        for f in glob.glob(os.path.join(vd, sd, '*.p1d')):
            tag = name + ('_lizard' if 'lizard' in sd else '')
            d = os.path.join(out, 'audio'); os.makedirs(d, exist_ok=True)
            shutil.copy(f, os.path.join(d, tag + '_' + os.path.basename(f))); n += 1
for kind in ('mahsul_dokulari', 'karakter_dokulari'):
    for f in glob.glob(os.path.join(src, '02_ortak', kind, '*.p2d')):
        d = os.path.join(out, 'ext_' + kind); os.makedirs(d, exist_ok=True); shutil.copy(f, d); n += 1
print(n, 'dosya')
