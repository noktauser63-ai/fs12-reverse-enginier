#include "Matrix4x4.h"

#include <math.h>

#include "Quaternion.h"

const float Matrix4x4::NEARZERO = 1.0000000359391298e-36f;

static const int kAxisIndex[3][2] = {{4, 8}, {8, 0}, {0, 4}};  // orijinal: Matrix4x4::rotate3x4::index

Matrix4x4::Matrix4x4(const Vector3& rotation, const Vector3& scale, const Vector3& translation) {
    identity();
    m[12] = translation.x;
    m[13] = translation.y;
    m[14] = translation.z;
    rotate3x4(rotation.z, AXIS_Z);
    rotate3x4(rotation.y, AXIS_Y);
    rotate3x4(rotation.x, AXIS_X);
    scale3x4(scale);
}

void Matrix4x4::identity() {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

Matrix4x4& Matrix4x4::mult(const Matrix4x4& a, const Matrix4x4& b) {
    // Cikti a ve b ile ortusebilir diye orijinal gibi dogrudan yazmak yerine once hesapla.
    float r[16];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            r[row * 4 + col] = b.m[row * 4 + 0] * a.m[0 * 4 + col] + b.m[row * 4 + 1] * a.m[1 * 4 + col] +
                               b.m[row * 4 + 2] * a.m[2 * 4 + col] + b.m[row * 4 + 3] * a.m[3 * 4 + col];
        }
    }
    for (int i = 0; i < 16; ++i) m[i] = r[i];
    return *this;
}

Matrix4x4& Matrix4x4::mult3x4(const Matrix4x4& a, const Matrix4x4& b) {
    float r[16];
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            r[row * 4 + col] = b.m[row * 4 + 0] * a.m[0 * 4 + col] + b.m[row * 4 + 1] * a.m[1 * 4 + col] +
                               b.m[row * 4 + 2] * a.m[2 * 4 + col];
        }
        r[row * 4 + 3] = 0.0f;
    }
    for (int col = 0; col < 3; ++col) {
        r[12 + col] = b.m[12] * a.m[0 * 4 + col] + b.m[13] * a.m[1 * 4 + col] + b.m[14] * a.m[2 * 4 + col] + a.m[12 + col];
    }
    r[15] = 1.0f;
    for (int i = 0; i < 16; ++i) m[i] = r[i];
    return *this;
}

void Matrix4x4::rotate3x4(float angle, AxisID axis) {
    float s = sinf(angle);
    float c = cosf(angle);
    int i = kAxisIndex[axis][0];
    int j = kAxisIndex[axis][1];
    for (int k = 0; k < 4; ++k) {
        float a = m[i + k];
        float b = m[j + k];
        m[i + k] = b * s + a * c;
        m[j + k] = b * c - a * s;
    }
}

Matrix4x4& Matrix4x4::scale3x4(const Vector3& s) {
    m[0] *= s.x; m[1] *= s.x; m[2] *= s.x;
    m[4] *= s.y; m[5] *= s.y; m[6] *= s.y;
    m[8] *= s.z; m[9] *= s.z; m[10] *= s.z;
    return *this;
}

Matrix4x4& Matrix4x4::translate3x4(const Vector3& t) {
    Vector3 r;
    rotateVector(t, r);
    m[12] += r.x;
    m[13] += r.y;
    m[14] += r.z;
    return *this;
}

void Matrix4x4::transformVector(const Vector3& in, Vector3& out) const {
    float x = in.x * m[0] + in.y * m[4] + in.z * m[8] + m[12];
    float y = in.x * m[1] + in.y * m[5] + in.z * m[9] + m[13];
    float z = in.x * m[2] + in.y * m[6] + in.z * m[10] + m[14];
    out.x = x; out.y = y; out.z = z;
}

void Matrix4x4::rotateVector(const Vector3& in, Vector3& out) const {
    float x = in.x * m[0] + in.y * m[4] + in.z * m[8];
    float y = in.x * m[1] + in.y * m[5] + in.z * m[9];
    float z = in.x * m[2] + in.y * m[6] + in.z * m[10];
    out.x = x; out.y = y; out.z = z;
}

void Matrix4x4::transformVectorOrthonormalInverted(const Vector3& in, Vector3& out) const {
    float dx = in.x - m[12], dy = in.y - m[13], dz = in.z - m[14];
    float x = dx * m[0] + dy * m[1] + dz * m[2];
    float y = dx * m[4] + dy * m[5] + dz * m[6];
    float z = dx * m[8] + dy * m[9] + dz * m[10];
    out.x = x; out.y = y; out.z = z;
}

float Matrix4x4::calcDeterminant3x3() const {
    float a = m[5], b = m[10], c = m[6], d = m[9], e = m[2], f = m[1];
    float t1 = (a * b - c * d) * m[0];
    float t2 = (f * b - e * d) * m[4];
    float t3 = (f * c - e * a) * m[8];
    return (t1 - t2) + t3;
}

bool Matrix4x4::invert3x4(const Matrix4x4& s) {
    float det = s.calcDeterminant3x3();
    if (!(fabsf(det) > NEARZERO)) return false;
    float inv = 1.0f / det;
    float ninv = -inv;
    float r[16];
    r[0] = inv * (s.m[5] * s.m[10] - s.m[6] * s.m[9]);
    r[1] = ninv * (s.m[1] * s.m[10] - s.m[2] * s.m[9]);
    r[2] = inv * (s.m[1] * s.m[6] - s.m[2] * s.m[5]);
    r[3] = 0.0f;
    r[4] = ninv * (s.m[4] * s.m[10] - s.m[6] * s.m[8]);
    r[5] = inv * (s.m[0] * s.m[10] - s.m[2] * s.m[8]);
    r[6] = ninv * (s.m[0] * s.m[6] - s.m[2] * s.m[4]);
    r[7] = 0.0f;
    r[8] = inv * (s.m[4] * s.m[9] - s.m[5] * s.m[8]);
    r[9] = ninv * (s.m[0] * s.m[9] - s.m[1] * s.m[8]);
    r[10] = inv * (s.m[0] * s.m[5] - s.m[1] * s.m[4]);
    r[11] = 0.0f;
    r[12] = -(r[0] * s.m[12] + r[4] * s.m[13] + r[8] * s.m[14]);
    r[13] = -(r[1] * s.m[12] + r[5] * s.m[13] + r[9] * s.m[14]);
    r[14] = -(r[2] * s.m[12] + r[6] * s.m[13] + r[10] * s.m[14]);
    r[15] = 1.0f;
    for (int i = 0; i < 16; ++i) m[i] = r[i];
    return true;
}

Matrix4x4& Matrix4x4::setRotation(const Quaternion& q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float wx = w * x, wy = w * y, wz = w * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float t;
    t = yy + zz; m[0] = 1.0f - (t + t);
    t = xy + wz; m[1] = t + t;
    t = xz - wy; m[2] = t + t;
    t = xy - wz; m[4] = t + t;
    t = xx + zz; m[5] = 1.0f - (t + t);
    t = yz + wx; m[6] = t + t;
    t = xz + wy; m[8] = t + t;
    t = yz - wx; m[9] = t + t;
    t = xx + yy; m[10] = 1.0f - (t + t);
    return *this;
}

Matrix4x4& Matrix4x4::setRotation(float angle, const Vector3& axis) {
    float len2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    if (len2 != 0.0f) {
        float s = sinf(angle * 0.5f);
        float inv = 1.0f / sqrtf(len2);
        float k = s * inv;
        Quaternion q;
        q.x = k * axis.x;
        q.y = k * axis.y;
        q.z = k * axis.z;
        q.w = cosf(angle * 0.5f);
        setRotation(q);
    }
    return *this;
}

void Matrix4x4::setDirection(const Vector3& dir, const Vector3& up) {
    Vector3 fwd = dir;
    fwd.normalize();
    Vector3 right = Vector3::crossProduct(up, fwd);
    right.normalize();
    Vector3 up2 = Vector3::crossProduct(fwd, right);
    m[0] = right.x; m[1] = right.y; m[2] = right.z;
    m[4] = up2.x;   m[5] = up2.y;   m[6] = up2.z;
    m[8] = fwd.x;   m[9] = fwd.y;   m[10] = fwd.z;
}

void Matrix4x4::look(const Vector3& pos, const Vector3& dir, const Vector3& up) {
    Vector3 fwd = dir;
    fwd.normalize();
    Vector3 right = Vector3::crossProduct(fwd, up);
    right.normalize();
    Vector3 up2 = Vector3::crossProduct(right, fwd);
    m[1] = up2.x;   m[2] = -fwd.x;  m[3] = 0.0f;
    m[5] = up2.y;   m[6] = -fwd.y;  m[7] = 0.0f;
    m[9] = up2.z;   m[10] = -fwd.z; m[11] = 0.0f;
    m[0] = right.x; m[4] = right.y; m[8] = right.z;
    m[12] = -(right.x * pos.x + right.y * pos.y + right.z * pos.z);
    m[13] = -(up2.x * pos.x + up2.y * pos.y + up2.z * pos.z);
    m[14] = fwd.x * pos.x + fwd.y * pos.y + fwd.z * pos.z;
    m[15] = 1.0f;
}

void Matrix4x4::lookAt(const Vector3& pos, const Vector3& target, const Vector3& up) {
    Vector3 dir(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    look(pos, dir, up);
}

void Matrix4x4::getEulerAngles(Vector3& out, const Vector3* scale) const {
    float isx = 1.0f, isy = 1.0f, isz = 1.0f;
    if (scale) {
        isx = (scale->x == 0.0f) ? 1.0f : 1.0f / scale->x;
        isy = (scale->y == 0.0f) ? 1.0f : 1.0f / scale->y;
        isz = (scale->z == 0.0f) ? 1.0f : 1.0f / scale->z;
    }
    float sy = -m[2] * isx;  // sin(y)
    float c2 = 1.0f - sy * sy;
    float cy;
    if (c2 > 1e-6f) {
        cy = (float)sqrt((double)c2);
        out.x = (float)atan2((double)((isy * m[6]) / cy), (double)((isz * m[10]) / cy));
        out.z = (float)atan2((double)((isx * m[1]) / cy), (double)((isx * m[0]) / cy));
    } else {
        out.x = (float)atan2((double)(-m[9] * isz), (double)(isy * m[5]));
        out.z = 0.0f;
        cy = 0.0f;
    }
    out.y = (float)atan2((double)sy, (double)cy);
}
