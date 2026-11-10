// MenuApp: menu akisi (ana menu -> kayit -> zorluk -> [oyun yer tutucu: baslangic menusu]).
// Orijinal Game::updateStateMain/Credits/Savegame/DifficultyLevel gecislerinin menu kismi. Oyun cekirdegi hazir olunca
// "oyun" yer tutucusu gercek Game durumuyla degisecek.
#pragma once
#include "game/GenericGUIManager.h"
#include "game/SaveGames.h"
#include "game/GameState.h"
#include <functional>

namespace fs12 {

class InputDevice;

class MenuApp {
public:
    MenuApp(GuiRenderDevice* dev, const GenericGUIManager::Textures& tex, int language);
    void update(InputDevice* in, float dt);
    void render();
    void renderHud(const GuiHudCtl& hud);   // oyun ici ust cubuk (orijinal renderBaseMenu)
    const GuiMenuCtl& ctl() const { return m_ctl; }
    GenericGUIManager& gui() { return m_gui; }
    // nextState tuketicisi (testler icin acik)
    void handleNext(uint32_t ns);
    // kayit yuvalari (ana kayit dosyasi); kayit menusu acilirken para/saat bilgisi buradan alinir
    void setSaves(SaveGames* s) { m_saves = s; }
    // oyun ici: true iken dunya cizilir (menu gizli); duraklatinca baslangic menusu acilir
    bool inGame() const { return m_playing && !m_paused; }
    bool playing() const { return m_playing; }
    // oyuncu durumu (para/araclar); degisince onStateChanged cagrilir (kalici kayit icin)
    GameState& state() { return m_state; }
    std::function<void()> onStateChanged;
    void setShopTexture(uint32_t texId);      // katalog ikon atlasi (0: orijinal karusel)
    void startGame();
    void pauseGame();
private:
    void enter(uint32_t guiState);
    void handleSaveEvent(uint32_t ns);
    void refreshSlots();
    GuiRenderDevice* m_dev;
    GenericGUIManager m_gui;
    GuiMenuCtl m_ctl;
    GuiPad m_pad;
    GuiCallbacks m_cb;
    int m_lang;
    float m_age = 0.f;
    SaveGames* m_saves = nullptr;
    bool m_playing = false, m_paused = false;
    int m_slot = 0, m_sub = 0, m_difficulty = 0;
    GameState m_state;
    uint32_t m_shopTex = 0;
    void fillShop();
};

} // namespace fs12
