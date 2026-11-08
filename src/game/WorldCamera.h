// WorldCamera: serbest kamera dokunmatik kontrolu. Sol %40: hareket (y: ileri/geri, x: yan), geri kalan: bakis.
#pragma once
#include "platform/InputDevice.h"
#include <cmath>

namespace fs12 {
struct WorldCamera {
    float pos[3] = {0.f, 2.5f, 0.f};
    float yaw = 0.f, pitch = 0.4f;
    uint64_t moveId = 0, lookId = 0;
    bool moving = false, looking = false;
    float speed = 18.0f;                 // m/s (tam kaydirma)
    void update(InputDevice* in, float dt) {
        bool seenM = false, seenL = false;
        for (int i = 0; i < in->numTouches(); ++i) {
            const InputDevice::Touch* t = in->touch(i);
            if (t->flags & 2) continue;
            if (t->flags & 1) {
                if (t->x < 0.4f && !moving) { moving = true; moveId = t->id; }
                else if (t->x >= 0.4f && !looking) { looking = true; lookId = t->id; }
            }
            if (moving && t->id == moveId) {
                seenM = true;
                float dx = t->x - t->startX, dy = t->y - t->startY;
                float fwd = clamp1(-dy * 5.f), side = clamp1(dx * 5.f);
                float s = std::sin(yaw), c = std::cos(yaw);
                pos[0] += (s * fwd + c * side) * speed * dt;
                pos[2] += (-c * fwd + s * side) * speed * dt;
            } else if (looking && t->id == lookId) {
                seenL = true;
                yaw += (t->x - t->prevX) * 3.2f;
                pitch += (t->y - t->prevY) * 2.0f;
                if (pitch < -0.6f) pitch = -0.6f;
                if (pitch > 1.4f) pitch = 1.4f;
            }
        }
        if (!seenM) moving = false;
        if (!seenL) looking = false;
    }
private:
    static float clamp1(float v) { return v < -1.f ? -1.f : (v > 1.f ? 1.f : v); }
};
} // namespace fs12
