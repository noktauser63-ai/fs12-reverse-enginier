#pragma once
namespace fs12 {
// Orijinal GLESHandheldRenderDevice::FontInfo (32 bayt): atlas (2048x2048 L8) icinde piksel degerleri
struct FontInfo { unsigned cp; float x, y, w, h, xoff, yoff, advance; };
extern const int kFontData03Count;
extern const FontInfo kFontData03[];
extern const int kFontTrCount;            // Turkce ek glifler (tools/patch_font_tr.py)
extern const FontInfo kFontTr[];
}
