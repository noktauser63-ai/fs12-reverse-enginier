// Orijinal: Matrix4x4 (64 bayt, float m[16]). Islem sirasi orijinal assembly/decompile ile ayni.
// m[col*4+row] (OpenGL duzeni); mult(a,b) -> this = a * b.
#pragma once
#include "Vector3.h"

class Quaternion;

class Matrix4x4 {
public:
    enum AxisID { AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2 };
    static const float NEARZERO;  // 1.0000000359391298e-36f

    float m[16];

    Matrix4x4() { identity(); }
    // (donus [radyan: x,y,z], olcek, oteleme). Sira: oteleme, Z, Y, X donusu, olcek.
    Matrix4x4(const Vector3& rotation, const Vector3& scale, const Vector3& translation);

    void identity();
    Matrix4x4& mult(const Matrix4x4& a, const Matrix4x4& b);    // this = a * b (4x4)
    Matrix4x4& mult3x4(const Matrix4x4& a, const Matrix4x4& b);  // 3x4 (son sutun 0,0,0,1)
    void rotate3x4(float angle, AxisID axis);
    Matrix4x4& scale3x4(const Vector3& s);
    Matrix4x4& translate3x4(const Vector3& t);  // oteleme, mevcut donusle dondurulerek eklenir
    void transformVector(const Vector3& in, Vector3& out) const;
    void rotateVector(const Vector3& in, Vector3& out) const;
    void transformVectorOrthonormalInverted(const Vector3& in, Vector3& out) const;
    float calcDeterminant3x3() const;
    bool invert3x4(const Matrix4x4& src);
    Matrix4x4& setRotation(const Quaternion& q);
    Matrix4x4& setRotation(float angle, const Vector3& axis);
    void setDirection(const Vector3& dir, const Vector3& up);
    void look(const Vector3& pos, const Vector3& dir, const Vector3& up);
    void lookAt(const Vector3& pos, const Vector3& target, const Vector3& up);
    // scale (null olabilir) ile ters olceklenerek euler acilari (x,y,z) cikarir.
    void getEulerAngles(Vector3& out, const Vector3* scale) const;
};
