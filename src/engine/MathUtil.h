// Orijinal: MathUtil / MathUtilHandheld (yalnizca oyunda cagrilan fonksiyonlar; ucgen kesisim/tangent
// yardimcilari kullanilmadigi icin ilgili sinif yazilirken gerekirse eklenecek).
#pragma once
#include "Plane.h"
#include "Vector3.h"

namespace MathUtil {
inline float degreeToRadian(float d) { return d * 0.017453292f; }   // 0x3c8efa35
inline float radianToDegree(float r) { return r * 57.29578f; }      // 0x42652ee1
inline float sign(float v) { return v > 0.0f ? 1.0f : (v < 0.0f ? -1.0f : 0.0f); }
// pts: (x,y) cift dizisi, n nokta. Cift-tek kurali.
inline bool pointInPoly2D(float px, float py, const float* pts, unsigned int n) {
    bool inside = false;
    int j = (int)n - 1;
    for (unsigned int i = 0; i < n; ++i) {
        float yi = pts[i * 2 + 1], yj = pts[j * 2 + 1];
        if ((yi > py) != (yj > py)) {
            float x = ((pts[j * 2] - pts[i * 2]) * (py - yi)) / (yj - yi) + pts[i * 2];
            if (px < x) inside = !inside;
        }
        j = (int)i;
    }
    return inside;
}
}  // namespace MathUtil

namespace MathUtilHandheld {
// out[16] sutun-oncelikli (OpenGL). Not: orijinalde out[14] = (far*near)/(far-near) (2 carpani yok).
void createPerspProjectionMatrix(float fovY, float aspect, float nearZ, float farZ, float* out);
// planes[6]: sol, sag, alt, ust, yakin, uzak (sirasiyla). m: 16'lik projeksiyon*view matrisi.
void createFrustumPlanes(const float* m, Plane* planes);
// aabb: {cx,cy,cz, hx,hy,hz}
bool aabbInFrustum(const float* aabb, const Plane* planes);
bool pointInAABB(const float* point, const float* aabb);
bool sphereInFrustum2D(const Plane* planes, float x, float z, float radius);
}  // namespace MathUtilHandheld
