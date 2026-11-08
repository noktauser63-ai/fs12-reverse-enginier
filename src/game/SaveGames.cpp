#include "game/SaveGames.h"
#include "platform/CryptoUtil.h"
#include "platform/SystemDevice.h"
#include <vector>

namespace fs12 {
static const char* kMaster = "master.dat";
static const char* kSlotFile[3] = {"save01.dat", "save02.dat", "save03.dat"};
static const char* kTemp = "tempSave.dat";

SaveGames::SaveGames() {}

static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }
static void wr32(uint8_t* p, uint32_t v) { memcpy(p, &v, 4); }

// Ana dosya (0x5c bayt): [0]=surum 0x26, [4]=crc32, 3 x 0x18 yuva (+8: kullanildi(1B), +0xc surum, +0x10 crc, +0x14 para/saat/dakika),
// [0x50]=gecici yuva, [0x54]=gecici crc, [0x58]=gecici surum.
void SaveGames::load(SystemDevice* dev, AppSaveGameCallbackInterface* cb) {
    m_dev = dev; m_cb = cb;
    std::vector<uint8_t> buf;
    if (!dev->loadFile(kMaster, buf, true) || buf.empty()) return;
    if (buf.size() == 0x4c) {          // eski bicim (0x24/0x25)
        uint32_t ver = rd32(buf.data());
        if (ver - 0x24u < 2) {
            uint32_t want = rd32(buf.data() + 4);
            wr32(buf.data() + 4, 0);
            uint32_t got = crc32(buf.data(), 0x4c);
            if (want != got) return;
            m_version = kVersion; 
            for (int i = 0; i < 3; ++i) {
                const uint8_t* p = buf.data() + 8 + i * 20;
                m_slot[i].used = p[0];
                m_slot[i].crc = rd32(buf.data() + 8 + i * 20 + 4);
                m_slot[i].version = ver;
                memcpy(&m_slot[i].data, buf.data() + 16 + i * 20, 12);
            }
            m_tmpSlot = (int32_t)rd32(buf.data() + 0x44);
            m_tmpCrc = rd32(buf.data() + 0x48);
            m_tmpVersion = ver;
        }
    } else if (buf.size() == 0x5c && rd32(buf.data()) == kVersion) {
        uint32_t want = rd32(buf.data() + 4);
        wr32(buf.data() + 4, 0);
        if (crc32(buf.data(), 0x5c) != want) return;
        m_version = rd32(buf.data());
        for (int i = 0; i < 3; ++i) {
            const uint8_t* p = buf.data() + 8 + i * 0x18;
            m_slot[i].used = p[0]; m_slot[i].version = rd32(p + 4); m_slot[i].crc = rd32(p + 8);
            m_slot[i].data.money = rd32(p + 12); m_slot[i].data.hours = rd32(p + 16); m_slot[i].data.minutes = rd32(p + 20);
        }
        m_tmpSlot = (int32_t)rd32(buf.data() + 0x50); m_tmpCrc = rd32(buf.data() + 0x54); m_tmpVersion = rd32(buf.data() + 0x58);
    }
}

void SaveGames::writeMaster() {
    uint8_t b[0x5c];
    memset(b, 0, sizeof b);
    wr32(b, kVersion);
    for (int i = 0; i < 3; ++i) {
        uint8_t* p = b + 8 + i * 0x18;
        p[0] = m_slot[i].used; wr32(p + 4, m_slot[i].version); wr32(p + 8, m_slot[i].crc);
        wr32(p + 12, m_slot[i].data.money); wr32(p + 16, m_slot[i].data.hours); wr32(p + 20, m_slot[i].data.minutes);
    }
    wr32(b + 0x50, (uint32_t)m_tmpSlot); wr32(b + 0x54, m_tmpCrc); wr32(b + 0x58, m_tmpVersion);
    wr32(b + 4, crc32(b, 0x5c));
    m_dev->saveFile(kMaster, b, sizeof b);
}

bool SaveGames::loadSavegame(const char* file, uint32_t crc, uint32_t version) {
    std::vector<uint8_t> buf;
    if (!m_dev->loadFile(file, buf, true) || buf.empty()) return false;
    if (crc32(buf.data(), buf.size()) != crc || version <= 0x26) return false;
    m_cb->deserialize(buf.data(), (uint32_t)buf.size(), false, version);
    return true;
}

void SaveGames::startTask(WorkerTaskCommand cmd, uint32_t slot, bool) {
    m_cmd = cmd; m_cmdSlot = (int)slot;
    processTask();
}

void SaveGames::processTask() {
    m_ok = true;
    const int s = m_cmdSlot;
    switch (m_cmd) {
    case CMD_NEW: m_cb->deserialize(nullptr, 0, true, kSlotVersion); return;
    case CMD_LOAD: m_ok = loadSavegame(kSlotFile[s], m_slot[s].crc, m_slot[s].version); return;
    case CMD_DELETE: m_slot[s].used = 0; break;
    case CMD_SAVE: {
        AppSaveGameSlotData d;
        std::vector<uint8_t> buf(kMaxSave);
        uint32_t n = m_cb->serialize(d, buf.data(), "_ZN11ApplicationD1Ev");
        m_dev->saveFile(kSlotFile[s], buf.data(), n);
        m_slot[s].crc = crc32(buf.data(), n);
        m_slot[s].data = d; m_slot[s].used = 1; m_slot[s].version = kSlotVersion;
        m_tmpSlot = -1; m_tmpCrc = 0;
        break;
    }
    case CMD_LOAD_TEMP:
        m_ok = loadSavegame(kTemp, m_tmpCrc, m_tmpVersion);
        if (!m_ok) { m_tmpCrc = 0; m_tmpSlot = -1; }
        return;
    case CMD_SAVE_TEMP: {
        AppSaveGameSlotData d;
        std::vector<uint8_t> buf(kMaxSave);
        uint32_t n = m_cb->serialize(d, buf.data(), "_ZN11ApplicationD1Ev");
        m_dev->saveFile(kTemp, buf.data(), n);
        m_tmpCrc = crc32(buf.data(), n); m_tmpSlot = s; m_tmpVersion = kSlotVersion;
        break;
    }
    case CMD_CLEAR_TEMP: m_tmpCrc = 0; m_tmpSlot = -1; break;
    default: return;   // 7: arsiv bicimlendirme (Android'de yok)
    }
    writeMaster();
}
} // namespace fs12
