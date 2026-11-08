// SaveGames: orijinal SaveGames sinifinin yeniden yazimi (ana kayit dosyasi + 3 yuva + gecici kayit).
// Orijinalde isci is parcacigi vardi; burada gorevler eszamanli calisir (isTaskPending hep false doner).
#pragma once
#include <cstdint>
#include <cstring>

namespace fs12 {
class SystemDevice;

struct AppSaveGameSlotData { uint32_t money = 0, hours = 0, minutes = 0; };

class SaveGames {
public:
    enum WorkerTaskCommand { CMD_NEW = 0, CMD_LOAD = 1, CMD_DELETE = 2, CMD_SAVE = 3, CMD_LOAD_TEMP = 4, CMD_SAVE_TEMP = 5,
                             CMD_CLEAR_TEMP = 6, CMD_FORMAT = 7 };
    struct AppSaveGameCallbackInterface {
        virtual ~AppSaveGameCallbackInterface() {}
        // vtbl+8: oyunu serilestirir, boyutu doner
        virtual uint32_t serialize(AppSaveGameSlotData& slot, uint8_t* buf, const char* tag) = 0;
        // vtbl+0xc: kayittan yukler; isNew=true -> yeni oyun
        virtual void deserialize(const uint8_t* buf, uint32_t size, bool isNew, uint32_t version) = 0;
    };
    static const uint32_t kVersion = 0x26, kSlotVersion = 0x27, kMaxSave = 0x22536;

    SaveGames();
    void load(SystemDevice* dev, AppSaveGameCallbackInterface* cb);
    void destroy() {}
    void startTask(WorkerTaskCommand cmd, uint32_t slot, bool sync = true);
    bool isTaskPending() const { return false; }
    bool wasLastTaskSuccessful() const { return m_ok; }
    // yuva bilgisi (ana kayit dosyasindan)
    bool slotUsed(int i) const { return m_slot[i].used != 0; }
    const AppSaveGameSlotData& slotData(int i) const { return m_slot[i].data; }
    bool hasTemp() const { return m_tmpSlot != -1; }

private:
    struct Slot { uint8_t used = 0; uint32_t version = kSlotVersion, crc = 0; AppSaveGameSlotData data; };
    void processTask();
    bool loadSavegame(const char* file, uint32_t crc, uint32_t version);
    void writeMaster();
    uint32_t m_version = kVersion;   // +0
    Slot m_slot[3];
    int32_t m_tmpSlot = -1;          // +0x50
    uint32_t m_tmpCrc = 0;           // +0x54
    uint32_t m_tmpVersion = kSlotVersion;  // +0x58
    SystemDevice* m_dev = nullptr;
    AppSaveGameCallbackInterface* m_cb = nullptr;
    int m_cmd = 0; int m_cmdSlot = 0; bool m_ok = true;
};
} // namespace fs12
