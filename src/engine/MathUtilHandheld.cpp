#include "MathUtil.h"

#include <math.h>

namespace MathUtilHandheld {

void createPerspProjectionMatrix(float fovY, float aspect, float nearZ, float farZ, float* o) {
    float f = 1.0f / tanf(fovY * 0.5f);
    float nf = farZ - nearZ;
    o[5] = f;
    o[0] = f / aspect;
    o[1] = o[2] = o[3] = o[4] = 0.0f;
    o[6] = o[7] = o[8] = o[9] = 0.0f;
    o[10] = (farZ + nearZ) / nf;
    o[11] = -1.0f;
    o[12] = o[13] = 0.0f;
    o[14] = (farZ * nearZ) / nf;
    o[15] = 0.0f;
}

void createFrustumPlanes(const float* m, Plane* p) {
    p[0] = Plane(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
    p[0].normalize();
    p[1] = Plane(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
    p[1].normalize();
    p[2] = Plane(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
    p[2].normalize();
    p[3] = Plane(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
    p[3].normalize();
    p[4] = Plane(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
    p[4].normalize();
    p[5] = Plane(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);
    p[5].normalize();
}

bool aabbInFrustum(const float* box, const Plane* planes) {
    float hx = box[3], hy = box[4], hz = box[5];
    float cx = box[0], cy = box[1], cz = box[2];
    for (int i = 0; i < 6; ++i) {
        const Plane& p = planes[i];
        float r = (hx * fabsf(p.a) + hy * fabsf(p.b)) + hz * fabsf(p.c);
        float dist = ((-cx * p.a) - cy * p.b) - cz * p.c - p.d;
        if (r < dist) return false;
    }
    return true;
}

bool pointInAABB(const float* pt, const float* box) {
    return pt[0] >= box[0] - box[3] && pt[0] <= box[0] + box[3] &&
           pt[1] >= box[1] - box[4] && pt[1] <= box[1] + box[4] &&
           pt[2] >= box[2] - box[5] && pt[2] <= box[2] + box[5];
}

bool sphereInFrustum2D(const Plane* planes, float x, float z, float radius) {
    for (int i = 0; i < 6; ++i) {
        float dist = (x * planes[i].a + z * planes[i].c) + planes[i].d;
        if (dist <= -radius) return false;
    }
    return true;
}

}  // namespace MathUtilHandheld
