// InputDevice: AndroidHandheldInputDevice'in yeniden yazimi (dokunmatik + gamepad + egim).
// Dokunma tablosu orijinaldeki gibi kare sonunda (updateFrameEnd) temizlenir.
#pragma once
#include <cstdint>

namespace fs12 {

// Gamepad dugme maskeleri (orijinal AndroidActivity tablosundan: 0x10/0x20 = dpad yatay, 0x40/0x80 = tetikler)
enum PadButton : uint32_t {
    BTN_A = 0x1, BTN_B = 0x2, BTN_X = 0x4, BTN_Y = 0x8,
    BTN_LEFT = 0x10, BTN_RIGHT = 0x20, BTN_L2 = 0x40, BTN_R2 = 0x80
};

class InputDevice {
public:
    struct Touch {
        uint64_t id = 0;
        float x = 0, y = 0;          // 0..1 (ekran orani)
        float prevX = 0, prevY = 0;  // bir onceki kare
        float startX = 0, startY = 0;
        double time = 0;             // saniye
        double prevTime = 0;         // onceki karedeki zaman (orijinal +0x30)
        uint32_t flags = 0;          // 1 = yeni basildi, 2 = bitti (bu karede), 4 = hareket etti
        bool gestureDisabled = false;
    };
    static const int kMaxTouches = 16;

    static InputDevice* create();
    static InputDevice* instance();

    // olay girisi (platform katmani cagirir)
    void touchBegan(uint64_t id, float x, float y, double t);
    void touchMoved(uint64_t id, float x, float y, double t);
    void touchEnded(uint64_t id, float x, float y, double t, bool cancelled = false);
    void touchCancelled(uint64_t id) { touchEnded(id, 0, 0, 0, true); }
    void backKeyPressed(bool down) { m_back = down; if (down) m_backEdge = true; }
    void updateAccelerometer(float x, float y, float z, bool landscapeFlip);
    void padAxes(float lx, float ly, float rx, float ry) { m_lx = lx; m_ly = ly; m_rx = rx; m_ry = ry; }
    void padButton(uint32_t mask, bool down);

    // sorgular
    int numTouches() const { return m_n; }
    const Touch* touch(int i) const { return (i >= 0 && i < m_n) ? &m_t[i] : nullptr; }
    const Touch* touchById(uint64_t id) const;
    int touchIndexById(uint64_t id) const;
    bool getTouch(float& x, float& y) const;      // ilk aktif dokunma
    bool getTouchUp(float& x, float& y) const;    // bu karede biten ilk dokunma
    bool backKeyPressed() const { return m_back; }
    bool backKeyEdge() const { return m_backEdge; }
    void getStick(float& x, float& y, bool left) const;
    uint32_t buttonsPressed() const { return m_pressed; }
    uint32_t buttonsHeld() const { return m_held; }
    uint32_t buttonsReleased() const { return m_released; }
    void getAccel(float& x, float& y, float& z) const { x = m_ax; y = m_ay; z = m_az; }
    void disableTouchForGestures(unsigned i) { if ((int)i < m_n) m_t[i].gestureDisabled = true; }

    // kaydirma yonu: -1/0/+1 (esik: 0.1)
    static int swipeX(float dx, float threshold = 0.1f) { return dx > threshold ? 1 : dx < -threshold ? -1 : 0; }

    void debugSetTouches(const Touch* t, int n) { m_n = n > kMaxTouches ? kMaxTouches : n; for (int i = 0; i < m_n; ++i) m_t[i] = t[i]; }  // testler icin
    void updateFrameEnd();

private:
    Touch m_t[kMaxTouches];
    int m_n = 0;
    bool m_back = false, m_backEdge = false;
    float m_ax = 0, m_ay = 0, m_az = 0;
    float m_lx = 0, m_ly = 0, m_rx = 0, m_ry = 0;
    uint32_t m_pressed = 0, m_held = 0, m_released = 0;
};

} // namespace fs12
