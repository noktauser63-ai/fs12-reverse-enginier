// CryptoUtil: standart RC4 + CRC32 (orijinal CryptoUtil::rc4_ksa/rc4_prga ve
// AndroidHandheldSystemDevice::getCRC32 ile ayni davranis).
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace fs12 {

struct Rc4State { uint8_t s[256]; uint8_t i = 0, j = 0; };

inline void rc4Ksa(Rc4State& st, const uint8_t* key, size_t keyLen) {
    for (int k = 0; k < 256; ++k) st.s[k] = (uint8_t)k;
    st.i = st.j = 0;
    uint8_t j = 0;
    for (int k = 0; k < 256; ++k) {
        j = (uint8_t)(j + st.s[k] + (keyLen ? key[k % keyLen] : 0));
        uint8_t t = st.s[k]; st.s[k] = st.s[j]; st.s[j] = t;
    }
}
inline void rc4Prga(Rc4State& st, uint8_t* out, size_t n) {
    for (size_t k = 0; k < n; ++k) {
        st.i = (uint8_t)(st.i + 1);
        st.j = (uint8_t)(st.j + st.s[st.i]);
        uint8_t t = st.s[st.i]; st.s[st.i] = st.s[st.j]; st.s[st.j] = t;
        out[k] = st.s[(uint8_t)(st.s[st.i] + st.s[st.j])];
    }
}

uint32_t crc32(const uint8_t* data, size_t n, uint32_t crc = 0);

// Kayit dosyasi gizleme: [16 bayt rastgele on ek][veri XOR RC4(anahtar = deviceID + onEk)]
std::vector<uint8_t> obfuscate(const std::string& deviceId, const uint8_t* data, size_t n,
                               const uint8_t prefix[16]);
// false: dosya 16 bayttan kisa
bool deobfuscate(const std::string& deviceId, const uint8_t* data, size_t n,
                 std::vector<uint8_t>& out);

} // namespace fs12
