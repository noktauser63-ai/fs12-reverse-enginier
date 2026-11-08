#include "CubicSpline2.h"

#include <math.h>
#include <string.h>

void CubicSpline2::load(unsigned int count, const float* points, unsigned int flags) {
    flags_ = flags;
    cum_ = nullptr;
    arc_ = nullptr;
    n_ = count;
    ctrl_ = new float[(count + 3) * 2];
    memset(ctrl_, 0, sizeof(float) * (count + 3) * 2);
    memcpy(ctrl_ + 2, points, (size_t)count * 8);
    init();
}

void CubicSpline2::destroy() {
    delete[] ctrl_;
    delete[] cum_;
    delete[] arc_;
    ctrl_ = nullptr; cum_ = nullptr; arc_ = nullptr;
}

void CubicSpline2::evaluate(unsigned int seg, float t, float& x, float& y) const {
    float b0 = ((((3.0f - t) * t) - 3.0f) * t + 1.0f) / 6.0f;
    float b1 = (((t * 3.0f - 6.0f) * t) * t + 4.0f) / 6.0f;
    float b2 = ((((t * -3.0f) + 3.0f) * t + 3.0f) * t + 1.0f) / 6.0f;
    float b3 = ((t * t) * t) / 6.0f;
    const float* p = ctrl_ + seg * 2;
    x = b0 * p[0];
    y = b0 * p[1];
    x = x + b1 * p[2];
    y = y + b1 * p[3];
    x = x + b2 * p[4];
    y = y + b2 * p[5];
    x = x + b3 * p[6];
    y = y + b3 * p[7];
}

float CubicSpline2::preCalcSegment(unsigned int seg) {
    float px, py, x, y;
    evaluate(seg, 0.0f, px, py);
    arc_[seg * 25] = 0.0f;
    float run = 0.0f;
    for (int i = 1; i < 25; ++i) {
        evaluate(seg, (float)i / 24.0f, x, y);
        float dx = x - px, dy = y - py;
        run = run + sqrtf(dx * dx + dy * dy);
        arc_[seg * 25 + i] = run;
        px = x; py = y;
    }
    return run;
}

void CubicSpline2::init() {
    unsigned int n = n_;
    if (n < 2) {
        for (int c = 0; c < 2; ++c) {
            ctrl_[c] = 0.0f;
            ctrl_[(n + 1) * 2 + c] = 0.0f;
            ctrl_[(n + 2) * 2 + c] = 0.0f;
        }
    } else if (flags_ & 4) {  // acik: uc noktalar dogrusal uzatilir
        for (int c = 0; c < 2; ++c) {
            ctrl_[c] = (ctrl_[2 + c] + ctrl_[2 + c]) - ctrl_[4 + c];
            ctrl_[(n + 1) * 2 + c] = (ctrl_[n * 2 + c] + ctrl_[n * 2 + c]) - ctrl_[(n - 1) * 2 + c];
            ctrl_[(n + 2) * 2 + c] = 0.0f;
        }
    } else {  // kapali: sar
        ctrl_[0] = ctrl_[n * 2];
        ctrl_[(n + 1) * 2] = ctrl_[2];
        ctrl_[(n + 2) * 2] = ctrl_[4];
        ctrl_[1] = ctrl_[n * 2 + 1];
        ctrl_[(n + 1) * 2 + 1] = ctrl_[3];
        ctrl_[(n + 2) * 2 + 1] = ctrl_[5];
    }
    delete[] cum_;
    cum_ = new float[n + 1];
    delete[] arc_;
    arc_ = new float[(n + 1) * 25];
    memset(arc_, 0, sizeof(float) * (n + 1) * 25);
    cum_[0] = 0.0f;
    for (unsigned int s = 1; s < n; ++s) cum_[s] = cum_[s - 1] + preCalcSegment(s - 1);
    if (flags_ & 4) {
        totalLen_ = n ? cum_[n - 1] : 0.0f;
        segCount_ = n;
        if (n) memset(arc_ + (n - 1) * 25, 0, sizeof(float) * 25);
    } else if (flags_ & 8) {
        cum_[n] = cum_[n - 1] + preCalcSegment(n - 1);
        totalLen_ = cum_[n];
        segCount_ = n + 1;
    }
}

// Ikili arama: lo = v'nin dustugu aralik basi, hi = lo+1
void CubicSpline2::getInterval(float v, const float* table, unsigned int count, int& hi, int& lo) {
    lo = 0;
    hi = (int)count;
    while (hi - lo >= 2) {
        int mid = (hi + lo) >> 1;
        if (v > table[mid]) lo = mid; else hi = mid;
    }
}

void CubicSpline2::getPosition(float t, float& x, float& y) const {
    if (flags_ & 4) {
        if (t < 0.0f) t = 0.0f; else if (t > 1.0f) t = 1.0f;
    } else {
        while (t > 1.0f) t = t - 1.0f;
        while (t < 0.0f) t = t + 1.0f;
    }
    float d = t * totalLen_;
    int hi, seg;
    getInterval(d, cum_, segCount_, hi, seg);
    float local = d - cum_[seg];
    const float* arc = arc_ + seg * 25;
    int lo2 = 0, hi2 = 1;
    if ((unsigned int)seg < segCount_ - 1u) getInterval(local, arc, 25, hi2, lo2);
    float f = (float)lo2;
    float span = arc[hi2] - arc[lo2];
    if (span > 0.0f) f = f + (local - arc[lo2]) / span;
    evaluate((unsigned int)seg, f / 24.0f, x, y);
}
