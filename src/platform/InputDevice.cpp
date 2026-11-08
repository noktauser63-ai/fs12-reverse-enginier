#include "InputDevice.h"

namespace fs12 {

static InputDevice* g_in = nullptr;
InputDevice* InputDevice::create() { if (!g_in) g_in = new InputDevice(); return g_in; }
InputDevice* InputDevice::instance() { return g_in; }

int InputDevice::touchIndexById(uint64_t id) const {
    for (int i = 0; i < m_n; ++i) if (m_t[i].id == id) return i;
    return -1;
}
const InputDevice::Touch* InputDevice::touchById(uint64_t id) const {
    int i = touchIndexById(id); return i < 0 ? nullptr : &m_t[i];
}

void InputDevice::touchBegan(uint64_t id, float x, float y, double t) {
    if (touchIndexById(id) >= 0 || m_n >= kMaxTouches) return;
    Touch& k = m_t[m_n++];
    k = Touch();
    k.id = id; k.x = k.prevX = k.startX = x; k.y = k.prevY = k.startY = y;
    k.time = k.prevTime = t; k.flags = 1;
}
void InputDevice::touchMoved(uint64_t id, float x, float y, double t) {
    int i = touchIndexById(id);
    if (i < 0) return;
    Touch& k = m_t[i];
    k.x = x; k.y = y; k.time = t; k.flags |= 4;
}
void InputDevice::touchEnded(uint64_t id, float x, float y, double t, bool cancelled) {
    int i = touchIndexById(id);
    if (i < 0) return;
    Touch& k = m_t[i];
    if (!cancelled) { k.x = x; k.y = y; k.time = t; }
    k.flags |= 2;
}

bool InputDevice::getTouch(float& x, float& y) const {
    for (int i = 0; i < m_n; ++i)
        if (!(m_t[i].flags & 2)) { x = m_t[i].x; y = m_t[i].y; return true; }
    return false;
}
bool InputDevice::getTouchUp(float& x, float& y) const {
    for (int i = 0; i < m_n; ++i)
        if (m_t[i].flags & 2) { x = m_t[i].x; y = m_t[i].y; return true; }
    return false;
}

void InputDevice::getStick(float& x, float& y, bool left) const {
    x = left ? m_lx : m_rx;
    y = left ? m_ly : m_ry;
}

void InputDevice::updateAccelerometer(float x, float y, float z, bool flip) {
    m_ax = flip ? -x : x; m_ay = flip ? -y : y; m_az = z;
}

void InputDevice::padButton(uint32_t mask, bool down) {
    if (down) {
        if (!(m_held & mask)) m_pressed |= mask;
        m_held |= mask; m_released &= ~mask;
    } else {
        m_held &= ~mask; m_pressed &= ~mask; m_released |= mask;
    }
}

void InputDevice::updateFrameEnd() {
    // biten dokunmalari at, kalanlarin onceki konumunu guncelle
    int w = 0;
    for (int i = 0; i < m_n; ++i) {
        if (m_t[i].flags & 2) continue;
        Touch k = m_t[i];
        k.flags &= ~(1u | 4u);
        k.prevX = k.x; k.prevY = k.y; k.prevTime = k.time;
        m_t[w++] = k;
    }
    m_n = w;
    m_backEdge = false;
    m_pressed = 0; m_released = 0;
}

} // namespace fs12
