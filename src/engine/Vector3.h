// Orijinal: Vector3 (12 bayt: x,y,z). libapp.so 0x570a0.. 0x59f2c.. Kayan nokta islem sirasi orijinalle ayni tutuldu
// (softfp ile uretilen kayit/sync sonuclarinin kucuk farklarla ayrismamasi icin).
#pragma once
#include <math.h>

struct Vector3 {
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    static Vector3 crossProduct(const Vector3& a, const Vector3& b) {
        return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }
    float dotProduct(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    // Sifir vektoru icin koruma yok (orijinal gibi).
    Vector3& normalize() {
        float inv = 1.0f / sqrtf(x * x + y * y + z * z);
        x *= inv; y *= inv; z *= inv;
        return *this;
    }
    void lerp(float t, const Vector3& a, const Vector3& b) {
        if (t <= 0.0f) { *this = a; return; }
        if (t >= 1.0f) { *this = b; return; }
        x = a.x + (b.x - a.x) * t;
        y = a.y + (b.y - a.y) * t;
        z = a.z + (b.z - a.z) * t;
    }
    // Aci vektorleri (radyan): her bilesen icin en kisa yoldan.
    void lerpEulerShortestPath(float t, const Vector3& a, const Vector3& b) {
        if (t <= 0.0f) { *this = a; return; }
        if (t >= 1.0f) { *this = b; return; }
        for (int i = 0; i < 3; ++i) {
            float d = b[i] - a[i];
            float ad = fabsf(d);
            if (ad > 6.2831855f - ad) d = (d < 0.0f ? -6.2831855f : 6.2831855f) - d;
            (*this)[i] = a[i] + t * d;
        }
    }
    void slerp(float t, const Vector3& a, const Vector3& b) {
        if (t <= 0.0f) { *this = a; return; }
        if (t >= 1.0f) { *this = b; return; }
        float dot = a.x * b.x + a.y * b.y + a.z * b.z;
        float w0, w1 = t;
        if (1.0f - dot > 1e-6f) {  // 0x358637bd
            float omega = acosf(dot);
            float so = sinf(omega);
            w0 = sinf((1.0f - t) * omega) / so;
            w1 = sinf(t * omega) / so;
        } else {
            w0 = 1.0f - t;
        }
        x = a.x * w0 + b.x * w1;
        y = a.y * w0 + b.y * w1;
        z = a.z * w0 + b.z * w1;
    }
};

struct Vector4 {
    float x, y, z, w;
    void set(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
};
