#include "game/GameState.h"
#include <cstring>
namespace fs12 {
static const int kSell = 70;   // satis: fiyatin %70'i
static bool drivable(const VehicleSpec& v) { return v.kind == VKind::Tractor || v.kind == VKind::Combine || v.kind == VKind::Pickup; }
static bool shopKind(const VehicleSpec&) { return true; }   // katalogdaki her sey dukkanda (ikon atlasi katalog sirasinda)
int GameState::shopCount() { size_t n; const VehicleSpec* v = vehicleSpecs(n); int c = 0; for (size_t i = 0; i < n; ++i) c += shopKind(v[i]); return c; }
int GameState::specIndex(int slot) {
    size_t n; const VehicleSpec* v = vehicleSpecs(n); int c = 0;
    for (size_t i = 0; i < n; ++i) if (shopKind(v[i])) { if (c == slot) return (int)i; ++c; }
    return -1;
}
GameState::GameState() {
    size_t n; const VehicleSpec* v = vehicleSpecs(n);
    for (size_t i = 0; i < n; ++i) if (!strcmp(v[i].id, "agrotronK420") || !strcmp(v[i].id, "deutz5465H")) owned[i] = 1;
    for (size_t i = 0; i < n; ++i) if (owned[i]) { active = (int)i; break; }
}
bool GameState::canBuy(int slot) const { int i = specIndex(slot); size_t n; const VehicleSpec* v = vehicleSpecs(n); return i >= 0 && !owned[i] && money >= v[i].price; }
bool GameState::canSell(int slot) const {
    int i = specIndex(slot); if (i < 0 || !owned[i]) return false;
    size_t n; const VehicleSpec* v = vehicleSpecs(n);
    if (!drivable(v[i])) return true;
    int c = 0; for (size_t k = 0; k < n; ++k) c += owned[k] && drivable(v[k]);
    return c > 1;      // en az bir surulebilir arac kalmali
}
bool GameState::buy(int slot) {
    if (!canBuy(slot)) return false;
    int i = specIndex(slot); size_t n; const VehicleSpec* v = vehicleSpecs(n);
    money -= v[i].price; owned[i] = 1; if (drivable(v[i])) active = i; return true;
}
bool GameState::sell(int slot) {
    if (!canSell(slot)) return false;
    int i = specIndex(slot); size_t n; const VehicleSpec* v = vehicleSpecs(n);
    money += (int)((int64_t)v[i].price * kSell / 100); owned[i] = 0;
    if (active == i) { active = -1; for (int k = 0; k < (int)n; ++k) if (owned[k] && drivable(v[k])) { active = k; break; } }
    return true;
}
bool GameState::select(int slot) { int i = specIndex(slot); size_t n; const VehicleSpec* v = vehicleSpecs(n); if (i < 0 || !owned[i] || !drivable(v[i])) return false; active = i; return true; }
const VehicleSpec* GameState::activeSpec() const { size_t n; const VehicleSpec* v = vehicleSpecs(n); return active >= 0 && active < (int)n ? &v[active] : nullptr; }
std::vector<uint8_t> GameState::toBytes() const {
    std::vector<uint8_t> b(4 + 4 + 4 + 32); memcpy(&b[0], "FS12", 4); memcpy(&b[4], &money, 4); memcpy(&b[8], &active, 4); memcpy(&b[12], owned, 32); return b;
}
bool GameState::fromBytes(const uint8_t* d, size_t n) {
    if (n < 44 || memcmp(d, "FS12", 4)) return false;
    GameState t; memcpy(&t.money, d + 4, 4); memcpy(&t.active, d + 8, 4); memcpy(t.owned, d + 12, 32);
    size_t cnt; vehicleSpecs(cnt);
    if (t.active < -1 || t.active >= (int)cnt) return false;
    *this = t; return true;
}
}
