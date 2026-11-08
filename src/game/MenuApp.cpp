#include "game/MenuApp.h"
#include "data/Credits.h"
#include "platform/InputDevice.h"

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
        if (ns == 9 && m_playing) { m_paused = false; }                       // geri: oyuna don
        else if (ns == 5 || ns == 8 || ns == 9) { m_playing = false; m_paused = false; enter(2); }
        break;
    default: break;
    }
}

void MenuApp::startGame() { m_playing = true; m_paused = false; enter(7); }
void MenuApp::pauseGame() { if (m_playing) { m_paused = true; enter(7); } }

void MenuApp::update(InputDevice* in, float dt) {
    if (inGame()) {
        float tx, ty;
        if (in->backKeyEdge() || (in->getTouchUp(tx, ty) && tx > 0.92f && ty < 0.1f)) pauseGame();
        return;
    }
    m_age += dt;
    if (m_age > 1.0f) m_ctl.menuActive = true;
    m_pad.pressed = in->buttonsPressed(); m_pad.held = in->buttonsHeld(); m_pad.released = in->buttonsReleased();
    m_gui.update(m_ctl, in, m_pad, m_cb, dt);
    handleNext(m_ctl.nextState);
    // geri tusu: alt menulerde bir ust menuye
    if (in->backKeyEdge()) {
        if (m_ctl.state == 4 || m_ctl.state == 3) enter(2);
        else if (m_ctl.state == 5) enter(4);
        else if (m_ctl.state == 7) { if (m_gui.rd(0x58cc) != 0) m_gui.wr(0x58cc, 0); else if (m_playing) m_paused = false; else enter(2); }
    }
}

void MenuApp::render() {
    if (inGame()) return;
    // Google Play satin alma kaldirildi: market sayfasindaki "Buycoin" dugmesi gizlenir (flags |= 0x80)
    if (m_ctl.state == 7 && m_gui.rd(0x58cc) == 1) { const uint32_t o = m_gui.rd(0x581c) * 0xb0 + 0xc; m_gui.wr(o, m_gui.rd(o) | 0x80); }
    m_gui.render(m_ctl, m_lang, kCreditLines);
}

} // namespace fs12
