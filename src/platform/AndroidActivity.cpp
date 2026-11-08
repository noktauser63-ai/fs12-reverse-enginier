#ifdef __ANDROID__
#include "AndroidActivity.h"
#include "InputDevice.h"
#include "SystemDevice.h"
#include <android/input.h>
#ifdef FS12_WITH_MOD
#include "mod/ModApi.h"
#endif
#include <android/log.h>
#include <GLES2/gl2.h>
#include <ctime>
#include <cmath>
#include <vector>

namespace fs12 {

static AndroidActivity* g_act = nullptr;
AndroidActivity* AndroidActivity::instance() { return g_act; }

int64_t AndroidActivity::timeMicros() {
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static void egl_err(const char* what) {
    EGLint e = eglGetError();
    if (e != EGL_SUCCESS) __android_log_print(ANDROID_LOG_ERROR, "NativeLog", "EGL error 0x%x at %s", e, what);
}

AndroidActivity::AndroidActivity(android_app* app, const GameCallbacks& cb) : m_app(app), m_cb(cb) {
    g_act = this;
    app->userData = this;
    app->onAppCmd = [](android_app* a, int32_t c) { ((AndroidActivity*)a->userData)->handleCmd(c); };
    app->onInputEvent = [](android_app* a, AInputEvent* e) -> int32_t { return ((AndroidActivity*)a->userData)->handleInput(e); };
}
AndroidActivity::~AndroidActivity() {
    unbind(); destroySurface();
    if (m_ctx != EGL_NO_CONTEXT) eglDestroyContext(m_dpy, m_ctx);
    if (m_dpy != EGL_NO_DISPLAY) eglTerminate(m_dpy);
    g_act = nullptr;
}

// Orijinal: ES2 renderable, derinlik >=16, r>=5 g>=6 b>=5, 16/5/6/5'e en yakin
bool AndroidActivity::chooseConfig() {
    m_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!eglInitialize(m_dpy, nullptr, nullptr)) return false;
    const EGLint attrs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
    EGLint n = 0;
    eglChooseConfig(m_dpy, attrs, nullptr, 0, &n);
    if (n <= 0) return false;
    std::vector<EGLConfig> cfgs(n);
    eglChooseConfig(m_dpy, attrs, cfgs.data(), n, &n);
    int best = -1, bestScore = 1 << 30;
    for (int i = 0; i < n; ++i) {
        EGLint r, g, b, d;
        eglGetConfigAttrib(m_dpy, cfgs[i], EGL_RED_SIZE, &r);
        eglGetConfigAttrib(m_dpy, cfgs[i], EGL_GREEN_SIZE, &g);
        eglGetConfigAttrib(m_dpy, cfgs[i], EGL_BLUE_SIZE, &b);
        eglGetConfigAttrib(m_dpy, cfgs[i], EGL_DEPTH_SIZE, &d);
        if (d < 16 || r < 5 || g < 6 || b < 5) continue;
        int score = abs(r - 5) + abs(g - 6) + abs(b - 5) + abs(d - 16);
        if (score < bestScore) { bestScore = score; best = i; }
    }
    if (best < 0) best = 0;
    m_cfg = cfgs[best];
    EGLint fmt; eglGetConfigAttrib(m_dpy, m_cfg, EGL_NATIVE_VISUAL_ID, &fmt);
    if (m_app->window) ANativeWindow_setBuffersGeometry(m_app->window, 0, 0, fmt);
    return true;
}

bool AndroidActivity::createContext() {
    const EGLint ca[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    m_ctx = eglCreateContext(m_dpy, m_cfg, EGL_NO_CONTEXT, ca);
    egl_err("eglCreateContext");
    return m_ctx != EGL_NO_CONTEXT;
}
bool AndroidActivity::createSurface() {
    m_surf = eglCreateWindowSurface(m_dpy, m_cfg, m_app->window, nullptr);
    egl_err("eglCreateWindowSurface");
    return m_surf != EGL_NO_SURFACE;
}
void AndroidActivity::destroySurface() {
    if (m_surf != EGL_NO_SURFACE) { eglDestroySurface(m_dpy, m_surf); m_surf = EGL_NO_SURFACE; }
}
void AndroidActivity::bind() { eglMakeCurrent(m_dpy, m_surf, m_surf, m_ctx); egl_err("eglMakeCurrent"); }
void AndroidActivity::unbind() { if (m_dpy != EGL_NO_DISPLAY) eglMakeCurrent(m_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }
void AndroidActivity::queryDims() {
    EGLint w = 0, h = 0;
    eglQuerySurface(m_dpy, m_surf, EGL_WIDTH, &w);
    eglQuerySurface(m_dpy, m_surf, EGL_HEIGHT, &h);
    m_w = w; m_h = h;
    glViewport(0, 0, w, h);
}
bool AndroidActivity::swap() { return m_surf != EGL_NO_SURFACE && eglSwapBuffers(m_dpy, m_surf); }

void AndroidActivity::getDisplayDpi(float& x, float& y) {
    // Orijinal: Java getDisplayXDpi/YDpi. Burada AConfiguration yeterli.
    int d = AConfiguration_getDensity(m_app->config);
    if (d <= 0 || d == ACONFIGURATION_DENSITY_ANY || d == ACONFIGURATION_DENSITY_NONE) d = 160;
    x = y = (float)d;
}

void AndroidActivity::handleCmd(int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (!m_app->window) break;
        if (m_ctx == EGL_NO_CONTEXT) { if (!chooseConfig() || !createContext()) { m_running = false; break; } }
        if (createSurface()) {
            bind(); queryDims(); m_hasWindow = true;
            if (!m_inited) { m_inited = true; if (m_cb.onInit) m_cb.onInit(m_app); }
            else if (m_cb.onGLRestored) m_cb.onGLRestored();
        }
        break;
    case APP_CMD_TERM_WINDOW:
        if (m_cb.onGLLost) m_cb.onGLLost();
        unbind(); destroySurface(); m_hasWindow = false;
        break;
    case APP_CMD_GAINED_FOCUS: m_focused = true; if (m_cb.onResume) m_cb.onResume(); break;
    case APP_CMD_LOST_FOCUS:
    case APP_CMD_PAUSE: if (m_focused && m_cb.onPause) m_cb.onPause(); m_focused = false; break;
    case APP_CMD_DESTROY: m_running = false; break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONFIG_CHANGED: if (m_hasWindow) queryDims(); break;
    default: break;
    }
}

int32_t AndroidActivity::handleInput(AInputEvent* e) {
#ifdef FS12_WITH_MOD
    if (ModFilterInputEvent(e)) return 1;   // cok oyunculu mod (ImGui menuleri/klavye) olayi yuttu
#endif
    InputDevice* in = InputDevice::instance();
    if (!in) return 0;
    int type = AInputEvent_getType(e);
    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int src = AInputEvent_getSource(e);
        if ((src & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK && (src & AINPUT_SOURCE_CLASS_JOYSTICK)) {
            auto dz = [](float v) { return std::fabs(v) < 0.1f ? 0.f : v; };
            float lx = AMotionEvent_getAxisValue(e, AMOTION_EVENT_AXIS_X, 0);
            float ly = AMotionEvent_getAxisValue(e, AMOTION_EVENT_AXIS_Y, 0);
            float rx = AMotionEvent_getAxisValue(e, AMOTION_EVENT_AXIS_Z, 0);
            float ry = AMotionEvent_getAxisValue(e, AMOTION_EVENT_AXIS_RZ, 0);
            in->padAxes(dz(lx), dz(ly), dz(rx), dz(ry));
            return 1;
        }
        int32_t act = AMotionEvent_getAction(e);
        int kind = act & AMOTION_EVENT_ACTION_MASK;
        int idx = (act & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        double t = AMotionEvent_getEventTime(e) / 1e9;
        float w = m_w ? (float)m_w : 1.f, h = m_h ? (float)m_h : 1.f;
        auto pid = [&](int i) { return (uint64_t)AMotionEvent_getPointerId(e, i); };
        auto px = [&](int i) { return AMotionEvent_getX(e, i) / w; };
        auto py = [&](int i) { return AMotionEvent_getY(e, i) / h; };
        switch (kind) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN: in->touchBegan(pid(idx), px(idx), py(idx), t); break;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP: in->touchEnded(pid(idx), px(idx), py(idx), t); break;
        case AMOTION_EVENT_ACTION_MOVE:
            for (size_t i = 0, n = AMotionEvent_getPointerCount(e); i < n; ++i) in->touchMoved(pid(i), px(i), py(i), t);
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            for (size_t i = 0, n = AMotionEvent_getPointerCount(e); i < n; ++i) in->touchCancelled(pid(i));
            break;
        }
        return 1;
    }
    if (type == AINPUT_EVENT_TYPE_KEY) {
        int code = AKeyEvent_getKeyCode(e);
        bool down = AKeyEvent_getAction(e) == AKEY_EVENT_ACTION_DOWN;
        if (code == AKEYCODE_BACK) { in->backKeyPressed(down); return 1; }
        uint32_t m = 0;
        switch (code) {
        case AKEYCODE_BUTTON_A: m = BTN_A; break;
        case AKEYCODE_BUTTON_B: m = BTN_B; break;
        case AKEYCODE_BUTTON_X: m = BTN_X; break;
        case AKEYCODE_BUTTON_Y: m = BTN_Y; break;
        case AKEYCODE_DPAD_LEFT: m = BTN_LEFT; break;
        case AKEYCODE_DPAD_RIGHT: m = BTN_RIGHT; break;
        case AKEYCODE_BUTTON_L2: m = BTN_L2; break;
        case AKEYCODE_BUTTON_R2: m = BTN_R2; break;
        }
        if (m) { in->padButton(m, down); return 1; }
    }
    return 0;
}

void AndroidActivity::run() {
#ifdef FS12_WITH_MOD
    ModInit();
#endif
    int64_t last = timeMicros();
    while (m_running) {
        int ev; android_poll_source* src;
        bool active = m_hasWindow && m_focused;
        while (ALooper_pollOnce(active ? 0 : -1, nullptr, &ev, (void**)&src) >= 0) {
            if (src) src->process(m_app, src);
            if (m_app->destroyRequested) { m_running = false; break; }
            active = m_hasWindow && m_focused;
        }
        if (!m_running) break;
        if (m_hasWindow && m_focused && m_cb.onFrame) {
            int64_t now = timeMicros();
            float dt = (now - last) / 1e6f; last = now;
            if (dt > 0.1f) dt = 0.1f;
            m_cb.onFrame(dt);
            if (InputDevice::instance()) InputDevice::instance()->updateFrameEnd();
        } else last = timeMicros();
    }
    if (m_cb.onShutdown) m_cb.onShutdown();
}

} // namespace fs12
#endif
