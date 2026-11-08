#include "CryptoUtil.h"
#include <cstring>

namespace fs12 {

uint32_t crc32(const uint8_t* data, size_t n, uint32_t crc) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < n; ++i) crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return ~crc;
}

static void makeKey(const std::string& id, const uint8_t* prefix, std::vector<uint8_t>& key) {
    key.assign(id.begin(), id.end());
    key.insert(key.end(), prefix, prefix + 16);
}

std::vector<uint8_t> obfuscate(const std::string& deviceId, const uint8_t* data, size_t n,
                               const uint8_t prefix[16]) {
    std::vector<uint8_t> key; makeKey(deviceId, prefix, key);
    Rc4State st; rc4Ksa(st, key.data(), key.size());
    std::vector<uint8_t> ks(n);
    if (n) rc4Prga(st, ks.data(), n);
    std::vector<uint8_t> out(16 + n);
    memcpy(out.data(), prefix, 16);
    for (size_t i = 0; i < n; ++i) out[16 + i] = data[i] ^ ks[i];
    return out;
}

bool deobfuscate(const std::string& deviceId, const uint8_t* data, size_t n,
                 std::vector<uint8_t>& out) {
    if (n < 16) return false;
    std::vector<uint8_t> key; makeKey(deviceId, data, key);
    Rc4State st; rc4Ksa(st, key.data(), key.size());
    size_t m = n - 16;
    std::vector<uint8_t> ks(m);
    if (m) rc4Prga(st, ks.data(), m);
    out.resize(m);
    for (size_t i = 0; i < m; ++i) out[i] = data[16 + i] ^ ks[i];
    return true;
}

} // namespace fs12
