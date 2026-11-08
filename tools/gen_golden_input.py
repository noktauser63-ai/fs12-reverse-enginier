#!/usr/bin/env python3
"""updateGuiButtons icin altin vektorler: PYTHONPATH=<unicorn> gen_golden_input.py <libapp.so> <cikti.txt>"""
import sys, os, struct, random
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from oracle import Oracle
so, out = sys.argv[1], sys.argv[2]
o = Oracle(so)
fb = lambda f: struct.unpack('<I', struct.pack('<f', f))[0]
bf = lambda u: struct.unpack('<f', struct.pack('<I', u & 0xffffffff))[0]
f32 = lambda f: bf(fb(f))
G = 'GenericGUIManager::'
LOAD = G + 'load(AndroidHandheldSystemDevice*, LocalizationStringManager*, GLESHandheldRenderDevice*, AndroidHandheldAudioDevice*, GenericHandheldAchievementDevice*, unsigned int, unsigned int)'
UPD = G + 'updateGuiButtons(GUIInterfaceDesc*, AndroidHandheldInputDevice*, unsigned int, unsigned int*)'
log = []
TB = [0]; NT = [0]; OPENRET = [0]
STRIDE = 0x48
o.hook('AndroidHandheldInputDevice::getNumTouches()', lambda oo: NT[0])
o.hook('AndroidHandheldInputDevice::getTouch(unsigned int)', lambda oo: TB[0] + STRIDE * oo.r(1))
def byid(oo):
    lo, hi = oo.r(2), oo.r(3)
    for i in range(NT[0]):
        p = TB[0] + STRIDE * i
        if oo.rd32(p) == lo and oo.rd32(p + 4) == hi: return p
    return 0
o.hook('AndroidHandheldInputDevice::getTouchById(unsigned long long)', byid)
o.hook('AndroidHandheldInputDevice::disableTouchForGestures(unsigned int)', lambda oo: log.append('D%d' % oo.r(1)) or 0)
o.hook('AudioSource::play(float, float, bool)', lambda oo: log.append('K%d' % (oo.r(0) & 1)) or 0)
def op(oo):
    s = oo.read(oo.r(1), 80).split(b'\0')[0].decode(); log.append('U' + ('M' if 'Manual' in s else 'S')); return OPENRET[0]
o.hook('AndroidHandheldSystemDevice::openUrl(char const*)', op)
o.hook(G + 'handleShopBuyCoins(unsigned int, GUIInterfaceDesc*)', lambda oo: log.append('B%d' % oo.r(1)) or 0)
o.hook('Texture::load(char const*, AndroidHandheldSystemDevice*, GLESHandheldRenderDevice*, bool, unsigned int)', lambda oo: 11)
o.hook('GLESHandheldRenderDevice::initFont(unsigned int, unsigned int, float, GLESHandheldRenderDevice::FontInfo const*)', lambda oo: 99)
random.seed(1234)
lines = []
SLOT = 3
A = [o.alloc(n) for n in (0x6000, 0x80000, 0x500, 0x100, 0x2000, 0x100, STRIDE * 16, 16, 64)]
def scenario(idx, aspect, mode):
    this, dev, sysd, loc, desc, inp, tb, ids_arr, buf = A
    TB[0] = tb
    o.write(this, bytes(0x6000)); o.write(desc, bytes(0x2000)); o.write(inp, bytes(0x100))
    o.wf(dev + 4, aspect); o.wf(dev + 0xc, 1.0); o.w32(loc, 0)
    o.call(G + 'GenericGUIManager()', this)
    o.call(LOAD, this, sysd, loc, dev, 0, 0, 480, 320)
    o.call(G + 'updateScreenScale(GLESHandheldRenderDevice*)', this, dev)
    sound = random.randint(0, 1); f4, f5, f6 = [random.randint(0, 1) for _ in range(3)]
    used = random.randint(0, 1); lp = idx % 11; openret = random.randint(0, 1); OPENRET[0] = openret
    o.w32(this + 0x583c, SLOT)
    o.write(desc, bytes([sound])); o.write(desc + 0x12f4, bytes([f4, f5, f6])); o.write(desc + 0x76a + SLOT, bytes([used]))
    for base, n in ((0x5bdc, 0x11), (0x5c20, 0x11), (0x5bb0, 0xb)):
        for k in range(n): o.w32(this + base + 4 * k, 0xffff)
        o.w32(this + base + 4 * lp, idx)
    # buton geometrisi
    rd = lambda off: o.rd32(this + 8 + idx * 0xb0 + off)
    rf = lambda off: bf(rd(off))
    bid, bx, by, bw, bh, al, anc = rd(0), rf(0x18), rf(0x1c), rf(0x20), rf(0x24), rd(0x28), rd(0x3c)
    m = [rf(0x74 + 4 * k) for k in range(4)]
    l = bx - m[0] - (bw * .5 if anc & 2 else bw if anc & 4 else 0)
    t = by - m[1] - 0  # y icin asagidaki
    t = by - m[1]
    if anc & 0x10: t -= bh * .5
    elif anc & 0x20: t -= bh
    cx = l + (bw + m[0] + m[2]) / 2; cy = t + (bh + m[1] + m[3]) / 2
    for k, v in enumerate((cx, cy, 0.0, 0.0)): o.wf(buf + 4 * k, v)
    o._call_raw(o.sym[G + 'applyScreenAlign(float&, float&, float&, float&, unsigned int)'], [this, buf, buf + 4, buf + 8, buf + 12, al])
    sxp, syp = o.rdf(buf), o.rdf(buf + 4)
    lines.append('SC %s %d %d %d %d %d %d %d %d %d %d' % (repr(f32(aspect)), idx, sound, f4, f5, f6, used, lp, openret, bid, mode))
    o.w32(ids_arr, idx)
    tid = (0x100000005 + random.randint(0, 3))
    pad = inp
    def frame(touches, padv):
        NT[0] = len(touches)
        for i, (x, y, fl) in enumerate(touches):
            p = tb + STRIDE * i
            o.w32(p, tid & 0xffffffff); o.w32(p + 4, tid >> 32); o.wf(p + 8, x); o.wf(p + 0xc, y); o.w32(p + 0x3c, fl); o.write(p + 0x40, b'\x01')
        o.w32(pad + 0x14, 0); o.w32(pad + 0x20, padv[0]); o.w32(pad + 0x24, padv[1]); o.w32(pad + 0x28, padv[2])
        log.clear()
        ret = o._call_raw(o.sym[UPD], [this, desc, inp, 1, ids_arr])
        lines.append('IN %d %s %08x %08x %08x' % (len(touches), ' '.join('%x %08x %08x %d' % (tid, fb(x), fb(y), fl) for x, y, fl in touches), *padv))
        lines.append('OUT %d %d %d %d %08x %08x %08x %s' % (ret & 1, o.rd32(desc + 0x1310), o.rd32(desc + 0x1314), o.rd32(inp + 0x14), o.rd32(inp + 0x20), o.rd32(inp + 0x24), o.rd32(inp + 0x28), ','.join(log) or '-'))
        b = lambda off: o.rd32(this + 8 + idx * 0xb0 + off)
        lines.append('BT %08x %08x %08x %08x %08x %02x' % (b(8), b(0x18), b(0x1c), b(0x10), b(0x14), o.read(this + 8 + idx * 0xb0 + 0xc, 1)[0]))
        ts, tl = o.rd32(this + 0x59a0), o.rd32(this + 0x59bc)
        words = [o.rd32(this + off) for off in (0x58cc, 0x5840, 0x5844, 0x58d0, 0x58d4, 0x588c)]
        for ti in (ts, tl):
            if ti < 130: words += [o.rd32(this + 8 + ti * 0xb0 + off) for off in (0x18, 0x1c, 0x20, 0x24)]
        lines.append('MW ' + ' '.join('%08x' % w for w in words))
    P0 = (0, 0, 0)
    if mode == 0:   # dokun-birak
        ins = (sxp, syp)
        frame([(*ins, 1)], P0); frame([(*ins, 0)], P0)
        frame([(f32(random.random()), f32(random.random()), 0)], P0)
        frame([(*ins, 0)], P0); frame([(*ins, 2)], P0); frame([], P0)
    elif mode == 1: # anlik dokunus + surukleme
        ins = (sxp, syp)
        frame([(*ins, 3)], P0); frame([], P0)
        frame([(*ins, 1)], P0)
        for _ in range(3): frame([(f32(random.random()), f32(random.random()), 0)], P0)
        frame([(f32(random.random()), f32(random.random()), 2)], P0); frame([], P0); frame([], P0); frame([], P0)
    elif mode == 2: # gamepad
        frame([], (3, 0, 0)); frame([], (0, 3, 0)); frame([], (0, 0, 3)); frame([], P0)
        frame([], (1, 0, 0)); frame([], (0, 1, 0)); frame([], (0, 0, 1)); frame([], (2, 2, 2)); frame([], (0, 2, 0)); frame([], (0, 0, 2)); frame([], P0)
    else:           # rastgele
        for _ in range(8):
            n = random.randint(0, 2)
            ts = [((sxp if random.random() < .5 else f32(random.random())), (syp if random.random() < .5 else f32(random.random())), random.choice([0, 1, 2, 3])) for _ in range(1)][:n]
            frame(ts, (random.randint(0, 3), random.randint(0, 3), random.randint(0, 3)))
# sayi: buton sayisi
this, dev, sysd, loc = A[:4]
o.wf(dev + 4, 1.7777778); o.wf(dev + 0xc, 1.0)
o.call(G + 'GenericGUIManager()', this); o.call(LOAD, this, sysd, loc, dev, 0, 0, 480, 320)
nb = o.rd32(this + 4)
print('buton', nb)
for i in range(nb):
    for mode in range(4):
        scenario(i, (1.7777778, 1.3333334, 2.1666667)[(i + mode) % 3], mode)
open(out, 'w').write('\n'.join(lines) + '\n')
print(len(lines), 'satir')
