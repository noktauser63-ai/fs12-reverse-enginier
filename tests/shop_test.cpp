// Dukkan: katalog karuseli, satin alma/satis, durum kaydi.
#include "game/MenuApp.h"
#include "platform/InputDevice.h"
#include <cstdio>
#include <string>
using namespace fs12;
struct CountDev : GuiRenderDevice {
    int ov = 0, tx = 0; std::string last;
    float aspect() const override { return 16.f / 9.f; }
    void setOverlayColor(const float*) override {} void setTextColor(const float*) override {} void setFontSize(float) override {}
    void renderOverlay(uint32_t, float, float, float, float, float, float, float, float) override { ++ov; }
    void renderText(uint32_t, float, float, uint32_t, const char* t) override { ++tx; last = t; }
};
static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("HATA satir %d: %s\n", __LINE__, #c); ++fails; } } while (0)
int main() {
    InputDevice* in = InputDevice::create();
    CountDev dev; GenericGUIManager::Textures tex{}; tex.font = 1; tex.elements = 2; tex.title = 7;
    MenuApp m(&dev, tex, 0);
    for (int i = 0; i < 100; ++i) { in->debugSetTouches(nullptr, 0); m.update(in, 0.016f); }
    m.setShopTexture(9);
    CHECK(m.ctl().shop.n == 9 && m.ctl().shop.extTex == 9);
    printf("butonlar: sell id=%d buy id=%d coin id=%d\n", m.gui().button((int)m.gui().rd(0x5814)).id, m.gui().button((int)m.gui().rd(0x5818)).id, m.gui().button((int)m.gui().rd(0x581c)).id);
    // K420 (0) ve 5465H (3) sahip; Magnum (2) alinabilir olmali (450000 >= 345000)
    CHECK(m.state().owned[GameState::specIndex(0)] && m.state().owned[GameState::specIndex(3)]);
    CHECK(m.state().canBuy(2) && !m.state().canBuy(0) && m.state().canSell(0));
    m.startGame(); m.pauseGame();
    m.gui().wr(0x58cc, 1); m.gui().wr(0x583c, 2);
    m.update(in, 0.016f); dev.ov = dev.tx = 0; m.render(); CHECK(dev.ov > 5 && dev.tx > 3);
    int before = m.state().money;
    m.handleNext(0x19);   // al (secili: Magnum)
    CHECK(m.state().owned[GameState::specIndex(2)] && m.state().money == before - 345000 && m.state().active == GameState::specIndex(2));
    m.handleNext(0x1a);   // sat
    CHECK(!m.state().owned[GameState::specIndex(2)] && m.state().money == before - 345000 + 241500);
    auto b = m.state().toBytes(); GameState g; g.money = 1; CHECK(g.fromBytes(b.data(), b.size()) && g.money == m.state().money && g.active == m.state().active);
    printf(fails ? "shop_test: %d HATA\n" : "shop_test: tamam\n", fails);
    return fails ? 1 : 0;
}
