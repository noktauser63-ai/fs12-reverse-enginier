// GuiDeviceGL: GuiRenderDevice'in Renderer (GLES2) uzerindeki gercek uygulamasi.
// Orijinal GLESHandheldRenderDevice::renderText / enqueueSet* mantigi (yazi hizalama, font olcegi) korunur.
#pragma once
#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/GenericGUIManager.h"
#include "data/Font.h"
#include "render/Font.h"
#include "render/Renderer.h"
#include <string>
#include <vector>

namespace fs12 {

class GuiDeviceGL : public GuiRenderDevice {
public:
    // doku kimlikleri (GenericGUIManager::Textures icin)
    enum TexId : uint32_t { T_NONE = 0, T_FONT = 1, T_ELEMENTS, T_SHOP_ALPHA, T_SHOP_BG, T_GESTURES, T_MAP, T_TITLE, T_LOGO, T_EDITION };

    GuiDeviceGL(Renderer* r, const char* language);
    GenericGUIManager::Textures textures() const;

    void beginFrame();   // alfa karisim + birim kare (overlay modu)
    float aspect() const override { return m_r->aspect(); }
    void setOverlayColor(const float* rgba) override { for (int i = 0; i < 4; ++i) m_ov[i] = rgba[i]; }
    void setTextColor(const float* rgba) override { for (int i = 0; i < 4; ++i) m_tx[i] = rgba[i]; }
    void setFontSize(float size) override { m_fontSize = size; }
    void setNumberMonospace(bool on, float w) override { m_monoOn = on; m_monoW = w; }
    void renderOverlay(uint32_t tex, float x, float y, float w, float h, float u, float v, float du, float dv) override;
    void renderText(uint32_t font, float x, float y, uint32_t flags, const char* utf8) override;

private:
    Texture2D* tex(uint32_t id);
    void drawLine(float x, float y, uint32_t flags, const std::string& line, float sx, float sy);
    Renderer* m_r;
    std::vector<FontInfo> m_table;   // orijinal tablo + Turkce ek glifler (kFontTr)
    Font m_font;
    std::string m_logo;
    float m_ov[4] = {1, 1, 1, 1}, m_tx[4] = {1, 1, 1, 1};
    float m_fontSize = 20.f;
    bool m_monoOn = false; float m_monoW = 0.f;
};

} // namespace fs12
#endif
