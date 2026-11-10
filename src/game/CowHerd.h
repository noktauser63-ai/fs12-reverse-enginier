// CowHerd: ahirdaki 7 ineğin (Cows::initRenderCow 0..6) yerlesimi ve durus/uyku secimi.
// Saf mantik (GL yok). Cizim: WorldView::setCowHerd. Modeller: objects/cowStanding.p3d, cowSleeping.p3d (FS12 bicimine cevrilmis).
// NOT: FS14'te inekler COW_ANIM sahder'i ile hareketli; burada sabit durus (ilk surum).
#pragma once
namespace fs12 {

struct CowSlot { float x, z, yaw; bool sleeping; int skin; };   // skin 0: cowStanding_diffuse, 1: cowStanding02_diffuse

class CowHerd {
public:
    static constexpr int kCount = 7;
    // merkez (x,y,z) ve ahirin yonu (radyan); inekler 2 sira halinde dizilir.
    void setPlacement(float x, float y, float z, float yaw);
    void update(float hourOfDay);          // gece bir kismi uyur
    const CowSlot* slots() const { return m_slot; }
    float y() const { return m_y; }
    static bool isNight(float hour) { return hour >= 21.f || hour < 5.f; }
private:
    CowSlot m_slot[kCount] = {};
    float m_cx = 0, m_y = 0, m_cz = 0, m_yaw = 0;
    float m_off[kCount][2] = {}, m_yawJit[kCount] = {};
};

} // namespace fs12
