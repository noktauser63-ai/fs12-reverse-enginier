// Orijinal: Plane {a,b,c,d}; duzlem denklemi a*x+b*y+c*z+d = 0.
#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"

class Plane {
public:
    float a, b, c, d;

    Plane() : a(0), b(1), c(0), d(0) {}
    Plane(float a_, float b_, float c_, float d_) : a(a_), b(b_), c(c_), d(d_) {}
    Plane(const Vector3& point, const Vector3& normal) { set(point, normal); }
    Plane(const Vector3& p1, const Vector3& p2, const Vector3& p3) {
        Vector3 e1(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
        Vector3 e2(p3.x - p1.x, p3.y - p1.y, p3.z - p1.z);
        Vector3 n = Vector3::crossProduct(e1, e2);
        n.normalize();
        a = n.x; b = n.y; c = n.z;
        d = -n.dotProduct(p1);
    }
    void set(const Vector3& point, const Vector3& normal) {
        a = normal.x; b = normal.y; c = normal.z;
        d = -normal.dotProduct(point);
    }
    const Vector3& normal() const { return *reinterpret_cast<const Vector3*>(&a); }
    void normalize() {
        float inv = 1.0f / sqrtf(normal().dotProduct(normal()));
        a *= inv; b *= inv; c *= inv; d *= inv;
    }
    void transform(const Matrix4x4& m) {
        float nd = -d;
        Vector3 p(nd * a, nd * b, nd * c);
        Vector3 n;
        m.rotateVector(normal(), n);
        n.normalize();
        a = n.x; b = n.y; c = n.z;
        Vector3 p2;
        m.transformVector(p, p2);
        d = -n.dotProduct(p2);
    }
    bool isFrontFacingTo(const Vector3& dir) const { return normal().dotProduct(dir) <= 0.0f; }
    float signedDistanceTo(const Vector3& p) const { return p.dotProduct(normal()) + d; }
    bool pointInHalfSpace(const Vector3& p) const { return (normal().dotProduct(p) + d) <= 0.0f; }
    bool sphereInHalfSpace(const Vector3& center, float radius) const {
        return (normal().dotProduct(center) + d) <= -radius;
    }
    bool intersectionWithLine(const Vector3& origin, const Vector3& dir, float& t) const {
        float den = dir.dotProduct(normal());
        if (fabsf(den) < 1e-4f) return false;  // 0x38d1b717
        t = -(origin.dotProduct(normal()) + d) / den;
        return true;
    }
    bool intersectionWithLine(const Vector3& origin, const Vector3& dir, Vector3& out) const {
        float t;
        if (!intersectionWithLine(origin, dir, t)) return false;
        out.x = origin.x + dir.x * t;
        out.y = origin.y + dir.y * t;
        out.z = origin.z + dir.z * t;
        return true;
    }
    // intersectionWithPlane: oyunda yalnizca kullanilmayan MathUtil ucgen testleri cagiriyor; yazilmadi.
};
