// MapData: gercek map01.m3d dosyasini ayristirir; tuketilen bayt dosya boyutuna esit olmali.
#include "game/MapData.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace fs12;
int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "/home/claude/fs12/apk2/assets/map/map01.m3d";
    std::ifstream f(path, std::ios::binary);
    if (!f) { printf("mapdata_test: dosya yok, atlandi\n"); return 0; }
    std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    MapData m;
    int bad = 0;
    if (!m.parse(d.data(), d.size())) { printf("HATA: ayristirma basarisiz\n"); return 1; }
    auto chk = [&](bool c, const char* w) { if (!c) { printf("HATA: %s\n", w); ++bad; } };
    chk(m.nx == 40 && m.ny == 40 && m.cells.size() == 1600, "izgara 40x40");
    chk(m.textures.size() == 33 && m.meshes.size() == 116 && m.objects.size() == 3118 && m.polygons.size() == 1347, "sayilar");
    chk(m.splines.size() == 2, "spline");
    chk(m.minX() == -390.0f && m.minY() == -410.0f, "sol-ust kose");
    for (auto& o : m.objects) { chk(o.mesh < m.meshes.size() && o.texture < m.textures.size(), "nesne indeksleri"); }
    for (auto& c : m.cells) for (auto i : c.objects) chk(i < m.objects.size(), "hucre nesne indeksi");
    {   // zemin yuksekligi: harita icinde cogu nokta bir ucgene dusmeli
        int hit = 0, tot = 0; float mn = 1e9f, mx = -1e9f, h;
        for (float z = -300; z < 300; z += 7.3f) for (float x = -300; x < 300; x += 7.3f) { ++tot; if (m.groundHeight(x, z, h)) { ++hit; if (h < mn) mn = h; if (h > mx) mx = h; } }
        printf("zemin: %d/%d nokta, yukseklik %.2f..%.2f, (10,-3)=%s\n", hit, tot, mn, mx, m.groundHeight(10, -3, h) ? "var" : "yok");
        chk(hit * 10 >= tot * 9, "zemin yuksekligi kapsami");
    }
    printf("mapdata_test: %s (%zu doku, %zu mesh, %zu nesne)\n", bad ? "HATA" : "tamam", m.textures.size(), m.meshes.size(), m.objects.size());
    return bad ? 1 : 0;
}
