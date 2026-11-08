#include "p1d.h"

#include <string.h>

namespace fs12 {
namespace audio {

static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

bool p1dParse(const uint8_t* data, size_t size, P1D& out) {
    if (!data || size < 8) return false;
    uint32_t type = rd32(data);
    uint32_t hdr = rd32(data + 4);
    if ((size_t)hdr + 8 > size) return false;
    out.type = type;
    out.payload = data + 8 + hdr;
    out.payloadSize = size - 8 - hdr;
    return type == P1D_WAV || type == P1D_OGG;
}

static const int kIndexTable[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const int kStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,    31,    34,
    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,   130,   143,   157,   173,
    190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,
    963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,
    4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
    24623, 27086, 29794, 32767};

struct ImaState {
    int pred;
    int index;
};

static inline int imaStep(ImaState& s, int nibble) {
    int step = kStepTable[s.index];
    // ffmpeg/libsndfile ile birebir ayni (yuvarlama): ((2*mag+1)*step)>>3
    int diff = ((2 * (nibble & 7) + 1) * step) >> 3;
    if (nibble & 8) diff = -diff;
    s.pred += diff;
    if (s.pred > 32767) s.pred = 32767;
    if (s.pred < -32768) s.pred = -32768;
    s.index += kIndexTable[nibble];
    if (s.index < 0) s.index = 0;
    if (s.index > 88) s.index = 88;
    return s.pred;
}

// MS IMA ADPCM (wFormatTag 0x11). Blok basligi: kanal basina int16 predictor, u8 index, u8 reserved.
// Mono: byte basina 2 ornek (dusuk nibble once). Stereo: kanal basina 4 byte'lik gruplar halinde dolasik.
static bool decodeImaAdpcm(const uint8_t* d, size_t n, int ch, int blockAlign, int samplesPerBlock, Pcm& out) {
    if (ch < 1 || ch > 2 || blockAlign < 4 * ch || samplesPerBlock < 1) return false;
    size_t blocks = n / blockAlign;
    size_t tail = n % blockAlign;
    out.samples.reserve((blocks + 1) * (size_t)samplesPerBlock * ch);
    for (size_t b = 0; b < blocks + (tail >= (size_t)4 * ch ? 1 : 0); ++b) {
        const uint8_t* blk = d + b * blockAlign;
        size_t blkLen = (b < blocks) ? (size_t)blockAlign : tail;
        ImaState st[2];
        std::vector<int16_t> tmp[2];
        for (int c = 0; c < ch; ++c) {
            st[c].pred = (int16_t)rd16(blk + c * 4);
            st[c].index = blk[c * 4 + 2];
            if (st[c].index > 88) st[c].index = 88;
            tmp[c].push_back((int16_t)st[c].pred);
        }
        size_t pos = 4 * ch;
        while (pos + 4 * ch <= blkLen && (int)tmp[0].size() < samplesPerBlock) {
            for (int c = 0; c < ch; ++c) {
                for (int i = 0; i < 4; ++i) {
                    uint8_t byte = blk[pos + c * 4 + i];
                    tmp[c].push_back((int16_t)imaStep(st[c], byte & 0xF));
                    tmp[c].push_back((int16_t)imaStep(st[c], byte >> 4));
                }
            }
            pos += 4 * ch;
        }
        size_t cnt = tmp[0].size();
        if ((int)cnt > samplesPerBlock) cnt = samplesPerBlock;
        for (size_t i = 0; i < cnt; ++i)
            for (int c = 0; c < ch; ++c) out.samples.push_back(tmp[c][i]);
    }
    return !out.samples.empty();
}

bool wavDecode(const uint8_t* d, size_t n, Pcm& out) {
    if (!d || n < 12 || memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) return false;
    size_t p = 12;
    int fmtTag = 0, ch = 0, rate = 0, blockAlign = 0, bits = 0, spb = 0;
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
    bool haveFmt = false;
    while (p + 8 <= n) {
        uint32_t len = rd32(d + p + 4);
        const uint8_t* body = d + p + 8;
        size_t avail = n - (p + 8);
        size_t clen = len < avail ? len : avail;
        if (memcmp(d + p, "fmt ", 4) == 0 && clen >= 16) {
            fmtTag = rd16(body);
            ch = rd16(body + 2);
            rate = (int)rd32(body + 4);
            blockAlign = rd16(body + 12);
            bits = rd16(body + 14);
            if (fmtTag == 0x11 && clen >= 20) spb = rd16(body + 18);
            haveFmt = true;
        } else if (memcmp(d + p, "data", 4) == 0) {
            data = body;
            dataLen = clen;
            break;
        }
        p += 8 + (size_t)len + (len & 1);
    }
    if (!haveFmt || !data || ch < 1 || ch > 2 || rate <= 0) return false;
    out.channels = ch;
    out.sampleRate = rate;
    out.samples.clear();
    if (fmtTag == 1 && bits == 16) {
        size_t cnt = dataLen / 2;
        cnt -= cnt % ch;
        out.samples.resize(cnt);
        for (size_t i = 0; i < cnt; ++i) out.samples[i] = (int16_t)rd16(data + i * 2);
        return cnt > 0;
    }
    if (fmtTag == 1 && bits == 8) {
        size_t cnt = dataLen - dataLen % ch;
        out.samples.resize(cnt);
        for (size_t i = 0; i < cnt; ++i) out.samples[i] = (int16_t)(((int)data[i] - 128) << 8);
        return cnt > 0;
    }
    if (fmtTag == 0x11) return decodeImaAdpcm(data, dataLen, ch, blockAlign, spb, out);
    return false;
}

}  // namespace audio
}  // namespace fs12
