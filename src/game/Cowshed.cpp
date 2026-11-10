#include "game/Cowshed.h"
#include <algorithm>
#include <cstring>

namespace fs12 {

int Cowshed::addBales(BaleType t, int count) {
    if (count <= 0) return 0;
    int& slot = (t == BaleType::Hay) ? hay : straw;
    int cap = (t == BaleType::Hay) ? prm.maxHayBales : prm.maxStrawBales;
    int ok = std::min(count, std::max(0, cap - slot));
    slot += ok;
    return ok;
}

void Cowshed::update(float dt, float hour) {
    if (dt <= 0.f) return;
    const float x = dt / prm.daySeconds;     // gunun kesri

    // --- Yem: yemlik bossa depodan balya al, "gunluk" orandan ye, yenen kadar sut potansiyeli birikir ---
    if (hayTrough < 0.001f && hay > 0) { --hay; hayTrough = prm.baleVolume; }
    {
        float want = x * prm.feedPerDay;
        // Orijinal decompile, yem yetmediginde "+0.0" gorunuyor (softfp operandi kaybolmus olabilir).
        // Burada yemlikte kalan kadari yenir.
        float eaten = std::min(want, hayTrough);
        hayTrough -= eaten;
        milkPotential += eaten;
    }

    // --- Altlik: sadece saman varken gubre uretilir ---
    if (strawBed < 0.001f && straw > 0) { --straw; strawBed = prm.baleVolume; }
    if (strawBed > 0.f) {
        float use = std::min(x * prm.strawPerDay, strawBed);
        manureLevel = std::min(manureLevel + x * prm.manurePerDay, prm.maxManure);
        strawBed -= use;
    }

    // --- Sagim: sabah/aksam bir saat; potansiyel bosalir, tanka sut girer ---
    bool morning = hour > prm.morningStart && hour < prm.morningEnd;
    bool evening = hour > prm.eveningStart && hour < prm.eveningEnd;
    if ((morning || evening) && milkPotential > 0.f) {
        float frac = hour - (morning ? prm.morningStart : prm.eveningStart);  // saat icindeki ilerleme
        float remaining = (1.f - frac) * prm.milkPotentialDrainPerHour;       // (1-frac)*6000*0.5
        float rate = (remaining <= milkPotential)
                         ? prm.milkOutPerHour
                         : (milkPotential / remaining) * prm.milkOutPerHour;
        milkTank = std::min(milkTank + (dt / 3600.f) * rate, prm.milkTankMax);
        milkPotential = std::max(0.f, milkPotential - (dt / 3600.f) * prm.milkPotentialDrainPerHour);
    }
}

float Cowshed::takeMilk(float l) { float t = std::max(0.f, std::min(l, milkTank)); milkTank -= t; return t; }
float Cowshed::takeManure(float l) { float t = std::max(0.f, std::min(l, manureLevel)); manureLevel -= t; return t; }

// Kayit duzeni (v1): u32 surum, i32 hay, i32 straw, f32 hayTrough, strawBed, milkPotential, milkTank, manureLevel
std::vector<uint8_t> Cowshed::serialize() const {
    std::vector<uint8_t> o(4 + 4 + 4 + 5 * 4);
    auto w32 = [&](size_t off, const void* p) { std::memcpy(&o[off], p, 4); };
    uint32_t ver = 1; int32_t h = hay, s = straw;
    w32(0, &ver); w32(4, &h); w32(8, &s);
    w32(12, &hayTrough); w32(16, &strawBed); w32(20, &milkPotential); w32(24, &milkTank); w32(28, &manureLevel);
    return o;
}

bool Cowshed::deserialize(const uint8_t* d, size_t n) {
    if (!d || n < 32) return false;
    uint32_t ver; std::memcpy(&ver, d, 4);
    if (ver != 1) return false;
    int32_t h, s; std::memcpy(&h, d + 4, 4); std::memcpy(&s, d + 8, 4);
    if (h < 0 || s < 0 || h > prm.maxHayBales || s > prm.maxStrawBales) return false;
    hay = h; straw = s;
    std::memcpy(&hayTrough, d + 12, 4); std::memcpy(&strawBed, d + 16, 4);
    std::memcpy(&milkPotential, d + 20, 4); std::memcpy(&milkTank, d + 24, 4); std::memcpy(&manureLevel, d + 28, 4);
    return true;
}

} // namespace fs12
