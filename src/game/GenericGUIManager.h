// GenericGUIManager: oyun ici arayuz (menu, dugmeler, yazi). Orijinal sinifin yeniden yazimi.
// Nesne duzeni (ofsetler) orijinal ile ayni tutuldu: baslangic durumu orijinal load() sonrasi goruntuden gelir,
// boylece 108 butonun tum verisi birebir korunur. Alan adlari cizim/guncelleme kodu yazildikca eklenir.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "game/GuiLayout.h"

namespace fs12 {

// Cizim cihazi arayuzu (GLESHandheldRenderDevice'in GUI'nin kullandigi kismi). Ana makine testinde iz kaydeder.
class GuiRenderDevice {
public:
    virtual ~GuiRenderDevice() {}
    virtual float aspect() const = 0;   // orijinal dev[4] * dev[0xc]
    virtual void setOverlayColor(const float* rgba) = 0;
    virtual void setTextColor(const float* rgba) = 0;
    virtual void setFontSize(float size) = 0;
    virtual void renderOverlay(uint32_t tex, float x, float y, float w, float h, float u, float v, float du, float dv) = 0;
    virtual void renderText(uint32_t font, float x, float y, uint32_t flags, const char* utf8) = 0;
    virtual void setNumberMonospace(bool, float) {}   // orijinal enqueueSetNumberMonospace (rakam genisligi 480 tabanli)
};

struct OverlayDesc { float w, h, u, v, du, dv; };   // 24 bayt (g_m_overlay* tablolari)

struct GuiButton {   // 0xb0 bayt
    int32_t id;            // 0x00 BUTTON_IDS
    uint32_t flags;        // 0x04 bit1 soluk, bit2 sag-hiza metin, bit3 doku yok, bit5 devre disi(2), bit7 gizli(0x80)...
    int32_t state;         // 0x08 0 bos, 1 basili, 2 tutuluyor, 3 birakildi
    uint8_t pressedLatch;  // 0x0c
    uint8_t pad0[3];
    uint32_t touchId[2];   // 0x10
    float x, y;            // 0x18
    float w, h;            // 0x20
    uint32_t align;        // 0x28 hizalama bayraklari (GuiLayout)
    uint32_t textAlign;    // 0x2c
    float textX, textY;    // 0x30
    float fontSize;        // 0x38
    uint32_t anchor;       // 0x3c
    float color[4];        // 0x40 rgba
    uint32_t tex;          // 0x50
    float u, v, du, dv;    // 0x54
    float pick[4];         // 0x64 secim alani x0,y0,x1,y1 (ekran orani)
    float margin[4];       // 0x74
    uint32_t pointCount;   // 0x84
    float points[4][2];    // 0x88
    uint32_t textRef;      // 0xa8 orijinalde char*; burada 1+dizin (GenericGUIManager::setButtonText), 0 = metin yok
    uint32_t pad;
    // GuiButton::isInsidePickAreaArea (orijinal float islem sirasi korunur)
    bool isInsidePick(float px, float py) const {
        float l = x - margin[0], t = y - margin[1];
        if (anchor & 2) l -= w * 0.5f; else if (anchor & 4) l -= w;
        if (anchor & 0x10) t -= h * 0.5f; else if (anchor & 0x20) t -= h;
        if (!(px >= l)) return false;
        float r = l + ((w + margin[0]) + margin[2]);
        if (!(px < r) || !(py >= t)) return false;
        float b = t + ((h + margin[1]) + margin[3]);
        return py < b;
    }
};
static_assert(sizeof(GuiButton) == 0xb0, "GuiButton 0xb0 bayt olmali");
static_assert(offsetof(GuiButton, align) == 0x28 && offsetof(GuiButton, color) == 0x40 && offsetof(GuiButton, pick) == 0x64 && offsetof(GuiButton, points) == 0x88 && offsetof(GuiButton, textRef) == 0xa8, "GuiButton alan ofsetleri");

// GUIInterfaceDesc'in kullanilan alanlari (orijinal ofsetler: +0x1310 sonraki durum, +0x1314 arguman ...)
struct GuiShopCtl {   // GUIInterfaceDesc market alanlari (+0x734..+0x108c, +0x12c, +0x1220/21 ...)
    bool flag1220 = false, flag1221 = false;
    uint8_t canA[18] = {}, canB[18] = {};                         // +0x5f0, +0x602 (buton etkin)
    uint8_t badgeA[18] = {}, badgeB[18] = {}, badgeC[18] = {};   // +0x734, +0x746, +0x758
    char alt[18][128] = {};                                       // +0x77c + slot*0x80
    const char* name[18] = {};                                    // +0x65c
    const char* desc[18] = {};                                    // +0x6a4
    uint32_t count[18] = {};                                      // +0x5a8
    uint32_t price[18] = {};                                      // +0x614
    uint32_t money = 0;                                           // +0x12c
};

struct GuiDialogCtl {   // GUIInterfaceDesc diyalog alanlari: +0x10 id[3], +0x1c metin[3], +0x28 satir sayisi, +0x107c kayit adlari
    uint32_t count = 0;
    int32_t id[3] = {-1, -1, -1};
    const char* text[3] = {};
    char slotName[3][128] = {};
};

struct GuiMenuCtl {
    bool soundOn = true;          // +0x00
    uint32_t state = 0;           // +0x04 aktif menu (2 ana, 3 jenerik, 4 kayit, 5 zorluk, 6 taban, 7 baslangic)
    uint32_t dialog = 0;          // +0x08 diyalog turu (0 yok)
    bool menuActive = false;      // +0x0c
    uint32_t eventAck = 0;        // +0x130c
    uint32_t nextState = 0;       // +0x1310
    uint32_t nextArg = 0;         // +0x1314
    int val12ec = 0, val12e8 = 0; // +0x12ec, +0x12e8 (yuzde degerleri)
    bool tapPrompt = false;       // +0x146 (ana menude 'dokun' istemi)
    bool flag12f4 = false, flag12f5 = false, flag12f6 = false;
    uint8_t saveUsed[3] = {};     // +0xfc..
    uint32_t saveMoney[3] = {}, saveHours[3] = {}, saveMinutes[3] = {};  // +0x100, +0x10c, +0x118
    GuiShopCtl shop;
    GuiDialogCtl dlg;
    uint8_t slotUsed[256] = {};   // +0x76a + manager[0x583c]
    uint32_t w1224[(0x12dc - 0x1224) / 4] = {};  // fiyat tablosu (+0x1224..) ve istatistikler (+0x1264..): ham sozcukler
    uint32_t statAt(uint32_t off) const { return w1224[(off - 0x1224) / 4]; }
    uint32_t priceAt(uint32_t off) const { return w1224[(off - 0x1224) / 4]; }
    float priceF(uint32_t off) const { float f; memcpy(&f, &w1224[(off - 0x1224) / 4], 4); return f; }
};
// Gamepad kenar/tutma maskeleri (orijinal giris cihazi +0x14,+0x20,+0x24,+0x28)
struct GuiPad {
    uint32_t word0 = 0;     // +0x00
    uint32_t request = 0;   // +0x14
    uint32_t r18 = 0, r1c = 0;  // +0x18, +0x1c
    uint32_t pressed = 0;   // +0x20
    uint32_t held = 0;      // +0x24
    uint32_t released = 0;  // +0x28
};
struct GuiCallbacks {
    void (*playClick)(void* user, bool soundOn) = nullptr;
    bool (*openUrl)(void* user, const char* url) = nullptr;
    void (*buyCoins)(void* user, int which) = nullptr;
    void* user = nullptr;
};
class InputDevice_fwd;

class GenericGUIManager {
public:
    static const int kStateBytes = 0x5c80;
    GenericGUIManager();
    // Orijinal load(): dokular verilir (sirayla font, guiElements, shopAlpha, shopBackground, gestures, map, title)
    struct Textures { uint32_t font, fontId, elements, shopAlpha, shopBg, gestures, map, title, logo, edition; };
    void load(GuiRenderDevice* dev, const Textures& t);

    // --- yerlesim ---
    void updateScreenScale() { m_scale.update(m_dev->aspect()); }
    const GuiScale& scale() const { return m_scale; }
    void renderOverlay(const OverlayDesc& d, float x, float y, uint32_t align, uint32_t anchor, uint32_t tex, float sw, float sh);
    void renderText(float x, float y, uint32_t align, uint32_t flags, const char* text);
    void renderOverlayF(float x, float y, float w, float h, float u, float v, float du, float dv, uint32_t align, uint32_t anchor, uint32_t tex);
    void renderStartMenuShop(const GuiMenuCtl& ctl, int language);
    void renderDialog(const GuiMenuCtl& ctl, int language);
    char m_dlgText[3][64] = {};
    void renderTitleScreen(float shade = 1.0f);
    void renderGuiButtons(const GuiRenderDevice*, uint32_t count, const uint32_t* ids, float wscale, float hscale);
    // loc: dil (0..9). desc: menu arayuz bayraklari
    void renderMainMenu(bool menuActive, bool tapPrompt, int language);
    // baslik cubugu: renderTitle(const char*)
    void renderTitle(const char* title);
    void renderStartMenuMain(int language);
    void renderStartMenuSettings(const GuiMenuCtl& ctl, int language);
    void renderStartMenuHelp(int language);
    void renderTitleYOffset(const char* text, float yOff);
    void renderStartMenuPrices(const GuiMenuCtl& ctl, int language);
    void renderTitle2(const char* left, const char* right);
    void renderStartMenuStatistics(const GuiMenuCtl& ctl, int language);
    void renderSaveGameMenu(const GuiMenuCtl& ctl, int language);
    static void formatNumber(char* out, uint32_t n);
    void renderCreditsMenu(const char* const* lines52);

    // --- durum erisimi (orijinal ofsetlerinde) ---
    GuiButton& button(int i);
    void setButtonText(int i, const char* t) { m_btnText[i] = t; button(i).textRef = t ? (uint32_t)(i + 1) : 0; }
    uint32_t buttonCount() const { return rd(4); }
    float phase() const { return rdf(0x5c68); }
    void setPhase(float p) { wrf(0x5c68, p); }
    uint32_t rd(uint32_t off) const { uint32_t v; memcpy(&v, m_raw + off, 4); return v; }
    float rdf(uint32_t off) const { float v; memcpy(&v, m_raw + off, 4); return v; }
    uint8_t rd8(uint32_t off) const { return m_raw[off]; }
    void wr8(uint32_t off, uint8_t v) { m_raw[off] = v; }
    void wr(uint32_t off, uint32_t v) { memcpy(m_raw + off, &v, 4); }
    void wrf(uint32_t off, float v) { memcpy(m_raw + off, &v, 4); }

    // Orijinal updateGuiButtons: ids[0..count) ters sirada islenir; en az bir dugme aktifse true
    bool updateGuiButtons(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, uint32_t count, const uint32_t* ids);

    // --- menu guncelleme (orijinal update / update* islevleri) ---
    void update(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt);
    void updateMainMenu(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void updateCreditsMenu(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt);
    void updateSaveGameMenu(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void updateStartMenuHelp(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void updateDifficultyLevelMenu(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void updateStartMenu(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt);
    void updateStartMenuShop(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb, float dt);
    void updateStartMenuMain(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void updateStartMenuSettings(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);
    void renderDifficultyLevelMenu(int language);
    void renderStartMenu(const GuiMenuCtl& ctl, int language, const char* const* credits52 = nullptr);
    // Orijinal GenericGUIManager::render: duruma gore menuyu cizer (5 zorluk, 6 taban TODO)
    void render(const GuiMenuCtl& ctl, int language, const char* const* credits52);
    void updateDialog(GuiMenuCtl& ctl, class InputDevice* in, GuiPad& pad, const GuiCallbacks& cb);

    static const char* getString(int id, int language);

private:
    GuiRenderDevice* m_dev = nullptr;
    GuiScale m_scale;
    alignas(8) uint8_t m_raw[kStateBytes];
    const char* m_btnText[130] = {};
};

} // namespace fs12
