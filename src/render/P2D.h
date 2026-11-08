// .p2d texture container (orijinal Texture::load / GLESHandheldRenderDevice::createTexture).
// Baslik 20 bayt (5 x u32 LE): [0]=format [1]=genislik [2]=yukseklik [3]=ek mip sayisi [4]=bayraklar
// Ardindan mip0..mipN ham veri (en buyukten kucuge, aralarinda dolgu yok).
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace fs12 {

enum P2DFormat {
    P2D_RGBA8 = 0,      // (varsayilan) 32 bpp
    P2D_RGB8 = 1, P2D_RGBA5551 = 2, P2D_RGBA4444 = 3, P2D_RGB565 = 4,
    P2D_LA8 = 5, P2D_L8 = 6, P2D_A8 = 7,
    P2D_PVRTC_RGB4 = 8, P2D_PVRTC_RGBA4 = 9, P2D_PVRTC_RGB2 = 10, P2D_PVRTC_RGBA2 = 11,
    P2D_ETC1 = 12, P2D_ETC1_B = 13
};

struct P2DInfo {
    uint32_t format = 0, width = 0, height = 0, mipExtra = 0, flags = 0;
    uint32_t padBytes = 0;  // son mip 1x1 dosyada kisa ise gereken sifir dolgu (yukleyici ekler)
    uint32_t levels() const { return mipExtra + 1; }
    bool compressed() const { return format >= P2D_PVRTC_RGB4 && format <= P2D_ETC1_B; }
    int bitsPerPixel() const;
    // 4 = repeat-modu bayragi vs 8 = (orijinalde TEXTURE_LOCATION 4/2/8)
    uint32_t location() const { return (flags & 4) ? 4 : ((flags & 8) ? 8 : 2); }
};

struct P2DLevel { uint32_t w, h; size_t offset, size; };

// false: baslik gecersiz / veri kisa
bool p2dParse(const uint8_t* data, size_t size, P2DInfo& info, std::vector<P2DLevel>& levels);

// ETC1 -> RGBA8 (w*h*4 bayt). Yazilim yedegi (GL_OES_compressed_ETC1_RGB8_texture yoksa).
void etc1DecodeRGBA(const uint8_t* blocks, uint32_t w, uint32_t h, uint8_t* outRGBA);

} // namespace fs12
