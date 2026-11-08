// Arac, haritanin statik cokgenlerinden gecmemeli (Box2D).
#include "game/MapData.h"
#include "game/MapPhysics.h"
#include "game/VehicleDrive.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
using namespace fs12;
int main(int argc, char** argv) {
    std::ifstream f(argc > 1 ? argv[1] : "map01.m3d", std::ios::binary);
    if (!f) { printf("mapphysics_test: harita yok, atlandi\n"); return 0; }
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), {});
    MapData m; if (!m.parse(d.data(), d.size())) { printf("HATA: harita\n"); return 1; }
    MapPhysics mp; mp.build(m);
    int bad = 0;
    auto chk = [&](bool c, const char* w) { if (!c) { printf("HATA: %s\n", w); ++bad; } };
    chk(mp.staticCount() >= 1300, "statik cokgen sayisi");
    // yalitilmis bir engel bul: etrafinda (x +-8, z 0..45) baska cokgen olmayan, genisligi 1.5..8 m olan
    auto box = [&](const MapPolygon& g, float& x0, float& x1, float& z0, float& z1) {
        float px, pz; memcpy(&px, &g.a, 4); memcpy(&pz, &g.b, 4); x0 = z0 = 1e9f; x1 = z1 = -1e9f;
        for (size_t i = 0; i < g.pts.size(); i += 2) { x0 = std::fmin(x0, px + g.pts[i]); x1 = std::fmax(x1, px + g.pts[i]); z0 = std::fmin(z0, pz + g.pts[i + 1]); z1 = std::fmax(z1, pz + g.pts[i + 1]); }
    };
    int pick = -1; float cx = 0, wallZ = 0;
    for (size_t k = 0; k < m.polygons.size() && pick < 0; ++k) {
        float x0, x1, z0, z1; box(m.polygons[k], x0, x1, z0, z1);
        if (x1 - x0 < 1.5f || x1 - x0 > 8.f || x0 < -300 || x1 > 300 || z0 < -300 || z1 > 300) continue;
        bool clear = true;
        for (size_t j = 0; j < m.polygons.size() && clear; ++j) {
            if (j == k) continue; float a0, a1, b0, b1; box(m.polygons[j], a0, a1, b0, b1);
            if (a1 > x0 - 8 && a0 < x1 + 8 && b1 > z0 - 1 && b0 < z1 + 50) clear = false;
        }
        if (clear) { pick = (int)k; cx = (x0 + x1) * 0.5f; wallZ = z1; }
    }
    chk(pick >= 0, "yalitilmis engel bulunamadi");
    if (pick < 0) return 1;
    const float startZ = wallZ + 40.f;
    mp.createVehicle(cx, startZ, 3.14159265f, 1.5f, 3.6f, 0.2f);   // -z yonune
    VehicleDrive v; v.x = cx; v.z = startZ; v.yaw = 3.14159265f; v.phys = &mp; v.throttle = 1.f;
    float minZ = 1e9f;
    for (int i = 0; i < 400; ++i) { v.update(0.025f, nullptr); if (v.z < minZ) minZ = v.z; }
    printf("son z=%.2f (duvar z=%.2f), hiz=%.2f\n", v.z, wallZ, v.speed);
    chk(minZ > wallZ, "araci duvardan gecmemeli");
    chk(v.z < startZ - 5.f, "arac ilerlemeli");
    printf("mapphysics_test: %s\n", bad ? "HATA" : "tamam");
    return bad;
}
