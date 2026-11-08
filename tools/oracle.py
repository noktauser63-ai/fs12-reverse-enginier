#!/usr/bin/env python3
"""Orijinal libapp.so'yu Unicorn'da calistiran 'kahin' (oracle): fonksiyon cagirir, GL cagrilarini kaydeder.
Kullanim (modul): from oracle import Oracle; o = Oracle(so); o.call('Sinif::yontem(...)', r0, r1, ...)
PYTHONPATH=<unicorn/bindings/python>"""
import struct, subprocess, sys, ctypes
_libm = ctypes.CDLL('libm.so.6')
_libm.cosf.restype = ctypes.c_float; _libm.cosf.argtypes = [ctypes.c_float]
_libm.sinf.restype = ctypes.c_float; _libm.sinf.argtypes = [ctypes.c_float]
from unicorn import *
from unicorn.arm_const import *

class Oracle:
    STUB, HEAP, STACK = 0x00f00000, 0x00a00000, 0x00e00000
    def __init__(self, so):
        self.d = d = open(so, 'rb').read()
        e_phoff, = struct.unpack_from('<I', d, 0x1c); e_phentsize, e_phnum = struct.unpack_from('<HH', d, 0x2a)
        loads = []; dyn = None
        for i in range(e_phnum):
            t, off, va, pa, fs, ms, fl, al = struct.unpack_from('<8I', d, e_phoff + i * e_phentsize)
            if t == 1: loads.append((off, va, fs, ms))
            if t == 2: dyn = (off, fs)
        self.top = max(va + ms for _, va, _, ms in loads)
        uc = self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        uc.mem_map(0, 0x01000000)
        for off, va, fs, ms in loads: uc.mem_write(va, d[off:off + fs])
        uc.mem_write(self.STUB, b'\x70\x47' * 0x4000)
        uc.mem_map(0xffff0000, 0x1000)
        uc.mem_write(0xffff0fc0, struct.pack('<I', 0xe12fff1e))
        tags = {}
        off, fs = dyn
        for i in range(0, fs, 8):
            tg, vl = struct.unpack_from('<II', d, off + i)
            if tg == 0: break
            tags.setdefault(tg, vl)
        self.tags = tags
        self.symtab, self.strtab = tags[6], tags[5]
        self.stubs = {}; self.stubnames = {}
        for tg_t, tg_s in ((17, 18), (23, 2)):
            if tg_t in tags: self._reloc(tags[tg_t], tags[tg_s])
        # sembol tablosu (mangled -> adres), thumb biti dahil
        self.sym = {}
        nsyms = tags[4] and None
        so_syms = subprocess.run(['readelf', '-sW', so], capture_output=True, text=True).stdout.splitlines()
        mang = []
        for l in so_syms:
            p = l.split()
            if len(p) >= 8 and p[6] != 'UND' and p[3] in ('FUNC', 'OBJECT') and int(p[1], 16): mang.append((p[7], int(p[1], 16)))
        dem = subprocess.run(['c++filt'], input='\n'.join(m[0] for m in mang), capture_output=True, text=True).stdout.split('\n')
        for (m, a), dn in zip(mang, dem): self.sym[dn] = a; self.sym[m] = a
        self.heap = self.HEAP
        self.hooks = {}          # adres(thumb biti siz) -> python fonksiyon(oracle)
        self.gl = []             # kaydedilen GL cagrilari (isim, r0..r3)
        uc.hook_add(UC_HOOK_CODE, self._hook)
        self._init()
    def _symname(self, idx):
        o = self.symtab + idx * 16; n, = struct.unpack_from('<I', self.d, o)
        e = self.d.index(b'\0', self.strtab + n); return self.d[self.strtab + n:e].decode()
    def _stub_for(self, name):
        if name not in self.stubs: self.stubs[name] = self.STUB + 2 * len(self.stubs); self.stubnames[self.stubs[name]] = name
        return self.stubs[name]
    def _reloc(self, rtab, rsz):
        uc = self.uc
        for i in range(0, rsz, 8):
            r_off, r_info = struct.unpack_from('<II', self.d, rtab + i); t, s = r_info & 0xff, r_info >> 8
            cur, = struct.unpack('<I', uc.mem_read(r_off, 4))
            if t in (21, 22): uc.mem_write(r_off, struct.pack('<I', self._stub_for(self._symname(s)) | 1))
            elif t == 2:
                sv, = struct.unpack_from('<I', self.d, self.symtab + s * 16 + 4)
                uc.mem_write(r_off, struct.pack('<I', (sv + cur) if sv else self._stub_for(self._symname(s)) | 1))
    def alloc(self, n):
        p = self.heap; self.heap += (n + 15) & ~15; return p
    def r(self, i): return self.uc.reg_read(UC_ARM_REG_R0 + i)
    def f32(self, u): return struct.unpack('<f', struct.pack('<I', u))[0]
    def _hook(self, uc, addr, size, ud):
        if addr == 0xffff0fc0:
            o, n, p = (self.r(i) for i in range(3)); cur, = struct.unpack('<I', uc.mem_read(p, 4))
            if cur == o: uc.mem_write(p, struct.pack('<I', n)); uc.reg_write(UC_ARM_REG_R0, 0)
            else: uc.reg_write(UC_ARM_REG_R0, 1)
            return
        h = self.hooks.get(addr)
        if h is not None:
            ret = h(self)
            uc.reg_write(UC_ARM_REG_R0, (ret or 0) & 0xffffffff)
            uc.reg_write(UC_ARM_REG_PC, uc.reg_read(UC_ARM_REG_LR) & ~1)
            if uc.reg_read(UC_ARM_REG_LR) & 1: uc.reg_write(UC_ARM_REG_CPSR, uc.reg_read(UC_ARM_REG_CPSR) | 0x20)
            return
        if not (self.STUB <= addr < self.STUB + 0x8000): return
        name = self.stubnames.get(addr, '?')
        ret = 0
        if name in ('_Znwj', '_Znaj'): ret = self.alloc(self.r(0))
        elif name in ('memcpy', 'memmove'): uc.mem_write(self.r(0), bytes(uc.mem_read(self.r(1), self.r(2)))); ret = self.r(0)
        elif name == 'memset': uc.mem_write(self.r(0), bytes([self.r(1) & 0xff]) * self.r(2)); ret = self.r(0)
        elif name == 'strlen':
            p = self.r(0); n = 0
            while bytes(uc.mem_read(p + n, 1))[0] != 0: n += 1
            ret = n
        elif name == 'strcpy':
            b = bytearray(); p = self.r(1)
            while True:
                c = bytes(uc.mem_read(p, 1))[0]; b.append(c); p += 1
                if c == 0: break
            uc.mem_write(self.r(0), bytes(b)); ret = self.r(0)
        elif name in ('sprintf', 'snprintf'):
            sp = uc.reg_read(UC_ARM_REG_SP); stk = [struct.unpack('<I', bytes(uc.mem_read(sp + 4 * i, 4)))[0] for i in range(6)]
            if name == 'snprintf': fmtp = self.r(2); args = [self.r(3)] + stk      # snprintf(buf, n, fmt, ...)
            else: fmtp = self.r(1); args = [self.r(2), self.r(3)] + stk
            f = bytearray(); p = fmtp
            while True:
                c = bytes(uc.mem_read(p, 1))[0]
                if c == 0: break
                f.append(c); p += 1
            out = ''; i = 0; ai = 0
            while i < len(f):
                ch = chr(f[i])
                if ch == '%' and i + 1 < len(f):
                    j = i + 1; zero = False; wd = ''
                    if chr(f[j]) == '0': zero = True; j += 1
                    while chr(f[j]).isdigit(): wd += chr(f[j]); j += 1
                    pr = None
                    if chr(f[j]) == '.':
                        j += 1; pd = ''
                        while chr(f[j]).isdigit(): pd += chr(f[j]); j += 1
                        pr = int(pd or 0)
                    n = chr(f[j]); i = j
                    if n == '%': out += '%'
                    elif n in 'du':
                        v = args[ai]; ai += 1
                        if n == 'd' and v & 0x80000000: v -= 1 << 32
                        t = str(v); w = int(wd or 0)
                        out += t.rjust(w, '0' if zero else ' ')
                    elif n == 'f':      # softfp: double cift yazmac/yigin, 8 bayt hizali
                        if ai % 2: ai += 1
                        d, = struct.unpack('<d', struct.pack('<II', args[ai], args[ai + 1])); ai += 2
                        out += ('%.*f' % (1 if pr is None else pr, d)).rjust(int(wd or 0))
                    elif n == 's':
                        q = args[ai]; ai += 1; t = b''
                        while True:
                            c = bytes(uc.mem_read(q, 1))[0]
                            if c == 0: break
                            t += bytes([c]); q += 1
                        out += t.decode('latin1')
                else: out += ch
                i += 1
            uc.mem_write(self.r(0), out.encode('latin1') + b'\0'); ret = len(out)
        elif name in ('cosf', 'sinf'):
            x = struct.unpack('<f', struct.pack('<I', self.r(0)))[0]
            y = (_libm.cosf if name == 'cosf' else _libm.sinf)(x); ret = struct.unpack('<I', struct.pack('<f', y))[0]
        elif name == '__cxa_guard_acquire': ret = 1
        elif name.startswith('gl') or name.startswith('egl'): self.gl.append((name, self.r(0), self.r(1), self.r(2), self.r(3)))
        self.last_stub = name
        uc.reg_write(UC_ARM_REG_R0, ret)
    def _call_raw(self, addr, args):
        uc = self.uc
        for i, a in enumerate(args[:4]): uc.reg_write(UC_ARM_REG_R0 + i, a & 0xffffffff)
        sp = self.STACK
        extra = args[4:]
        sp -= 4 * len(extra)
        for i, a in enumerate(extra): uc.mem_write(sp + 4 * i, struct.pack('<I', a & 0xffffffff))
        uc.reg_write(UC_ARM_REG_SP, sp)
        uc.reg_write(UC_ARM_REG_LR, (self.STUB + 0x7ff0) | 1)
        uc.emu_start(addr, self.STUB + 0x7ff0, count=50_000_000)
        return uc.reg_read(UC_ARM_REG_R0)
    def _init(self):
        ia, ias = self.tags[25], self.tags[27]
        for i in range(0, ias, 4):
            fn, = struct.unpack('<I', self.uc.mem_read(ia + i, 4))
            if fn not in (0, 0xffffffff): self._call_raw(fn, [])
    def addr(self, name): return self.sym[name]
    def call(self, name, *args):
        a = self.sym[name] if isinstance(name, str) else name
        return self._call_raw(a, list(args))
    def hook(self, name, fn): self.hooks[self.sym[name] & ~1] = fn
    def read(self, a, n): return bytes(self.uc.mem_read(a, n))
    def write(self, a, b): self.uc.mem_write(a, b)
    def w32(self, a, v): self.uc.mem_write(a, struct.pack('<I', v & 0xffffffff))
    def wf(self, a, f): self.uc.mem_write(a, struct.pack('<f', f))
    def rd32(self, a): return struct.unpack('<I', self.read(a, 4))[0]
    def rdf(self, a): return struct.unpack('<f', self.read(a, 4))[0]
