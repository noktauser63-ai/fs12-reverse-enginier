// Font: orijinal font03 (2048x2048 L8 atlas + 1171 glif) ile metin yerlesimi ve cizimi.
// Koordinatlar "480x320 sanal ekran" pikseli olarak olculur; scale=1 -> atlastaki orijinal boyut.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "data/Font.h"

namespace fs12 {

struct GlyphQuad { float x, y, w, h, u, v, du, dv; };   // x,y,w,h ekran orani (0..1), uv doku orani

class Font {
public:
    Font(const FontInfo* table, int count, int texW, int texH);
    // UTF-8 -> kod noktalari
    static void decodeUtf8(const char* s, std::vector<unsigned>& out);
    int glyphIndex(unsigned cp) const;
    // bir satirin genisligi (ekran orani, yatay)
    float width(const char* utf8, float scale, float aspect) const;
    // satiri yerlestirir; x,y = sol-ust (ekran orani). align: 0 sol, 1 orta, 2 sag
    void layout(const char* utf8, float x, float y, float scale, float aspect, int align,
                std::vector<GlyphQuad>& out) const;
    // Orijinal renderText: sx = (boyut/64)*aspectX, sy = (boyut/64)*aspectY (ayri yatay/dikey olcek)
    float widthXY(const char* utf8, float sx, float mono = 0.0f) const;
    void layoutXY(const char* utf8, float x, float y, float sx, float sy, int align, std::vector<GlyphQuad>& out, float mono = 0.0f) const;
    float lineHeight(float scale) const { return scale * 30.f / 320.f; }
    static const int kVirtualW = 480, kVirtualH = 320;
private:
    const FontInfo* m_t; int m_n; float m_tw, m_th;
    std::unordered_map<unsigned, int> m_map;
};

} // namespace fs12
