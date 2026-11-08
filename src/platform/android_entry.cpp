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
WorldView* g_world = nullptr;
WorldCamera g_cam;
VehicleDrive g_veh;
MapPhysics* g_phys = nullptr;
bool g_wasIn = false;

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
    g_phys->createVehicle(g_veh.x, g_veh.z, g_veh.yaw, 1.5f, 3.6f, 0.2f);
    g_veh.phys = g_phys;
    g_world->addDeutz7545(g_veh.x, 0.f, g_veh.z, g_veh.yaw);   // Deutz 7545 bicerdover
    g_veh.camYaw = 3.14159265f - g_veh.yaw;
    g_menu = new MenuApp(g_dev, g_dev->textures(), sd->language());
    static struct NoGameCb : SaveGames::AppSaveGameCallbackInterface {   // Game sinifi gelene kadar yer tutucu
        uint32_t serialize(AppSaveGameSlotData&, uint8_t*, const char*) override { return 0; }
        void deserialize(const uint8_t*, uint32_t, bool, uint32_t) override {}
    } s_cb;
    static SaveGames s_saves;
    s_saves.load(sd, &s_cb);
    g_menu->setSaves(&s_saves);
}
void onGLLost() { if (g_r) g_r->contextLost(); }
void onGLRestored() { if (g_r) { g_r->resize(AndroidActivity::instance()->width(), AndroidActivity::instance()->height()); g_r->contextRestored(); } }
void onFrame(float dt) {
    InputDevice* in = InputDevice::instance();
    g_menu->update(in, dt);
    if (g_menu->inGame() && g_world && g_world->loaded()) {
        g_veh.readTouch(in);
        g_veh.update(dt > 0.05f ? 0.05f : dt, &g_world->map());
        g_world->setDeutzPose(g_veh.x, g_veh.y, g_veh.z, g_veh.yaw, g_veh.pitch, g_veh.steer, g_veh.spin);
        g_cam.pos[0] = g_veh.camPos[0]; g_cam.pos[1] = g_veh.camPos[1]; g_cam.pos[2] = g_veh.camPos[2];
        g_cam.yaw = g_veh.camYaw; g_cam.pitch = g_veh.camPitch;
    }
    if (g_menu->playing()) g_r->clear(0.55f, 0.7f, 0.9f); else g_r->clear(0.f, 0.f, 0.f);   // gokyuzu rengi
    if (g_menu->playing() && g_world && g_world->loaded()) {
        g_world->setCamera(g_cam.pos[0], g_cam.pos[1], g_cam.pos[2], g_cam.yaw, g_cam.pitch);
        g_world->render();
    }
    g_dev->beginFrame();
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
