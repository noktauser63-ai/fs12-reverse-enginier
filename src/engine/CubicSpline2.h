// Orijinal: CubicSpline2 — 2B uniform kubik B-spline, yay uzunluguna gore parametrelenir.
// flags: 4 = acik egri (uc noktalar dogrusal uzatilir, t kenetlenir), 8 = kapali dongu (t sarilir).
#pragma once

class CubicSpline2 {
public:
    CubicSpline2() {}
    // points: (x,y) ciftleri, count nokta.
    void load(unsigned int count, const float* points, unsigned int flags);
    void destroy();
    // t in [0,1] (kapali egride sarilir): konum -> x,y
    void getPosition(float t, float& x, float& y) const;
    float totalLength() const { return totalLen_; }

private:
    void init();
    float preCalcSegment(unsigned int seg);
    void evaluate(unsigned int seg, float t, float& x, float& y) const;
    static void getInterval(float v, const float* table, unsigned int count, int& hi, int& lo);

    unsigned int flags_ = 0;
    float* cum_ = nullptr;       // 0x4: kumulatif uzunluk (segCount_ girdi)
    float* ctrl_ = nullptr;      // 0x8: (n+3) nokta, ilk nokta phantom
    unsigned int n_ = 0;         // 0x14
    float totalLen_ = 0.f;       // 0x18
    unsigned int segCount_ = 0;  // 0x1c
    float* arc_ = nullptr;       // 0x20: segment basina 25 kumulatif uzunluk
};
