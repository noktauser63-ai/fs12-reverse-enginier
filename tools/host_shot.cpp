// Ana makinede (Mesa llvmpipe, EGL pbuffer) MenuApp'i cizip PPM ekran goruntusu uretir.
// Derleme: tools/build_host_shot.sh   Kullanim: host_shot <assetDir> <cikti_oneki> [dil] [sahne ...]
#include "game/GuiDeviceGL.h"
#include "game/MenuApp.h"
#include "game/WorldView.h"
#include "game/VehicleDrive.h"
#include "game/MapPhysics.h"
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
    if (getenv("FS12_EXTSHOP")) app.setShopTexture(GuiDeviceGL::T_SHOP_EXT);
    InputDevice* in = InputDevice::create();
    std::string pre = argv[2];
    auto frame = [&](int k) { for (int i = 0; i < k; ++i) { in->debugSetTouches(nullptr, 0); app.update(in, 0.05f); } r.clear(0, 0, 0); dev.beginFrame(); app.render(); glFinish(); };
    frame(30); savePPM(pre + "_main.ppm", W, H);
    app.handleNext(0x24); frame(2); savePPM(pre + "_save.ppm", W, H);
    app.handleNext(10); frame(2); savePPM(pre + "_difficulty.ppm", W, H);
    app.handleNext(0x11); app.pauseGame(); frame(2); savePPM(pre + "_start.ppm", W, H);
    for (int p : {1, 2, 3, 4, 5}) { app.gui().wr(0x58cc, p); frame(2); savePPM(pre + "_page" + std::to_string(p) + ".ppm", W, H); }
    {   // dunya gorunumu: argv[4..] = x y z yaw pitch
        WorldView wv(&r, sd);
        if (wv.load()) {
            float v[5] = {0, 8, 0, 0, 0.3f};
            for (int i = 0; i < 5 && argc > 4 + i; ++i) v[i] = (float)atof(argv[4 + i]);
            const char* vid = getenv("FS12_VEH"); const VehicleSpec* vs = vid ? findVehicle(vid) : nullptr;
            if (vs) { printf("arac %s: %d\n", vid, (int)wv.addVehicleSpec(*vs)); } else wv.addDeutz7545(10, 0, -3, 0.5f);
            MapPhysics mp; mp.build(wv.map()); VehicleDrive veh; mp.createVehicle(veh.x, veh.z, veh.yaw, 1.5f, 3.6f, 0.2f); veh.phys = &mp; printf("statik: %d\n", mp.staticCount()); veh.camYaw = 3.14159265f - veh.yaw; veh.throttle = 1.f; veh.steerIn = 0.25f;
            for (int i = 0; i < 100; ++i) { veh.update(0.05f, &wv.map()); if (i == 40) veh.steerIn = -0.2f; }
            printf("arac: x=%.1f y=%.2f z=%.1f yaw=%.2f hiz=%.1f pitch=%.2f\n", veh.x, veh.y, veh.z, veh.yaw, veh.speed, veh.pitch);
            if (vs) wv.setSpecPose(veh.x, veh.y, veh.z, veh.yaw, veh.pitch, veh.steer, veh.spin); else wv.setDeutzPose(veh.x, veh.y, veh.z, veh.yaw, veh.pitch, veh.steer, veh.spin);
            if (argc <= 4) { v[0] = veh.camPos[0]; v[1] = veh.camPos[1]; v[2] = veh.camPos[2]; v[3] = veh.camYaw; v[4] = veh.camPitch; }   // arguman yoksa takip kamerasi
            wv.setCamera(v[0], v[1], v[2], v[3], v[4]);
            if (const char* ic = getenv("FS12_ICONS")) {   // dukkan ikonlari: her arac 3/4 gorunum, macenta arka plan
                wv.propsOnly = true;
                size_t n; const VehicleSpec* all = vehicleSpecs(n);
                for (size_t i = 0; i < n; ++i) {
                    const VehicleSpec& sp = all[i];
                    wv.addVehicleSpec(sp);
                    wv.setSpecPose(0, 0, 0, 0, 0, 0.0f, 0.0f);
                    const float D = sp.kind == VKind::Combine ? 15.f : (sp.kind == VKind::Planter ? 20.f : (sp.kind == VKind::Cultivator ? 13.f : 11.f)), A = 0.75f;
                    wv.setCamera(D * sinf(A), sp.kind == VKind::Combine ? 4.0f : 3.2f, D * cosf(A), -A, 0.17f);
                    r.clear(1.f, 0.f, 1.f); wv.render(); glFinish();
                    savePPM(std::string(ic) + "_" + sp.id + ".ppm", W, H);
                    printf("ikon %s\n", sp.id);
                }
                return 0;
            }
            r.clear(0.55f, 0.7f, 0.9f); wv.render(); glFinish();
            printf("dunya: %d cizim\n", wv.lastDrawCalls);
            savePPM(pre + "_world.ppm", W, H);
        } else printf("harita yuklenemedi\n");
    }
    printf("tamam\n");
    return 0;
}
