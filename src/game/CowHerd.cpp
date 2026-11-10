#include "game/CowHerd.h"
#include "engine/RandomGenerator.h"
#include <cmath>

namespace fs12 {

void CowHerd::setPlacement(float x, float y, float z, float yaw) {
    m_cx = x; m_y = y; m_cz = z; m_yaw = yaw;
    // Model boyutu ~1.05 x 3.44 m (cowStanding): yan yana 2.4 m, arka arkaya 4.2 m aralik.
    RandomGenerator rng;                     // yerel, sabit tohum -> her acilista ayni dizilim
    const float dx = 2.4f, dz = 4.2f;
    for (int i = 0; i < kCount; ++i) {
        const int row = i / 4, col = i % 4;   // 4 + 3 inek
        const float lx = (col - 1.5f) * dx + (row ? dx * 0.5f : 0.f);
        const float lz = (row - 0.5f) * dz;
        const float c = cosf(yaw), s = sinf(yaw);
        m_off[i][0] = lx * c + lz * s;        // yaw ile don
        m_off[i][1] = -lx * s + lz * c;
        m_yawJit[i] = (rng.getRandomFloat() - 0.5f) * 0.5f;
        m_slot[i].skin = i & 1;
    }
    update(12.f);
}

void CowHerd::update(float hour) {
    const bool night = isNight(hour);
    for (int i = 0; i < kCount; ++i) {
        m_slot[i].x = m_cx + m_off[i][0];
        m_slot[i].z = m_cz + m_off[i][1];
        m_slot[i].yaw = m_yaw + m_yawJit[i];
        m_slot[i].sleeping = night && (i & 1);   // geceleri tekler yatar, ciftler ayakta
    }
}

} // namespace fs12
