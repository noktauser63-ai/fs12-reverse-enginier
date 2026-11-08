// GUI yerlesim matematigi (orijinal GenericGUIManager::updateScreenScale / applyScreenAlign / ... ).
// ARM softfp ile ayni sonuc icin islem sirasi korunur; derleme -ffp-contract=off ile yapilir.
#pragma once
#include <cstdint>

namespace fs12 {

struct GuiScale {
    float sx = 1.f, sy = 1.f;   // orijinal this+0x5c6c / 0x5c70
    // dev4 * dev0xc = ekran en/boy orani (orijinal render cihazi alanlari)
    void update(float aspect) {
        float s = aspect * 1.0f;
        if (!(s > 1.5f)) { sx = 1.0f; sy = s / 1.5f; }
        else { sx = 1.5f / s; sy = 1.0f; }
    }

    // Hizalama bayraklari (applyScreenAlign): bit0 X olcekleme yok, bit2 merkez-X, bit3 sag-X,
    //                                          bit4 Y olcekleme yok, bit6 merkez-Y, bit7 alt-Y
    void alignX(float& x, uint32_t a) const {
        if (a & 1) return;
        if (a & 4)      x = (x - 0.5f) * sx + 0.5f;
        else if (a & 8) x = (x - 1.0f) * sx + 1.0f;
        else            x = x * sx;
    }
    void alignY(float& y, uint32_t a) const {
        if (a & 0x10) return;
        if (a & 0x40)      y = (y - 0.5f) * sy + 0.5f;
        else if (a & 0x80) y = (y - 1.0f) * sy + 1.0f;
        else               y = y * sy;
    }
    void align(float& x, float& y, float& w, float& h, uint32_t a) const {
        if (!(a & 1)) { w = w * sx; alignX(x, a); }
        if (!(a & 0x10)) { h = h * sy; alignY(y, a); }
    }
    void undoX(float& x, uint32_t a) const {
        if (a & 1) return;
        if (a & 4)      x = (x - 0.5f) / sx + 0.5f;
        else if (a & 8) x = (x - 1.0f) / sx + 1.0f;
        else            x = x / sx;
    }
    void undoY(float& y, uint32_t a) const {
        if (a & 0x10) return;
        if (a & 0x40)      y = (y - 0.5f) / sy + 0.5f;
        else if (a & 0x80) y = (y - 1.0f) / sy + 1.0f;
        else               y = y / sy;
    }
    void undo(float& x, float& y, uint32_t a) const { undoX(x, a); undoY(y, a); }
};

// FUN_0003e960: sol-ust yerine merkez/sag/alt capa (bit1 merkez-X, bit2 sag-X, bit4 merkez-Y, bit5 alt-Y)
inline void anchorShift(float& x, float& y, float w, float h, uint32_t a) {
    if (a & 2)      { w = w * 0.5f; x = x - w; }
    else if (a & 4) { x = x - w; }
    if (a & 0x10)      { h = h * 0.5f; y = y - h; }
    else if (a & 0x20) { y = y - h; }
}

} // namespace fs12
