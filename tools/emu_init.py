#!/usr/bin/env python3
"""libapp.so (32-bit ARM) statik baslaticilarini (INIT_ARRAY) Unicorn ile calistirip .data/.bss goruntusunu cikarir.
Orijinaldeki GuiButtonDesc/overlay tablolari gibi calisma zamaninda kurulan tablolari dogrudan okumak icin.
Kullanim: PYTHONPATH=<unicorn/bindings/python> emu_init.py <libapp.so> <cikti.bin>
Cikti: tum sanal bellek (0..memsz) duz goruntu; adres = vaddr (Ghidra adresi - 0x10000)."""
import struct, subprocess, sys
from unicorn import *
from unicorn.arm_const import *

so, outp = sys.argv[1], sys.argv[2]
d = open(so, 'rb').read()
e_phoff, = struct.unpack_from('<I', d, 0x1c); e_phentsize, e_phnum = struct.unpack_from('<HH', d, 0x2a)
loads = []; dyn = None
for i in range(e_phnum):
    t, off, va, pa, fs, ms, fl, al = struct.unpack_from('<8I', d, e_phoff + i * e_phentsize)
    if t == 1: loads.append((off, va, fs, ms))
    if t == 2: dyn = (off, fs)
top = max(va + ms for _, va, _, ms in loads)
TOP = (top + 0xfff) & ~0xfff
uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
MEM = 0x01000000                       # 16 MB: kutuphane + yigin + yigit + stub
uc.mem_map(0, MEM)
for off, va, fs, ms in loads: uc.mem_write(va, d[off:off + fs])
STUB = 0x00f00000; HEAP = 0x00a00000; STACK = 0x00e00000
uc.mem_write(STUB, b'\x70\x47' * 0x4000)
uc.mem_map(0xffff0000, 0x1000)
uc.mem_write(0xffff0fc0, struct.pack('<I', 0xe12fff1e))   # kuser __kernel_cmpxchg: bx lr (kanca islemi yapar)          # her stub: bx lr
# dinamik tablo
tags = {}; rel = []
off, fs = dyn
for i in range(0, fs, 8):
    tg, vl = struct.unpack_from('<II', d, off + i)
    if tg == 0: break
    tags.setdefault(tg, vl)
symtab, strtab = tags[6], tags[5]
def symname(idx):
    o = symtab + idx * 16
    n, = struct.unpack_from('<I', d, o)
    e = d.index(b'\0', strtab + n); return d[strtab + n:e].decode()
stubs = {}
def stub_for(name):
    if name not in stubs: stubs[name] = STUB + 2 * len(stubs)
    return stubs[name]
def apply(rtab, rsz):
    for i in range(0, rsz, 8):
        r_off, r_info = struct.unpack_from('<II', d, rtab + i)
        t, s = r_info & 0xff, r_info >> 8
        cur, = struct.unpack('<I', uc.mem_read(r_off, 4))
        if t == 23: uc.mem_write(r_off, struct.pack('<I', cur))              # RELATIVE (taban 0)
        elif t in (21, 22): uc.mem_write(r_off, struct.pack('<I', stub_for(symname(s)) | 1))  # GLOB_DAT/JUMP_SLOT
        elif t == 2:   # ABS32
            sv, = struct.unpack_from('<I', d, symtab + s * 16 + 4)
            uc.mem_write(r_off, struct.pack('<I', (sv + cur) if sv else stub_for(symname(s)) | 1))
for tg_t, tg_s in ((17, 18), (23, 2)):
    if tg_t in tags: apply(tags[tg_t], tags[tg_s] if tg_t == 17 else tags[2])
heap = [HEAP]
def alloc(n): p = heap[0]; heap[0] += (n + 15) & ~15; return p
log = []
def hook(uc, addr, size, ud):
    if addr == 0xffff0fc0:
        o, n, p = (uc.reg_read(UC_ARM_REG_R0 + i) for i in range(3))
        cur, = struct.unpack('<I', uc.mem_read(p, 4))
        if cur == o:
            uc.mem_write(p, struct.pack('<I', n)); uc.reg_write(UC_ARM_REG_R0, 0)
        else: uc.reg_write(UC_ARM_REG_R0, 1)
        return
    if not (STUB <= addr < STUB + 0x8000): return
    name = [k for k, v in stubs.items() if v == addr]
    name = name[0] if name else '?'
    r = lambda i: uc.reg_read(UC_ARM_REG_R0 + i)
    ret = 0
    if name in ('_Znwj', '_Znaj'): ret = alloc(r(0))
    elif name == 'memcpy' or name == 'memmove': uc.mem_write(r(0), bytes(uc.mem_read(r(1), r(2)))); ret = r(0)
    elif name == 'memset': uc.mem_write(r(0), bytes([r(1) & 0xff]) * r(2)); ret = r(0)
    elif name == '__cxa_guard_acquire': ret = 1
    else: log.append(name)
    uc.reg_write(UC_ARM_REG_R0, ret)
uc.hook_add(UC_HOOK_CODE, hook)
ia, ias = tags[25], tags[27]
funcs = [struct.unpack('<I', uc.mem_read(ia + i, 4))[0] for i in range(0, ias, 4)]
uc.reg_write(UC_ARM_REG_SP, STACK)
for fn in funcs:
    if fn in (0, 0xffffffff): continue
    uc.reg_write(UC_ARM_REG_LR, STUB + 0x7ff0 | 1)
    uc.reg_write(UC_ARM_REG_SP, STACK)
    try:
        uc.emu_start(fn, (STUB + 0x7ff0), count=20_000_000)
    except UcError as ex:
        print('init hata', hex(fn), ex, 'pc=%#x lr=%#x'%(uc.reg_read(UC_ARM_REG_PC), uc.reg_read(UC_ARM_REG_LR)))
open(outp, 'wb').write(bytes(uc.mem_read(0, TOP)))
print('init fonksiyonlari:', len(funcs), ' bellek:', hex(TOP), ' cagrilmis stublar:', sorted(set(log))[:20])
