// Renderer: GLESHandheldRenderDevice'in temiz yeniden yazimi (GLES2).
// Texture (.p2d), shader (assets/shader/*.vsh|fsh), mesh (.p3d), 2D overlay.
#pragma once
#if defined(__ANDROID__) || defined(FS12_GL)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "platform/SystemDevice.h"

namespace fs12 {

enum Attrib { ATTR_POSITION = 0, ATTR_NORMAL = 1, ATTR_TEXCOORD0 = 2 };

struct Texture2D {
    GLuint id = 0;
    int w = 0, h = 0;
    std::string path;
    bool mipmaps = true;
    bool repeat = true;
};

struct Program {
    GLuint id = 0;
    std::string vs, fs;
    // sik kullanilan uniform konumlari (-1 = yok)
    GLint mvp = -1, mv = -1, mw = -1, tex0 = -1, tex1 = -1, tex2 = -1;
    GLint dAmbient = -1, dDiffuse = -1, dDir = -1, sPos = -1, sDiffuse = -1, sDir = -1, sCone = -1;
    GLint ovColor = -1, ovTexVecs = -1, ovCoordVecs = -1, ovRotScale = -1;
    GLint uniform(const char* name) const { return glGetUniformLocation(id, name); }
};

struct Mesh {  // .p3d: [u32 vertSayisi][u32 indisSayisi][vert*24 bayt][indis*u16][indis tekse 2 bayt dolgu]
    GLuint vbo = 0, ibo = 0;
    uint32_t vertCount = 0, indexCount = 0;
    size_t bytesUsed = 0;
    const uint8_t* mem = nullptr;   // bellekten (m3d) olusturulduysa: [nv][ni][...] blogu
};

class Renderer {
public:
    explicit Renderer(SystemDevice* sys) : m_sys(sys) {}
    ~Renderer();

    void init(int w, int h);          // GL baglamindan sonra
    void resize(int w, int h);
    void contextLost();               // GL kaynaklari gecersiz (CPU tarafi kalir)
    void contextRestored();           // yazilim yeniden olusturur (texture/shader/mesh)

    // --- kaynaklar ---
    Texture2D* texture(const char* assetPath, bool mipmaps = true);   // onbellekli
    Program* program(const char* vsName, const char* fsName);          // "vertexShaderDefault","fragmentShaderDefault"
    Mesh* mesh(const char* assetPath, size_t* bytesUsed = nullptr);
    // m3d icindeki gomulu mesh blogu (veri, Renderer yasadikca gecerli kalmalidir)
    Mesh* meshFromMemory(const std::string& key, const uint8_t* chunk);
    // yakin/uzak ve gorus acisi ile perspektif matrisi (sutun-oncelikli) olusturur
    static void perspective(float* m, float fovYRad, float aspect, float zn, float zf);
    static void multiply(float* out, const float* a, const float* b);   // out = a * b (sutun-oncelikli)

    // --- cizim ---
    void clear(float r, float g, float b, float a = 1.f);
    void beginOverlay();   // alfa karisim, derinlik kapali, birim kare VBO
    // x,y,w,h: ekran orani 0..1 (sol-ust orijin). u,v,du,dv: texture dikdortgeni (0..1, sol-ust orijin)
    void overlay(const Texture2D* t, float x, float y, float w, float h,
                 float u = 0, float v = 0, float du = 1, float dv = 1,
                 const float* rgba = nullptr, bool alphaFromR = false);
    void drawMesh(const Mesh* m);   // Position/Normal/TexCoord0 baglar ve cizer
    void use(Program* p);

    int width() const { return m_w; }
    int height() const { return m_h; }
    float aspect() const { return m_h ? (float)m_w / m_h : 1.f; }
    void setAlphaBlend(bool on);
    void setDepthTest(bool test, bool write);

private:
    GLuint compile(GLenum type, const std::string& src, const std::string& name);
    void uploadTexture(Texture2D& t);
    void buildProgram(Program& p);
    void buildMesh(Mesh& m, const std::string& path);

    SystemDevice* m_sys;
    int m_w = 0, m_h = 0;
    bool m_etc1 = false;
    GLuint m_quadVbo = 0;
    std::map<std::string, Texture2D*> m_tex;
    std::map<std::string, Program*> m_prog;
    std::map<std::string, Mesh*> m_mesh;
    Program* m_cur = nullptr;
    bool m_blend = false;
};

} // namespace fs12
#endif
