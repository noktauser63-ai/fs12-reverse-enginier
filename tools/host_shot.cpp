// Ana makinede (Mesa llvmpipe, EGL pbuffer) MenuApp'i cizip PPM ekran goruntusu uretir.
// Derleme: tools/build_host_shot.sh   Kullanim: host_shot <assetDir> <cikti_oneki> [dil] [sahne ...]
#include "game/GuiDeviceGL.h"
#include "game/MenuApp.h"
#include "game/WorldView.h"
#include "platform/InputDevice.h"
#include "platform/SystemDevice.h"
#include "render/Renderer.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace fs12;
int __android_log_print(int, const char*, const char* fmt, ...) { return 0; }

static void savePPM(const std::string& path, int w, int h) {
    std::vector<unsigned char> px((size_t)w * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    FILE* f = fopen(path.c_str(), "wb"); fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; --y) for (int x = 0; x < w; ++x) fwrite(&px[((size_t)y * w + x) * 4], 1, 3, f);
    fclose(f);
}

int main(int argc, char** argv) {
    if (argc < 3) { printf("kullanim: host_shot <assetDir> <oneki> [dil] [sahne...]\n"); return 2; }
    const int W = 960, H = 540;
    typedef EGLDisplay (*GetPlat)(EGLenum, void*, const EGLint*);
    GetPlat gp = (GetPlat)eglGetProcAddress("eglGetPlatformDisplayEXT");
    EGLDisplay dpy = gp ? gp(0x31DD /*EGL_PLATFORM_SURFACELESS_MESA*/, EGL_DEFAULT_DISPLAY, nullptr) : eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint maj, min; if (!eglInitialize(dpy, &maj, &min)) { printf("EGL baslatilamadi\n"); return 1; }
    const EGLint ca[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE};
    EGLConfig cfg; EGLint n; eglChooseConfig(dpy, ca, &cfg, 1, &n);
    const EGLint pa[] = {EGL_WIDTH, W, EGL_HEIGHT, H, EGL_NONE};
    EGLSurface sf = eglCreatePbufferSurface(dpy, cfg, pa);
    eglBindAPI(EGL_OPENGL_ES_API);
    const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext cx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, xa);
    if (!eglMakeCurrent(dpy, sf, sf, cx)) { printf("EGL baglam hatasi\n"); return 1; }
    printf("GL: %s\n", glGetString(GL_RENDERER));
    std::string lang = argc > 3 ? argv[3] : "en";
    SystemDevice* sd = createHostSystemDevice(argv[1], "/tmp/fs12_shot_files");
    Renderer r(sd); r.init(W, H);
    GuiDeviceGL dev(&r, lang.c_str());
    MenuApp app(&dev, dev.textures(), languageFromCode(lang.c_str()));
    InputDevice* in = InputDevice::create();
    std::string pre = argv[2];
    auto frame = [&](int k) { for (int i = 0; i < k; ++i) { in->debugSetTouches(nullptr, 0); app.update(in, 0.05f); } r.clear(0, 0, 0); dev.beginFrame(); app.render(); glFinish(); };
    frame(30); savePPM(pre + "_main.ppm", W, H);
    app.handleNext(0x24); frame(2); savePPM(pre + "_save.ppm", W, H);
    app.handleNext(10); frame(2); savePPM(pre + "_difficulty.ppm", W, H);
    app.handleNext(0x11); frame(2); savePPM(pre + "_start.ppm", W, H);
    for (int p : {1, 2, 3, 4, 5}) { app.gui().wr(0x58cc, p); frame(2); savePPM(pre + "_page" + std::to_string(p) + ".ppm", W, H); }
    {   // dunya gorunumu: argv[4..] = x y z yaw pitch
        WorldView wv(&r, sd);
        if (wv.load()) {
            float v[5] = {0, 8, 0, 0, 0.3f};
            for (int i = 0; i < 5 && argc > 4 + i; ++i) v[i] = (float)atof(argv[4 + i]);
            wv.setCamera(v[0], v[1], v[2], v[3], v[4]);
            r.clear(0.55f, 0.7f, 0.9f); wv.render(); glFinish();
            printf("dunya: %d cizim\n", wv.lastDrawCalls);
            savePPM(pre + "_world.ppm", W, H);
        } else printf("harita yuklenemedi\n");
    }
    printf("tamam\n");
    return 0;
}
