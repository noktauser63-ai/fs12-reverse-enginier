#include "P2D.h"
#include <algorithm>
#include <cstring>

namespace fs12 {

int P2DInfo::bitsPerPixel() const {
    static const int bpp[] = {32, 24, 16, 16, 16, 16, 8, 8, 4, 4, 2, 2, 4, 4};
    return format <= 13 ? bpp[format] : 32;
}

static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

bool p2dParse(const uint8_t* d, size_t n, P2DInfo& in, std::vector<P2DLevel>& lv) {
    if (n < 20) return false;
    in.format = rd32(d); in.width = rd32(d + 4); in.height = rd32(d + 8);
    in.mipExtra = rd32(d + 12); in.flags = rd32(d + 16);
    if (in.width == 0 || in.height == 0 || in.width > 8192 || in.height > 8192 || in.mipExtra > 14) return false;
    lv.clear(); in.padBytes = 0;
    uint32_t w = in.width, h = in.height;
    size_t off = 20;
    const bool pvr = in.format >= P2D_PVRTC_RGB4 && in.format <= P2D_PVRTC_RGBA2;
    for (uint32_t i = 0; i < in.levels(); ++i) {
        // orijinal: size = bpp * w * h / 8 ; sikistirilmislarda en kucuk boyut 4 (w,h icin)
        uint32_t cw = w, ch = h;
        if (in.compressed()) { cw = std::max(cw, 4u); ch = std::max(ch, 4u); }
        size_t sz = (size_t)in.bitsPerPixel() * cw * ch / 8;
        (void)pvr;
        if (off + sz > n) {
            // oyun dosyalarinda son (1x1) mip 7 bayt kisa; orijinal yukleyici tampon sonunu asarak okur
            if (i + 1 == in.levels() && off + sz - n <= 8) in.padBytes = (uint32_t)(off + sz - n);
            else return false;
        }
        lv.push_back({w, h, off, sz});
        off += sz;
        w = std::max(w >> 1, 1u); h = std::max(h >> 1, 1u);
    }
    return true;
}

static inline uint8_t clamp8(int v) { return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v; }

void etc1DecodeRGBA(const uint8_t* blocks, uint32_t w, uint32_t h, uint8_t* out) {
    static const int tbl[8][4] = {{2,8,-2,-8},{5,17,-5,-17},{9,29,-9,-29},{13,42,-13,-42},
                                  {18,60,-18,-60},{24,80,-24,-80},{33,106,-33,-106},{47,183,-47,-183}};
    const uint32_t bw = (w + 3) / 4, bh = (h + 3) / 4;
    for (uint32_t by = 0; by < bh; ++by)
        for (uint32_t bx = 0; bx < bw; ++bx) {
            const uint8_t* b = blocks + ((size_t)by * bw + bx) * 8;
            int base[2][3];
            bool diff = b[3] & 2, flip = b[3] & 1;
            if (diff) {
                for (int c = 0; c < 3; ++c) {
                    int a = b[c] >> 3; int dv = b[c] & 7; if (dv & 4) dv -= 8;
                    int a2 = a + dv; a2 = std::max(0, std::min(31, a2));
                    base[0][c] = (a << 3) | (a >> 2);
                    base[1][c] = (a2 << 3) | (a2 >> 2);
                }
            } else {
                for (int c = 0; c < 3; ++c) {
                    base[0][c] = (b[c] >> 4) * 17;
                    base[1][c] = (b[c] & 15) * 17;
                }
            }
            int cw[2] = {(b[3] >> 5) & 7, (b[3] >> 2) & 7};
            uint32_t bits = ((uint32_t)b[4] << 24) | (b[5] << 16) | (b[6] << 8) | b[7];
            for (int x = 0; x < 4; ++x)
                for (int y = 0; y < 4; ++y) {
                    uint32_t px = bx * 4 + x, py = by * 4 + y;
                    if (px >= w || py >= h) continue;
                    int sub = flip ? (y >= 2) : (x >= 2);
                    int idx = x * 4 + y;
                    int val = (((bits >> (idx + 16)) & 1) << 1) | ((bits >> idx) & 1);
                    int m = tbl[cw[sub]][val];
                    uint8_t* o = out + ((size_t)py * w + px) * 4;
                    o[0] = clamp8(base[sub][0] + m); o[1] = clamp8(base[sub][1] + m);
                    o[2] = clamp8(base[sub][2] + m); o[3] = 255;
                }
        }
}

} // namespace fs12
