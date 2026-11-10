// WorldView: harita (map*.m3d) 3B gorunumu. Orijinal Map::enqueueRender* mantiginin sadelestirilmis ilk surumu:
// zemin karolari + harita nesneleri, varsayilan golgelendirici, serbest kamera. (Isik haritasi/tarla katmanlari sonra.)
#pragma once
#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/MapData.h"
#include "game/CowHerd.h"
#include "render/Renderer.h"
#include <string>
#include <vector>

namespace fs12 {
class WorldView {
public:
    WorldView(Renderer* r, SystemDevice* sys) : m_r(r), m_sys(sys) {}
    bool load(const char* assetPath = "map/map01.m3d");
    bool loaded() const { return m_loaded; }
    // kamera: konum + yaw (Y ekseni, radyan) + pitch (asagi pozitif)
    void setCamera(float x, float y, float z, float yaw, float pitch) { m_cam[0] = x; m_cam[1] = y; m_cam[2] = z; m_yaw = yaw; m_pitch = pitch; }
    void render();
    const MapData& map() const { return m_map; }
    float cam[3] = {0, 0, 0};
    void getCamera(float* p, float& yaw, float& pitch) const { p[0] = m_cam[0]; p[1] = m_cam[1]; p[2] = m_cam[2]; yaw = m_yaw; pitch = m_pitch; }
    void hideObject(size_t index) { m_hidden.push_back(index); }   // harita nesnesini cizme (ambar kaldirma)
    void setCowHerd(const CowHerd* h) { m_herd = h; }   // nullptr: inek cizme
    float drawDistance = 300.0f;
    int lastDrawCalls = 0;
private:
    void drawMesh(const Mesh* mesh, Texture2D* tex, const float* model, bool alphaTest);
    Renderer* m_r; SystemDevice* m_sys;
    std::vector<uint8_t> m_file;
    MapData m_map;
    std::vector<Mesh*> m_mesh;
    std::vector<Texture2D*> m_tex;
    float m_cam[3] = {0, 0, 0}, m_yaw = 0, m_pitch = 0;
    float m_view[16], m_proj[16];
    std::vector<size_t> m_hidden;
    const CowHerd* m_herd = nullptr;
    void renderCows();
    bool m_loaded = false;
};
} // namespace fs12
#endif
