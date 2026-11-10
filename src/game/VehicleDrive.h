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
    float wheelRadius = 0.9f;
    float wheelBase = 3.6329f;           // on aks z=+2.291, arka aks z=-1.3419
    float maxSpeed = 8.3333333f, maxSteer = 0.55f;
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
    // Yakit (orijinal Vehicle: depo desc+0x138, tuketim desc+0x13c L/m, bos depoda hiz tavani 4.2 m/s)
    float fuelTank = 900.f, fuelRate = 0.015f, fuel = 900.f, emptyT = 0.f;
    float thr = 0.f;                      // yumusatilmis gaz (-1..1), orijinal Vehicle::update gaz rampasi
    void setFuel(float tank, float rate) { fuelTank = tank; fuelRate = rate; fuel = tank; thr = 0; }
    void update(float dt, const MapData* map) {
        // gaz rampasi: <=6.9 m/s'de 2.0/s, >=16.6 m/s'de 1.6/s (cizgisel), insan surusunde x1.5
        const float av = std::fabs(speed);
        const float tt = av <= 6.9f ? 0.f : (av >= 16.6f ? 1.f : (av - 6.9f) / 9.7f);
        const float ramp = (2.0f + (1.6f - 2.0f) * tt) * 1.5f;
        const float tgt = throttle;
        if (thr < tgt) { thr += ramp * dt; if (thr > tgt) thr = tgt; }
        else if (thr > tgt) { thr -= ramp * dt; if (thr < tgt) thr = tgt; }
        float vmax = maxSpeed;
        if (fuel <= 0.f) vmax = std::fmin(vmax, 4.2f);
        float target = thr * vmax;
        if (thr < 0) target *= 0.7f;                             // geri vites x0.7
        const float k = 1.f - std::pow(0.85f, dt * 60.f);        // 0.85/0.15 kare basina yumusatma
        speed += (target - speed) * k;
        if (std::fabs(speed) < 0.02f && throttle == 0.f) speed = 0.f;
        fuel -= std::fabs(speed) * dt * fuelRate; if (fuel < 0.f) fuel = 0.f;
        if (fuel <= 0.f && std::fabs(speed) < 0.1f) { emptyT += dt; if (emptyT > 8.f) { fuel = fuelTank; emptyT = 0.f; } } else emptyT = 0.f;
        // direksiyon: hiza gore azalan aci (yuksek hizda keskin donus yok), yumusak donus
        const float lim = maxSteer * (1.f - 0.55f * std::fmin(1.f, std::fabs(speed) / std::fmax(1.f, maxSpeed)));
        const float ts = steerIn * lim;
        steer += (ts - steer) * std::fmin(1.f, 5.f * dt);
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
        spin += speed * dt / wheelRadius;
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
