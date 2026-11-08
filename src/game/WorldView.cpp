#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/WorldView.h"
#include <cmath>
#include <cstring>

namespace fs12 {

bool WorldView::load(const char* assetPath) {
    if (!m_sys->loadAsset(assetPath, m_file)) return false;
    if (!m_map.parse(m_file.data(), m_file.size())) return false;
    m_mesh.clear(); m_tex.clear();
    for (size_t i = 0; i < m_map.meshes.size(); ++i) {
        const uint8_t* chunk = m_map.meshes[i].vertices - 8;   // [nv][ni] basligi dahil blok
        m_mesh.push_back(m_r->meshFromMemory(std::string("m3d:") + assetPath + ":" + std::to_string(i), chunk));
    }
    for (auto& n : m_map.textures) m_tex.push_back(m_r->texture((std::string("map/") + n).c_str()));
    m_loaded = true;
    return true;
}

void WorldView::drawMesh(const Mesh* mesh, Texture2D* tex, const float* model, bool alphaTest) {
    Program* p = m_r->program("vertexShaderDefault", alphaTest ? "fragmentShaderDefaultAT" : "fragmentShaderDefault");
    if (!p || !p->id || !mesh || !tex || !tex->id) return;
    m_r->use(p);
    float mv[16], mvp[16];
    Renderer::multiply(mv, m_view, model);
    Renderer::multiply(mvp, m_proj, mv);
    glUniformMatrix4fv(p->mvp, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(p->mv, 1, GL_FALSE, mv);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    if (p->tex0 >= 0) glUniform1i(p->tex0, 0);
    m_r->drawMesh(mesh);
    ++lastDrawCalls;
}

void WorldView::render() {
    if (!m_loaded) return;
    lastDrawCalls = 0;
    // gorunum matrisi: once -konum, sonra yaw (Y), sonra pitch (X)
    const float cy = cosf(m_yaw), sy = sinf(m_yaw), cp = cosf(m_pitch), sp = sinf(m_pitch);
    float T[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, -m_cam[0], -m_cam[1], -m_cam[2], 1};
    float RY[16] = {cy,0,-sy,0, 0,1,0,0, sy,0,cy,0, 0,0,0,1};
    float RX[16] = {1,0,0,0, 0,cp,sp,0, 0,-sp,cp,0, 0,0,0,1};
    float tmp[16];
    Renderer::multiply(tmp, RY, T);
    Renderer::multiply(m_view, RX, tmp);
    Renderer::perspective(m_proj, 1.0472f, m_r->aspect(), 0.5f, 900.0f);

    m_r->setAlphaBlend(false);
    m_r->setDepthTest(true, true);
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
    // isik (gorunum uzayinda yon)
    float dw[3] = {0.35f, -0.85f, 0.4f};
    const float len = sqrtf(dw[0] * dw[0] + dw[1] * dw[1] + dw[2] * dw[2]);
    for (float& v : dw) v /= len;
    float dv[3];
    for (int r = 0; r < 3; ++r) dv[r] = m_view[0 + r] * dw[0] + m_view[4 + r] * dw[1] + m_view[8 + r] * dw[2];
    for (const char* fs : {"fragmentShaderDefault", "fragmentShaderDefaultAT"}) {
        Program* p = m_r->program("vertexShaderDefault", fs);
        if (!p || !p->id) continue;
        m_r->use(p);
        const float amb[3] = {0.45f, 0.47f, 0.5f}, dif[3] = {0.75f, 0.72f, 0.65f}, none[3] = {0, 0, 0}, cone[2] = {1, 0};
        if (p->dAmbient >= 0) glUniform3fv(p->dAmbient, 1, amb);
        if (p->dDiffuse >= 0) glUniform3fv(p->dDiffuse, 1, dif);
        if (p->dDir >= 0) glUniform3fv(p->dDir, 1, dv);
        if (p->sDiffuse >= 0) glUniform3fv(p->sDiffuse, 1, none);
        const float sp4[4] = {0, 0, 0, 1};
        if (p->sPos >= 0) glUniform4fv(p->sPos, 1, sp4);
        if (p->sDir >= 0) glUniform3fv(p->sDir, 1, dv);
        if (p->sCone >= 0) glUniform2fv(p->sCone, 1, cone);
    }
    const float R2 = drawDistance * drawDistance;
    // zemin karolari
    for (uint32_t iy = 0; iy < m_map.ny; ++iy) for (uint32_t ix = 0; ix < m_map.nx; ++ix) {
        const MapCellRec& c = m_map.cells[(size_t)iy * m_map.nx + ix];
        const float x = m_map.originX + ix * m_map.cellW, z = m_map.originY + iy * m_map.cellH;
        const float dx = x - m_cam[0], dz = z - m_cam[2];
        if (dx * dx + dz * dz > R2) continue;
        const float ca = cosf(c.rotation), sa = sinf(c.rotation);
        const float M[16] = {ca,0,-sa,0, 0,1,0,0, sa,0,ca,0, x, c.unknown, z, 1};
        if (c.texture < m_mesh.size() && c.fieldDesc < m_tex.size()) drawMesh(m_mesh[c.texture], m_tex[c.fieldDesc], M, false);
    }
    // nesneler
    for (const MapObjectRec& o : m_map.objects) {
        const float cx = (o.aabb[0] + o.aabb[3]) * 0.5f, cz = (o.aabb[2] + o.aabb[5]) * 0.5f;
        const float dx = cx - m_cam[0], dz = cz - m_cam[2];
        if (dx * dx + dz * dz > R2) continue;
        if (o.mesh < m_mesh.size() && o.texture < m_tex.size()) drawMesh(m_mesh[o.mesh], m_tex[o.texture], o.matrix, (o.flags & 2) != 0);
    }
    glDisable(GL_CULL_FACE);
}
} // namespace fs12
#endif
