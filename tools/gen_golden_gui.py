#!/usr/bin/env python3
"""Orijinal GUI cizim cagrilarini (oracle) kaydedip normalize iz (trace) dosyasi uretir.
Kullanim: PYTHONPATH=<unicorn/bindings/python> gen_golden_gui.py <libapp.so> <cikti_dizini>"""
import sys, glob, struct, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from oracle import Oracle
from unicorn.arm_const import *
so, outdir = sys.argv[1], sys.argv[2]
o = Oracle(so)
H = lambda u: '%08x' % u
trace = []
def stack(oo, n):
    sp = oo.uc.reg_read(UC_ARM_REG_SP); return [struct.unpack('<I', oo.read(sp + 4 * i, 4))[0] for i in range(n)]
def cstr(oo, p):
    b = oo.read(p, 200); return b.split(b'\0')[0].decode('utf-8', 'replace')
def h_overlay(oo): st = stack(oo, 6); trace.append('O ' + ' '.join(H(x) for x in [oo.r(1), oo.r(2), oo.r(3)] + st)); return 0
def h_color(oo): p = oo.r(1); trace.append('C ' + ' '.join(H(oo.rd32(p + 4 * i)) for i in range(4))); return 0
def h_tcolor(oo): p = oo.r(1); trace.append('T ' + ' '.join(H(oo.rd32(p + 4 * i)) for i in range(4))); return 0
def h_font(oo): trace.append('F ' + H(oo.r(1))); return 0
def h_mono(oo): trace.append('M %d %s' % (oo.r(1) & 0xff, H(oo.r(2)))); return 0
def h_text(oo): st = stack(oo, 2); trace.append('R %s %s %s %s "%s"' % (H(oo.r(1)), H(oo.r(2)), H(oo.r(3)), H(st[0]), cstr(oo, st[1]))); return 0
SIG = {
 'GLESHandheldRenderDevice::renderOverlay(unsigned int, float, float, float, float, float, float, float, float)': h_overlay,
 'GLESHandheldRenderDevice::enqueueSetOverlayColor(float const*)': h_color,
 'GLESHandheldRenderDevice::enqueueSetTextColor(float const*)': h_tcolor,
 'GLESHandheldRenderDevice::enqueueSetFontSize(float)': h_font,
 'GLESHandheldRenderDevice::enqueueSetNumberMonospace(bool, float)': h_mono,
 'GLESHandheldRenderDevice::renderText(unsigned int, float, float, unsigned int, char const*)': h_text}
for k, v in SIG.items(): o.hook(k, v)
tid = [10]
A = [o.alloc(n) for n in (0x6000, 0x80000, 0x500, 0x100, 0x2000)]
CREDS = o.alloc(32 * 52)
def tex(oo): tid[0] += 1; return tid[0]
o.hook('Texture::load(char const*, AndroidHandheldSystemDevice*, GLESHandheldRenderDevice*, bool, unsigned int)', tex)
o.hook('GLESHandheldRenderDevice::initFont(unsigned int, unsigned int, float, GLESHandheldRenderDevice::FontInfo const*)', lambda oo: 99)
# LocalizationStringManager gercek tablo uzerinde calissin: getString dogal calisir (m_strings .data'da)
GUI = 'GenericGUIManager::'
LOAD = GUI + 'load(AndroidHandheldSystemDevice*, LocalizationStringManager*, GLESHandheldRenderDevice*, AndroidHandheldAudioDevice*, GenericHandheldAchievementDevice*, unsigned int, unsigned int)'
os.makedirs(outdir, exist_ok=True)
out_state = open(os.path.join(outdir, 'gui_initial_state.bin'), 'wb')
def scenario(name, aspect, lang, desc_words, phase, fn='renderMainMenu', menu=None, scroll=None, dbytes=None, mgr=None, blobs=None):
    tid[0] = 10
    this, dev, sysd, loc, desc = A
    for a, n in zip(A, (0x6000, 0x80000, 0x500, 0x100, 0x2000)): o.write(a, bytes(n))
    o.wf(dev + 4, aspect); o.wf(dev + 0xc, 1.0)
    o.w32(loc, lang)
    o.call(GUI + 'GenericGUIManager()', this)
    o.call(LOAD, this, sysd, loc, dev, 0, 0, 480, 320)
    if name == 'main_a' and aspect > 1.7 and out_state.tell() == 0: out_state.write(o.read(this, 0x5c80))
    for off, v in desc_words.items(): o.w32(desc + off, v)
    for off, v in (dbytes or {}).items(): o.write(desc + off, bytes([v]))
    for off, bs in (blobs or {}).items(): o.write(desc + off, bs)
    o.wf(this + 0x5c68, phase)
    if menu is not None: o.w32(this + 0x58cc, menu)
    mg = mgr(this) if callable(mgr) else (mgr or {})
    mgr = mg
    for off, v in mg.items(): o.w32(this + off, v)
    if scroll is not None:
        o.wf(this + 0x588c, scroll)
        for i in range(52):
            sp = CREDS + 32 * i; o.write(sp, ('Credit line %d' % i).encode() + b'\0'); o.w32(desc + 0x2c + 4 * i, sp)
    trace.clear()
    o.call(GUI + fn + '(GUIInterfaceDesc*, LocalizationStringManager*, GLESHandheldRenderDevice*)', this, desc, loc, dev)
    open(os.path.join(outdir, 'gui_trace_%s.txt' % name), 'w').write('# aspect=%.9g lang=%d phase=%.9g c=%d x146=%d fn=%s scroll=%.9g dw=%s db=%s mw=%s\n' % (aspect, lang, phase, desc_words.get(0xc, 0), desc_words.get(0x146, 0), fn, scroll if scroll is not None else 0.0, ','.join('%x:%x' % kv for kv in desc_words.items()) or '-', ','.join('%x:%x' % kv for kv in (dbytes or {}).items()) or '-', ','.join('%x:%x' % kv for kv in (mgr or {}).items()) or '-') + '\n'.join(trace) + '\n')
    print(name, len(trace), 'cagri')
scenario('main_a', 1.7777778, 0, {0xc: 1, 0x146: 1}, 0.25)
scenario('main_b', 1.7777778, 0, {0xc: 0, 0x146: 0}, 0.0)
scenario('main_c', 1.3333334, 1, {0xc: 1, 0x146: 0}, 0.0)
scenario('main_d', 2.1666667, 2, {0xc: 1, 0x146: 1}, 0.6)

scenario('startmain_a', 1.7777778, 0, {}, 0.0, 'renderStartMenuMain')
scenario('startmain_b', 1.3333334, 3, {}, 0.0, 'renderStartMenuMain')

scenario('credits_a', 1.7777778, 0, {}, 0.0, 'renderCreditsMenu', scroll=3.0)
scenario('credits_b', 1.7777778, 0, {}, 0.0, 'renderCreditsMenu', scroll=-50.0)
scenario('credits_c', 1.3333334, 0, {}, 0.0, 'renderCreditsMenu', scroll=-20.5)

SET = 'renderStartMenuSettings'
scenario('settings_a', 1.7777778, 0, {0x12ec: 70, 0x12e8: 35}, 0.0, SET, dbytes={0x12f4: 1, 0x12f5: 0, 0x12f6: 1})
scenario('settings_b', 1.3333334, 2, {0x12ec: 0, 0x12e8: 100}, 0.0, SET, dbytes={0x12f4: 0, 0x12f5: 1, 0x12f6: 0})

HELP = 'renderStartMenuHelp'
for st in range(4):
    scenario('help_%d' % st, 1.7777778 if st % 2 == 0 else 1.3333334, st, {}, 0.0, HELP, mgr={0x58d0: st})

SAVE = 'renderSaveGameMenu'
scenario('save_a', 1.7777778, 0, {0x100: 1234567, 0x104: 999, 0x108: 0, 0x10c: 12, 0x110: 3, 0x114: 0, 0x118: 5, 0x11c: 59, 0x120: 0}, 0.0, SAVE, dbytes={0xfc: 1, 0xfd: 1, 0xfe: 0})
scenario('save_b', 1.3333334, 1, {0x100: 4000000000, 0x104: 1000, 0x108: 77777, 0x10c: 100, 0x110: 7, 0x114: 8, 0x118: 9, 0x11c: 10, 0x120: 11}, 0.0, SAVE, dbytes={0xfc: 0, 0xfd: 1, 0xfe: 1})

STAT = 'renderStartMenuStatistics'
sw_a = {0x1264: 12345, 0x1268: 999, 0x126c: 123456789, 0x1298: 7, 0x129c: 125, 0x12a0: 0, 0x12a4: 3599, 0x12a8: 50, 0x12ac: 100, 0x12b0: 0,
        0x12b4: 1000000, 0x12b8: 42, 0x12bc: 9999999, 0x12c0: 10000000, 0x12c4: 3, 0x12c8: 7, 0x12cc: 1, 0x12d0: 2, 0x12d4: 5, 0x12d8: 17}
scenario('stats_a', 1.7777778, 0, sw_a, 0.0, STAT)
sw_b = {k: v + 1 for k, v in sw_a.items()}; sw_b[0x12cc] = 2; sw_b[0x129c] = 0
scenario('stats_b', 1.3333334, 2, sw_b, 0.0, STAT)

PRI = 'renderStartMenuPrices'
import struct as _st
f2u = lambda f: _st.unpack('<I', _st.pack('<f', f))[0]
pw_a = {0x1224 + 4 * k: (k * 7 + 3) * (1 if k % 5 else 0) for k in range(16)}
pw_a.update({0x1270: 2, 0x1274: 1, 0x1278: 3, 0x127c: 2, 0x1280: 0, 0x1284: f2u(1.5), 0x1288: f2u(2.25), 0x128c: 95, 0x1290: 3601})
scenario('prices_a', 1.7777778, 0, pw_a, 0.0, PRI)
pw_b = {0x1224 + 4 * k: k * 11 + 100 for k in range(16)}
pw_b.update({0x1270: 1, 0x1274: 2, 0x127c: 1, 0x1284: f2u(3.0), 0x128c: 0})
scenario('prices_b', 1.3333334, 2, pw_b, 0.0, PRI)

SHOP = 'renderStartMenuShop'
STR = o.alloc(0x2000)
def shop_case(name, aspect, lang, sel, scroll, alt_owned, f1220, f1221, seed):
    dw = {}; db = {}; blobs = {}
    for i in range(18):
        np_ = STR + i * 64; dp = STR + 0x800 + i * 64
        o.write(np_, ('Name%d' % i).encode() + b'\0'); o.write(dp, ('Desc%d' % i).encode() + b'\0')
        dw[0x65c + 4 * i] = np_
        dw[0x6a4 + 4 * i] = dp if (i + seed) % 3 else 0
        dw[0x5a8 + 4 * i] = (i * 7 + seed) % 50
        dw[0x614 + 4 * i] = (i * 12345 + seed * 977) % 3000000
        db[0x734 + i] = (i + seed) % 2; db[0x746 + i] = (i * seed) % 2; db[0x758 + i] = (i + 1) % 3 == 0
        db[0x76a + i] = 1 if (i in alt_owned) else 0
        if i in alt_owned and (i + seed) % 2: db[0x77c + i * 0x80] = ord('A'); blobs[0x77c + i * 0x80] = ('Alt%d' % i).encode() + b'\0'
    dw[0x12c] = 2500000 + seed
    db[0x1220] = f1220; db[0x1221] = f1221
    scenario(name, aspect, lang, dw, 0.0, SHOP, dbytes=db, mgr={0x583c: sel, 0x5840: f2u(scroll)}, blobs=blobs)
shop_case('shop_a', 1.7777778, 0, 4, 0.0, {4, 5}, 0, 0, 1)
shop_case('shop_b', 1.3333334, 2, 0, 0.35, {0, 17}, 1, 0, 2)
shop_case('shop_c', 1.7777778, 1, 17, -0.4, set(), 0, 1, 3)
shop_case('shop_d', 2.1666667, 3, 9, 0.0, {9}, 0, 0, 4)

DLG = 'renderDialog'
DSTR = o.alloc(0x400)
def dlg_case(name, aspect, lang, typ, state, count, ids, ptrs, slots, base_flag, ys=(0.2, 0.8), scale=1.0):
    dw = {0x28: count}; db = {}; blobs = {}
    for i in range(3):
        dw[0x10 + 4 * i] = ids[i] & 0xffffffff
        if ptrs[i]:
            o.write(DSTR + 64 * i, ('Msg%d' % i).encode() + b'\0'); dw[0x1c + 4 * i] = DSTR + 64 * i
        else: dw[0x1c + 4 * i] = 0
    for k in range(3):
        if slots[k]: db[0x107c + 0x80 * k] = ord('S'); blobs[0x107c + 0x80 * k] = ('Slot%d' % k).encode() + b'\0'
    def mg(this):
        m = {0x5c78: typ, 0x5c74: state, 0x5964: f2u(ys[0]), 0x5968: f2u(ys[1]), 0x5808: base_flag}
        ids_ = [o.rd32(this + off) for off in (0x5948, 0x594c, 0x5950, 0x5954, 0x5958, 0x595c, 0x5960)]
        m[0x591c] = {1: 2, 2: 0, 3: 1, 4: 4}[typ]
        lst = {1: [ids_[0], ids_[1]], 2: [], 3: [ids_[3]], 4: [ids_[2], ids_[4], ids_[5], ids_[6]]}[typ]
        for j, v in enumerate(lst): m[0x5920 + 4 * j] = v
        return m
    scenario(name, aspect, lang, dw, 0.0, DLG, dbytes=db, mgr=mg, blobs=blobs)
dlg_case('dlg_a', 1.7777778, 0, 1, 7, 2, [0x20, -1, -1], [0, 1, 0], [0, 0, 0], 1)
dlg_case('dlg_b', 1.3333334, 1, 2, 7, 3, [-1, 0x21, -1], [1, 0, 1], [0, 0, 0], 1, ys=(0.15, 0.85))
dlg_case('dlg_c', 1.7777778, 2, 3, 6, 1, [0x22, -1, -1], [0, 0, 0], [0, 0, 0], 1)
dlg_case('dlg_d', 2.1666667, 0, 4, 7, 2, [0x23, 0x24, -1], [0, 0, 0], [1, 0, 1], 1)

BASE = 'renderBaseMenu'
scenario('base_a', 1.7777778, 0, {}, 0.0, BASE)
scenario('base_b', 1.7777778, 0, {0x124: 7, 0x12c: 123456789, 0x130: 5, 0x134: 12, 0x138: 34, 0x59c: 3, 0x5a0: f2u(0.756)}, 0.25, BASE, dbytes={0x128: 1, 0x5a4: 1})
scenario('base_c', 1.3333334, 1, {0x124: 999, 0x12c: 42, 0x130: 12, 0x134: 3, 0x138: 9, 0x59c: 2, 0x5a0: f2u(0.4)}, 0.0, BASE, dbytes={0x128: 0, 0x5a4: 1})
