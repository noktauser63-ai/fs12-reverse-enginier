// p2d_dump <in.p2d> <out.ppm>: ilk mip'i PPM olarak yazar (gorsel dogrulama icin)
#include "render/P2D.h"
#include <cstdio>
#include <vector>
using namespace fs12;
int main(int c, char** v) {
    if (c < 3) return 2;
    FILE* f = fopen(v[1], "rb"); std::vector<uint8_t> d; int ch; while ((ch = fgetc(f)) != EOF) d.push_back(ch); fclose(f);
    P2DInfo in; std::vector<P2DLevel> lv;
    if (!p2dParse(d.data(), d.size(), in, lv)) { printf("parse FAIL\n"); return 1; }
    printf("fmt=%u %ux%u levels=%u flags=%u bytes=%zu\n", in.format, in.width, in.height, in.levels(), in.flags, d.size());
    std::vector<uint8_t> rgba((size_t)in.width * in.height * 4, 255);
    const uint8_t* p = d.data() + lv[0].offset;
    size_t N = (size_t)in.width * in.height;
    switch (in.format) {
    case P2D_ETC1: case P2D_ETC1_B: etc1DecodeRGBA(p, in.width, in.height, rgba.data()); break;
    case P2D_RGBA8: for (size_t i = 0; i < N * 4; ++i) rgba[i] = p[i]; break;
    case P2D_L8: for (size_t i = 0; i < N; ++i) rgba[i*4] = rgba[i*4+1] = rgba[i*4+2] = p[i]; break;
    case P2D_RGB565: for (size_t i = 0; i < N; ++i) { uint16_t q = p[i*2] | (p[i*2+1] << 8);
        rgba[i*4] = ((q >> 11) & 31) * 255 / 31; rgba[i*4+1] = ((q >> 5) & 63) * 255 / 63; rgba[i*4+2] = (q & 31) * 255 / 31; } break;
    default: printf("unsupported preview\n");
    }
    FILE* o = fopen(v[2], "wb"); fprintf(o, "P6\n%u %u\n255\n", in.width, in.height);
    for (size_t i = 0; i < N; ++i) fwrite(&rgba[i*4], 1, 3, o);
    fclose(o);
}
