// SystemDevice: AndroidHandheldSystemDevice'in yeniden yazimi.
// Varlik okuma (APK assets), kayit dosyalari, log, dil, thread/semafor, zaman.
#pragma once
#include <cstdint>
#include <cstdarg>
#include <string>
#include <vector>
#include <functional>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

namespace fs12 {

enum Language { LANG_EN = 0, LANG_DE, LANG_FR, LANG_JA, LANG_ZH, LANG_RU, LANG_ES, LANG_IT, LANG_PT, LANG_KO, LANG_TR };

// "en","de",... -> orijinal tablo (en0 de1 fr2 ja3 zh4 ru5 es6 it7 pt8 ko9). Bilinmeyen: en.
int languageFromCode(const char* code);

class SystemDevice {
public:
    virtual ~SystemDevice() {}
    static SystemDevice* instance();
    static void setInstance(SystemDevice* d);

    // Salt-okunur oyun varliklari (assets/)
    virtual bool loadAsset(const char* name, std::vector<uint8_t>& out) = 0;
    virtual bool loadAssetPartial(const char* name, uint32_t off, uint32_t len, std::vector<uint8_t>& out) = 0;
    virtual int64_t assetSize(const char* name) = 0;

    // Yazilabilir dosyalar (kayit oyunlari) - filesDir altinda
    virtual bool loadSave(const char* name, std::vector<uint8_t>& out) = 0;
    virtual bool saveFile(const char* name, const uint8_t* data, size_t n) = 0;
    bool saveFileObfuscated(const char* name, const uint8_t* data, size_t n);
    bool loadFileObfuscated(const char* name, std::vector<uint8_t>& out);

    // loadFile(name, ..., fromAssets): orijinal imza ile ayni mantik
    bool loadFile(const char* name, std::vector<uint8_t>& out, bool fromSaves) {
        return fromSaves ? loadSave(name, out) : loadAsset(name, out);
    }

    virtual std::string deviceId() { return "fs12-local-device"; }
    virtual std::string languageCode() { return "en"; }
    int language() { return languageFromCode(languageCode().c_str()); }
    virtual void log(const char* fmt, va_list ap) = 0;
    void logf(const char* fmt, ...);
    virtual bool openURL(const char*) { return false; }

    // Lisans/yedek: kaldirildi
    bool checkLicense() { return true; }
    bool newVersionAvailable() { return false; }
    bool lowMemoryDevice() { return false; }
    void requestCloudBackup() {}
    void requestCloudRestore() {}

    static int64_t microseconds();   // monotonik, getCurrentMicroSeconds
};

// Ana makine / test uygulamasi (dosya sistemi tabanli)
SystemDevice* createHostSystemDevice(const std::string& assetDir, const std::string& filesDir);

#ifdef __ANDROID__
SystemDevice* createAndroidSystemDevice(AAssetManager* mgr, const std::string& filesDir,
                                        const std::string& deviceId, const std::string& lang);
#endif

} // namespace fs12
