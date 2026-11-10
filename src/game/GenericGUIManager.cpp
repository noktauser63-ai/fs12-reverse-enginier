#include <cmath>
#include <cstdio>
#include <cstring>
#include "game/GenericGUIManager.h"
#include "game/ModBridge.h"
#include "data/GuiStatics.h"
#include "data/Strings.h"

namespace fs12 {

static_assert(sizeof(OverlayDesc) == 24, "OverlayDesc");

// Baslik cizgisi tanimi: orijinalde yazilabilir global (mod yuksekligi [1] gecici 0 yapar) -> degistirilebilir kopya.
static unsigned* titleUnderlineMutable() {
    static unsigned w[6];
    static bool init = false;
    if (!init) { memcpy(w, g_m_overlayTitleUnderlineRight, sizeof w); init = true; }
    return w;
}
float* modTitleUnderlineImpl() { return reinterpret_cast<float*>(titleUnderlineMutable()); }

static const OverlayDesc& od(const unsigned* words) { return *reinterpret_cast<const OverlayDesc*>(words); }

GenericGUIManager::GenericGUIManager() {
    memset(m_raw, 0, sizeof m_raw);
}

GuiButton& GenericGUIManager::button(int i) {
    return *reinterpret_cast<GuiButton*>(m_raw + 8 + (size_t)i * 0xb0);
}

const char* GenericGUIManager::getString(int id, int language) {
    if (id < 0 || id >= kStringCount || language < 0 || language > 10) return "";
    if (language == 10) return kStringsTr[id];       // Turkce (LANG_TR)
    return kStrings[id][language];
}

void GenericGUIManager::load(GuiRenderDevice* dev, const Textures& t) {
    m_dev = dev;
    memcpy(m_raw, kGuiInitialState, kStateBytes);        // orijinal load() sonrasi durum (108 buton dahil)
    memset(m_btnText, 0, sizeof m_btnText);
    for (int i = 0; i < 130; ++i) { GuiButton& b = button(i); if (i < (int)buttonCount()) b.textRef = 0; }
    wr(0x58c4, t.font);   wr(0x58c8, t.fontId);
    wr(0x58a0, t.elements); wr(0x58ac, t.shopAlpha); wr(0x58b0, t.shopBg);
    wr(0x58a4, t.gestures); wr(0x58a8, t.map);       wr(0x58b4, t.title);
    wr(0x58bc, t.logo);   wr(0x58c0, t.edition);
    updateScreenScale();
    // dugmelerin dokusu: orijinalde load() icinde 0x58a0 (guiElements) verilir
    for (uint32_t i = 0; i < buttonCount(); ++i) button((int)i).tex = t.elements;
}

// ---------------- cizim ----------------
void GenericGUIManager::renderOverlay(const OverlayDesc& d, float x, float y, uint32_t align, uint32_t anchor,
                                      uint32_t tex, float sw, float sh) {
    float w = sw * d.w;
    float h = sh * d.h;
    anchorShift(x, y, w, h, anchor);
    m_scale.align(x, y, w, h, align);
    m_dev->renderOverlay(tex, x, y, w, h, d.u, d.v, d.du, d.dv);
}

void GenericGUIManager::renderText(float x, float y, uint32_t align, uint32_t flags, const char* text) {
    m_scale.alignX(x, align);
    m_scale.alignY(y, align);
    m_dev->renderText(rd(0x58c8), x, y, flags, text);
}

void GenericGUIManager::renderTitleScreen(float shade) {
    float s = m_dev->aspect();
    float f = (s >= 1.3333334f) ? (1.0f / s) : 0.75f;
    const float col[4] = {shade, shade, shade, 1.0f};
    m_dev->setOverlayColor(col);
    m_dev->renderOverlay(rd(0x58b4), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f - f, 1.0f, f);
}

void GenericGUIManager::renderGuiButtons(const GuiRenderDevice*, uint32_t count, const uint32_t* ids,
                                         float wscale, float hscale) {
    const float G = rdf(0x5890);
    for (uint32_t n = 0; n < count; ++n) {
        GuiButton& b = button((int)ids[n]);
        if (b.flags & 0x80) continue;
        float x = b.x, y = b.y;
        const char* text = (b.textRef && b.textRef <= 130) ? m_btnText[b.textRef - 1] : nullptr;
        float w = wscale * b.w;
        float h = hscale * b.h;
        anchorShift(x, y, w, h, b.anchor);
        m_scale.align(x, y, w, h, b.align);
        float tx = 0, ty = 0, textX = 0, textY = 0;
        if (text) {
            tx = b.textX; ty = b.textY;
            uint32_t ta = b.textAlign;
            if (ta & 2) tx = tx + w * 0.5f;
            else if (ta & 4) tx = tx + w;
            if (ta & 0x10) ty = ty + h * 0.5f;
            else if (ta & 0x20) ty = ty + h;
            if (!(b.align & 1)) tx = tx * m_scale.sx;
            if (!(b.align & 0x10)) ty = ty * m_scale.sy;
            textY = ty + y;
            textX = (ta & 4) ? 0.98f : tx + x;
        }
        const bool pressed = (uint32_t)(b.state - 1) < 2 && !(b.flags & 4);
        if (pressed) {
            if (!(b.flags & 8)) {
                float c[4] = {b.color[0] * 0.75f * G, b.color[1] * 0.75f * G, b.color[2] * 0.75f * G, b.color[3]};
                m_dev->setOverlayColor(c);
                m_dev->renderOverlay(b.tex, x, y, w, h, b.u, b.v, b.du, b.dv);
            }
            if (text) {
                if (b.fontSize != 0.0f) m_dev->setFontSize(b.fontSize);
                float t = G * 0.8f;
                float c[4] = {t, t, t, 1.0f};
                m_dev->setOverlayColor(c);   // orijinalde basili durumda metin rengi bu cagri ile ayarlanir
                m_dev->renderText(rd(0x58c8), textX, textY, b.textAlign, text);
            }
        } else {
            float a = b.color[3];
            if (b.flags & 2) a = a * 0.5f;
            if (!(b.flags & 8)) {
                float c[4] = {b.color[0] * G, b.color[1] * G, b.color[2] * G, a};
                m_dev->setOverlayColor(c);
                m_dev->renderOverlay(b.tex, x, y, w, h, b.u, b.v, b.du, b.dv);
            }
            if (text) {
                if (b.fontSize != 0.0f) m_dev->setFontSize(b.fontSize);
                float c[4] = {G, G, G, a};
                m_dev->setTextColor(c);
                m_dev->renderText(rd(0x58c8), textX, textY, b.textAlign, text);
            }
        }
    }
}

void GenericGUIManager::renderMainMenu(bool menuActive, bool tapPrompt, int lang) {
    renderTitleScreen();
    static const float white[4] = {1, 1, 1, 1};
    m_dev->setOverlayColor(white);
    renderOverlay(od(g_m_overlayGameLogo), 0.5f, 0.05f, 0x24, 0xa, rd(0x58bc), 1.0f, 1.0f);
    renderOverlay(od(g_m_overlayGameLogoEdition), 0.99f, 0.69f, 0x24, 0x24, rd(0x58c0), 1.0f, 1.0f);
    if (!menuActive) {
        m_dev->setFontSize(20.0f);
        renderText(0.5f, 0.7f, 0x44, 0x12, getString(0x4d, lang));
        return;
    }
    if (tapPrompt) {
        float c[4] = {1.0f, 1.0f, 1.0f, 0.0f};
        float a = phase() + 0.4f;
        a = a + a;
        if (a > 1.0f) { a = a - 1.0f; a = 1.0f - a; }
        if (a < 0.0f) a = 0.0f; else if (a > 1.0f) a = 1.0f;
        c[3] = a;
        m_dev->setTextColor(c);
        m_dev->setFontSize(20.0f);
        renderText(0.5f, 0.7f, 0x44, 0x12, getString(0x119, lang));
        c[3] = 1.0f;
        m_dev->setTextColor(c);
        setButtonText((int)rd(0x5a2c), getString(0x108, lang));
    }
    renderGuiButtons(m_dev, rd(0x5a28), reinterpret_cast<const uint32_t*>(m_raw + 0x5a2c), 1.0f, 1.0f);
}

} // namespace fs12

namespace fs12 {
void GenericGUIManager::renderTitle(const char* title) {
    float c[4] = {rdf(0x5890), rdf(0x5890), rdf(0x5890), 1.0f};
    m_dev->setOverlayColor(c);
    renderOverlay(od(titleUnderlineMutable()), 1.0f, 0.09f, 0x28, 0xc, rd(0x58a0), 1.0f, 1.0f);
    m_dev->setFontSize(26.0f);
    renderText(0.98f, 0.02f, 0x28, 0xc, title);
}

// Orijinal renderTitle(char const*, char const*): kisa sol + kisa sag cizgi, iki baslik metni (istatistik/fiyat menuleri)
void GenericGUIManager::renderTitle2(const char* left, const char* right) {
    float c[4] = {rdf(0x5890), rdf(0x5890), rdf(0x5890), 1.0f};
    m_dev->setOverlayColor(c);
    renderOverlay(od(g_m_overlayTitleUnderlineLeftShort), 0.0f, 0.09f, 0x22, 9, rd(0x58a0), 1.0f, 1.0f);
    renderOverlay(od(g_m_overlayTitleUnderlineRightShort), 1.0f, 0.09f, 0x28, 0xc, rd(0x58a0), 1.0f, 1.0f);
    m_dev->setFontSize(26.0f);
    renderText(0.02f, 0.02f, 0x22, 9, left);
    renderText(0.98f, 0.02f, 0x28, 0xc, right);
}

void GenericGUIManager::renderStartMenuMain(int lang) {
    renderTitleScreen(rdf(0x58b8));
    renderTitle(getString(10, lang));
    static const float white[4] = {1, 1, 1, 1};
    m_dev->setOverlayColor(white);
    static const int sid[8] = {0x15, 0x16, 0x10a, 0x12e, 0x17, 0x12d, 0xc, 0xd};
    for (int i = 0; i < 8; ++i) setButtonText((int)rd(0x5a74 + 4 * i), getString(sid[i], lang));
    renderGuiButtons(m_dev, 9, (const uint32_t*)(m_raw + 0x5a74), 1.0f, 1.0f);
    if (modBridge().startMenuMainAfter) modBridge().startMenuMainAfter(this);
}
}

namespace fs12 {
void GenericGUIManager::renderCreditsMenu(const char* const* lines) {
    static const float white[4] = {1, 1, 1, 1};
    m_dev->setOverlayColor(white);
    m_dev->setFontSize(28.0f);
    m_dev->renderOverlay(rd(0x58a4), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0009765625f, 0.0009765625f);
    if (rdf(0x588c) < -36.0f) wrf(0x588c, 9.0f);
    for (int i = 0; i < 0x34; ++i) {
        float y = (((float)(unsigned)i * 30.0f) + (rdf(0x588c) * 40.0f)) / 320.0f;
        if (y > -0.1f && y < 1.0f) renderText(0.5f, y, 0x44, 0x12, lines[i]);
    }
}
}

namespace fs12 {
void GenericGUIManager::renderStartMenuSettings(const GuiMenuCtl& ctl, int lang) {
    renderTitleScreen(rdf(0x58b8));
    renderTitle(getString(0x17, lang));
    const char* labels[5] = {getString(0x33, lang), getString(0x1a, lang), getString(0x1b, lang), getString(0x19, lang), getString(0x18, lang)};
    m_dev->setFontSize(24.0f);
    const bool fl[3] = {ctl.flag12f4, ctl.flag12f5, ctl.flag12f6};
    for (int i = 0; i < 3; ++i) setButtonText((int)rd(0x5af0 + 4 * i), getString(fl[i] ? 0xf : 0x10, lang));
    for (int i = 0; i < 5; ++i) {
        float y = button((int)rd(0x5af0 + 4 * i)).y;
        renderOverlay(od(g_m_overlayLineRightLong), 1.0f, y + 0.05f, 0x28, 0x14, rd(0x58a0), 1.0f, 1.0f);
        renderOverlay(od(g_m_overlayLineRightLong), 1.0f, y - 0.05f, 0x28, 0x14, rd(0x58a0), 1.0f, 1.0f);
        renderText(0.1f, y, 0x28, 0x11, labels[i]);
    }
    char buf[32];
    const float x0 = button((int)rd(0x5af0)).x;
    if (ctl.val12ec == 0) snprintf(buf, sizeof buf, "%s", getString(0x1d, lang)); else snprintf(buf, sizeof buf, "%d%%", ctl.val12ec);
    renderText(x0, button((int)rd(0x5afc)).y, 0x28, 0x12, buf);
    if (ctl.val12e8 == 0) snprintf(buf, sizeof buf, "%s", getString(0x1d, lang)); else snprintf(buf, sizeof buf, "%d%%", ctl.val12e8);
    renderText(x0, button((int)rd(0x5b00)).y, 0x28, 0x12, buf);
    renderGuiButtons(m_dev, 8, (const uint32_t*)(m_raw + 0x5af0), 1.0f, 1.0f);
}
}

namespace fs12 {
static inline float bitsF(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }
void GenericGUIManager::renderStartMenuHelp(int lang) {
    const uint32_t tex = rd(0x58a4), t2 = rd(0x58a4);
    m_dev->renderOverlay(tex, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0009765625f, 0.0009765625f);
    (void)t2;
    const uint32_t st = rd(0x58d0);
    if (st == 0) {
        renderTitle(getString(0x117, lang));
        m_dev->setFontSize(18.0f);
        renderOverlay(od(g_m_overlayHelpSteering), 0.5f, 0.45f, 4, 0x12, rd(0x58a4), 1.0f, 1.0f);
        renderText(0.5f, 0.85f, 0x44, 0x12, getString(0x118, lang));
    } else if (st == 1) {
        renderTitle(getString(0x114, lang));
        m_dev->setFontSize(18.0f);
        renderOverlay(od(g_m_overlayHelpVehicleAccelerate), bitsF(0x3eaa7efa), 0.4f, 0x44, 0x12, rd(0x58a4), 1.0f, 1.0f);
        renderText(bitsF(0x3eaa7efa), 0.65f, 0x44, 0xa, getString(0x115, lang));
        renderOverlay(od(g_m_overlayHelpVehicleSwitch), bitsF(0x3f558106), 0.4f, 0x44, 0x12, rd(0x58a4), 1.0f, 1.0f);
        renderText(bitsF(0x3f558106), 0.65f, 0x44, 0xa, getString(0x116, lang));
    } else if (st == 2) {
        renderTitle(getString(0x110, lang));
        m_dev->setFontSize(18.0f);
        renderOverlay(od(g_m_overlayHelpCameraRot), bitsF(0x3e29fbe7), 0.625f, 0x44, 0x22, rd(0x58a4), 1.0f, 1.0f);
        renderText(bitsF(0x3e29fbe7), 0.65f, 0x44, 0xa, getString(0x111, lang));
        renderOverlay(od(g_m_overlayHelpCameraZoom), 0.5f, 0.625f, 0x44, 0x22, rd(0x58a4), 1.0f, 1.0f);
        renderText(0.5f, 0.65f, 0x44, 0xa, getString(0x112, lang));
        renderOverlay(od(g_m_overlayHelpCameraReset), bitsF(0x3f558106), 0.625f, 0x44, 0x22, rd(0x58a4), 1.0f, 1.0f);
        renderText(bitsF(0x3f558106), 0.65f, 0x44, 0xa, getString(0x113, lang));
    }
    m_dev->setFontSize(16.0f);
    renderText(0.5f, 0.95f, 0x44, 0x12, getString(0x119, lang));
}
}

namespace fs12 {
void GenericGUIManager::formatNumber(char* out, uint32_t n) {
    if (n > 999999999u) n = 999999999u;
    if (n > 999999u) sprintf(out, "%d'%03d'%03d", (int)((n / 1000000u) % 1000u), (int)((n / 1000u) % 1000u), (int)(n % 1000u));
    else if (n > 999u) sprintf(out, "%d'%03d", (int)((n / 1000u) % 1000u), (int)(n % 1000u));
    else sprintf(out, "%d", (int)(n % 1000u));
}

void GenericGUIManager::renderSaveGameMenu(const GuiMenuCtl& ctl, int lang) {
    if (modBridge().saveMenuBefore) modBridge().saveMenuBefore(this);
    renderTitleScreen(rdf(0x58b8));
    renderTitle(getString(3, lang));
    renderGuiButtons(m_dev, rd(0x5a3c), (const uint32_t*)(m_raw + 0x5a40), 1.0f, 1.0f);
    static const float white[4] = {1, 1, 1, 1};
    m_dev->setOverlayColor(white);
    static const char* const letters[3] = {"A", "B", "C"};
    char buf[128];
    for (int i = 0; i < 3; ++i) {
        const GuiButton& b = button((int)rd(0x5a40 + 4 * i));
        float x = b.x, y = b.y;
        anchorShift(x, y, b.w, b.h, b.textAlign);
        if (ctl.saveUsed[i]) {
            m_dev->setFontSize(25.0f);
            snprintf(buf, sizeof buf, "%s: ", getString(0x11a, lang));
            formatNumber(buf + strlen(buf), ctl.saveMoney[i]);
            renderText(0.98f, (b.h * 0.25f) + y, b.align, 0x14, buf);
            snprintf(buf, sizeof buf, "%s: %d:%02d", getString(0x38, lang), (int)ctl.saveHours[i], (int)ctl.saveMinutes[i]);
            renderText(0.98f, (b.h * 0.75f) + y, b.align, 0x14, buf);
            snprintf(buf, sizeof buf, "%s - %s", letters[i], getString(5, lang));
        } else {
            snprintf(buf, sizeof buf, "%s - %s", letters[i], getString(4, lang));
        }
        m_dev->setFontSize(35.0f);
        renderText(x + 0.06f, (b.h * 0.5f) + y, b.align, 0x11, buf);
    }
    if (modBridge().saveMenuAfter) modBridge().saveMenuAfter(this);
}
}

float* fs12ModTitleUnderline() { return fs12::modTitleUnderlineImpl(); }  // mod arayuzu (src/mod/ModApi.h)

namespace fs12 {
void GenericGUIManager::renderDifficultyLevelMenu(int lang) {
    renderTitleScreen();
    renderTitle(getString(6, lang));
    setButtonText((int)rd(0x5b14), getString(7, lang));
    setButtonText((int)rd(0x5b18), getString(8, lang));
    setButtonText((int)rd(0x5b1c), getString(9, lang));
    renderGuiButtons(m_dev, rd(0x5b10), (const uint32_t*)(m_raw + 0x5b14), 1.0f, 1.0f);
}

void GenericGUIManager::renderStartMenu(const GuiMenuCtl& ctl, int lang, const char* const*) {
    switch (rd(0x58cc)) {
    case 0: renderStartMenuMain(lang); break;
    case 4: renderStartMenuHelp(lang); break;
    case 2: renderStartMenuStatistics(ctl, lang); break;
    case 1: renderStartMenuShop(ctl, lang); break;
    case 3: renderStartMenuPrices(ctl, lang); break;
    case 5: renderStartMenuSettings(ctl, lang); break;
    default: renderTitleScreen(); break;   // TODO: market/istatistik/fiyatlar
    }
}

void GenericGUIManager::render(const GuiMenuCtl& ctl, int lang, const char* const* credits52) {
    updateScreenScale();
    switch (ctl.state) {
    case 2: renderMainMenu(ctl.menuActive, ctl.tapPrompt, lang); break;
    case 3: if (credits52) renderCreditsMenu(credits52); break;
    case 4: renderSaveGameMenu(ctl, lang); break;
    case 5: renderDifficultyLevelMenu(lang); break;
    case 7: renderStartMenu(ctl, lang, credits52); break;
    case 6: renderBaseMenu(ctl, lang); break;
    default: break;
    }
    if (rd(0x5c78)) renderDialog(ctl, lang);
}
} // namespace fs12

namespace fs12 {
// Orijinal GenericGUIManager::renderStartMenuStatistics (disassembly'den): iki sutun, rakamlar sabit genislikte.
void GenericGUIManager::renderStartMenuStatistics(const GuiMenuCtl& ctl, int lang) {
    renderTitleScreen(rdf(0x58b8));
    renderTitle2(getString(0x39, lang), getString(0x16, lang));
    renderGuiButtons(m_dev, rd(0x5a98), (const uint32_t*)(m_raw + 0x5a9c), 1.0f, 1.0f);
    m_dev->setFontSize(16.0f);
    m_dev->setNumberMonospace(true, bitsF(0x40d1999a));
    char buf[64];
    auto num = [&](uint32_t off) { uint32_t v = ctl.statAt(off); if (v > 9999999u) v = 9999999u; formatNumber(buf, v); };
    auto row = [&](int id, float lx, float vx, float y, uint32_t al) {
        renderText(lx, y, al, 9, getString(id, lang));
        renderText(vx, y, al, 0xc, buf);
    };
    const float kL = bitsF(0x3ca3d70a), kV = bitsF(0x3ed9999a);
    num(0x1264); row(0x2a, kL, kV, bitsF(0x3e19999a), 0x22);
    num(0x1268); row(0x2b, kL, kV, bitsF(0x3e500000), 0x22);
    num(0x126c); row(0x2c, kL, kV, bitsF(0x3e833334), 0x22);
    num(0x1298); row(0x3e, kL, kV, bitsF(0x3e9e6667), 0x22);
    // silo tablosu basligi + 3 satir
    const float hy = bitsF(0x3ed4ccce);
    renderText(kL, hy, 0x22, 9, getString(0x3a, lang));
    renderText(bitsF(0x3e800000), hy, 0x22, 0xc, getString(0x3c, lang));
    renderText(kV, hy, 0x22, 0xc, getString(0x3d, lang));
    for (int i = 1; i <= 3; ++i) {
        snprintf(buf, sizeof buf, "%s %d", getString(0x3b, lang), i);
        const float y = (float)i * bitsF(0x3d59999a) + hy;
        renderText(kL, y, 0x22, 9, buf);
        snprintf(buf, sizeof buf, "%d%%", (int)ctl.statAt(0x12a8 + 4 * (i - 1)));
        renderText(bitsF(0x3e800000), y, 0x22, 0xc, buf);
        const uint32_t t = ctl.statAt(0x129c + 4 * (i - 1));
        if (t == 0) strcpy(buf, "--:--"); else snprintf(buf, sizeof buf, "%2d:%02d", (int)(t / 60u), (int)(t % 60u));
        renderText(kV, y, 0x22, 0xc, buf);
    }
    const float rl = bitsF(0x3f028f5c), rv = bitsF(0x3f7ae148);
    num(0x12b4); row(0x2d, rl, rv, bitsF(0x3e19999a), 0x28);
    num(0x12b8); row(0x2e, rl, rv, bitsF(0x3e500000), 0x28);
    num(0x12bc); row(0x2f, rl, rv, bitsF(0x3e833333), 0x28);
    num(0x12c0); row(0x30, rl, rv, bitsF(0x3e9e6666), 0x28);
    snprintf(buf, sizeof buf, "%d:%02d", (int)ctl.statAt(0x12c4), (int)ctl.statAt(0x12c8)); row(0x31, rl, rv, bitsF(0x3eb99999), 0x28);
    snprintf(buf, sizeof buf, "%d/%d", (int)ctl.statAt(0x12d0), (int)ctl.statAt(0x12d4)); row(0x32, rl, rv, bitsF(0x3ed4cccc), 0x28);
    snprintf(buf, sizeof buf, "%d", (int)ctl.statAt(0x12d8)); row(0x33, rl, rv, bitsF(0x3eefffff), 0x28);
    renderText(rl, bitsF(0x3f059999), 0x28, 9, getString(0x34, lang));
    renderText(rv, bitsF(0x3f059999), 0x28, 0xc, getString((int)ctl.statAt(0x12cc) + 7, lang));
    m_dev->setNumberMonospace(false, 0.0f);
}
} // namespace fs12

namespace fs12 {
void GenericGUIManager::renderTitleYOffset(const char* text, float yOff) {
    float c[4] = {rdf(0x5890), rdf(0x5890), rdf(0x5890), 1.0f};
    m_dev->setOverlayColor(c);
    renderOverlay(od(titleUnderlineMutable()), 1.0f, yOff + 0.09f, 0x28, 0xc, rd(0x58a0), 1.0f, 1.0f);
    m_dev->setFontSize(26.0f);
    renderText(0.98f, yOff + 0.02f, 0x28, 0xc, text);
}

// Orijinal renderStartMenuPrices: 4x4 fiyat tablosu (satis noktasi x urun) + yuksek talep listesi.
void GenericGUIManager::renderStartMenuPrices(const GuiMenuCtl& ctl, int lang) {
    renderTitleScreen(rdf(0x58b8));
    renderTitle(getString(0x15, lang));
    renderTitleYOffset(getString(0x35, lang), bitsF(0x3ef5c28f));
    renderGuiButtons(m_dev, rd(0x5ac4), (const uint32_t*)(m_raw + 0x5ac8), 1.0f, 1.0f);
    m_dev->setFontSize(16.0f);
    m_dev->setNumberMonospace(true, bitsF(0x40d1999a));
    const float lx = bitsF(0x3ca3d70a), y0 = bitsF(0x3e6b3334), dy = bitsF(0x3d59999a);
    renderText(lx, y0, 0x28, 9, getString(0x26, lang));
    renderText(lx, bitsF(0x3e90cccd), 0x28, 9, getString(0x27, lang));
    renderText(lx, bitsF(0x3eac0000), 0x28, 9, getString(0x28, lang));
    renderText(lx, bitsF(0x3ec73334), 0x28, 9, getString(0x29, lang));
    char buf[128];
    for (int i = 0; i < 4; ++i) {
        const float x = (float)i * bitsF(0x3e6147ae) + bitsF(0x3ea3d70c);
        renderText(x, bitsF(0x3e19999a), 0x28, 0xc, getString(0x1e + i, lang));
        for (int j = 0; j < 4; ++j) {
            const uint32_t v = ctl.priceAt(0x1224 + 16 * j + 4 * i);
            if (v == 0) { buf[0] = '-'; buf[1] = 0; } else snprintf(buf, sizeof buf, "%d", (int)v);
            renderText(x, (float)j * dy + y0, 0x28, 0xc, buf);
        }
    }
    const float hx = bitsF(0x3ea3d70c), hy = bitsF(0x3f2147ae);
    renderText(hx, hy, 0x28, 0xc, getString(0x37, lang));
    renderText(bitsF(0x3f0a3d72), hy, 0x28, 0xc, getString(0x36, lang));
    renderText(bitsF(0x3f428f5d), hy, 0x28, 0xc, getString(0x38, lang));
    const uint32_t cnt = ctl.priceAt(0x1270);
    for (uint32_t k = 0; k < cnt && k < 2; ++k) {
        const float y = (float)k * dy + bitsF(0x3f35ae14);
        renderText(lx, y, 0x28, 9, getString((int)ctl.priceAt(0x1274 + 4 * k) + 0x26, lang));
        renderText(hx, y, 0x28, 0xc, getString((int)ctl.priceAt(0x127c + 4 * k) + 0x22, lang));
        snprintf(buf, sizeof buf, "%1.1fx", (double)ctl.priceF(0x1284 + 4 * k));
        renderText(bitsF(0x3f0a3d72), y, 0x28, 0xc, buf);
        const uint32_t t = ctl.priceAt(0x128c + 4 * k);
        snprintf(buf, sizeof buf, "%2d:%02d", (int)(t / 60u), (int)(t % 60u));
        renderText(bitsF(0x3f428f5d), y, 0x28, 0xc, buf);
    }
    m_dev->setFontSize(16.0f);
    const float c8[4] = {bitsF(0x3f4ccccd), bitsF(0x3f4ccccd), bitsF(0x3f4ccccd), bitsF(0x3f4ccccd)};
    m_dev->setOverlayColor(c8);
    snprintf(buf, sizeof buf, "%s %s %s", getString(0x61, lang), getString(0x62, lang), getString(0x63, lang));
    renderText(bitsF(0x3e4ccccd), bitsF(0x3f70a3d7), 0x22, 0x21, buf);
    m_dev->setNumberMonospace(false, 0.0f);
}
} // namespace fs12

namespace fs12 {
void GenericGUIManager::renderOverlayF(float x, float y, float w, float h, float u, float v, float du, float dv,
                                       uint32_t align, uint32_t anchor, uint32_t tex) {
    const OverlayDesc d = {w, h, u, v, du, dv};
    renderOverlay(d, x, y, align, anchor, tex, 1.0f, 1.0f);
}

// Orijinal renderStartMenuShop: market ekrani (dairesel karusel + fiyat/bakiye yazilari).
void GenericGUIManager::renderStartMenuShop(const GuiMenuCtl& ctl, int lang) {
    const GuiShopCtl& sh = ctl.shop;
    const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    m_dev->setOverlayColor(one);
    float s = 1.0f / (1.0f * m_dev->aspect());
    if (!(s <= 1.0f)) s = 1.0f;
    m_dev->renderOverlay(rd(0x58b0), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.5f - s * 0.5f, 1.0f, s);
    setButtonText((int)rd(0x5814), getString(0x10c, lang));
    setButtonText((int)rd(0x5818), getString(0x10b, lang));
    setButtonText((int)rd(0x581c), sh.extTex && sh.useLabel ? sh.useLabel : getString(0x10f, lang));
    if (sh.flag1221) button((int)rd(0x581c)).flags |= 0x80;
    const int sel = (int)rd(0x583c);
    bool alt = false;
    if (!sh.flag1221 && ctl.slotUsed[sel]) {
        setButtonText((int)rd(0x5818), getString(0x10d, lang));
        setButtonText((int)rd(0x5814), getString(0x10e, lang));
        alt = true;
    }
    uint32_t cnt = rd(0x5810);
    uint32_t idsOff = 0x5814;
    if (alt && !sh.flag1220) { idsOff = 0x5818; cnt -= 1; }
    renderGuiButtons(m_dev, cnt, (const uint32_t*)(m_raw + idsOff), 1.0f, 1.0f);
    m_dev->setOverlayColor(one);
    m_dev->setFontSize(24.0f);
    char buf[128], buf2[128];
    snprintf(buf, sizeof buf, "%s (%d)", sh.name[sel] ? sh.name[sel] : "", (int)sh.count[sel]);
    renderText(bitsF(0x3f7ae148), bitsF(0x3ca3d70a), 0x48, 0xc, buf);
    if (sh.desc[sel]) {
        m_dev->setFontSize(21.0f);
        renderText(bitsF(0x3f7ae148), bitsF(0x3db851ec), 0x48, 0xc, sh.desc[sel]);
    }
    const GuiButton& bb = button((int)rd(0x5818));
    float by = bb.y;
    if (bb.anchor & 0x10) by = by - bb.h * 0.5f;
    else if (bb.anchor & 0x20) by = by - bb.h;
    const float ypos = by + bb.h * 0.5f;
    m_dev->setFontSize(19.0f);
    const float xpos = (button((int)rd(0x5814)).h - bb.h) * 0.5f + 0.5f;
    snprintf(buf, 0x74, "%s: ", getString(0x11a, lang));
    formatNumber(buf + strlen(buf), sh.price[sel]);
    if (alt) {
        float dy = 0.0f;
        if (sh.alt[sel][0]) {
            snprintf(buf2, sizeof buf2, "%s: %s", getString(0x10d, lang), sh.alt[sel]);
            renderText(xpos, ypos - bitsF(0x3ccccccd), 0x44, 0x12, buf2);
            dy = bitsF(0x3ccccccd);
        }
        const float g[4] = {bitsF(0x3f333333), bitsF(0x3f333333), bitsF(0x3f333333), 1.0f};
        m_dev->setOverlayColor(g);
        renderText(xpos, ypos + dy, 0x44, 0x12, buf);
        m_dev->setOverlayColor(one);
    } else {
        renderText(xpos, ypos, 0x44, 0x12, buf);
    }
    m_dev->setFontSize(21.0f);
    snprintf(buf, sizeof buf, "%s: ", getString(0x11b, lang));
    formatNumber(buf + strlen(buf), sh.money);
    renderText(bitsF(0x3df5c28f), bitsF(0x3f6ae148), 0x82, 0x11, buf);
    const uint32_t tex = rd(0x58ac);
    const OverlayDesc* items = reinterpret_cast<const OverlayDesc*>(g_shopItemData);   // adim 0x1c: OverlayDesc + 1 sozcuk
    auto item = [&](int i) { return od(g_shopItemData + i * 7); };
    const uint32_t logo = sh.extTex ? 0u : g_shopItemData[sel * 7 + 6];
    const uint32_t itex = sh.extTex ? sh.extTex : tex;
    const int N = (int)sh.n;
    (void)items;
    if (logo) renderOverlay(od(g_m_shopLogoOffsets + logo * 6), bitsF(0x3ca3d70a), bitsF(0x3cf5c28f), 0x42, 9, tex, 1.0f, 1.0f);
    static const int offs[7] = {-3, 3, -2, 2, -1, 1, 0};
    const float scroll = rdf(0x5840);
    for (int k = 0; k < 7; ++k) {
        int i = offs[k] + sel;
        if (i < 0) i += N; else if (i > N - 1) i -= N;
        const float a1 = 180.0f - (float)offs[k] * 60.0f;
        const float ang = a1 + scroll * 60.0f;
        const float rad = ang + 90.0f;
        const float r = rad * bitsF(0x3c8efa35);
        const float c = cosf(r);
        const float sn = sinf(r);
        float d = sn * bitsF(0x3fa66666);
        d = d * 0.5f; d = d + 1.0f; d = d + bitsF(0x3f266666);
        float X = c / d;
        float Y = 0.5f / d; Y = Y - 0.5f; Y = Y / bitsF(0x3e90b216); Y = Y + 0.5f; Y = Y * bitsF(0x3e3851ec); Y = Y + bitsF(0x3ec7ae14);
        X = X / bitsF(0x3f1b26ca); X = X * bitsF(0x3f2e147b); X = X * 0.5f; X = X + 0.5f;
        const float sc = 1.5f / d;
        float f = fabsf(ang - 180.0f);
        f = 180.0f - f; f = f / 180.0f;
        const float col[4] = {1.0f, 1.0f, 1.0f, f};
        m_dev->setOverlayColor(col);
        renderOverlay(sh.extTex ? sh.ext[i] : item(i), X, Y, 0x44, 0x12, itex, sc, sc);
        if (!sh.extTex && ctl.slotUsed[i]) { const float s2 = sc * bitsF(0x3f666666); renderOverlay(od(g_m_overlayLock), X, Y, 0x44, 0x12, tex, s2, s2); }
        if ((double)fabsf(X - 0.5f) < 1e-4) {
            float bx;
            if (sh.badgeA[sel]) { renderOverlayF(bitsF(0x3f0f5c29), bitsF(0x3ebd70a4), bitsF(0x3d4ccccd), bitsF(0x3d99999a), bitsF(0x3d440000), bitsF(0x3f008000), bitsF(0x3d900000), bitsF(0x3e100000), 0x44, 0x12, rd(0x58a0)); bx = bitsF(0x3f1eb852); }
            else bx = bitsF(0x3f0f5c29);
            if (sh.badgeB[sel]) { renderOverlayF(bx, bitsF(0x3ebd70a4), bitsF(0x3d4ccccd), bitsF(0x3d99999a), bitsF(0x3e470000), bitsF(0x3f008000), bitsF(0x3d900000), bitsF(0x3e100000), 0x44, 0x12, rd(0x58a0)); bx = bx + bitsF(0x3d75c290); }
            if (sh.badgeC[sel]) renderOverlayF(bx, bitsF(0x3ebd70a4), bitsF(0x3d4ccccd), bitsF(0x3d99999a), bitsF(0x3df80000), bitsF(0x3f008000), bitsF(0x3d900000), bitsF(0x3e100000), 0x44, 0x12, rd(0x58a0));
        }
    }
}
} // namespace fs12

namespace fs12 {
// Orijinal renderDialog: yari saydam arka plan, ustte/altta ince seritler, tipe gore dugme metinleri ve en fazla 3 satir metin.
void GenericGUIManager::renderDialog(const GuiMenuCtl& ctl, int lang) {
    const GuiDialogCtl& dl = ctl.dlg;
    m_dev->setFontSize(35.0f);
    const float c0[4] = {0.0f, 0.0f, 0.0f, bitsF(0x3f59999a)};
    m_dev->setOverlayColor(c0);
    const uint32_t tex = rd(0x58a0);
    const float top = rdf(0x5964), bot = rdf(0x5968);
    float x = 0.0f, y = top, w = 1.0f, h = bot - top;
    anchorShift(x, y, 1.0f, h, 9);
    m_scale.align(x, y, w, h, 0x41);
    m_dev->renderOverlay(tex, x, y, w, h, bitsF(0x3e960000), bitsF(0x3b000000), bitsF(0x3ed40000), bitsF(0x3b000000));
    if (rd(0x5c74) != 6) {
        const float c1[4] = {0.0f, 0.0f, 0.0f, bitsF(0x3f266666)};
        m_dev->setOverlayColor(c1);
        const float y0 = y, y1 = y0 + h;
        m_dev->renderOverlay(tex, 0.0f, 0.0f, 1.0f, y0, 0.5f, bitsF(0x3b000000), bitsF(0x3a800000), bitsF(0x3b000000));
        m_dev->renderOverlay(tex, 0.0f, y1, 1.0f, 1.0f - y1, 0.5f, bitsF(0x3b000000), bitsF(0x3a800000), bitsF(0x3b000000));
    }
    const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    m_dev->setOverlayColor(one);
    for (int k = 0; k < 2; ++k) {
        float ex = 0.0f, ey = k == 0 ? top : bot, ew = 1.0f, eh = bitsF(0x3bcccccd);
        anchorShift(ex, ey, 1.0f, eh, k == 0 ? 9u : 0x21u);
        m_scale.align(ex, ey, ew, eh, 0x41);
        m_dev->renderOverlay(tex, ex, ey, ew, eh, bitsF(0x3e960000), 0.0f, bitsF(0x3ed40000), bitsF(0x3c000000));
    }
    float tx = 0.5f, ty = top + bitsF(0x3da00000);
    const uint32_t type = rd(0x5c78);
    float lineBase = bitsF(0x3dbf1412), fs = 25.0f;
    if (type == 1) {
        setButtonText((int)rd(0x5948), getString(0xf, lang));
        setButtonText((int)rd(0x594c), getString(0x10, lang));
    } else if (type == 3) {
        setButtonText((int)rd(0x5954), getString(0x11, lang));
    } else if (type == 4) {
        setButtonText((int)rd(0x5950), getString(0x12, lang));
        for (int j = 0; j < 3; ++j) {
            const char* p = dl.slotName[j][0] ? dl.slotName[j] : getString(0x14, lang);
            if (p[0]) snprintf(m_dlgText[j], 64, "%s\n%s: %s", getString(0x11c + j, lang), getString(0x13, lang), p);
            else { snprintf(m_dlgText[j], 64, "%s", getString(0x11c + j, lang)); wr8(0x5808, 0); }
        }
        setButtonText((int)rd(0x5958), m_dlgText[0]);
        setButtonText((int)rd(0x595c), m_dlgText[1]);
        setButtonText((int)rd(0x5960), m_dlgText[2]);
        if (!rd8(0x5808)) {
            const float ny = rdf(0x580c) + bitsF(0x3d23d70a);
            button((int)rd(0x5958)).y = ny; button((int)rd(0x595c)).y = ny; button((int)rd(0x5960)).y = ny;
        }
    } else if (type == 2) {
        ty = 0.5f - ((float)(dl.count - 1u) * bitsF(0x3e0f4f0e)) * 0.5f;
        lineBase = bitsF(0x3e0f4f0e); fs = bitsF(0x42160000);
    }
    m_dev->setFontSize(fs);
    const float lineH = lineBase * m_scale.sy;
    m_scale.alignX(tx, 0x44);
    m_scale.alignY(ty, 0x44);
    for (uint32_t i = 0; i < dl.count && i < 3; ++i) {
        const char* t = dl.text[i];
        if (dl.id[i] >= 0) t = getString(dl.id[i], lang);
        m_dev->renderText(rd(0x58c8), tx, (float)i * lineH + ty, 0x12, t ? t : "");
    }
    renderGuiButtons(m_dev, rd(0x591c), (const uint32_t*)(m_raw + 0x5920), 1.0f, 1.0f);
}
} // namespace fs12

namespace fs12 {
// Orijinal GenericGUIManager::renderBaseMenu (disassembly'den): oyun ici ust cubuk.
// NOT: harita/minimap blogu (this+0x58e8 > 0) ve tarla simgesi (+0x143) henuz yok.
void GenericGUIManager::renderBaseMenu(const GuiMenuCtl& ctl, int) {
    const GuiHudCtl& h = ctl.hud;
    const unsigned* bar = g_m_overlayMainBar;
    auto bf = [&](int i) { float f; memcpy(&f, &bar[i], 4); return f; };
    m_dev->setFontSize(bitsF(0x41b00000));
    m_dev->setNumberMonospace(true, bitsF(0x41100000));
    float x = 0.5f, y = 0.0f, w = bf(0), hh = bf(1);
    { float cw = w, ch = hh; anchorShift(x, y, cw, ch, 10); }
    const float wm = (w - bitsF(0x3ee00000)) - bitsF(0x3cbbbbbc);           // sp+0x34
    float x124 = (x + bitsF(0x3ee00000)) + bitsF(0x3cbbbbbc);               // sp+0x9c
    float ty = y + bitsF(0x3d533333);
    m_scale.alignY(ty, 0x24);
    float tx[5] = {x + bitsF(0x3e188889), x + bitsF(0x3ed5ddde), x + bitsF(0x3f0fbbbc), x + bitsF(0x3f2b3333), x + bitsF(0x3f304444)};
    for (float& t : tx) m_scale.alignX(t, 0x24);
    const float sx = m_scale.sx;
    const float s24 = sx * bitsF(0x3cbbbbbc);
    m_scale.align(x, y, w, hh, 0x24);
    m_scale.alignX(x124, 0x24);
    const float s28 = sx * bitsF(0x3ee00000);
    const float s20 = wm * sx;
    const float one[4] = {1.0f, 1.0f, 1.0f, 1.0f}, red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    m_dev->setOverlayColor(one);
    const float du = bitsF(0x3ed20000), pad = bitsF(0x3cb00000);
    m_dev->renderOverlay(rd(0x58a0), x, y, s28, hh, bf(2), bf(3), du, bf(5));
    m_dev->renderOverlay(rd(0x58a0), x124, y, s20, hh, (bf(2) + du) + pad, bf(3), (bf(4) - pad) - du, bf(5));
    float us;
    switch (h.sel59c) { case 2: us = bitsF(0x3f498000); break; case 3: us = bitsF(0x3f508000); break; case 4: us = bitsF(0x3f57c000); break; default: us = bf(2) + du; }
    m_dev->renderOverlay(rd(0x58a0), s28 + x, y, s24, hh, us, bitsF(0x3f338000), pad, bitsF(0x3e020000));
    char buf[64];
    snprintf(buf, sizeof buf, "%03d", (int)h.v124);
    if (h.f128) { m_dev->setOverlayColor(red); m_dev->renderText(rd(0x58c8), tx[0], ty, 0x14, buf); m_dev->setOverlayColor(one); }
    else m_dev->renderText(rd(0x58c8), tx[0], ty, 0x14, buf);
    formatNumber(buf, h.money > 99999999u ? 99999999u : h.money);
    m_dev->renderText(rd(0x58c8), tx[1], ty, 0x14, buf);
    snprintf(buf, sizeof buf, "%3d%%", (int)(h.pct * 100.0f));
    if (h.f5a4 && phase() >= 0.0f) { m_dev->setOverlayColor(red); m_dev->renderText(rd(0x58c8), tx[2], ty, 0x14, buf); m_dev->setOverlayColor(one); }
    else m_dev->renderText(rd(0x58c8), tx[2], ty, 0x14, buf);
    snprintf(buf, sizeof buf, "%02d", (int)h.v130);
    m_dev->renderText(rd(0x58c8), tx[3], ty, 0x14, buf);
    snprintf(buf, sizeof buf, "%02d:%02d", (int)h.hours, (int)h.minutes);
    m_dev->renderText(rd(0x58c8), tx[4], ty, 0x11, buf);
    m_dev->setNumberMonospace(false, 0.0f);
    renderGuiButtons(m_dev, rd(0x59a8), (const uint32_t*)(m_raw + 0x59ac), 1.0f, 1.0f);
}
} // namespace fs12
