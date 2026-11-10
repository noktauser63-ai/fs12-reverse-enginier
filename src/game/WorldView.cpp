#if defined(__ANDROID__) || defined(FS12_GL)
#include "game/WorldView.h"
#include <cmath>
#include <cstring>

namespace fs12 {
static float bitsF(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }

bool WorldView::load(const char* assetPath) {
    if (!m_sys->loadAsset(assetPath, m_file)) return false;
    if (!m_map.parse(m_file.data(), m_file.size())) return false;
    m_mesh.clear(); m_tex.clear();
    for (size_t i = 0; i < m_map.meshes.size(); ++i) {
        const uint8_t* chunk = m_map.meshes[i].vertices - 8;   // [nv][ni] basligi dahil blok
        m_mesh.push_back(m_r->meshFromMemory(std::string("m3d:") + assetPath + ":" + std::to_string(i), chunk));
    }
    for (auto& n : m_map.textures) m_tex.push_back(m_r->texture((std::string("map/") + n).c_str()));
    m_lightmap = m_r->texture("map/map01LightMap_P.p2d");
    if (useGrass) buildGrass();
    // zemin karolari: REPEAT yerine kenara kenetle (karo kenari karsi kenarla karismasin -> cizgi gorunumu olmasin)
    for (const auto& c : m_map.cells) if (c.fieldDesc < m_tex.size() && m_tex[c.fieldDesc] && m_tex[c.fieldDesc]->id) {
        glBindTexture(GL_TEXTURE_2D, m_tex[c.fieldDesc]->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    m_loaded = true;
    return true;
}

// Yesil zemin karolarinda (map01 fieldDesc: 6,7,8,13,17,18,21,22,28) sabit tohumlu ot/cali yerlestirme.
void WorldView::buildGrass() {
    m_grass.clear();
    static const int kGreen[] = {6, 7, 8, 13, 17, 18, 21, 22, 28};
    for (size_t ci = 0; ci < m_map.cells.size(); ++ci) {
        const MapCellRec& c = m_map.cells[ci];
        bool g = false; for (int t : kGreen) g |= (int)c.fieldDesc == t;
        if (!g) continue;
        const int ix = (int)(ci % m_map.nx), iy = (int)(ci / m_map.nx);
        const float cx = m_map.originX + ix * m_map.cellW, cz = m_map.originY + iy * m_map.cellH;
        uint32_t h = (uint32_t)ci * 2654435761u + 12345u;
        auto rnd = [&]() { h ^= h << 13; h ^= h >> 17; h ^= h << 5; return (h & 0xffff) / 65535.f; };
        for (int k = 0; k < 10; ++k) {
            const float x = cx + (rnd() - 0.5f) * m_map.cellW * 0.95f, z = cz + (rnd() - 0.5f) * m_map.cellH * 0.95f;
            const float yaw = rnd() * 6.2831853f, sc = 0.5f + rnd() * 0.5f;
            float y;
            if (!m_map.groundHeight(x, z, y)) continue;
            const float cs = cosf(yaw) * sc, sn = sinf(yaw) * sc;
            Tuft t = {{cs, 0, -sn, 0, 0, sc, 0, 0, sn, 0, cs, 0, x, y, z, 1}};
            m_grass.push_back(t);
        }
    }
}

bool WorldView::addProp(const char* p3d, const char* tex, float x, float y, float z, float yaw) {
    m_propData.emplace_back();
    if (!m_sys->loadAsset(p3d, m_propData.back())) { m_propData.pop_back(); return false; }
    Mesh* mesh = m_r->meshFromMemory(std::string("p3d:") + p3d, m_propData.back().data());
    Texture2D* t = m_r->texture(tex);
    if (!mesh || !t) return false;
    const float c = cosf(yaw), s = sinf(yaw);
    Prop pr{mesh, t, {c,0,-s,0, 0,1,0,0, s,0,c,0, x,y,z,1}};
    m_props.push_back(pr);
    return true;
}

bool WorldView::addDeutz7545(float x, float y, float z, float yaw) {
    const char* tex = "objects/objectsShared_diffuse.p2d";
    m_deutz0 = (int)m_props.size();
    bool ok = addProp("objects/deutz7545_frame.p3d", tex, x, y, z, yaw);
    struct W { const char* mesh; float lx, ly, lz; };
    static const W w[4] = {
        {"objects/deutz7545_wheelFrontRight.p3d", bitsF(0xbfa347d8), bitsF(0x3f6c9ba6), bitsF(0x40129fe8)},
        {"objects/deutz7545_wheelFrontLeft.p3d",  bitsF(0x3fa347d8), bitsF(0x3f6c9ba6), bitsF(0x40129fe8)},
        {"objects/deutz7545_wheelBackRight.p3d",  bitsF(0xbfa21c58), bitsF(0x3f332f45), bitsF(0xbfabc212)},
        {"objects/deutz7545_wheelBackLeft.p3d",   bitsF(0x3fa21c58), bitsF(0x3f332f45), bitsF(0xbfabc212)}};
    const float c = cosf(yaw), s = sinf(yaw);
    for (const W& wh : w)   // yerel (lx,ly,lz) -> dunya: Y ekseni etrafinda donus
        ok &= addProp(wh.mesh, tex, x + c * wh.lx + s * wh.lz, y + wh.ly, z - s * wh.lx + c * wh.lz, yaw);
    return ok;
}

void WorldView::removeVehicle() {
    if (m_spec0 < 0) return;
    size_t n = 1 + (size_t)m_specFixed + (size_t)m_specOffs;
    if (m_spec) for (int i = 0; i < m_spec->nWheels; ++i) n += m_spec->wheels[i].fender ? 2 : 1;
    if ((size_t)m_spec0 + n <= m_props.size()) m_props.erase(m_props.begin() + m_spec0, m_props.begin() + m_spec0 + n);
    m_spec = nullptr; m_spec0 = -1; m_specFixed = 0; m_specOffs = 0;
}
bool WorldView::addVehicleSpec(const VehicleSpec& v) {
    removeVehicle();
    m_spec = &v; m_spec0 = (int)m_props.size();
    m_specFixed = 0; m_specOffs = 0;
    bool ok = addProp(v.body, v.bodyTex, 0, 0, 0, 0);
    for (const char* fp : v.fixedParts) if (fp) { ok &= addProp(fp, v.bodyTex, 0, 0, 0, 0); ++m_specFixed; }
    for (const PartSpec& op : v.offsetParts) if (op.mesh) { ok &= addProp(op.mesh, op.tex ? op.tex : v.bodyTex, 0, 0, 0, 0); ++m_specOffs; }
    for (int i = 0; i < v.nWheels; ++i) {
        ok &= addProp(v.wheels[i].mesh, v.wheelTex, 0, 0, 0, 0);
        if (v.wheels[i].fender) ok &= addProp(v.wheels[i].fender, v.bodyTex, 0, 0, 0, 0);
    }
    return ok;
}
void WorldView::setSpecPose(float x, float y, float z, float yaw, float pitch, float steer, float spin) {
    if (!m_spec || m_spec0 < 0) return;
    const float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(-pitch), sp = sinf(-pitch);
    const float RY[16] = {cy,0,-sy,0, 0,1,0,0, sy,0,cy,0, x,y,z,1};
    const float RX[16] = {1,0,0,0, 0,cp,sp,0, 0,-sp,cp,0, 0,0,0,1};
    float B[16]; Renderer::multiply(B, RY, RX);
    size_t k = (size_t)m_spec0;
    if (k >= m_props.size()) return;
    memcpy(m_props[k++].m, B, sizeof(B));
    for (int i = 0; i < m_specFixed && k < m_props.size(); ++i) memcpy(m_props[k++].m, B, sizeof(B));
    for (const PartSpec& op : m_spec->offsetParts) if (op.mesh && k < m_props.size()) {
        const float T[16] = {op.sx,0,0,0, 0,1,0,0, 0,0,1,0, op.x, op.y, op.z, 1}; float M[16]; Renderer::multiply(M, B, T); memcpy(m_props[k++].m, M, sizeof(M)); }
    for (int i = 0; i < m_spec->nWheels; ++i) {
        const WheelSpec& w = m_spec->wheels[i];
        const float st = w.steer ? steer : 0.f, cs = cosf(st), ss = sinf(st), cw = cosf(spin), sw = sinf(spin);
        const float T[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, w.x, w.y, w.z, 1};
        const float S[16] = {cs,0,-ss,0, 0,1,0,0, ss,0,cs,0, 0,0,0,1};
        const float R[16] = {1,0,0,0, 0,cw,sw,0, 0,-sw,cw,0, 0,0,0,1};
        float a[16], b[16], c[16];
        Renderer::multiply(a, T, S); Renderer::multiply(b, a, R); Renderer::multiply(c, B, b);
        if (k < m_props.size()) memcpy(m_props[k++].m, c, sizeof(c));
        if (w.fender) { float f[16]; Renderer::multiply(f, B, a); if (k < m_props.size()) memcpy(m_props[k++].m, f, sizeof(f)); }
    }
}

static const float kDeutzWheel[4][4] = {{-1.2756f, 0.9243f, 2.291f, 0.9243f}, {1.2756f, 0.9243f, 2.291f, 0.9243f},
                                       {-1.2665f, 0.6999f, -1.3419f, 0.6999f}, {1.2665f, 0.6999f, -1.3419f, 0.6999f}};   // x,y,z,yaricap
void WorldView::setDeutzPose(float x, float y, float z, float yaw, float pitch, float steer, float spin) {
    if (m_deutz0 < 0 || m_props.size() < (size_t)m_deutz0 + 5) return;
    const float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(-pitch), sp = sinf(-pitch);
    // govde: RY(yaw) * RX(-pitch) (sutun-oncelikli)
    const float RY[16] = {cy,0,-sy,0, 0,1,0,0, sy,0,cy,0, x,y,z,1};
    const float RX[16] = {1,0,0,0, 0,cp,sp,0, 0,-sp,cp,0, 0,0,0,1};
    float B[16]; Renderer::multiply(B, RY, RX);
    memcpy(m_props[m_deutz0].m, B, sizeof(B));
    for (int i = 0; i < 4; ++i) {
        const float* w = kDeutzWheel[i];
        const float st = i < 2 ? steer : 0.f, cs = cosf(st), ss = sinf(st), cw = cosf(spin), sw = sinf(spin);
        const float T[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, w[0], w[1], w[2], 1};
        const float S[16] = {cs,0,-ss,0, 0,1,0,0, ss,0,cs,0, 0,0,0,1};
        const float R[16] = {1,0,0,0, 0,cw,sw,0, 0,-sw,cw,0, 0,0,0,1};
        float a[16], b[16], c[16];
        Renderer::multiply(a, T, S); Renderer::multiply(b, a, R); Renderer::multiply(c, B, b);
        memcpy(m_props[m_deutz0 + 1 + i].m, c, sizeof(c));
    }
}

void WorldView::drawMesh(const Mesh* mesh, Texture2D* tex, const float* model, bool alphaTest, bool lightmap) {
    lightmap = lightmap && m_lightmap && m_lightmap->id;
    Program* p = lightmap ? m_r->program("vertexShaderWorldLightmap", "fragmentShaderWorldLightmap")
                          : m_r->program("vertexShaderDefault", alphaTest ? "fragmentShaderDefaultAT" : "fragmentShaderDefault");
    if (!p || !p->id || !mesh || !tex || !tex->id) return;
    m_r->use(p);
    if (lightmap) {
        const GLint mw = p->uniform("u_mwMatrix");
        if (mw >= 0) glUniformMatrix4fv(mw, 1, GL_FALSE, model);
        const GLint t1 = p->uniform("s_TexUnit1");
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_lightmap->id);
        if (t1 >= 0) glUniform1i(t1, 1);
    }
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
    const char* progs[3][2] = {{"vertexShaderDefault", "fragmentShaderDefault"}, {"vertexShaderDefault", "fragmentShaderDefaultAT"},
                               {"vertexShaderWorldLightmap", "fragmentShaderWorldLightmap"}};
    for (auto& pn : progs) {
        Program* p = m_r->program(pn[0], pn[1]);
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
    if (!propsOnly) for (uint32_t iy = 0; iy < m_map.ny; ++iy) for (uint32_t ix = 0; ix < m_map.nx; ++ix) {
        const MapCellRec& c = m_map.cells[(size_t)iy * m_map.nx + ix];
        const float x = m_map.originX + ix * m_map.cellW, z = m_map.originY + iy * m_map.cellH;
        const float dx = x - m_cam[0], dz = z - m_cam[2];
        if (dx * dx + dz * dz > R2) continue;
        const float ca = cosf(c.rotation), sa = sinf(c.rotation);
        const float M[16] = {ca,0,-sa,0, 0,1,0,0, sa,0,ca,0, x, c.unknown, z, 1};
        if (c.texture < m_mesh.size() && c.fieldDesc < m_tex.size()) drawMesh(m_mesh[c.texture], m_tex[c.fieldDesc], M, false, useLightmap);
    }
    // nesneler
    if (!propsOnly) for (const MapObjectRec& o : m_map.objects) {
        const float cx = (o.aabb[0] + o.aabb[3]) * 0.5f, cz = (o.aabb[2] + o.aabb[5]) * 0.5f;
        const float dx = cx - m_cam[0], dz = cz - m_cam[2];
        if (dx * dx + dz * dz > R2) continue;
        if (o.mesh < m_mesh.size() && o.texture < m_tex.size()) drawMesh(m_mesh[o.mesh], m_tex[o.texture], o.matrix, (o.flags & 2) != 0, false);
    }
    if (!propsOnly && useGrass && m_mesh.size() > 48 && m_tex.size() > 4 && m_mesh[48] && m_tex[4]) {
        const float G2 = grassDistance * grassDistance;
        for (const Tuft& t : m_grass) {
            const float dx = t.m[12] - m_cam[0], dz = t.m[14] - m_cam[2];
            if (dx * dx + dz * dz > G2) continue;
            drawMesh(m_mesh[48], m_tex[4], t.m, true, false);
        }
    }
    for (const Prop& pr : m_props) drawMesh(pr.mesh, pr.tex, pr.m, false, false);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_CULL_FACE);
}
} // namespace fs12
#endif
