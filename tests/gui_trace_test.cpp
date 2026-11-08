// GUI cizim izini orijinal libapp.so'dan (Unicorn kahini) alinan altin izle bit-bit karsilastirir.
#include "game/GenericGUIManager.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace fs12;

static std::string hx(float f) { uint32_t u; memcpy(&u, &f, 4); char b[16]; snprintf(b, sizeof b, "%08x", u); return b; }
static std::string hu(uint32_t u) { char b[16]; snprintf(b, sizeof b, "%08x", u); return b; }

struct TraceDevice : GuiRenderDevice {
    float asp; std::vector<std::string> lines;
    float aspect() const override { return asp; }
    void setOverlayColor(const float* c) override { lines.push_back("C " + hx(c[0]) + " " + hx(c[1]) + " " + hx(c[2]) + " " + hx(c[3])); }
    void setTextColor(const float* c) override { lines.push_back("T " + hx(c[0]) + " " + hx(c[1]) + " " + hx(c[2]) + " " + hx(c[3])); }
    void setFontSize(float s) override { lines.push_back("F " + hx(s)); }
    void renderOverlay(uint32_t t, float x, float y, float w, float h, float u, float v, float du, float dv) override {
        lines.push_back("O " + hu(t) + " " + hx(x) + " " + hx(y) + " " + hx(w) + " " + hx(h) + " " + hx(u) + " " + hx(v) + " " + hx(du) + " " + hx(dv));
    }
    void setNumberMonospace(bool on, float w) override { lines.push_back("M " + std::to_string(on ? 1 : 0) + " " + hx(w)); }
    void renderText(uint32_t f, float x, float y, uint32_t fl, const char* s) override {
        lines.push_back("R " + hu(f) + " " + hx(x) + " " + hx(y) + " " + hu(fl) + " \"" + s + "\"");
    }
};

static char names[18][16], descs[18][16];
int main(int argc, char** argv) {
    for (int i = 0; i < 18; ++i) { snprintf(names[i], 16, "Name%d", i); snprintf(descs[i], 16, "Desc%d", i); }
    std::string dir = argc > 1 ? argv[1] : "tests/golden";
    int bad = 0, total = 0;
    for (const char* name : {"main_a", "main_b", "main_c", "main_d", "startmain_a", "startmain_b", "credits_a", "credits_b", "credits_c", "settings_a", "settings_b", "help_0", "help_1", "help_2", "help_3", "save_a", "save_b", "stats_a", "stats_b", "prices_a", "prices_b", "shop_a", "shop_b", "shop_c", "shop_d", "dlg_a", "dlg_b", "dlg_c", "dlg_d"}) {
        std::ifstream in(dir + "/gui_trace_" + name + ".txt");
        if (!in) { printf("yok: %s\n", name); return 2; }
        std::string hdr; std::getline(in, hdr);
        float aspect, phase; int lang, c, x146; char fn[64] = "renderMainMenu"; float scroll = 0; static char dw[8192], db[8192], mw[256]; strcpy(dw, "-"); strcpy(db, "-"); strcpy(mw, "-");
        sscanf(hdr.c_str(), "# aspect=%f lang=%d phase=%f c=%d x146=%d fn=%63s scroll=%f dw=%8191s db=%8191s mw=%255s", &aspect, &lang, &phase, &c, &x146, fn, &scroll, dw, db, mw);
        GuiMenuCtl ctl;
        for (char* t = strtok(dw, ","); t && strcmp(t, "-"); t = strtok(nullptr, ",")) { unsigned o, v; sscanf(t, "%x:%x", &o, &v); if (o >= 0x1224 && o < 0x12dc) ctl.w1224[(o - 0x1224) / 4] = v; if (o >= 0x65c && o < 0x65c + 72) ctl.shop.name[(o - 0x65c) / 4] = v ? names[(o - 0x65c) / 4] : nullptr;
            if (o >= 0x6a4 && o < 0x6a4 + 72) ctl.shop.desc[(o - 0x6a4) / 4] = v ? descs[(o - 0x6a4) / 4] : nullptr;
            if (o >= 0x5a8 && o < 0x5a8 + 72) ctl.shop.count[(o - 0x5a8) / 4] = v;
            if (o >= 0x614 && o < 0x614 + 72) ctl.shop.price[(o - 0x614) / 4] = v;
            if (o == 0x12c) ctl.shop.money = v;
            if (o >= 0x10 && o < 0x1c) ctl.dlg.id[(o - 0x10) / 4] = (int)v;
            if (o >= 0x1c && o < 0x28) { static char mm[3][8]; int i = (o - 0x1c) / 4; snprintf(mm[i], 8, "Msg%d", i); ctl.dlg.text[i] = v ? mm[i] : nullptr; }
            if (o == 0x28) ctl.dlg.count = v;
            if (o == 0x12ec) ctl.val12ec = (int)v; if (o == 0x12e8) ctl.val12e8 = (int)v; if (o >= 0x100 && o < 0x10c) ctl.saveMoney[(o - 0x100) / 4] = v; if (o >= 0x10c && o < 0x118) ctl.saveHours[(o - 0x10c) / 4] = v; if (o >= 0x118 && o < 0x124) ctl.saveMinutes[(o - 0x118) / 4] = v; }
        for (char* t = strtok(db, ","); t && strcmp(t, "-"); t = strtok(nullptr, ",")) { unsigned o, v; sscanf(t, "%x:%x", &o, &v); if (o >= 0xfc && o < 0xff) ctl.saveUsed[o - 0xfc] = v; if (o >= 0x734 && o < 0x746) ctl.shop.badgeA[o - 0x734] = v;
            if (o >= 0x746 && o < 0x758) ctl.shop.badgeB[o - 0x746] = v;
            if (o >= 0x758 && o < 0x76a) ctl.shop.badgeC[o - 0x758] = v;
            if (o >= 0x76a && o < 0x77c) ctl.slotUsed[o - 0x76a] = v;
            if (o >= 0x77c && o < 0x77c + 18 * 0x80 && (o - 0x77c) % 0x80 == 0) snprintf(ctl.shop.alt[(o - 0x77c) / 0x80], 128, "Alt%u", (o - 0x77c) / 0x80);
            if (o >= 0x107c && o < 0x107c + 3 * 0x80 && (o - 0x107c) % 0x80 == 0) snprintf(ctl.dlg.slotName[(o - 0x107c) / 0x80], 128, "Slot%u", (o - 0x107c) / 0x80);
            if (o == 0x1220) ctl.shop.flag1220 = v;
            if (o == 0x1221) ctl.shop.flag1221 = v;
            if (o == 0x12f4) ctl.flag12f4 = v; if (o == 0x12f5) ctl.flag12f5 = v; if (o == 0x12f6) ctl.flag12f6 = v; }
        std::vector<std::string> gold; std::string l;
        while (std::getline(in, l)) if (!l.empty()) gold.push_back(l);
        TraceDevice dev; dev.asp = aspect;
        GenericGUIManager g;
        GenericGUIManager::Textures t = {11, 99, 12, 13, 14, 15, 16, 17, 480, 320};
        g.load(&dev, t);
        g.setPhase(phase);
        for (char* t = strtok(mw, ","); t && strcmp(t, "-"); t = strtok(nullptr, ",")) { unsigned o, v; sscanf(t, "%x:%x", &o, &v); g.wr(o, v); }
        if (!strcmp(fn, "renderCreditsMenu")) {
            g.wrf(0x588c, scroll);
            static char buf[52][32]; const char* ln[52];
            for (int i = 0; i < 52; ++i) { snprintf(buf[i], 32, "Credit line %d", i); ln[i] = buf[i]; }
            g.renderCreditsMenu(ln);
        }
        else if (!strcmp(fn, "renderSaveGameMenu")) g.renderSaveGameMenu(ctl, lang);
        else if (!strcmp(fn, "renderDialog")) g.renderDialog(ctl, lang);
        else if (!strcmp(fn, "renderStartMenuShop")) g.renderStartMenuShop(ctl, lang);
        else if (!strcmp(fn, "renderStartMenuPrices")) g.renderStartMenuPrices(ctl, lang);
        else if (!strcmp(fn, "renderStartMenuStatistics")) g.renderStartMenuStatistics(ctl, lang);
        else if (!strcmp(fn, "renderStartMenuHelp")) g.renderStartMenuHelp(lang);
        else if (!strcmp(fn, "renderStartMenuSettings")) g.renderStartMenuSettings(ctl, lang);
        else if (!strcmp(fn, "renderStartMenuMain")) g.renderStartMenuMain(lang);
        else g.renderMainMenu(c != 0, x146 != 0, lang);
        { std::vector<std::string> flat; for (auto& e : dev.lines) { std::istringstream ls(e); std::string q; while (std::getline(ls, q)) flat.push_back(q); } dev.lines = flat; }
        bool ok = gold.size() == dev.lines.size();
        for (size_t i = 0; ok && i < gold.size(); ++i) ok = gold[i] == dev.lines[i];
        ++total;
        if (!ok) {
            ++bad; printf("FARK %s (altin %zu satir, bizim %zu satir)\n", name, gold.size(), dev.lines.size());
            for (size_t i = 0; i < std::max(gold.size(), dev.lines.size()); ++i) {
                std::string a = i < gold.size() ? gold[i] : "-", b = i < dev.lines.size() ? dev.lines[i] : "-";
                printf("  %s %s\n    altin: %s\n    bizim: %s\n", a == b ? " " : "!", "", a.c_str(), b.c_str());
            }
        }
    }
    printf("gui_trace_test: %d senaryo, %d fark\n", total, bad);
    return bad ? 1 : 0;
}
