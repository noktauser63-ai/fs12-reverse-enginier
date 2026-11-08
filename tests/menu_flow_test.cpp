// MenuApp akisi: ana menu -> kayit -> zorluk -> baslangic menusu, geri tusu, cizim cokmeden calisir.
#include "game/MenuApp.h"
#include "platform/InputDevice.h"
#include "platform/SystemDevice.h"
#include "data/Font.h"
#include "render/Font.h"
#include <cstring>
#include <vector>
#include <cstdio>
#include <initializer_list>
using namespace fs12;

struct CountDev : GuiRenderDevice {
    int ov = 0, tx = 0;
    float aspect() const override { return 16.f / 9.f; }
    void setOverlayColor(const float*) override {}
    void setTextColor(const float*) override {}
    void setFontSize(float) override {}
    void renderOverlay(uint32_t, float, float, float, float, float, float, float, float) override { ++ov; }
    void renderText(uint32_t, float, float, uint32_t, const char*) override { ++tx; }
};

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("HATA satir %d: %s\n", __LINE__, #c); ++fails; } } while (0)

static void tapAt(MenuApp& m, InputDevice* in, float x, float y) {
    InputDevice::Touch t; t.id = 1; t.x = t.prevX = t.startX = x; t.y = t.prevY = t.startY = y; t.flags = 1;
    in->debugSetTouches(&t, 1); m.update(in, 0.016f); in->updateFrameEnd();
    t.flags = 2; in->debugSetTouches(&t, 1); m.update(in, 0.016f); in->updateFrameEnd();
    in->debugSetTouches(nullptr, 0);
}

int main() {
    InputDevice* in = InputDevice::create();
    CountDev dev;
    GenericGUIManager::Textures tex{}; tex.font = 1; tex.elements = 2; tex.title = 7;
    MenuApp m(&dev, tex, 0);
    CHECK(m.ctl().state == 2);
    for (int i = 0; i < 100; ++i) { in->debugSetTouches(nullptr, 0); m.update(in, 0.016f); }
    CHECK(m.ctl().menuActive);
    m.render(); CHECK(dev.ov > 0 && dev.tx > 0);
    m.handleNext(0x24); CHECK(m.ctl().state == 4);
    dev.ov = dev.tx = 0; m.render(); CHECK(dev.tx > 0);
    m.handleNext(9); CHECK(m.ctl().state == 2);
    m.handleNext(0x25); CHECK(m.ctl().state == 3);
    for (int i = 0; i < 6; ++i) m.update(in, 0.5f);   // krediler asagidan yukari kayar
    dev.ov = dev.tx = 0; m.render(); CHECK(dev.tx > 0);
    tapAt(m, in, 0.5f, 0.5f); CHECK(m.ctl().state == 2);
    m.handleNext(0x24); m.handleNext(13); CHECK(m.ctl().state == 4 && m.ctl().dialog == 1);   // silme onayi
    dev.tx = 0; m.render(); CHECK(dev.tx > 0);
    m.handleNext(7); CHECK(m.ctl().dialog == 0);
    m.handleNext(10); CHECK(m.ctl().state == 5);
    dev.tx = 0; m.render(); CHECK(dev.tx >= 4);
    m.handleNext(0x11); CHECK(m.ctl().state == 7 && m.inGame());
    m.pauseGame(); CHECK(!m.inGame() && m.playing() && m.ctl().state == 7);
    m.handleNext(9); CHECK(m.inGame());                    // geri: oyuna don
    m.pauseGame(); m.handleNext(8); CHECK(!m.playing() && m.ctl().state == 2);   // cikis: basliga
    m.handleNext(0x24); m.handleNext(10); m.handleNext(0x11); m.pauseGame();
    for (int p : {0, 4, 5}) { m.gui().wr(0x58cc, p); dev.ov = dev.tx = 0; m.render(); CHECK(dev.ov > 0); m.update(in, 0.016f); }
    m.handleNext(5); CHECK(m.ctl().state == 2);
    // Turkce: dil kodu, metin tablosu, ek glifler
    CHECK(languageFromCode("tr") == LANG_TR && languageFromCode("tr-TR") == LANG_TR && languageFromCode("en") == LANG_EN);
    CHECK(strcmp(GenericGUIManager::getString(4, LANG_TR), "Yeni Oyun") == 0);
    CHECK(strcmp(GenericGUIManager::getString(4, LANG_EN), "New Game") == 0);
    {
        std::vector<FontInfo> t(kFontData03, kFontData03 + kFontData03Count);
        t.insert(t.end(), kFontTr, kFontTr + kFontTrCount);
        Font f(t.data(), (int)t.size(), 2048, 2048), g(kFontData03, kFontData03Count, 2048, 2048);
        const char* tr = "\xc3\x87\xc4\x9f\xc4\xb1\xc4\xb0\xc5\x9f\xc5\x9e\xc3\x9c\xc4\x9e";   // Ç ğ ı İ ş Ş Ü Ğ
        CHECK(f.width(tr, 1.f, 16.f / 9.f) > g.width(tr, 1.f, 16.f / 9.f) + 0.05f);
    }
    printf(fails ? "menu_flow_test: %d hata\n" : "menu_flow_test: tamam\n", fails);
    return fails ? 1 : 0;
}
