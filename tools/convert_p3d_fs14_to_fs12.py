#!/usr/bin/env python3
"""FS14 .p3d -> FS12 .p3d.
FS14: [u32 surum=1][u32 vert][u32 indis][vert*28: pos f3, normal 4xi8, tangent 4xi8, uv f2][indis*u16][indis tekse +2 dolgu]
FS12: [u32 vert][u32 indis][vert*24: pos f3, normal 4xi8, uv f2][indis*u16][indis tekse +2 dolgu]
(FS12 IndexedTriangleMesh::load: vertex stride 0x18, kanit: libapp.so decompile.)
Kullanim: convert_p3d_fs14_to_fs12.py girdi.p3d [cikti.p3d]   (klasor de verilebilir)
"""
import struct, sys, os

def convert(data: bytes) -> bytes:
    ver, nv, ni = struct.unpack_from('<3I', data, 0)
    if ver != 1:
        raise ValueError(f'beklenmeyen surum {ver}')
    voff, ioff = 12, 12 + nv * 28
    if ioff + ni * 2 > len(data):
        raise ValueError('dosya kisa (28 bayt vertex varsayimi tutmadi)')
    out = bytearray(struct.pack('<2I', nv, ni))
    for i in range(nv):
        v = data[voff + i * 28: voff + (i + 1) * 28]
        out += v[0:16]      # pos(12) + normal(4)
        out += v[20:28]     # uv(8)  (tangent 16..20 atilir)
    out += data[ioff: ioff + ni * 2]
    if ni & 1:
        out += b'\0\0'
    return bytes(out)

def main():
    if len(sys.argv) < 2: print(__doc__); sys.exit(1)
    src = sys.argv[1]; dst = sys.argv[2] if len(sys.argv) > 2 else None
    if os.path.isdir(src):
        dst = dst or src.rstrip('/') + '_fs12'; os.makedirs(dst, exist_ok=True)
        for f in sorted(os.listdir(src)):
            if f.endswith('.p3d'):
                open(os.path.join(dst, f), 'wb').write(convert(open(os.path.join(src, f), 'rb').read()))
                print('ok', f)
    else:
        dst = dst or src.replace('.p3d', '_fs12.p3d')
        open(dst, 'wb').write(convert(open(src, 'rb').read())); print('ok', dst)

if __name__ == '__main__':
    main()
