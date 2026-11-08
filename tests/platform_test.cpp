#include "platform/CryptoUtil.h"
#include "platform/SystemDevice.h"
#include "platform/InputDevice.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#define CHECK(c) do{ if(!(c)){ printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#c); return 1; } }while(0)
using namespace fs12;
int main() {
    // RC4 bilinen test vektoru: key "Key", plaintext "Plaintext" -> BBF316E8D940AF0AD3
    Rc4State st; rc4Ksa(st, (const uint8_t*)"Key", 3);
    uint8_t ks[9]; rc4Prga(st, ks, 9);
    const char* pt = "Plaintext"; const uint8_t exp[9] = {0xBB,0xF3,0x16,0xE8,0xD9,0x40,0xAF,0x0A,0xD3};
    for (int i = 0; i < 9; ++i) CHECK((uint8_t)(pt[i] ^ ks[i]) == exp[i]);
    // CRC32("123456789") = CBF43926
    CHECK(crc32((const uint8_t*)"123456789", 9) == 0xCBF43926u);
    // kayit sifreleme gidis-donus
    uint8_t prefix[16]; for (int i = 0; i < 16; ++i) prefix[i] = (uint8_t)(i * 7 + 1);
    const char* msg = "savegame-data";
    auto ob = obfuscate("dev123", (const uint8_t*)msg, strlen(msg), prefix);
    CHECK(ob.size() == 16 + strlen(msg));
    std::vector<uint8_t> back; CHECK(deobfuscate("dev123", ob.data(), ob.size(), back));
    CHECK(back.size() == strlen(msg) && !memcmp(back.data(), msg, back.size()));
    CHECK(deobfuscate("other", ob.data(), ob.size(), back) && memcmp(back.data(), msg, back.size()));
    // dil tablosu
    CHECK(languageFromCode("de") == 1 && languageFromCode("ko") == 9 && languageFromCode("xx") == 0);
    // host sistem cihazi
    system("rm -rf /tmp/fs12_pt && mkdir -p /tmp/fs12_pt/assets");
    FILE* f = fopen("/tmp/fs12_pt/assets/a.bin", "wb"); fwrite("0123456789", 1, 10, f); fclose(f);
    SystemDevice* sd = createHostSystemDevice("/tmp/fs12_pt/assets", "/tmp/fs12_pt/files");
    std::vector<uint8_t> v;
    CHECK(sd->assetSize("a.bin") == 10 && sd->loadAssetPartial("a.bin", 3, 4, v) && v.size() == 4 && v[0] == '3');
    CHECK(sd->loadAssetPartial("a.bin", 8, 100, v) && v.size() == 2);
    CHECK(!sd->loadAsset("yok.bin", v));
    CHECK(sd->saveFileObfuscated("s.sav", (const uint8_t*)msg, strlen(msg)));
    CHECK(sd->loadFileObfuscated("s.sav", v) && v.size() == strlen(msg) && !memcmp(v.data(), msg, v.size()));
    CHECK(sd->checkLicense());
    delete sd;
    // girdi
    InputDevice* in = InputDevice::create();
    in->touchBegan(5, .25f, .5f, 1.0); in->touchBegan(9, .75f, .5f, 1.0);
    CHECK(in->numTouches() == 2);
    in->touchMoved(5, .3f, .5f, 1.1);
    float x, y; CHECK(in->getTouch(x, y) && x == .3f);
    in->updateFrameEnd();
    CHECK(in->touchById(5)->prevX == .3f && !(in->touchById(5)->flags & 1));
    in->touchEnded(5, .3f, .5f, 1.2);
    CHECK(in->getTouchUp(x, y));
    in->updateFrameEnd();
    CHECK(in->numTouches() == 1 && in->touch(0)->id == 9);
    in->padButton(BTN_A, true); CHECK(in->buttonsPressed() & BTN_A);
    in->updateFrameEnd(); CHECK(!(in->buttonsPressed() & BTN_A) && (in->buttonsHeld() & BTN_A));
    delete in;
    printf("platform_test OK\n");
    return 0;
}
