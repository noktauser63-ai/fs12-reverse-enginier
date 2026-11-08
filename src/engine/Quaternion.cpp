#include "Quaternion.h"

#include <math.h>

#include "Matrix4x4.h"

Quaternion::Quaternion(const Vector3& e) {
    Matrix4x4 m(e, Vector3(1.f, 1.f, 1.f), Vector3(0.f, 0.f, 0.f));
    set(m);
}

Quaternion& Quaternion::mult(float s) {
    x *= s; y *= s; z *= s; w *= s;
    return *this;
}

Quaternion& Quaternion::normalise() {
    float inv = 1.0f / sqrtf(((x * x + y * y) + z * z) + w * w);
    x *= inv; y *= inv; z *= inv; w *= inv;
    return *this;
}

// Matris duzeni m[col*4+row]; asagidaki indeksler orijinaldeki bayt ofsetlerinden (/4) alindi.
void Quaternion::set(const Matrix4x4& mt) {
    const float* m = mt.m;
    float m0 = m[0], m5 = m[5], m10 = m[10];
    float trace = (m0 + m5) + m10;
    if (trace >= 0.0f) {
        float s = sqrtf(trace + 1.0f);
        w = s * 0.5f;
        float t = 0.5f / s;
        x = (m[6] - m[9]) * t;
        y = (m[8] - m[2]) * t;
        z = (m[1] - m[4]) * t;
        return;
    }
    bool yBig = m5 > m0;
    float maxXY = yBig ? m5 : m0;
    if (!(m10 > maxXY)) {
        if (yBig) {
            float s = sqrtf((m5 - (m10 + m0)) + 1.0f);
            y = s * 0.5f;
            float t = 0.5f / s;
            z = (m[9] + m[6]) * t;
            x = (m[4] + m[1]) * t;
            w = (m[8] - m[2]) * t;
        } else {
            float s = sqrtf((m0 - (m5 + m10)) + 1.0f);
            x = s * 0.5f;
            float t = 0.5f / s;
            y = (m[4] + m[1]) * t;
            z = (m[2] + m[8]) * t;
            w = (m[6] - m[9]) * t;
        }
    } else {
        float s = sqrtf((m10 - (m0 + m5)) + 1.0f);
        z = s * 0.5f;
        float t = 0.5f / s;
        x = (m[2] + m[8]) * t;
        y = (m[9] + m[6]) * t;
        w = (m[1] - m[4]) * t;
    }
}

void Quaternion::nlerp(const Quaternion& a, const Quaternion& b, float t) {
    x = b.x - a.x; y = b.y - a.y; z = b.z - a.z; w = b.w - a.w;
    mult(t);
    x += a.x; y += a.y; z += a.z; w += a.w;
    normalise();
}

void Quaternion::nlerpShortestPath(const Quaternion& a, const Quaternion& b, float t) {
    if (a.dotProduct(b) < 0.0f) {
        x = -b.x - a.x; y = -b.y - a.y; z = -b.z - a.z; w = -b.w - a.w;
    } else {
        x = b.x - a.x; y = b.y - a.y; z = b.z - a.z; w = b.w - a.w;
    }
    mult(t);
    x += a.x; y += a.y; z += a.z; w += a.w;
    normalise();
}

static inline float clampDot(float d) { return d > 1.0f ? 1.0f : (d < -1.0f ? -1.0f : d); }

void Quaternion::slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = ((a.x * b.x + a.y * b.y) + a.z * b.z) + a.w * b.w;
    float omega = acosf(clampDot(dot));
    if (!(fabsf(omega) < 0.001f)) {
        float so = sinf(omega);
        Quaternion qa = a, qb = b;
        qa.mult(sinf(omega * (1.0f - t)));
        qb.mult(sinf(omega * t));
        x = qb.x + qa.x; y = qb.y + qa.y; z = qb.z + qa.z; w = qb.w + qa.w;
        mult(1.0f / so);
    } else {
        *this = a;
    }
}

void Quaternion::slerpShortestPath(const Quaternion& a, const Quaternion& b, float t) {
    float dot = a.dotProduct(b);
    float omega = acosf(clampDot(dot));
    if (!(fabsf(omega) < 0.001f)) {
        float inv = 1.0f / sinf(omega);
        float w0 = sinf((1.0f - t) * omega) * inv;
        float w1 = sinf(t * omega) * inv;
        Quaternion qa = a, qb = b;
        qb.mult(w1);
        qa.mult(w0);
        x = qa.x + qb.x; y = qa.y + qb.y; z = qa.z + qb.z; w = qa.w + qb.w;
        if (dot < 0.0f) normalise();
    } else {
        *this = a;
    }
}

void Quaternion::addShortestPath(const Quaternion& q, float t) {
    Quaternion tmp = q;
    if (dotProduct(q) < 0.0f) { tmp.x = -tmp.x; tmp.y = -tmp.y; tmp.z = -tmp.z; tmp.w = -tmp.w; }
    tmp.mult(t);
    x += tmp.x; y += tmp.y; z += tmp.z; w += tmp.w;
}

void Quaternion::getEulerAngles(Vector3& out) const {
    Matrix4x4 m;
    for (int i = 0; i < 16; ++i) m.m[i] = 0.0f;
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    m.setRotation(*this);
    m.getEulerAngles(out, nullptr);
}
