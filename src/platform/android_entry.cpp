// android_main: platform duman testi uygulamasi. Gercek Game sinifi hazir oldukca burasi ona baglanacak.
#ifdef __ANDROID__
#include "AndroidActivity.h"
#include "InputDevice.h"
#include "SystemDevice.h"
#include "audio/engine.h"
#include "render/Renderer.h"
#include "game/GuiDeviceGL.h"
#include "game/MenuApp.h"
#include "game/WorldView.h"
#include "game/WorldCamera.h"
#include "game/VehicleDrive.h"
#include "game/MapPhysics.h"
#include "game/VehicleCatalog.h"
#include <GLES2/gl2.h>
#include <cmath>
#include <string>

using namespace fs12;

namespace {
audio::Engine* g_audio = nullptr;
audio::Output* g_out = nullptr;
Renderer* g_r = nullptr;
GuiDeviceGL* g_dev = nullptr;
MenuApp* g_menu = nullptr;
static const VehicleSpec* g_spec = nullptr;
WorldView* g_world = nullptr;
WorldCamera g_cam;
VehicleDrive g_veh;
MapPhysics* g_phys = nullptr;
bool g_wasIn = false;

// Aktif araci sahneye/surus modeline uygular (dukkandan secilen arac; yoksa varsayilan Magnum, o da yoksa orijinal Deutz 7545)
static void applyVehicle(const VehicleSpec* want) {
    if (!want) want = findVehicle("magnum");
    g_veh.speed = 0.f;
    if (want && want->nWheels >= 3 && g_world->addVehicleSpec(*want)) {
        float fz = -1e9f, rz = 1e9f, r = 0.9f;
        for (int i = 0; i < want->nWheels; ++i) { if (want->wheels[i].z > fz) fz = want->wheels[i].z; if (want->wheels[i].z < rz) rz = want->wheels[i].z; r = want->wheels[i].radius; }
        g_veh.wheelBase = fz > rz ? fz - rz : 3.0f; g_veh.maxSpeed = want->topKmh / 3.6f; g_veh.wheelRadius = r;
        g_phys->createVehicle(g_veh.x, g_veh.z, g_veh.yaw, 1.5f, g_veh.wheelBase * 0.5f + 1.4f, 0.2f);
        g_spec = want;
    } else {
        g_spec = nullptr;
        g_phys->createVehicle(g_veh.x, g_veh.z, g_veh.yaw, 1.5f, 3.6f, 0.2f);
        g_world->addDeutz7545(g_veh.x, 0.f, g_veh.z, g_veh.yaw);
    }
}
void onInit(android_app* app) {
    SystemDevice* sd = SystemDevice::instance();
    sd->logf("FS12: GL hazir, ses baslatiliyor");
    g_audio = new audio::Engine();
    std::vector<uint8_t> d;
    if (sd->loadAsset("audio/music01.p1d", d) && g_audio->musicOpen(d.data(), d.size())) {
        g_audio->musicPlay(true);
        sd->logf("FS12: music01 caliyor");
    } else sd->logf("FS12: music01 bulunamadi");
    g_out = audio::createDefaultOutput();
    if (g_out) g_out->start(g_audio);
    g_r = new Renderer(sd);
    g_r->init(AndroidActivity::instance()->width(), AndroidActivity::instance()->height());
    g_dev = new GuiDeviceGL(g_r, sd->languageCode().c_str());
    g_world = new WorldView(g_r, sd);
    g_world->load();
    g_phys = new MapPhysics();
    g_phys->build(g_world->map());
    g_veh.phys = g_phys;
    g_veh.camYaw = 3.14159265f - g_veh.yaw;
    g_menu = new MenuApp(g_dev, g_dev->textures(), sd->language());
    static struct NoGameCb : SaveGames::AppSaveGameCallbackInterface {   // Game sinifi gelene kadar yer tutucu
        uint32_t serialize(AppSaveGameSlotData&, uint8_t*, const char*) override { return 0; }
        void deserialize(const uint8_t*, uint32_t, bool, uint32_t) override {}
    } s_cb;
    static SaveGames s_saves;
    s_saves.load(sd, &s_cb);
    g_menu->setSaves(&s_saves);
    {   // oyuncu durumu (para/araclar)
        std::vector<uint8_t> sv;
        if (sd->loadSave("fs12_state.bin", sv)) g_menu->state().fromBytes(sv.data(), sv.size());
        g_menu->onStateChanged = [] { auto b = g_menu->state().toBytes(); SystemDevice::instance()->saveFile("fs12_state.bin", b.data(), b.size()); };
        g_menu->setShopTexture(GuiDeviceGL::T_SHOP_EXT);
        applyVehicle(g_menu->state().activeSpec());
    }
}
void onGLLost() { if (g_r) g_r->contextLost(); }
void onGLRestored() { if (g_r) { g_r->resize(AndroidActivity::instance()->width(), AndroidActivity::instance()->height()); g_r->contextRestored(); } }
static float g_gameClock = 8.0f * 60.0f;   // oyun ici dakika (08:00 baslangic)
void onFrame(float dt) {
    InputDevice* in = InputDevice::instance();
    const bool wasIn = g_menu->inGame();
    g_menu->update(in, dt);
    if (g_menu->inGame() && !wasIn && g_world && g_world->loaded()) {   // oyuna girerken secili araci uygula
        const VehicleSpec* w = g_menu->state().activeSpec();
        if (w && w != g_spec) applyVehicle(w);
    }
    if (g_menu->inGame() && g_world && g_world->loaded()) {
        g_veh.readTouch(in);
        g_veh.update(dt > 0.05f ? 0.05f : dt, &g_world->map());
        if (g_spec) g_world->setSpecPose(g_veh.x, g_veh.y, g_veh.z, g_veh.yaw, g_veh.pitch, g_veh.steer, g_veh.spin);
        else g_world->setDeutzPose(g_veh.x, g_veh.y, g_veh.z, g_veh.yaw, g_veh.pitch, g_veh.steer, g_veh.spin);
        g_cam.pos[0] = g_veh.camPos[0]; g_cam.pos[1] = g_veh.camPos[1]; g_cam.pos[2] = g_veh.camPos[2];
        g_cam.yaw = g_veh.camYaw; g_cam.pitch = g_veh.camPitch;
    }
    if (g_menu->playing()) g_r->clear(0.55f, 0.7f, 0.9f); else g_r->clear(0.f, 0.f, 0.f);   // gokyuzu rengi
    if (g_menu->playing() && g_world && g_world->loaded()) {
        g_world->setCamera(g_cam.pos[0], g_cam.pos[1], g_cam.pos[2], g_cam.yaw, g_cam.pitch);
        g_world->render();
    }
    g_dev->beginFrame();
    if (g_menu->inGame()) {
        g_gameClock += dt * 1.0f; if (g_gameClock >= 1440.f) g_gameClock -= 1440.f;   // 1 gercek sn = 1 oyun dk
        GuiHudCtl h; h.money = (uint32_t)(g_menu->state().money < 0 ? 0 : g_menu->state().money); h.hours = (uint32_t)(g_gameClock / 60.f); h.minutes = (uint32_t)g_gameClock % 60; h.sel59c = 0; h.v124 = (uint32_t)(std::fabs(g_veh.speed) * 3.6f + 0.5f); h.pct = 1.0f;   // sol sayi = hiz km/h (tahmin), % = depo
        g_menu->renderHud(h);
    }
    g_menu->render();
    AndroidActivity::instance()->swap();
}
void onPause() { if (g_out) g_out->setPaused(true); }
void onResume() { if (g_out) g_out->setPaused(false); }
void onShutdown() {
    if (g_out) { g_out->stop(); delete g_out; g_out = nullptr; }
    delete g_menu; g_menu = nullptr;
    delete g_world; g_world = nullptr;
    delete g_dev; g_dev = nullptr;
    delete g_r; g_r = nullptr;
    delete g_audio; g_audio = nullptr;
}
} // namespace

extern "C" void android_main(android_app* app) {
    std::string files = app->activity->internalDataPath ? app->activity->internalDataPath : "/data/local/tmp";
    SystemDevice::setInstance(createAndroidSystemDevice(app->activity->assetManager, files, "fs12-device", "en"));
    InputDevice::create();
    GameCallbacks cb;
    cb.onInit = onInit; cb.onFrame = onFrame; cb.onPause = onPause; cb.onResume = onResume; cb.onShutdown = onShutdown; cb.onGLLost = onGLLost; cb.onGLRestored = onGLRestored;
    AndroidActivity act(app, cb);
    act.run();
}
#endif
