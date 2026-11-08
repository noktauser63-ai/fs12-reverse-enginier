#include "RandomGenerator.h"

#include <string.h>

RandomGenerator RandomGenerator::SharedGenerator;

float RandomGenerator::getRandomFloat() {
    // Orijinaldeki double carpan: 0x3DEFFFFFFFDFFFFF (~2.3283064365386963e-10)
    static const uint64_t kBits = 0x3DEFFFFFFFDFFFFFull;
    double k;
    memcpy(&k, &kBits, sizeof(k));
    return (float)((double)getRandom() * k);
}
