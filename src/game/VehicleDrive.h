// VehicleDrive: basit arac surusu (bisiklet modeli) + takip kamerasi. Orijinal Vehicle/Wheel fizigi (Box2D) gelene kadar gecici,
// ama tekerlek konumlari/ayarlar orijinal EntityManager tablosundan. Model: ileri = +z (yaw 0), dunya ileri = (sin yaw, cos yaw).
#pragma once
#include "platform/InputDevice.h"
#include "game/MapData.h"
#include "game/MapPhysics.h"
#include <cmath>

namespace fs12 {
struct VehicleDrive {
    float x = 10.f, z = -3.f, yaw = 0.5f, speed = 0.f, steer = 0.f;
    float y = 0.f, pitch = 0.f, spin = 0.f;
    float wheelBase = 3.6329f;           // on aks z=+2.291, arka aks z=-1.3419
    float maxSpeed = 8.0f, maxSteer = 0.55f;
    float camYaw = 0.f;                  // takip kamerasi (WorldView yaw'i)
    float camPos[3] = {0, 0, 0}, camPitch = 0.3f;
    float throttle = 0.f, steerIn = 0.f; // -1..1 (dokunmatik/test girisi)
    uint64_t gasId = 0, steerId = 0; bool gas = false, str = false;

    void readTouch(InputDevice* in) {
        bool sg = false, ss = false;
        for (int i = 0; i < in->numTouches(); ++i) {
            const InputDevice::Touch* t = in->touch(i);
            if (t->flags & 2) continue;
            if (t->flags & 1) {
                if (t->x < 0.4f && !gas) { gas = true; gasId = t->id; }
                else if (t->x >= 0.4f && !str) { str = true; steerId = t->id; }
            }
            if (gas && t->id == gasId) { sg = true; throttle = clamp1(-(t->y - t->startY) * 5.f); }
            if (str && t->id == steerId) { ss = true; steerIn = clamp1((t->x - t->startX) * 6.f); }
        }
        if (!sg) { gas = false; throttle = 0; }
        if (!ss) { str = false; steerIn = 0; }
    }
    MapPhysics* phys = nullptr;           // varsa: carpisma Box2D ile
    void update(float dt, const MapData* map) {
        float target = throttle * maxSpeed;
        if (throttle < 0) target = throttle * maxSpeed * 0.5f;
        const float rate = (std::fabs(target) > std::fabs(speed) && target * speed >= 0) ? 3.0f : 6.0f;
        if (speed < target) { speed += rate * dt; if (speed > target) speed = target; }
        else if (speed > target) { speed -= rate * dt; if (speed < target) speed = target; }
        if (throttle == 0.f) speed *= (1.f - 0.8f * dt);
        const float ts = steerIn * maxSteer;
        steer += (ts - steer) * std::fmin(1.f, 8.f * dt);
        const float yawRate = speed / wheelBase * std::tan(steer);
        if (phys) {
            phys->step(dt, speed, yawRate);
            x = phys->x(); z = phys->z(); yaw = phys->yaw();
            const float fs = phys->forwardSpeed();
            if (std::fabs(fs) < std::fabs(speed)) speed = fs;   // carpismada yavasla
        } else {
            yaw += yawRate * dt;
            x += std::sin(yaw) * speed * dt; z += std::cos(yaw) * speed * dt;
        }
        spin += speed * dt / 0.9f;
        float hf = 0, hb = 0, h;
        const float sx = std::sin(yaw), cx = std::cos(yaw);
        bool okF = map && map->groundHeight(x + sx * 2.291f, z + cx * 2.291f, hf);
        bool okB = map && map->groundHeight(x - sx * 1.3419f, z - cx * 1.3419f, hb);
        if (okF && okB) { pitch += (std::atan2(hf - hb, wheelBase) - pitch) * std::fmin(1.f, 10.f * dt); y += ((hf + hb) * 0.5f - y) * std::fmin(1.f, 12.f * dt); }
        else if (map && map->groundHeight(x, z, h)) y = h;
        // takip kamerasi: aracin arkasi
        const float tyaw = 3.14159265f - yaw;
        float d = tyaw - camYaw; while (d > 3.14159265f) d -= 6.2831853f; while (d < -3.14159265f) d += 6.2831853f;
        camYaw += d * std::fmin(1.f, 3.f * dt);
        camPos[0] = x - std::sin(-camYaw + 3.14159265f) * 0.f - std::sin(3.14159265f - camYaw) * 9.f;
        camPos[2] = z - std::cos(3.14159265f - camYaw) * 9.f;
        float gh = y; if (map && map->groundHeight(camPos[0], camPos[2], gh)) {}
        camPos[1] = std::fmax(y + 4.2f, gh + 1.5f);
    }
private:
    static float clamp1(float v) { return v < -1.f ? -1.f : (v > 1.f ? 1.f : v); }
};
} // namespace fs12
