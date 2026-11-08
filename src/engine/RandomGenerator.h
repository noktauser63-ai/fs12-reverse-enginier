// Orijinal: RandomGenerator (libapp.so 0x5b9fc-0x5babc). Iki 16-bit carry'li carpimli uretec (MWC).
// Kayit dosyalarindaki tarlalar bu uretece bagli oldugu icin ayni sayi dizisini uretmek zorunlu.
#pragma once
#include <stdint.h>

class RandomGenerator {
public:
    RandomGenerator() : s0(0x0a8dc1edu), s1(0x1fd3f0f5u) {}

    uint32_t getRandom() {
        s0 = (s0 & 0xffffu) * 0x9069u + (s0 >> 16);
        s1 = (s1 & 0xffffu) * 18000u + (s1 >> 16);
        return (s0 << 16) + s1;
    }
    float getRandomFloat();  // [0,1): getRandom() * (cok az 2^-32'den kucuk) double sabiti
    int getRandomMinMax(int minV, int maxV) { return minV + (int)(getRandom() % (uint32_t)((maxV - minV) + 1)); }
    float getRandomMinMaxFloat(float minV, float maxV) { return minV + (maxV - minV) * getRandomFloat(); }
    uint64_t getSeed() const { return ((uint64_t)s0 << 32) | s1; }
    void setSeed(uint64_t seed) { s0 = (uint32_t)(seed >> 32); s1 = (uint32_t)seed; }

    static RandomGenerator SharedGenerator;

private:
    uint32_t s0, s1;
};
