#include "render/Font.h"
#include "data/Strings.h"
#include <cstdio>
#include <cstring>
using namespace fs12;
#define CHECK(c) do{ if(!(c)){ printf("FAIL %d %s\n",__LINE__,#c); return 1; } }while(0)
int main() {
    Font f(kFontData03, kFontData03Count, 2048, 2048);
    CHECK(kStringCount == 312);
    // tum ingilizce dizeler icin her karakterin glifi olmali (bos glif haric)
    int missing = 0, total = 0;
    for (int i = 0; i < kStringCount; ++i)
        for (int l = 0; l < 10; ++l) {
            std::vector<unsigned> cps; Font::decodeUtf8(kStrings[i][l], cps);
            for (unsigned c : cps) { ++total; if (f.glyphIndex(c) < 0 && c != '\n') { ++missing; if (missing < 8) printf("eksik glif U+%04X (dize %d dil %d)\n", c, i, l); } }
        }
    printf("karakter=%d eksik=%d\n", total, missing);
    CHECK(missing == 0);
    float w = f.width("New Game", 1.f, 1.5f);
    printf("genislik('New Game')=%.4f\n", w);
    CHECK(w > 0.05f && w < 0.6f);
    std::vector<GlyphQuad> q; f.layout("Load", 0.5f, 0.5f, 1.f, 1.5f, 1, q);
    CHECK(q.size() == 4 && q[0].x < 0.5f);
    printf("font_test OK\n");
}
