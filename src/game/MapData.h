// MapData: map*.m3d dosyasinin CPU tarafi (orijinal Map::load'un dosya bicimi). GL/fizik baglantisi ayri katmanda.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fs12 {

struct MapMeshRef { uint32_t vertCount = 0, indexCount = 0; const uint8_t* vertices = nullptr; const uint16_t* indices = nullptr; };

struct MapObjectRec {              // 0x64 bayt: MapObject::load
    uint32_t flags = 0;            // bit0 yaprak/seffaf..., bit1 alfa testi, bit2 ..., bit4/6..: bkz. MapObject::enqueueRender
    uint32_t texture = 0, mesh = 0;
    float matrix[16] = {};         // sutun-oncelikli 4x4
    float aabb[6] = {};            // min xyz, max xyz
};

struct MapPolygon { uint32_t a = 0, b = 0; std::vector<float> pts; };   // statik fizik cokgenleri (b2PolygonShape)

struct MapCellRec {                // 0x18 + n*4 bayt: GridCell::load
    uint32_t fieldDesc = 0, texture = 0;
    float rotation = 0, unknown = 0;
    uint32_t flags = 0;            // bit0: tarla
    std::vector<uint32_t> objects;
};

struct MapSpline { uint32_t flag = 0; std::vector<float> pts; };

struct MapData {
    float originX = 0, originY = 0, cellW = 0, cellH = 0;
    uint32_t nx = 0, ny = 0;
    std::vector<std::string> textures;
    std::vector<MapMeshRef> meshes;
    std::vector<MapObjectRec> objects;
    std::vector<MapPolygon> polygons;
    std::vector<MapCellRec> cells;           // nx*ny, satir-oncelikli (y dis dongu)
    std::vector<MapSpline> splines;
    float minX() const { return originX - cellW * 0.5f; }   // Map+0x1c
    float minY() const { return originY - cellH * 0.5f; }   // Map+0x20
    // Dosya verisi MapData yasadikca gecerli kalmalidir (mesh isaretcileri icine bakar).
    // zemin yuksekligi (dunya x,z): hucre zemin agi ucgenlerinden. false: harita disi / ucgen bulunamadi
    bool groundHeight(float x, float z, float& h) const;
    // zemin agi UV'si (dunya x,z): hucre dokusunda o noktanin koordinati
    bool groundUV(float x, float z, float& u, float& v) const;
    bool parse(const uint8_t* data, size_t size);
};
} // namespace fs12
