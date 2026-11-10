#include "game/CowHerd.h"
#include <cstdio>
#include <cmath>
using namespace fs12;
static int bad = 0;
#define CHECK(c) do { if (!(c)) { printf("HATA satir %d: %s\n", __LINE__, #c); ++bad; } } while (0)
int main() {
    CowHerd h; h.setPlacement(100.f, 0.f, -50.f, 0.f);
    const CowSlot* s = h.slots();
    // gunduz hepsi ayakta
    h.update(12.f); for (int i = 0; i < CowHerd::kCount; ++i) CHECK(!s[i].sleeping);
    // gece tekler yatar
    h.update(23.f); int sl = 0; for (int i = 0; i < CowHerd::kCount; ++i) sl += s[i].sleeping; CHECK(sl == 3);
    // inekler ust uste binmez (min 2 m)
    for (int i = 0; i < CowHerd::kCount; ++i) for (int j = i + 1; j < CowHerd::kCount; ++j) {
        float dx = s[i].x - s[j].x, dz = s[i].z - s[j].z; CHECK(sqrtf(dx * dx + dz * dz) > 2.0f);
    }
    // ayni yerlesim ayni sonucu verir
    CowHerd h2; h2.setPlacement(100.f, 0.f, -50.f, 0.f); CHECK(h2.slots()[3].x == s[3].x && h2.slots()[3].yaw == s[3].yaw);
    printf(bad ? "BASARISIZ (%d)\n" : "TUM TESTLER GECTI\n", bad); return bad ? 1 : 0;
}
