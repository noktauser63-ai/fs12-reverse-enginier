#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/GuiDeviceGL.h"

namespace fs12 {

static std::vector<FontInfo> mergedFontTable() {
    std::vector<FontInfo> t(kFontData03, kFontData03 + kFontData03Count);
    t.insert(t.end(), kFontTr, kFontTr + kFontTrCount);
    return t;
}

GuiDeviceGL::GuiDeviceGL(Renderer* r, const char* language)
    : m_r(r), m_table(mergedFontTable()), m_font(m_table.data(), (int)m_table.size(), 2048, 2048) {
    m_logo = std::string("gui/logos_") + ((language && language[0] == 'd' && language[1] == 'e') ? "de" : "en") + ".p2d";
    for (uint32_t i = T_FONT; i <= T_EDITION; ++i) tex(i);   // onyukleme
}

GenericGUIManager::Textures GuiDeviceGL::textures() const {
    GenericGUIManager::Textures t{};
    t.font = T_FONT; t.fontId = 0; t.elements = T_ELEMENTS; t.shopAlpha = T_SHOP_ALPHA; t.shopBg = T_SHOP_BG;
    t.gestures = T_GESTURES; t.map = T_MAP; t.title = T_TITLE; t.logo = T_LOGO; t.edition = T_EDITION;
    return t;
}

Texture2D* GuiDeviceGL::tex(uint32_t id) {
    switch (id) {
    case T_FONT: return m_r->texture("gui/font03_P.p2d", false);
    case T_ELEMENTS: return m_r->texture("gui/genericGuiElements_UN.p2d");
    case T_SHOP_ALPHA: return m_r->texture("gui/genericShopGUIAlpha_UN.p2d");
    case T_SHOP_BG: return m_r->texture("gui/genericShopBackground_PN.p2d");
    case T_GESTURES: return m_r->texture("gui/genericTouchGestures_N.p2d");
    case T_MAP: return m_r->texture("gui/GUIMap_N.p2d");
    case T_TITLE: return m_r->texture("gui/genericTitleScreen_PN.p2d");
    case T_LOGO: return m_r->texture(m_logo.c_str());
    case T_EDITION: return m_r->texture("gui/MultipEditionStamp_U.p2d");
    default: return nullptr;
    }
}

void GuiDeviceGL::beginFrame() {
    m_r->beginOverlay();
    static const float w[4] = {1, 1, 1, 1};
    setOverlayColor(w); setTextColor(w);
}

void GuiDeviceGL::renderOverlay(uint32_t id, float x, float y, float w, float h, float u, float v, float du, float dv) {
    // Renderer::overlay orijinalin (u, 1-v, du, -dv) / (2x-1, 1-2y, 2w, -2h) kodlamasini kullanir
    m_r->overlay(tex(id), x, y, w, h, u, v, du, dv, m_ov, false);
}

// Orijinal renderText(font, x, y, flags, char*): dikey: bit4 ortala, bit5 alta; satir yuksekligi = boyut/320 * dikeyOlcek.
// Yatay (her satir): bit1 ortala, bit2 saga. '\n': y += satirYuksekligi * 1.1.
void GuiDeviceGL::renderText(uint32_t, float x, float y, uint32_t flags, const char* utf8) {
    if (!utf8) return;
    float asp = m_r->aspect(), s0, s1;     // dev[0xbf0c], dev[0xbf10]
    if (!(asp > 1.5f)) { s0 = 1.0f; s1 = asp / 1.5f; } else { s0 = 1.5f / asp; s1 = 1.0f; }
    float scale = m_fontSize / 64.0f;      // yazi tipi dogal boyutu 64
    float sx = scale * s0, sy = scale * s1;
    float lineH = (m_fontSize / 320.0f) * s1;
    if (flags & 0x10) y -= lineH * 0.5f; else if (flags & 0x20) y -= lineH;
    std::string line;
    for (const char* p = utf8;; ++p) {
        if (*p == '\n' || *p == 0) {
            if (!line.empty()) drawLine(x, y, flags, line, sx, sy);
            line.clear();
            if (*p == 0) break;
            y += lineH * 1.1f;
        } else line.push_back(*p);
    }
}

void GuiDeviceGL::drawLine(float x, float y, uint32_t flags, const std::string& line, float sx, float sy) {
    int al = (flags & 2) ? 1 : ((flags & 4) ? 2 : 0);
    std::vector<GlyphQuad> q;
    float asp = m_r->aspect(), s0 = !(asp > 1.5f) ? 1.0f : 1.5f / asp;
    m_font.layoutXY(line.c_str(), x, y, sx, sy, al, q, m_monoOn ? m_monoW / 480.0f * s0 : 0.0f);
    Texture2D* ft = tex(T_FONT);
    for (const GlyphQuad& g : q) m_r->overlay(ft, g.x, g.y, g.w, g.h, g.u, g.v, g.du, g.dv, m_tx, true);
}

} // namespace fs12
#endif
