#include <stdio.h>
#include <math.h>
#include "../src/engine/RandomGenerator.h"
#include "../src/engine/BitVector.h"
#include "../src/engine/Matrix4x4.h"
#include "../src/engine/Quaternion.h"
#include "../src/engine/MathUtil.h"
#include "../src/engine/CubicSpline2.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); ++fails; } } while (0)
static bool near_(float a, float b, float e = 1e-4f) { return fabsf(a - b) < e; }

int main() {
    // Rastgele: ilk degerler elle hesaplanan MWC ile ayni olmali
    RandomGenerator r;
    uint32_t s0 = 0x0a8dc1edu, s1 = 0x1fd3f0f5u;
    for (int i = 0; i < 5; ++i) {
        s0 = (s0 & 0xffff) * 0x9069u + (s0 >> 16);
        s1 = (s1 & 0xffff) * 18000u + (s1 >> 16);
        CHECK(r.getRandom() == (s0 << 16) + s1);
    }
    uint64_t sd = r.getSeed(); r.setSeed(sd); CHECK(r.getSeed() == sd);
    float f = r.getRandomFloat(); CHECK(f >= 0.f && f <= 1.f);

    BitVector bv(20, false);
    bv.setAndClearBits(3, 5, 0x15); CHECK(bv.readBits(3, 5) == 0x15); CHECK(bv.readBit(3));
    bv.clearAll(); CHECK(bv.readBits(3, 5) == 0);

    // Euler -> Quaternion -> Euler
    Vector3 e(0.3f, -0.7f, 1.1f), e2;
    Quaternion q(e);
    float n = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    CHECK(near_(n, 1.0f));
    q.getEulerAngles(e2);
    printf("euler in (%.4f %.4f %.4f) out (%.4f %.4f %.4f)\n", e.x, e.y, e.z, e2.x, e2.y, e2.z);
    Quaternion q2(e2);
    CHECK(near_(fabsf(q.dotProduct(q2)), 1.0f, 1e-3f));

    // Matris: tersleme
    Matrix4x4 m(Vector3(0.2f, 0.4f, -0.6f), Vector3(1.f, 2.f, 0.5f), Vector3(3.f, 4.f, 5.f));
    Matrix4x4 inv, id;
    CHECK(inv.invert3x4(m));
    id.mult3x4(m, inv);
    for (int i = 0; i < 16; ++i) CHECK(near_(id.m[i], (i % 5 == 0) ? 1.f : 0.f, 1e-4f));
    // slerp uc noktalar
    Quaternion a, b(Vector3(0.f, 1.f, 0.f)), s;
    s.slerp(a, b, 0.f); CHECK(near_(s.dotProduct(a), 1.f));
    s.slerp(a, b, 1.f); CHECK(near_(s.dotProduct(b), 1.f, 1e-3f));
    s.slerpShortestPath(a, b, 0.5f);
    CHECK(near_(s.dotProduct(s), 1.f, 1e-3f));
    // Vector3
    Vector3 v(3, 0, 4); v.normalize(); CHECK(near_(v.x, 0.6f) && near_(v.z, 0.8f));
    Vector3 c = Vector3::crossProduct(Vector3(1, 0, 0), Vector3(0, 1, 0)); CHECK(near_(c.z, 1.f));
    // MathUtil / frustum / spline
    CHECK(near_(MathUtil::radianToDegree(MathUtil::degreeToRadian(90.f)), 90.f, 1e-3f));
    float poly[] = {0,0, 4,0, 4,4, 0,4};
    CHECK(MathUtil::pointInPoly2D(2, 2, poly, 4)); CHECK(!MathUtil::pointInPoly2D(5, 2, poly, 4));
    float proj[16]; MathUtilHandheld::createPerspProjectionMatrix(1.0f, 1.5f, 1.f, 100.f, proj);
    Plane pl[6]; MathUtilHandheld::createFrustumPlanes(proj, pl);
    float boxIn[6] = {0, 0, -10, 1, 1, 1}, boxOut[6] = {0, 0, 10, 1, 1, 1};
    CHECK(MathUtilHandheld::aabbInFrustum(boxIn, pl)); CHECK(!MathUtilHandheld::aabbInFrustum(boxOut, pl));
    float pts[] = {0,0, 10,0, 10,10, 0,10};
    CubicSpline2 sp; sp.load(4, pts, 8);   // kapali
    float sx, sy, sx2, sy2; sp.getPosition(0.f, sx, sy); sp.getPosition(1.f, sx2, sy2);
    printf("spline len=%.3f p0=(%.3f,%.3f) p1=(%.3f,%.3f)\n", sp.totalLength(), sx, sy, sx2, sy2);
    CHECK(sp.totalLength() > 20.f && near_(sx, sx2, 1e-2f) && near_(sy, sy2, 1e-2f));
    sp.destroy();
    printf("fails=%d\n", fails);
    return fails ? 1 : 0;
}
