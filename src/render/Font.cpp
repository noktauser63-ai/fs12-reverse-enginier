#include "Font.h"

namespace fs12 {

Font::Font(const FontInfo* t, int n, int tw, int th) : m_t(t), m_n(n), m_tw((float)tw), m_th((float)th) {
    for (int i = 0; i < n; ++i) m_map[t[i].cp] = i;
}

void Font::decodeUtf8(const char* s, std::vector<unsigned>& out) {
    out.clear();
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        unsigned c = *p++;
        int extra = 0;
        if (c < 0x80) extra = 0;
        else if ((c >> 5) == 6) { c &= 0x1f; extra = 1; }
        else if ((c >> 4) == 14) { c &= 0x0f; extra = 2; }
        else if ((c >> 3) == 30) { c &= 0x07; extra = 3; }
        else continue;
        while (extra-- && (*p & 0xc0) == 0x80) c = (c << 6) | (*p++ & 0x3f);
        out.push_back(c);
    }
}

int Font::glyphIndex(unsigned cp) const {
    auto it = m_map.find(cp);
    return it == m_map.end() ? -1 : it->second;
}

// Piksel-kare duzen: yatay olcek = scale * (1.5/aspect), dikey olcek = scale
static inline float sxOf(float scale, float aspect) { return scale * 1.5f / aspect; }

float Font::width(const char* s, float scale, float aspect) const { return widthXY(s, sxOf(scale, aspect)); }

float Font::widthXY(const char* s, float sx, float mono) const {
    std::vector<unsigned> cps; decodeUtf8(s, cps);
    float w = 0;
    for (unsigned c : cps) { int g = glyphIndex(c); if (g >= 0) w += (mono > 0 && c >= '0' && c <= '9') ? mono : m_t[g].advance / kVirtualW * sx; }
    return w;
}

void Font::layout(const char* s, float x, float y, float scale, float aspect, int align,
                  std::vector<GlyphQuad>& out) const {
    layoutXY(s, x, y, sxOf(scale, aspect), scale, align, out);
}

void Font::layoutXY(const char* s, float x, float y, float sx, float sy, int align,
                    std::vector<GlyphQuad>& out, float mono) const {
    std::vector<unsigned> cps; decodeUtf8(s, cps);
    float pen = x;
    if (align == 1) pen -= widthXY(s, sx, mono) * 0.5f;
    else if (align == 2) pen -= widthXY(s, sx, mono);
    for (unsigned c : cps) {
        int g = glyphIndex(c);
        if (g < 0) continue;
        const FontInfo& f = m_t[g];
        GlyphQuad q;
        q.x = pen + f.xoff / kVirtualW * sx;
        q.y = y + f.yoff / kVirtualH * sy;
        q.w = f.w / kVirtualW * sx;
        q.h = f.h / kVirtualH * sy;
        q.u = f.x / m_tw; q.v = f.y / m_th; q.du = f.w / m_tw; q.dv = f.h / m_th;
        if (f.w > 0 && f.h > 0) out.push_back(q);
        pen += (mono > 0 && c >= '0' && c <= '9') ? mono : f.advance / kVirtualW * sx;
    }
}

} // namespace fs12
