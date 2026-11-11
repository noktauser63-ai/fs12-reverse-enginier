#include "game/MenuApp.h"
#include "data/Credits.h"
#include "platform/InputDevice.h"
#include <cstdio>

namespace fs12 {

MenuApp::MenuApp(GuiRenderDevice* dev, const GenericGUIManager::Textures& tex, int language) : m_dev(dev), m_lang(language) {
    m_gui.load(dev, tex);
    m_ctl.state = 2;
    m_ctl.tapPrompt = true;
}

void MenuApp::enter(uint32_t s) {
    m_ctl.state = s;
    m_ctl.dialog = 0;
    if (s == 3) m_gui.wrf(0x588c, 9.0f);       // kredi kaydirma baslangici
    if (s == 7) m_gui.wr(0x58cc, 0);           // baslangic menusu ana sayfa
    if (s == 4) { m_sub = 0; refreshSlots(); }   // kayit menusu: yuva bilgileri
}

// Orijinal Game::updateStateSavegame: 10..12 yuva (dolu: yukle, bos: zorluk menusu), 13..15 sil (onay diyalogu), 6/7 evet/hayir.
void MenuApp::handleSaveEvent(uint32_t ns) {
    auto resetDialog = [&] { m_sub = 0; m_ctl.dialog = 0; m_ctl.dlg = GuiDialogCtl(); };
    if (m_sub == 1) {                        // silme onayi
        if (ns == 6) {
            if (m_saves) m_saves->startTask(SaveGames::CMD_DELETE, (uint32_t)m_slot);
            resetDialog(); refreshSlots();
        } else if (ns == 7) resetDialog();
        return;
    }
    if (ns == 9) { enter(2); return; }
    if (ns >= 10 && ns <= 12) {
        m_slot = (int)ns - 10;
        if (m_ctl.saveUsed[m_slot]) {
            if (m_saves) m_saves->startTask(SaveGames::CMD_LOAD, (uint32_t)m_slot);
            startGame();
        } else enter(5);
    } else if (ns >= 13 && ns <= 15) {
        m_slot = (int)ns - 13;
        m_sub = 1;
        m_ctl.dialog = 1;
        m_ctl.dlg = GuiDialogCtl();
        m_ctl.dlg.count = 1; m_ctl.dlg.id[0] = 0x4f;
    }
}

void MenuApp::refreshSlots() {
    if (!m_saves) return;
    for (int i = 0; i < 3; ++i) {
        m_ctl.saveUsed[i] = m_saves->slotUsed(i);
        m_ctl.saveMoney[i] = m_saves->slotData(i).money;
        m_ctl.saveHours[i] = m_saves->slotData(i).hours;
        m_ctl.saveMinutes[i] = m_saves->slotData(i).minutes;
    }
}

// ns: updateGuiButtons / update* tarafindan yazilan nextState
void MenuApp::handleNext(uint32_t ns) {
    if (!ns) return;
    switch (m_ctl.state) {
    case 2: if (ns == 0x24) enter(4); else if (ns == 0x25) enter(3); break;
    case 3: if (ns == 0x25) enter(2); break;
    case 4: handleSaveEvent(ns); break;
    case 5:
        if (ns == 9) enter(4);
        else if (ns >= 0x10 && ns <= 0x12) {   // zorluk secildi: yeni oyun gorevi (orijinal updateStateDifficultyLevel)
            m_difficulty = ns - 0x10;
            if (m_saves) m_saves->startTask(SaveGames::CMD_NEW, (uint32_t)m_slot);
            startGame();
        }
        break;
    case 7:
        if (m_shopTex && (ns == 0x19 || ns == 0x1a || ns == 0x1c || ns == 0x1e)) {   // dukkan: 0x19 al, 0x1a sat, 0x1e sur (secili oge = karusel indeksi)
            const int slot = (int)m_gui.rd(0x583c);
            bool ok = ns == 0x1a ? m_state.sell(slot) : ns == 0x1e ? m_state.select(slot) : m_state.buy(slot);
            if (ok && onStateChanged) onStateChanged();
            fillShop(); m_ctl.nextState = 0; break;
        }
        if (ns == 9 && m_playing) { m_paused = false; }                       // geri: oyuna don
        else if (ns == 5 || ns == 8 || ns == 9) { m_playing = false; m_paused = false; enter(2); }
        break;
    default: break;
    }
}

void MenuApp::setShopTexture(uint32_t t) { m_shopTex = t; fillShop(); }

// Dukkan karuselini katalogdan doldurur (ad, aciklama, fiyat, sahiplik, bakiye, dugme durumlari).
void MenuApp::fillShop() {
    GuiShopCtl& sh = m_ctl.shop;
    const int N = m_shopTex ? GameState::shopCount() : 18;
    if (!m_shopTex) { sh.n = 18; sh.extTex = 0; return; }
    sh.n = (uint32_t)N; sh.extTex = m_shopTex;
    size_t cnt; const VehicleSpec* v = vehicleSpecs(cnt);
    static char desc[32][128];
    for (int s = 0; s < N && s < 32; ++s) {
        const int i = GameState::specIndex(s); const VehicleSpec& sp = v[i];
        sh.name[s] = sp.name; sh.price[s] = (uint32_t)sp.price; sh.count[s] = m_state.owned[i] ? 1 : 0;
        char* d = desc[s];
        if (sp.kind == VKind::Combine) snprintf(d, 128, "%s | %.0f PS | %.0f km/h | %.1f m | %.0f L", sp.brand, sp.ps, sp.topKmh, sp.headerM, sp.tankL);
        else if (sp.kind == VKind::Planter || sp.kind == VKind::Cultivator) snprintf(d, 128, "%s | %.1f m", sp.brand, sp.headerM);
        else snprintf(d, 128, "%s | %.0f PS | %.0f Nm | %.0f km/h", sp.brand, sp.ps, sp.torqueNm, sp.topKmh);
        sh.desc[s] = d;
        sh.canUse[s] = m_state.owned[i] && m_state.active != i && (sp.kind == VKind::Tractor || sp.kind == VKind::Combine || sp.kind == VKind::Pickup);
        sh.canA[s] = m_state.canBuy(s); sh.canB[s] = m_state.canSell(s);
        sh.badgeA[s] = sh.badgeB[s] = sh.badgeC[s] = 0;
        sh.ext[s] = {0.175f, 0.225f, (float)((s % 4) * 256) / 1024.f, (float)((s / 4) * 219) / 1024.f, 256.f / 1024.f, 219.f / 1024.f};
        m_ctl.slotUsed[s] = 0;
    }
    sh.useLabel = m_lang == 10 ? "S\xc3\xbcr" : m_lang == 1 ? "Fahren" : "Drive";
    sh.money = (uint32_t)(m_state.money < 0 ? 0 : m_state.money);
}

void MenuApp::startGame() { m_playing = true; m_paused = false; enter(7); }
void MenuApp::pauseGame() { if (m_playing) { m_paused = true; enter(7); } }

void MenuApp::update(InputDevice* in, float dt) {
    if (inGame()) {
        float tx, ty;
        if (in->backKeyEdge()) { pauseGame(); return; }
        if (in->getTouchUp(tx, ty) && ty < 0.16f) {
            if (tx > 0.90f) pauseGame();                                          // sag ust: menu (buton 1, id 7)
            else if (tx < 0.10f) { pauseGame(); m_gui.wr(0x58cc, 1); m_gui.wr(0x5840, 0); m_gui.wr(0x5844, 0); }   // sol ust: market (buton 0, id 6)
        }
        return;
    }
    m_age += dt;
    if (m_age > 1.0f) m_ctl.menuActive = true;
    m_pad.pressed = in->buttonsPressed(); m_pad.held = in->buttonsHeld(); m_pad.released = in->buttonsReleased();
    fillShop();
    m_gui.update(m_ctl, in, m_pad, m_cb, dt);
    handleNext(m_ctl.nextState);
    // geri tusu: alt menulerde bir ust menuye
    if (in->backKeyEdge()) {
        if (m_ctl.state == 4 || m_ctl.state == 3) enter(2);
        else if (m_ctl.state == 5) enter(4);
        else if (m_ctl.state == 7) { if (m_gui.rd(0x58cc) != 0) m_gui.wr(0x58cc, 0); else if (m_playing) m_paused = false; else enter(2); }
    }
}

void MenuApp::renderHud(const GuiHudCtl& hud) {
    // oyun ici ust butonlar: 0 = market (id 6), 1 = menu (id 7) -- orijinal updateBaseMenu listesi
    m_gui.wr(0x59a8, 2); m_gui.wr(0x59ac, 0); m_gui.wr(0x59b0, 1);
    GuiMenuCtl c = m_ctl; c.state = 6; c.hud = hud;
    m_gui.renderBaseMenu(c, m_lang);
}
void MenuApp::render() {
    if (inGame()) return;
    // Google Play satin alma kaldirildi: market sayfasindaki "Buycoin" dugmesi gizlenir (flags |= 0x80)
    if (m_ctl.state == 7 && m_gui.rd(0x58cc) == 1 && !m_shopTex) { const uint32_t o = m_gui.rd(0x581c) * 0xb0 + 0xc; m_gui.wr(o, m_gui.rd(o) | 0x80); }
    m_gui.render(m_ctl, m_lang, kCreditLines);
}

} // namespace fs12
