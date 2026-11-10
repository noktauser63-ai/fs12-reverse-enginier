// GenericGUIManager::updateGuiButtons — dokunma/gamepad durum makinesi (orijinal akisla).
#include "GenericGUIManager.h"
#include "ModBridge.h"
#include "../platform/InputDevice.h"
#include <cfloat>
#include <cmath>
#include <cstring>
namespace { inline float bitsF(unsigned u) { float f; memcpy(&f, &u, 4); return f; } }

namespace fs12 {

static inline float bf(const GuiButton& b, uint32_t off) { float v; memcpy(&v, (const uint8_t*)&b + off, 4); return v; }
static inline void setbf(GuiButton& b, uint32_t off, float v) { memcpy((uint8_t*)&b + off, &v, 4); }

bool GenericGUIManager::updateGuiButtons(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, uint32_t count, const uint32_t* ids) {
    bool any = false;
    auto click = [&]() { if (cb.playClick) cb.playClick(cb.user, ctl.soundOn); };
    for (uint32_t n = count; n != 0; --n) {
        int idx = (int)ids[n - 1];
        GuiButton& b = button(idx);
        uint32_t fl = b.flags;
        if (fl & 2) continue;
        uint32_t st = (uint32_t)b.state;
        int id = b.id;
        float cx = b.x, cy = b.y;
        float tx = 0.0f, ty = 0.0f;
        auto touchPos = [&](const InputDevice::Touch* t) {
            tx = t->x; ty = t->y;
            m_scale.undo(tx, ty, b.align);
        };
        // ---- durum guncelleme ----
        bool done = false;   // LAB_0003f844'e atla
        if (st == 0) {
            if (!(fl & 0x40)) {
                for (int i = 0; i < in->numTouches(); ++i) {
                    const InputDevice::Touch* t = in->touch(i);
                    if (!(t->flags & 1)) continue;
                    touchPos(t);
                    if (b.isInsidePick(tx, ty)) {
                        in->disableTouchForGestures((unsigned)i);
                        st = (t->flags & 2) ? 3 : 1;
                        b.touchId[0] = (uint32_t)t->id; b.touchId[1] = (uint32_t)(t->id >> 32);
                        break;
                    }
                }
                if (id == 0x1c || id == 0x2d) {
                    if (pad.pressed & 2) { pad.pressed &= ~2u; st = 1; b.pressedLatch = 1; }
                } else if (id == 0x1d && (pad.pressed & 1)) {
                    pad.pressed &= ~1u; st = 1; b.pressedLatch = 1;
                }
            }
        } else {
            bool touchPath = true;
            if (b.pressedLatch) {
                if (id == 0x1c || id == 0x2d || id == 0x1d) {
                    uint32_t bit = (id == 0x1d) ? 1u : 2u;
                    if (pad.held & bit) {
                        if (st == 1) st = 2;
                        touchPath = false; done = true;
                    } else if (pad.released & bit) {
                        pad.released &= ~bit; b.pressedLatch = 0; st = 3;
                        touchPath = false; done = true;
                    }
                }
            }
            if (touchPath) {
                uint64_t tid = (uint64_t)b.touchId[0] | ((uint64_t)b.touchId[1] << 32);
                const InputDevice::Touch* t = in->touchById(tid);
                if (!t) { b.touchId[0] = b.touchId[1] = 0; st = 0; }
                else {
                    touchPos(t);
                    if (b.isInsidePick(tx, ty)) st = (t->flags & 2) ? 3 : 2;
                    else if ((fl & 0x21) == 0) { b.touchId[0] = b.touchId[1] = 0; st = 0; }
                }
            }
        }
        (void)done;
        // ---- LAB_0003f844 ----
        if (fl & 1) {
            // surukleme/yakalama dugmesi
            if (st - 1 < 2) { cx = tx; cy = ty; }
            else if (b.pointCount != 0) {
                float best = FLT_MAX; int bi = 0; float bdx = 0, bdy = 0;
                for (uint32_t i = 0; i < b.pointCount; ++i) {
                    float dx = b.points[i][0] - cx, dy = b.points[i][1] - cy;
                    float d = dx * dx + dy * dy;
                    if (d < best) {
                        if (dy >= -0.001f || !(fl & 0x10)) { best = d; bi = (int)i; bdx = dx; bdy = dy; }
                    }
                }
                if (bdx > -0.005f && bdx < 0.005f) cx = b.points[bi][0]; else cx = cx + bdx * 0.25f;
                if (bdy > -0.005f && bdy < 0.005f) cy = b.points[bi][1]; else cy = cy + bdy * 0.25f;
            }
            { float lo = b.pick[0]; if (cx < lo) cx = lo; else { float hi = b.pick[2]; if (cx > hi) cx = hi; } }
            { float lo = b.pick[1]; if (cy < lo) cy = lo; else { float hi = b.pick[3]; if (cy > hi) cy = hi; } }
        } else if (st != 3) {
            if (st - 1 < 2) {
                if (id == 4) {
                    GuiButton& tr = button((int)rd(0x59a0));
                    float lo = tr.pick[0], v = tx;
                    if (tx < lo) v = lo; else { float hi = tr.pick[2]; if (tx > hi) v = hi; }
                    tr.x = v;
                } else if (id == 10) {
                    ctl.nextState = 3;
                } else if (id == 2) {
                    GuiButton& tr = button((int)rd(0x59bc));
                    float v = ty, lo = tr.pick[1];
                    if (ty < lo) v = lo; else { float hi = tr.pick[3]; if (ty > hi) v = hi; }
                    tr.y = v;
                }
            }
        } else {
            // birakildi: islev
            auto ns = [&](uint32_t v) { ctl.nextState = v; };
            auto searchList = [&](uint32_t base, int cnt, uint32_t newState) {
                for (int i = 0; i < cnt; ++i)
                    if (rd(base + 4 * i) == (uint32_t)idx) { ctl.nextArg = (uint32_t)i; ns(newState); return; }
            };
            switch (id) {
            case 6: ns(0x18); wr(0x58cc, 1); wr(0x5840, 0); wr(0x5844, 0); break;
            case 7: ns(0x18); wr(0x58cc, 0); break;
            case 8: ns(4); break;
            case 9: pad.request = 0x400; break;
            case 0xb: ns(2); break;
            case 0xc: case 0xd: case 0xe: case 0xf: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
                ns((uint32_t)id - 2); break;
            case 0x15: case 0x1f: ns(0x13); break;
            case 0x16: wr(0x58cc, 2); click(); break;
            case 0x17: wr(0x58cc, 3); click(); break;
            case 0x18: wr(0x58d4, rd(0x58d4) + 1); click(); break;
            case 0x19: wr(0x58d4, rd(0x58d4) - 1); click(); break;
            case 0x1a: wr(0x58cc, 5); click(); break;
            case 0x1b: ns(0x14); break;
            case 0x1c: ns(6); break;
            case 0x1d: ns(7); break;
            case 0x1e: wr(0x58cc, 0); wr(0x58d0, 0); click(); break;
            // NOT: orijinal ikili burada kayitli-yuva dizinini bayt ofseti gibi okur (this+slot, hizasiz) — davranis aynen korunur
            case 0x20: ns(ctl.slotUsed[rd(0x583c) & 255] ? 0x1c : 0x19); ctl.nextArg = rd(rd(0x583c)); break;
            case 0x21:
                ns(0x1a);
                ctl.nextArg = ctl.slotUsed[rd(0x583c) & 255] ? rd(0x1d) : rd(0x583c);
                break;
            case 0x22: ns(0x1b); ctl.nextArg = rd(0x583c); break;
            case 0x23: ns(0x1e); break;
            case 0x24: ns(ctl.flag12f4 ? 0x2f : 0x2e); break;
            case 0x25: ns(ctl.flag12f5 ? 0x31 : 0x30); break;
            case 0x26: ns(ctl.flag12f6 ? 0x33 : 0x32); break;
            case 0x27: ns(0x2b); break;
            case 0x28: ns(0x2a); break;
            case 0x29: ns(0x29); break;
            case 0x2a: ns(0x28); break;
            case 0x2b: ns(0x2d); break;
            case 0x2c: ns(0x2c); break;
            case 0x2d: ns(8); break;
            case 0x2e: pad.request = 0x800; click(); break;
            case 0x2f: ns(5); break;
            case 0x30: {
                pad.request = 0x10;
                GuiButton& tr = button((int)rd(0x59bc));
                tr.y = rdf(0x5914);
                break; }
            case 0x31: searchList(0x5bdc, 0x11, 0x21); break;
            case 0x32: searchList(0x5c20, 0x11, 0x22); break;
            case 0x33: searchList(0x5bb0, 0xb, 0x23); break;
            case 0x34: ns(0x25); wrf(0x588c, 9.0f); break;
            case 0x35: wr(0x58cc, 4); wr(0x58d0, 0); click(); break;
            case 0x36: wr(0x58cc, 6); click(); break;
            case 0x37: wr(0x58cc, 7); click(); break;
            case 0x38: case 0x39: {
                click();
                const char* url = id == 0x38 ? "http://www.farming-simulator.com/mobile/fs2012/fs2012ManualAndroid.php"
                                             : "http://www.farming-simulator.com/mobile/fs2012/fs2012SupportAndroid.php";
                bool ok = cb.openUrl ? cb.openUrl(cb.user, url) : false;
                if (!ok) ns(0x20);
                break; }
            case 0x3a: ns(0x15); break;
            case 0x3b: ns(9); break;
            case 0x3c: if (cb.buyCoins) cb.buyCoins(cb.user, 0); break;
            case 0x3d: if (cb.buyCoins) cb.buyCoins(cb.user, 1); break;
            case 0x3e: if (cb.buyCoins) cb.buyCoins(cb.user, 2); break;
            default: break;
            }
        }
        b.x = cx; b.y = cy; b.state = (int32_t)st;
        if (st != 0) any = true;
    }
    return any;
}


// ---- ust duzey menu guncellemeleri ----
void GenericGUIManager::updateMainMenu(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    if (!ctl.menuActive) return;
    updateGuiButtons(ctl, in, pad, cb, rd(0x5a28), (const uint32_t*)(m_raw + 0x5a2c));
    float x, y;
    if (in->getTouchUp(x, y)) ctl.nextState = 0x24;
}

void GenericGUIManager::updateCreditsMenu(GuiMenuCtl& ctl, InputDevice* in, GuiPad&, const GuiCallbacks&, float dt) {
    wrf(0x588c, rdf(0x588c) - dt);
    float x, y;
    if (in->getTouchUp(x, y)) ctl.nextState = 0x25;
}

void GenericGUIManager::updateSaveGameMenu(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    wr(0x5a3c, 4);
    if (ctl.saveUsed[0]) { wr(0x5a50, rd(0x5a68)); wr(0x5a3c, 5); }
    if (ctl.saveUsed[1]) { uint32_t n = rd(0x5a3c); wr(0x5a40 + 4 * n, rd(0x5a6c)); wr(0x5a3c, n + 1); }
    if (ctl.saveUsed[2]) { uint32_t n = rd(0x5a3c); wr(0x5a40 + 4 * n, rd(0x5a70)); wr(0x5a3c, n + 1); }
    updateGuiButtons(ctl, in, pad, cb, rd(0x5a3c), (const uint32_t*)(m_raw + 0x5a40));
}

void GenericGUIManager::updateStartMenuHelp(GuiMenuCtl& ctl, InputDevice* in, GuiPad&, const GuiCallbacks& cb) {
    float x, y;
    if (in->getTouchUp(x, y)) {
        if (cb.playClick) cb.playClick(cb.user, ctl.soundOn);
        uint32_t n = rd(0x58d0) + 1;
        wr(0x58d0, n);
        if (n > 2) wr(0x58cc, 0);
    }
}

void GenericGUIManager::updateDifficultyLevelMenu(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    updateGuiButtons(ctl, in, pad, cb, rd(0x5b10), (const uint32_t*)(m_raw + 0x5b14));
}

void GenericGUIManager::updateStartMenuMain(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    updateGuiButtons(ctl, in, pad, cb, 9, (const uint32_t*)(m_raw + 0x5a74));
}

void GenericGUIManager::updateStartMenuSettings(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    updateGuiButtons(ctl, in, pad, cb, 8, (const uint32_t*)(m_raw + 0x5af0));
}

// 0 ana, 1 market, 2 istatistik, 3 fiyatlar, 4 yardim, 5 ayarlar (sayfa = this+0x58cc). 1-3 henuz portlanmadi.
void GenericGUIManager::updateStartMenu(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt) {
    switch (rd(0x58cc)) {
    case 0: updateStartMenuMain(ctl, in, pad, cb); break;
    case 1: updateStartMenuShop(ctl, in, pad, cb, dt); break;
    case 2: updateGuiButtons(ctl, in, pad, cb, rd(0x5a98), (const uint32_t*)(m_raw + 0x5a9c)); break;   // istatistik
    case 3: updateGuiButtons(ctl, in, pad, cb, rd(0x5ac4), (const uint32_t*)(m_raw + 0x5ac8)); break;   // fiyatlar
    case 4: updateStartMenuHelp(ctl, in, pad, cb); break;
    case 5: updateStartMenuSettings(ctl, in, pad, cb); break;
    default: break;   // TODO: market/istatistik/fiyatlar
    }
}

void GenericGUIManager::updateDialog(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb) {
    wr(0x591c, 0);
    switch (ctl.dialog) {
    case 3: wr(0x5920, rd(0x5954)); wr(0x591c, 1); break;
    case 4: wr(0x5920, rd(0x5950)); wr(0x5924, rd(0x5958)); wr(0x5928, rd(0x595c)); wr(0x592c, rd(0x5960)); wr(0x591c, 4); break;
    case 1: wr(0x5920, rd(0x5948)); wr(0x5924, rd(0x594c)); wr(0x591c, 2); break;
    default: break;
    }
    button((int)rd(0x59bc)).y = rdf(0x5914);
    pad.word0 = 0;
    updateGuiButtons(ctl, in, pad, cb, rd(0x591c), (const uint32_t*)(m_raw + 0x5920));
}

// Orijinal GenericGUIManager::update. Henuz portlanmayan menuler (zorluk, taban, baslangic alt-menuleri) 'todo' kancalari ile birakildi.
void GenericGUIManager::update(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt) {
    if (modBridge().updateBefore) modBridge().updateBefore(this);
    if (ctl.eventAck != 0) {
        if (ctl.eventAck == 1 && ctl.state == 7) wr(0x58cc, 3);
        ctl.eventAck = 0;
    }
    ctl.nextState = 0;
    pad.request = 0; pad.r18 = 0; pad.r1c = 0;
    wr(0x5c74, ctl.state); wr(0x5c78, ctl.dialog);
    if (ctl.dialog == 0) {
        switch (ctl.state) {
        case 2: updateMainMenu(ctl, in, pad, cb); break;
        case 3: updateCreditsMenu(ctl, in, pad, cb, dt); break;
        case 4: updateSaveGameMenu(ctl, in, pad, cb); break;
        case 5: updateDifficultyLevelMenu(ctl, in, pad, cb); break;
        case 7: updateStartMenu(ctl, in, pad, cb, dt); break;
        default: break;  // TODO: 6 taban menusu
        }
    } else {
        updateDialog(ctl, in, pad, cb);
    }
    float ph = rdf(0x5c68) + dt;
    wrf(0x5c68, ph);
    while (rdf(0x5c68) > 0.6f) wrf(0x5c68, rdf(0x5c68) - 1.0f);
    if (modBridge().updateAfter) modBridge().updateAfter(this);
}

} // namespace fs12

namespace fs12 {
// Orijinal updateStartMenuShop (decompile'dan): butonlari etkinlestirir, karuseli parmakla surukler,
// birakinca en yakin ogeye atalet + yay ile oturtur. Ham float islem sirasi korunmustur.
void GenericGUIManager::updateStartMenuShop(GuiMenuCtl& ctl, InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt) {
    const GuiShopCtl& sh = ctl.shop;
    int sel = (int)rd(0x583c);
    GuiButton& b1 = button((int)rd(0x5814));
    GuiButton& b2 = button((int)rd(0x5818));
    if (!sh.canB[sel]) b1.flags |= 2u; else b1.flags &= ~2u;
    if (!sh.canA[sel]) b2.flags |= 2u; else b2.flags &= ~2u;
    if (ctl.slotUsed[sel]) { b1.flags &= ~2u; b2.flags &= ~2u; }
    if (sh.extTex) { GuiButton& b3 = button((int)rd(0x581c)); if (sh.canUse[sel]) b3.flags &= ~2u; else b3.flags |= 2u; }
    updateGuiButtons(ctl, in, pad, cb, rd(0x5810), (const uint32_t*)(m_raw + 0x5814));

    float scroll = rdf(0x5840), vel = rdf(0x5844);
    bool done = false;   // LAB_00041f36
    for (int i = 0; i < in->numTouches() && !done; ++i) {
        const InputDevice::Touch* t = in->touch(i);
        if (t->flags & 2) continue;
        if (!(t->y < bitsF(0x3f0ccccd) && t->y > bitsF(0x3dcccccd))) continue;   // 0x3f0ccccd / 0x3dcccccd
        float d = t->x - t->prevX;
        float delta, v;
        if (!(fabsf(d) > bitsF(0x3b03126f))) {   // 0x3b03126f
            if (t->time == t->prevTime) { done = true; break; }
            delta = 0.0f; v = 0.0f;
        } else {
            delta = d * 1.96f;        // 0x3ffae148
            const float dtt = (float)(t->time - t->prevTime);
            v = delta / dtt;
        }
        scroll = scroll + delta;
        if (v < -20.0f) v = -20.0f; else if (v > 20.0f) v = 20.0f;
        vel = v;
        done = true;
        wrf(0x5840, scroll); wrf(0x5844, vel);
        goto normalize;
    }
    if (!done) {
        const float k14 = bitsF(0x3fb33333);
        if (!(fabsf(vel) <= k14)) {
            if (!(vel > k14)) {
                if (vel < -k14) {
                    float n = vel + dt * 4.0f;
                    if (!(n <= -k14)) n = -k14;
                    vel = n;
                }
            } else {
                float n = vel - dt * 4.0f;
                if (!(n >= k14)) n = k14;
                vel = n;
            }
            wrf(0x5844, vel);
            scroll = scroll + dt * vel;
            wrf(0x5840, scroll);
        } else {
            float hi = vel > 0 ? bitsF(0x3e99999a) : bitsF(0x3f333333);
            float lo = vel < 0 ? bitsF(0xbe99999a) : bitsF(0xbf333333);
            bool hit; float tgt;
            if (scroll > hi) {
                float n = vel + ((1.0f - scroll) * 35.0f) * dt;
                if (!(n <= k14)) n = k14;
                vel = n; scroll = scroll + vel * dt; tgt = 1.0f; hit = scroll >= 1.0f;
            } else if (scroll < lo) {
                float n = vel - ((scroll + 1.0f) * 35.0f) * dt;
                if (!(n >= -k14)) n = -k14;
                vel = n; scroll = scroll + vel * dt; tgt = -1.0f; hit = scroll <= -1.0f;
            } else if (scroll > 0) {
                float n = vel - (scroll * 35.0f) * dt;
                if (!(n >= -k14)) n = -k14;
                vel = n; scroll = scroll + vel * dt; tgt = 0.0f; hit = scroll <= 0.0f;
            } else {
                float n = vel - (scroll * 35.0f) * dt;
                if (!(n <= k14)) n = k14;
                vel = n; scroll = scroll + vel * dt; tgt = 0.0f; hit = scroll >= 0.0f;
            }
            wrf(0x5844, vel); wrf(0x5840, scroll);
            if (hit) { wrf(0x5840, tgt); wrf(0x5844, 0.0f); scroll = tgt; }
        }
    }
normalize:
    sel = (int)rd(0x583c);
    scroll = rdf(0x5840);
    while (scroll <= -1.0f) { --sel; if (sel < 0) sel = (int)sh.n - 1; scroll = scroll + 1.0f; }
    while (scroll >= 1.0f) { ++sel; if (sel > (int)sh.n - 1) sel = 0; scroll = scroll - 1.0f; }
    wr(0x583c, (uint32_t)sel); wrf(0x5840, scroll);
}
} // namespace fs12
