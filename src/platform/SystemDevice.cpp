#include "SystemDevice.h"
#include "CryptoUtil.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <random>
#include <sys/stat.h>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/log.h>
#endif

namespace fs12 {

static SystemDevice* g_sys = nullptr;
SystemDevice* SystemDevice::instance() { return g_sys; }
void SystemDevice::setInstance(SystemDevice* d) { g_sys = d; }

int languageFromCode(const char* c) {
    static const char* t[] = {"en","de","fr","ja","zh","ru","es","it","pt","ko","tr"};
    if (c) for (int i = 0; i < 11; ++i) if (c[0] == t[i][0] && c[1] == t[i][1]) return i;
    return LANG_EN;
}

int64_t SystemDevice::microseconds() {
    timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void SystemDevice::logf(const char* fmt, ...) { va_list ap; va_start(ap, fmt); log(fmt, ap); va_end(ap); }

bool SystemDevice::saveFileObfuscated(const char* name, const uint8_t* data, size_t n) {
    uint8_t prefix[16];
    std::random_device rd;
    for (auto& b : prefix) b = (uint8_t)rd();
    auto buf = obfuscate(deviceId(), data, n, prefix);
    return saveFile(name, buf.data(), buf.size());
}
bool SystemDevice::loadFileObfuscated(const char* name, std::vector<uint8_t>& out) {
    std::vector<uint8_t> raw;
    if (!loadSave(name, raw)) return false;
    return deobfuscate(deviceId(), raw.data(), raw.size(), out);
}

namespace {

bool readFile(const std::string& p, uint32_t off, int64_t len, std::vector<uint8_t>& out) {
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long sz = ftell(f);
    if (off > (uint32_t)sz) { fclose(f); return false; }
    uint32_t n = (len < 0 || (int64_t)off + len > sz) ? (uint32_t)(sz - off) : (uint32_t)len;
    fseek(f, off, SEEK_SET);
    out.resize(n);
    size_t r = n ? fread(out.data(), 1, n, f) : 0;
    fclose(f);
    return r == n;
}
bool writeFile(const std::string& p, const uint8_t* d, size_t n) {
    FILE* f = fopen(p.c_str(), "wb");
    if (!f) return false;
    size_t w = n ? fwrite(d, 1, n, f) : 0;
    fclose(f);
    return w == n;
}

class HostSystem : public SystemDevice {
public:
    HostSystem(std::string a, std::string f) : assets(std::move(a)), files(std::move(f)) {
        mkdir(files.c_str(), 0755);
    }
    bool loadAsset(const char* n, std::vector<uint8_t>& o) override { return readFile(assets + "/" + n, 0, -1, o); }
    bool loadAssetPartial(const char* n, uint32_t off, uint32_t len, std::vector<uint8_t>& o) override {
        return readFile(assets + "/" + n, off, len, o);
    }
    int64_t assetSize(const char* n) override {
        struct stat st; return stat((assets + "/" + n).c_str(), &st) ? -1 : (int64_t)st.st_size;
    }
    bool loadSave(const char* n, std::vector<uint8_t>& o) override { return readFile(files + "/" + n, 0, -1, o); }
    bool saveFile(const char* n, const uint8_t* d, size_t sz) override { return writeFile(files + "/" + n, d, sz); }
    void log(const char* fmt, va_list ap) override { vfprintf(stderr, fmt, ap); fputc('\n', stderr); }
private:
    std::string assets, files;
};

#ifdef __ANDROID__
class AndroidSystem : public SystemDevice {
public:
    AndroidSystem(AAssetManager* m, std::string f, std::string id, std::string l)
        : mgr(m), files(std::move(f)), id(std::move(id)), lang(std::move(l)) { mkdir(files.c_str(), 0755); }
    bool loadAsset(const char* n, std::vector<uint8_t>& o) override {
        AAsset* a = AAssetManager_open(mgr, n, AASSET_MODE_BUFFER);
        if (!a) return false;
        off_t len = AAsset_getLength(a);
        o.resize(len);
        int r = len ? AAsset_read(a, o.data(), len) : 0;
        AAsset_close(a);
        return r == len;
    }
    bool loadAssetPartial(const char* n, uint32_t off, uint32_t len, std::vector<uint8_t>& o) override {
        AAsset* a = AAssetManager_open(mgr, n, AASSET_MODE_RANDOM);
        if (!a) return false;
        off_t total = AAsset_getLength(a);
        if ((off_t)off > total) { AAsset_close(a); return false; }
        if ((off_t)(off + len) > total) len = (uint32_t)(total - off);
        AAsset_seek(a, off, SEEK_SET);
        o.resize(len);
        int r = len ? AAsset_read(a, o.data(), len) : 0;
        AAsset_close(a);
        return r == (int)len;
    }
    int64_t assetSize(const char* n) override {
        AAsset* a = AAssetManager_open(mgr, n, AASSET_MODE_UNKNOWN);
        if (!a) return -1;
        int64_t l = AAsset_getLength(a); AAsset_close(a); return l;
    }
    bool loadSave(const char* n, std::vector<uint8_t>& o) override { return readFile(files + "/" + n, 0, -1, o); }
    bool saveFile(const char* n, const uint8_t* d, size_t sz) override { return writeFile(files + "/" + n, d, sz); }
    std::string deviceId() override { return id; }
    std::string languageCode() override { return lang; }
    void log(const char* fmt, va_list ap) override { __android_log_vprint(ANDROID_LOG_INFO, "NativeLog", fmt, ap); }
private:
    AAssetManager* mgr; std::string files, id, lang;
};
#endif

} // namespace

SystemDevice* createHostSystemDevice(const std::string& a, const std::string& f) { return new HostSystem(a, f); }
#ifdef __ANDROID__
SystemDevice* createAndroidSystemDevice(AAssetManager* m, const std::string& f, const std::string& id, const std::string& l) {
    return new AndroidSystem(m, f, id, l);
}
#endif

} // namespace fs12
