// updateGuiButtons: orijinalden (Unicorn kahini) alinan vektorlerle bit-bit karsilastirma.
#include "game/GenericGUIManager.h"
#include "platform/InputDevice.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace fs12;

static std::vector<std::string> g_log;
struct Dev : GuiRenderDevice {
    float asp = 1.7777778f;
    float aspect() const override { return asp; }
    void setOverlayColor(const float*) override {}
    void setTextColor(const float*) override {}
    void setFontSize(float) override {}
    void renderOverlay(uint32_t, float, float, float, float, float, float, float, float) override {}
    void renderText(uint32_t, float, float, uint32_t, const char*) override {}
};
static std::string hu(uint32_t u) { char b[16]; snprintf(b, sizeof b, "%08x", u); return b; }
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static float bitsf(uint32_t u) { float f; memcpy(&f, &u, 4); return f; }

int main(int argc, char** argv) {
    std::ifstream in(argc > 1 ? argv[1] : "tests/golden/gui_input.txt");
    if (!in) { puts("gui_input.txt yok"); return 2; }
    std::string line;
    Dev dev; GenericGUIManager g; GuiMenuCtl ctl; GuiPad pad; InputDevice* inp = InputDevice::create();
    GuiCallbacks cb;
    cb.playClick = [](void*, bool s) { g_log.push_back(std::string("K") + (s ? "1" : "0")); };
    static int openret = 0;
    cb.openUrl = [](void*, const char* u) { g_log.push_back(strstr(u, "Manual") ? "UM" : "US"); return openret != 0; };
    cb.buyCoins = [](void*, int w) { g_log.push_back("B" + std::to_string(w)); };
    int idx = 0, frames = 0, bad = 0, mode = 0; uint32_t slotv = 3;
    std::vector<std::string> expect;
    std::string pendingIn;
    auto cmp = [&](const std::string& what, const std::string& a, const std::string& b) {
        if (a != b) { if (++bad <= 10) printf("FARK buton %d mod %d kare %d %s\n  altin: %s\n  bizim: %s\n", idx, mode, frames, what.c_str(), a.c_str(), b.c_str()); }
    };
    while (std::getline(in, line)) {
        std::istringstream ss(line); std::string tag; ss >> tag;
        if (tag == "SC") {
            float aspect; std::string as; int sound, f4, f5, f6, used, lp, open, bid;
            ss >> as >> idx >> sound >> f4 >> f5 >> f6 >> used >> lp >> open >> bid >> mode;
            aspect = std::stof(as); dev.asp = aspect; openret = open;
            g = GenericGUIManager();
            GenericGUIManager::Textures t = {11, 99, 12, 13, 14, 15, 16, 17, 480, 320};
            g.load(&dev, t); g.updateScreenScale();
            g.wr(0x583c, slotv);
            ctl = GuiMenuCtl(); ctl.soundOn = sound; ctl.flag12f4 = f4; ctl.flag12f5 = f5; ctl.flag12f6 = f6; ctl.slotUsed[slotv] = used;
            const uint32_t bases[3] = {0x5bdc, 0x5c20, 0x5bb0}; const int ns[3] = {0x11, 0x11, 0xb};
            for (int b = 0; b < 3; ++b) { for (int k = 0; k < ns[b]; ++k) g.wr(bases[b] + 4 * k, 0xffff); g.wr(bases[b] + 4 * lp, (uint32_t)idx); }
            pad = GuiPad(); frames = 0;
        } else if (tag == "IN") {
            int n; ss >> n; InputDevice::Touch t[16];
            for (int i = 0; i < n; ++i) {
                std::string ids, xs, ys; int fl; ss >> ids >> xs >> ys >> fl;
                t[i] = InputDevice::Touch(); t[i].id = std::stoull(ids, nullptr, 16);
                t[i].x = bitsf((uint32_t)std::stoul(xs, nullptr, 16)); t[i].y = bitsf((uint32_t)std::stoul(ys, nullptr, 16)); t[i].flags = fl;
            }
            std::string p0, p1, p2; ss >> p0 >> p1 >> p2;
            pad.pressed = (uint32_t)std::stoul(p0, nullptr, 16); pad.held = (uint32_t)std::stoul(p1, nullptr, 16); pad.released = (uint32_t)std::stoul(p2, nullptr, 16);
            pad.request = 0;
            inp->debugSetTouches(t, n);
            g_log.clear();
            uint32_t id = (uint32_t)idx;
            bool r = g.updateGuiButtons(ctl, inp, pad, cb, 1, &id);
            { std::vector<std::string> d; for (int i = 0; i < n; ++i) if (inp->touch(i)->gestureDisabled) d.push_back("D" + std::to_string(i)); g_log.insert(g_log.begin(), d.begin(), d.end()); }
            char b[256];
            std::string lg; for (size_t i = 0; i < g_log.size(); ++i) lg += (i ? "," : "") + g_log[i]; if (lg.empty()) lg = "-";
            snprintf(b, sizeof b, "%d %u %u %u %08x %08x %08x %s", r ? 1 : 0, ctl.nextState, ctl.nextArg, pad.request, pad.pressed, pad.held, pad.released, lg.c_str());
            ++frames;
            std::string got = b;
            std::string a; std::getline(in, a); { std::istringstream s2(a); std::string tg; s2 >> tg; std::getline(s2, a); a = a.substr(1); }
            cmp("OUT", a, got);
            const GuiButton& bt = g.button(idx);
            std::string bg; { std::string l2; std::getline(in, l2); std::istringstream s3(l2); std::string tg; s3 >> tg; std::getline(s3, bg); bg = bg.substr(1); }
            snprintf(b, sizeof b, "%08x %08x %08x %08x %08x %02x", (uint32_t)bt.state, fbits(bt.x), fbits(bt.y), bt.touchId[0], bt.touchId[1], bt.pressedLatch);
            cmp("BT", bg, b);
            std::string mw; { std::string l3; std::getline(in, l3); std::istringstream s4(l3); std::string tg; s4 >> tg; std::getline(s4, mw); mw = mw.substr(1); }
            std::string mine;
            for (uint32_t off : {0x58cc, 0x5840, 0x5844, 0x58d0, 0x58d4, 0x588c}) mine += hu(g.rd(off)) + " ";
            for (uint32_t ti : {g.rd(0x59a0), g.rd(0x59bc)}) if (ti < 130) for (uint32_t off : {0x18, 0x1c, 0x20, 0x24}) { uint32_t v; memcpy(&v, (const uint8_t*)&g.button((int)ti) + off, 4); mine += hu(v) + " "; }
            mine.pop_back();
            cmp("MW", mw, mine);
        }
    }
    printf("gui_input_test: %d fark\n", bad);
    return bad ? 1 : 0;
}
