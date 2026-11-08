// AndroidActivity: EGL/ES2 baglami, olay dongusu, girdi yonlendirme (NativeActivity + native_app_glue).
#pragma once
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <android_native_app_glue.h>

namespace fs12 {

// Oyun tarafinin bagli oldugu geri cagrimlar (Game sinifi bunlari doldurur)
struct GameCallbacks {
    void (*onInit)(android_app* app) = nullptr;       // GL hazir olunca ilk kez
    void (*onFrame)(float dt) = nullptr;               // her kare (GL bagli)
    void (*onPause)() = nullptr;
    void (*onResume)() = nullptr;
    void (*onShutdown)() = nullptr;
    void (*onGLLost)() = nullptr;                      // yuzey yok oldu -> GL kaynaklari gecersiz
    void (*onGLRestored)() = nullptr;
};

class AndroidActivity {
public:
    explicit AndroidActivity(android_app* app, const GameCallbacks& cb);
    ~AndroidActivity();

    void run();                       // ana dongu (android_main'den cagrilir)
    int  width() const { return m_w; }
    int  height() const { return m_h; }
    float aspect() const { return m_h ? (float)m_w / (float)m_h : 1.f; }
    void getDisplayDpi(float& x, float& y);
    bool swap();                      // presentGLESFramebuffer
    static int64_t timeMicros();

    static AndroidActivity* instance();

private:
    bool chooseConfig();
    bool createContext();
    bool createSurface();
    void destroySurface();
    void bind();
    void unbind();
    void queryDims();
    void handleCmd(int32_t cmd);
    int32_t handleInput(AInputEvent* e);

    android_app* m_app;
    GameCallbacks m_cb;
    EGLDisplay m_dpy = EGL_NO_DISPLAY;
    EGLSurface m_surf = EGL_NO_SURFACE;
    EGLContext m_ctx = EGL_NO_CONTEXT;
    EGLConfig  m_cfg = nullptr;
    int  m_w = 0, m_h = 0;
    bool m_hasWindow = false, m_focused = false, m_inited = false, m_running = true;
};

} // namespace fs12
#endif
