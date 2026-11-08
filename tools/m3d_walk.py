#!/usr/bin/env python3
"""map*.m3d bicimini Map::load ile ayni sirayla yurur; tuketilen bayt == dosya boyutu ise bicim dogrudur."""
import struct, sys
d = open(sys.argv[1], 'rb').read()
p = 0
def u32(): 
    global p; v, = struct.unpack_from('<I', d, p); p += 4; return v
def f32():
    global p; v, = struct.unpack_from('<f', d, p); p += 4; return v
x0, y0 = f32(), f32(); nx, ny = u32(), u32(); dx, dy = f32(), f32()
ntex = u32()
tex = []
for i in range(ntex):
    s = d[p:p + 128].split(b'\0')[0].decode(); p += 128; tex.append(s)
nmesh = u32(); meshes = []
for i in range(nmesh):
    nv, ni = u32(), u32(); meshes.append((nv, ni)); p += nv * 24 + ni * 2 + (2 if ni & 1 else 0)
nobj = u32(); p += nobj * 0x64
npoly = u32(); polys = 0
for i in range(npoly):
    a, b, n = u32(), u32(), u32(); p += n * 8; polys += 1
cells = nx * ny; cc = 0
for i in range(cells):
    p += 0x14; k = u32(); p += k * 4 - 0 + 0   # alan 0x14: fieldDesc, tex, rot, ?, flags ; sonra k indeks
print('header', x0, y0, nx, ny, dx, dy, 'tex', ntex, 'mesh', nmesh, 'obj', nobj, 'poly', npoly)
print('cells bitti p=', p, 'dosya', len(d))
nsp = u32(); print('spline', nsp)
for i in range(nsp):
    n = u32(); u32(); p += n * 8
print('son p=', p, 'dosya', len(d), 'FARK', len(d) - p)
