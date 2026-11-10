#if defined(__ANDROID__) || defined(FS12_GL)
#include "Renderer.h"
#include "P2D.h"
#include <android/log.h>
#include <cstring>
#include <cmath>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeLog", __VA_ARGS__)

namespace fs12 {

Renderer::~Renderer() {
    for (auto& k : m_tex) delete k.second;
    for (auto& k : m_prog) delete k.second;
    for (auto& k : m_mesh) delete k.second;
}

void Renderer::init(int w, int h) {
    m_w = w; m_h = h;
    const char* ext = (const char*)glGetString(GL_EXTENSIONS);
    m_etc1 = false;   // cihazda ETC1 donanim yolu bozuk/gurultulu cikti; yazilim cozucu ana makinede dogrulandi
    (void)ext;
    LOGI("GL: %s | %s | ETC1=%d", glGetString(GL_RENDERER), glGetString(GL_VERSION), (int)m_etc1);
    contextRestored();
}
void Renderer::resize(int w, int h) { m_w = w; m_h = h; glViewport(0, 0, w, h); }

void Renderer::contextLost() {
    for (auto& k : m_tex) k.second->id = 0;
    for (auto& k : m_prog) k.second->id = 0;
    for (auto& k : m_mesh) { k.second->vbo = 0; k.second->ibo = 0; }
    m_quadVbo = 0; m_cur = nullptr;
}

void Renderer::contextRestored() {
    // birim kare (triangle strip): (0,0) (0,1) (1,0) (1,1)  - orijinal initOverlay ile ayni
    static const float quad[12] = {0,0,0, 0,1,0, 1,0,0, 1,1,0};
    glGenBuffers(1, &m_quadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    for (auto& k : m_tex) uploadTexture(*k.second);
    for (auto& k : m_prog) buildProgram(*k.second);
    for (auto& k : m_mesh) buildMesh(*k.second, k.first);
    glViewport(0, 0, m_w, m_h);
}

// ---------------- texture ----------------
void Renderer::uploadTexture(Texture2D& t) {
    std::vector<uint8_t> raw;
    if (!m_sys->loadAsset(t.path.c_str(), raw)) { LOGI("texture bulunamadi: %s", t.path.c_str()); return; }
    P2DInfo in; std::vector<P2DLevel> lv;
    if (!p2dParse(raw.data(), raw.size(), in, lv)) { LOGI("p2d bozuk: %s", t.path.c_str()); return; }
    if (in.padBytes) raw.resize(raw.size() + in.padBytes, 0);
    t.w = in.width; t.h = in.height;
    glGenTextures(1, &t.id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (in.mipExtra && t.mipmaps) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLenum ifmt = GL_RGBA, type = GL_UNSIGNED_BYTE;
    bool etc = in.format == P2D_ETC1 || in.format == P2D_ETC1_B;
    switch (in.format) {
        case P2D_RGB8: ifmt = GL_RGB; break;
        case P2D_RGBA5551: type = GL_UNSIGNED_SHORT_5_5_5_1; break;
        case P2D_RGBA4444: type = GL_UNSIGNED_SHORT_4_4_4_4; break;
        case P2D_RGB565: ifmt = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
        case P2D_LA8: ifmt = GL_LUMINANCE_ALPHA; break;
        case P2D_L8: ifmt = GL_LUMINANCE; break;
        case P2D_A8: ifmt = GL_ALPHA; break;
        default: break;
    }
    std::vector<uint8_t> tmp;
    for (size_t i = 0; i < lv.size(); ++i) {
        if (!t.mipmaps && i > 0) break;
        const uint8_t* p = raw.data() + lv[i].offset;
        uint32_t w = lv[i].w, h = lv[i].h;
        if (etc) {
            if (m_etc1) {
                glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)i, GL_ETC1_RGB8_OES, w, h, 0, (GLsizei)lv[i].size, p);
            } else {   // yazilim yedegi
                tmp.assign((size_t)w * h * 4, 255);
                etc1DecodeRGBA(p, w, h, tmp.data());
                glTexImage2D(GL_TEXTURE_2D, (GLint)i, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp.data());
            }
        } else if (in.format >= P2D_PVRTC_RGB4 && in.format <= P2D_PVRTC_RGBA2) {
            LOGI("PVRTC desteklenmiyor: %s", t.path.c_str());
            return;
        } else {
            glTexImage2D(GL_TEXTURE_2D, (GLint)i, ifmt, w, h, 0, ifmt, type, p);
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

Texture2D* Renderer::texture(const char* path, bool mipmaps) {
    auto it = m_tex.find(path);
    if (it != m_tex.end()) return it->second;
    Texture2D* t = new Texture2D(); t->path = path; t->mipmaps = mipmaps;
    uploadTexture(*t);
    m_tex[path] = t;
    return t;
}

// ---------------- shader ----------------
GLuint Renderer::compile(GLenum type, const std::string& src, const std::string& name) {
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        LOGI("shader hatasi %s: %s", name.c_str(), log);
        glDeleteShader(s); return 0;
    }
    return s;
}

void Renderer::buildProgram(Program& p) {
    std::vector<uint8_t> v, f;
    if (!m_sys->loadAsset(("shader/" + p.vs + ".vsh").c_str(), v) ||
        !m_sys->loadAsset(("shader/" + p.fs + ".fsh").c_str(), f)) { LOGI("shader dosyasi yok: %s/%s", p.vs.c_str(), p.fs.c_str()); return; }
    GLuint vs = compile(GL_VERTEX_SHADER, std::string(v.begin(), v.end()), p.vs);
    GLuint fs = compile(GL_FRAGMENT_SHADER, std::string(f.begin(), f.end()), p.fs);
    if (!vs || !fs) return;
    p.id = glCreateProgram();
    glAttachShader(p.id, vs); glAttachShader(p.id, fs);
    glBindAttribLocation(p.id, ATTR_POSITION, "Position");
    glBindAttribLocation(p.id, ATTR_NORMAL, "Normal");
    glBindAttribLocation(p.id, ATTR_TEXCOORD0, "TexCoord0");
    glLinkProgram(p.id);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok = 0; glGetProgramiv(p.id, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(p.id, sizeof(log), nullptr, log); LOGI("link hatasi: %s", log); p.id = 0; return; }
    p.mvp = p.uniform("u_mvpMatrix"); p.mv = p.uniform("u_mvMatrix"); p.mw = p.uniform("u_mwMatrix");
    p.tex0 = p.uniform("s_TexUnit0"); p.tex1 = p.uniform("s_TexUnit1"); p.tex2 = p.uniform("s_TexUnit2");
    p.dAmbient = p.uniform("u_dLightAmbient"); p.dDiffuse = p.uniform("u_dLightDiffuse"); p.dDir = p.uniform("u_dLightDirection");
    p.sPos = p.uniform("u_sLightPosition"); p.sDiffuse = p.uniform("u_sLightDiffuse"); p.sDir = p.uniform("u_sLightDirection");
    p.sCone = p.uniform("u_sLightConeParams");
    p.ovColor = p.uniform("u_Color"); p.ovTexVecs = p.uniform("u_TexVecs");
    p.ovCoordVecs = p.uniform("u_CoordVecs"); p.ovRotScale = p.uniform("u_RotationAndScale");
}

Program* Renderer::program(const char* vsName, const char* fsName) {
    std::string key = std::string(vsName) + "|" + fsName;
    auto it = m_prog.find(key);
    if (it != m_prog.end()) return it->second;
    Program* p = new Program(); p->vs = vsName; p->fs = fsName;
    buildProgram(*p);
    m_prog[key] = p;
    return p;
}

void Renderer::use(Program* p) {
    if (p == m_cur) return;
    m_cur = p;
    glUseProgram(p ? p->id : 0);
}

// ---------------- mesh ----------------
void Renderer::buildMesh(Mesh& m, const std::string& path) {
    std::vector<uint8_t> d;
    if (m.mem) {
        uint32_t vc2, ic2; memcpy(&vc2, m.mem, 4); memcpy(&ic2, m.mem + 4, 4);
        d.assign(m.mem, m.mem + 8 + (size_t)vc2 * 24 + (size_t)ic2 * 2);
    } else if (!m_sys->loadAsset(path.c_str(), d) || d.size() < 8) { LOGI("mesh yok: %s", path.c_str()); return; }
    uint32_t vc, ic; memcpy(&vc, d.data(), 4); memcpy(&ic, d.data() + 4, 4);
    size_t vbytes = (size_t)vc * 24, ibytes = (size_t)ic * 2;
    if (8 + vbytes + ibytes > d.size()) { LOGI("mesh bozuk: %s", path.c_str()); return; }
    m.vertCount = vc; m.indexCount = ic;
    m.bytesUsed = 8 + vbytes + ibytes + ((ic & 1) ? 2 : 0);
    glGenBuffers(1, &m.vbo); glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, vbytes, d.data() + 8, GL_STATIC_DRAW);
    glGenBuffers(1, &m.ibo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibytes, d.data() + 8 + vbytes, GL_STATIC_DRAW);
}

Mesh* Renderer::meshFromMemory(const std::string& key, const uint8_t* chunk) {
    auto it = m_mesh.find(key);
    if (it != m_mesh.end()) return it->second;
    Mesh* m = new Mesh(); m->mem = chunk; buildMesh(*m, key); m_mesh[key] = m; return m;
}

void Renderer::perspective(float* m, float fovY, float aspect, float zn, float zf) {
    const float f = 1.0f / tanf(fovY * 0.5f);
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = f / aspect; m[5] = f; m[10] = (zf + zn) / (zn - zf); m[11] = -1.0f; m[14] = 2.0f * zf * zn / (zn - zf);
}
void Renderer::multiply(float* out, const float* a, const float* b) {
    float t[16];
    for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) {
        float s = 0; for (int k = 0; k < 4; ++k) s += a[k * 4 + r] * b[c * 4 + k];
        t[c * 4 + r] = s;
    }
    memcpy(out, t, sizeof t);
}

Mesh* Renderer::mesh(const char* path, size_t* used) {
    auto it = m_mesh.find(path);
    Mesh* m;
    if (it != m_mesh.end()) m = it->second;
    else { m = new Mesh(); buildMesh(*m, path); m_mesh[path] = m; }
    if (used) *used = m->bytesUsed;
    return m;
}

void Renderer::drawMesh(const Mesh* m) {
    if (!m || !m->vbo) return;
    glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibo);
    // yerlesim: pos float3 (0) | normal 4 x int8 normalize (12) | uv float2 (16)
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 24, (const void*)0);
    glVertexAttribPointer(ATTR_NORMAL, 4, GL_BYTE, GL_TRUE, 24, (const void*)12);
    glVertexAttribPointer(ATTR_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 24, (const void*)16);
    glEnableVertexAttribArray(ATTR_POSITION);
    glEnableVertexAttribArray(ATTR_NORMAL);
    glEnableVertexAttribArray(ATTR_TEXCOORD0);
    glDrawElements(GL_TRIANGLES, m->indexCount, GL_UNSIGNED_SHORT, nullptr);
}

// ---------------- 2D ----------------
void Renderer::clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void Renderer::setAlphaBlend(bool on) {
    if (on == m_blend) return;
    m_blend = on;
    if (on) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); } else glDisable(GL_BLEND);
}
void Renderer::setDepthTest(bool test, bool write) {
    if (test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(write ? GL_TRUE : GL_FALSE);
}

void Renderer::beginOverlay() {
    setDepthTest(false, false);
    setAlphaBlend(true);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(ATTR_POSITION);
    glDisableVertexAttribArray(ATTR_NORMAL);
    glDisableVertexAttribArray(ATTR_TEXCOORD0);
}

void Renderer::overlay(const Texture2D* t, float x, float y, float w, float h,
                       float u, float v, float du, float dv, const float* rgba, bool alphaFromR) {
    if (!t || !t->id) return;
    Program* p = program("vertexShader2DOverlay", alphaFromR ? "fragmentShader2DOverlayR8" : "fragmentShader2DOverlay");
    if (!p || !p->id) return;
    use(p);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->id);
    if (p->tex0 >= 0) glUniform1i(p->tex0, 0);
    static const float white[4] = {1, 1, 1, 1};
    glUniform4fv(p->ovColor, 1, rgba ? rgba : white);
    float rs[3] = {0, 1, 1};
    glUniform3fv(p->ovRotScale, 1, rs);
    // orijinal renderOverlay: texVecs = (u, 1-v, du, -dv); coordVecs = (2x-1, 1-2y, 2w, -2h)
    float tv[4] = {u, 1.f - v, du, -dv};
    float cv[4] = {2.f * x - 1.f, 1.f - 2.f * y, 2.f * w, -2.f * h};
    glUniform4fv(p->ovTexVecs, 1, tv);
    glUniform4fv(p->ovCoordVecs, 1, cv);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

} // namespace fs12
#endif
