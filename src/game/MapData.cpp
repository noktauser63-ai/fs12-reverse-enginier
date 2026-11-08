#include <cmath>
#include <cstring>
#include "game/MapData.h"
#include <cstring>

namespace fs12 {
namespace {
struct Reader {
    const uint8_t* d; size_t n, p = 0; bool ok = true;
    bool need(size_t k) { if (p + k > n) ok = false; return ok; }
    uint32_t u32() { uint32_t v = 0; if (need(4)) { memcpy(&v, d + p, 4); p += 4; } return v; }
    float f32() { float v = 0; if (need(4)) { memcpy(&v, d + p, 4); p += 4; } return v; }
    const uint8_t* take(size_t k) { if (!need(k)) return nullptr; const uint8_t* r = d + p; p += k; return r; }
};
}

bool MapData::parse(const uint8_t* data, size_t size) {
    Reader r{data, size};
    originX = r.f32(); originY = r.f32(); nx = r.u32(); ny = r.u32(); cellW = r.f32(); cellH = r.f32();
    const uint32_t ntex = r.u32();
    if (!r.ok || ntex > 4096 || nx > 4096 || ny > 4096) return false;
    textures.clear();
    for (uint32_t i = 0; i < ntex; ++i) {
        const uint8_t* s = r.take(128); if (!s) return false;
        textures.emplace_back((const char*)s, strnlen((const char*)s, 128));
    }
    const uint32_t nmesh = r.u32();
    if (!r.ok || nmesh > 65536) return false;
    meshes.assign(nmesh, MapMeshRef());
    for (auto& m : meshes) {
        m.vertCount = r.u32(); m.indexCount = r.u32();
        m.vertices = r.take((size_t)m.vertCount * 24); if (!m.vertices) return false;
        m.indices = (const uint16_t*)r.take((size_t)m.indexCount * 2); if (!m.indices) return false;
        if (m.indexCount & 1) { if (!r.take(2)) return false; }   // 4 bayta hizalama dolgusu
    }
    const uint32_t nobj = r.u32();
    if (!r.ok || nobj > 1000000) return false;
    objects.assign(nobj, MapObjectRec());
    for (auto& o : objects) {
        o.flags = r.u32(); o.texture = r.u32(); o.mesh = r.u32();
        const uint8_t* m = r.take(0x40); const uint8_t* b = r.take(0x18); if (!m || !b) return false;
        memcpy(o.matrix, m, 0x40); memcpy(o.aabb, b, 0x18);
    }
    const uint32_t npoly = r.u32();
    if (!r.ok || npoly > 1000000) return false;
    polygons.assign(npoly, MapPolygon());
    for (auto& g : polygons) {
        g.a = r.u32(); g.b = r.u32();
        const uint32_t n = r.u32();
        const uint8_t* pts = r.take((size_t)n * 8); if (!pts) return false;
        g.pts.resize((size_t)n * 2); memcpy(g.pts.data(), pts, (size_t)n * 8);
    }
    cells.assign((size_t)nx * ny, MapCellRec());
    for (auto& c : cells) {
        c.fieldDesc = r.u32(); c.texture = r.u32(); c.rotation = r.f32(); c.unknown = r.f32(); c.flags = r.u32();
        const uint32_t k = r.u32();
        if (!r.ok || k > 100000) return false;
        c.objects.resize(k);
        const uint8_t* o = r.take((size_t)k * 4); if (!o) return false;
        memcpy(c.objects.data(), o, (size_t)k * 4);
    }
    const uint32_t nsp = r.u32();
    if (!r.ok || nsp > 4096) return false;
    splines.assign(nsp, MapSpline());
    for (auto& s : splines) {
        const uint32_t n = r.u32(); s.flag = r.u32();
        const uint8_t* pts = r.take((size_t)n * 8); if (!pts) return false;
        s.pts.resize((size_t)n * 2); memcpy(s.pts.data(), pts, (size_t)n * 8);
    }
    return r.ok && r.p == size;
}

bool MapData::groundHeight(float x, float z, float& h) const {
    const int ix = (int)((x - minX()) / cellW), iy = (int)((z - minY()) / cellH);
    if (x < minX() || z < minY() || ix < 0 || iy < 0 || ix >= (int)nx || iy >= (int)ny) return false;
    const MapCellRec& c = cells[(size_t)iy * nx + ix];
    if (c.texture >= meshes.size()) return false;
    const MapMeshRef& m = meshes[c.texture];
    const float dx = x - (originX + ix * cellW), dz = z - (originY + iy * cellH);
    const float ca = std::cos(c.rotation), sa = std::sin(c.rotation);
    const float lx = ca * dx - sa * dz, lz = sa * dx + ca * dz;   // dunya -> yerel (Y donusunun tersi)
    for (uint32_t t = 0; t + 2 < m.indexCount; t += 3) {
        float p[3][3];
        for (int k = 0; k < 3; ++k) memcpy(p[k], m.vertices + (size_t)m.indices[t + k] * 24, 12);
        const float d = (p[1][2] - p[2][2]) * (p[0][0] - p[2][0]) + (p[2][0] - p[1][0]) * (p[0][2] - p[2][2]);
        if (std::fabs(d) < 1e-9f) continue;
        const float a = ((p[1][2] - p[2][2]) * (lx - p[2][0]) + (p[2][0] - p[1][0]) * (lz - p[2][2])) / d;
        const float b = ((p[2][2] - p[0][2]) * (lx - p[2][0]) + (p[0][0] - p[2][0]) * (lz - p[2][2])) / d;
        const float g = 1.f - a - b;
        if (a < -1e-4f || b < -1e-4f || g < -1e-4f) continue;
        h = a * p[0][1] + b * p[1][1] + g * p[2][1] + c.unknown;
        return true;
    }
    return false;
}
} // namespace fs12
