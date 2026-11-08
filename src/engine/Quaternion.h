// Orijinal: Quaternion. Bellek duzeni: w, x, y, z (w ilk 4 bayt).
#pragma once
#include "Vector3.h"

class Matrix4x4;

class Quaternion {
public:
    float w, x, y, z;

    Quaternion() : w(1.f), x(0.f), y(0.f), z(0.f) {}
    explicit Quaternion(const Vector3& eulerRadians);  // Matrix4x4(euler, 1, 0) -> set()

    Quaternion& mult(float s);
    Quaternion& normalise();
    float dotProduct(const Quaternion& o) const { return w * o.w + x * o.x + y * o.y + z * o.z; }
    void set(const Matrix4x4& m);
    void nlerp(const Quaternion& a, const Quaternion& b, float t);
    void nlerpShortestPath(const Quaternion& a, const Quaternion& b, float t);
    void slerp(const Quaternion& a, const Quaternion& b, float t);
    void slerpShortestPath(const Quaternion& a, const Quaternion& b, float t);
    void addShortestPath(const Quaternion& q, float t);
    void getEulerAngles(Vector3& out) const;
};
