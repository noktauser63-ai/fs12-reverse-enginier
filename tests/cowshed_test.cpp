// Cowshed: kapasite, gunluk sut dengesi, kayit gidis-donus.
#include "game/Cowshed.h"
#include <cstdio>
#include <cmath>
using namespace fs12;
static int bad = 0;
#define CHECK(c) do { if (!(c)) { printf("HATA satir %d: %s\n", __LINE__, #c); ++bad; } } while (0)

int main() {
    Cowshed c;
    CHECK(c.addBales(BaleType::Hay, 40) == 32);       // 32 ustu reddedilir
    CHECK(c.addBales(BaleType::Hay, 1) == 0);
    CHECK(c.addBales(BaleType::Straw, 10) == 10);
    CHECK(c.hayBales() == 32 && c.strawBales() == 10);

    // 3 oyun gunu, 1 dk adim
    const float dt = 60.f;
    for (int i = 0; i < 3 * 24 * 60; ++i) c.update(dt, std::fmod(i * dt / 3600.f, 24.f));
    // Gunde en fazla 2 sagim saati * 1000 L = 2000 L; 3 gunde <= 6000 L ve > 0
    CHECK(c.milk() > 0.f && c.milk() <= 6000.5f);
    CHECK(c.manure() > 0.f);
    printf("3 gun: sut=%.1f gubre=%.1f ot=%d saman=%d\n", c.milk(), c.manure(), c.hayBales(), c.strawBales());

    // Tank siniri
    Cowshed d; d.addBales(BaleType::Hay, 32);
    for (int i = 0; i < 40 * 24 * 60; ++i) d.update(dt, std::fmod(i * dt / 3600.f, 24.f));
    CHECK(d.milk() <= 14000.f + 0.01f);

    // Yem yoksa sut yok, saman yoksa gubre yok
    Cowshed e; for (int i = 0; i < 24 * 60; ++i) e.update(dt, std::fmod(i * dt / 3600.f, 24.f));
    CHECK(e.milk() == 0.f && e.manure() == 0.f);

    // Satis
    float money = c.sellAllMilk();
    CHECK(c.milk() == 0.f && money > 0.f);

    // Kayit
    auto b = c.serialize();
    Cowshed f; CHECK(f.deserialize(b.data(), b.size()));
    CHECK(f.hayBales() == c.hayBales() && f.manure() == c.manure() && f.milk() == c.milk());
    b[0] = 9; CHECK(!f.deserialize(b.data(), b.size()));

    printf(bad ? "BASARISIZ (%d)\n" : "TUM TESTLER GECTI\n", bad);
    return bad ? 1 : 0;
}
