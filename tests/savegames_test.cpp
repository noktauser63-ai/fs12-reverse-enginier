// SaveGames: yuva kaydet/yukle/sil, ana dosya bicimi ve gecici kayit turu.
#include "game/SaveGames.h"
#include "platform/CryptoUtil.h"
#include "platform/SystemDevice.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using namespace fs12;
static int bad = 0;
#define CHECK(c) do { if (!(c)) { printf("HATA satir %d: %s\n", __LINE__, #c); ++bad; } } while (0)

struct Cb : SaveGames::AppSaveGameCallbackInterface {
    std::vector<uint8_t> state = {1, 2, 3, 4, 5}; bool lastNew = false; std::vector<uint8_t> got; uint32_t gotVer = 0;
    uint32_t serialize(AppSaveGameSlotData& s, uint8_t* buf, const char*) override {
        s.money = 4242; s.hours = 7; s.minutes = 31;
        for (size_t i = 0; i < state.size(); ++i) buf[i] = state[i];
        return (uint32_t)state.size();
    }
    void deserialize(const uint8_t* b, uint32_t n, bool isNew, uint32_t v) override { lastNew = isNew; got.assign(b, b + n); gotVer = v; }
};

int main() {
    std::string dir = "/tmp/fs12_savetest";
    system(("rm -rf " + dir + " && mkdir -p " + dir + "/assets " + dir + "/files").c_str());
    SystemDevice* dev = createHostSystemDevice(dir + "/assets", dir + "/files");
    Cb cb;
    {
        SaveGames sg; sg.load(dev, &cb);
        CHECK(!sg.slotUsed(0) && !sg.hasTemp());
        sg.startTask(SaveGames::CMD_NEW, 0);
        CHECK(cb.lastNew && cb.gotVer == 0x27);
        sg.startTask(SaveGames::CMD_SAVE, 1);
        CHECK(sg.slotUsed(1) && sg.slotData(1).money == 4242);
        sg.startTask(SaveGames::CMD_SAVE_TEMP, 2);
        CHECK(sg.hasTemp());
    }
    {   // yeniden baslat: ana dosyadan okunur
        SaveGames sg; sg.load(dev, &cb);
        CHECK(sg.slotUsed(1) && !sg.slotUsed(0) && sg.slotData(1).hours == 7 && sg.slotData(1).minutes == 31);
        CHECK(sg.hasTemp());
        sg.startTask(SaveGames::CMD_LOAD, 1);
        CHECK(sg.wasLastTaskSuccessful() && cb.got == cb.state && !cb.lastNew && cb.gotVer == 0x27);
        cb.got.clear();
        sg.startTask(SaveGames::CMD_LOAD_TEMP, 0);
        CHECK(sg.wasLastTaskSuccessful() && cb.got == cb.state);
        sg.startTask(SaveGames::CMD_DELETE, 1);
        CHECK(!sg.slotUsed(1));
        sg.startTask(SaveGames::CMD_CLEAR_TEMP, 0);
        CHECK(!sg.hasTemp());
    }
    {   // bozuk crc -> yuva kullanilmaz
        std::vector<uint8_t> m; dev->loadFile("master.dat", m, true);
        CHECK(m.size() == 0x5c);
        m[10] ^= 0xff; dev->saveFile("master.dat", m.data(), m.size());
        SaveGames sg; sg.load(dev, &cb);
        CHECK(!sg.slotUsed(1));
    }
    printf("savegames_test: %s\n", bad ? "HATA" : "tamam");
    return bad ? 1 : 0;
}
