// WorldView: harita (map*.m3d) 3B gorunumu. Orijinal Map::enqueueRender* mantiginin sadelestirilmis ilk surumu:
// zemin karolari + harita nesneleri, varsayilan golgelendirici, serbest kamera. (Isik haritasi/tarla katmanlari sonra.)
#pragma once
#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/MapData.h"
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
    // sahne nesnesi (arac parcasi vb.): objects/<ad>.p3d + dokusu; model = yaw (Y) + konum
    bool addProp(const char* p3d, const char* tex, float x, float y, float z, float yaw);
    // Deutz 7545 (EntityManager::init tablosundan: tekerlek konumlari VehicleDesc +0x714..)
    bool addDeutz7545(float x, float y, float z, float yaw);
    // Deutz 7545 pozu: konum, yaw (Y), pitch (on yukari pozitif), on tekerlek direksiyonu, tekerlek donusu
    void setDeutzPose(float x, float y, float z, float yaw, float pitch, float steer, float spin);
    void render();
    const MapData& map() const { return m_map; }
    float cam[3] = {0, 0, 0};
    void getCamera(float* p, float& yaw, float& pitch) const { p[0] = m_cam[0]; p[1] = m_cam[1]; p[2] = m_cam[2]; yaw = m_yaw; pitch = m_pitch; }
    float drawDistance = 300.0f;
    int lastDrawCalls = 0;
    bool useLightmap = true;   // zemin karolarinda isik haritasi (orijinal fragmentShaderWorldLightmap)
private:
    void drawMesh(const Mesh* mesh, Texture2D* tex, const float* model, bool alphaTest, bool lightmap = false);
    Renderer* m_r; SystemDevice* m_sys;
    std::vector<uint8_t> m_file;
    MapData m_map;
    std::vector<Mesh*> m_mesh;
    std::vector<Texture2D*> m_tex;
    Texture2D* m_lightmap = nullptr;
    struct Prop { Mesh* mesh; Texture2D* tex; float m[16]; };
    std::vector<Prop> m_props;
    int m_deutz0 = -1;
    std::vector<std::vector<uint8_t>> m_propData;
    float m_cam[3] = {0, 0, 0}, m_yaw = 0, m_pitch = 0;
    float m_view[16], m_proj[16];
    bool m_loaded = false;
};
} // namespace fs12
#endif
