// GuiLayout'u orijinal libapp.so'dan (Unicorn kahini ile) uretilmis altin vektorlerle bit-bit karsilastirir.
#include "game/GuiLayout.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
using namespace fs12;
static uint32_t bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static int bad = 0, total = 0;
static void chk(const char* what, int line, float got, float exp) {
    ++total;
    if (bits(got) != bits(exp)) { if (++bad < 10) printf("FARK %s satir %d: %.9g beklenen %.9g\n", what, line, got, exp); }
}
int main(int argc, char** argv) {
    FILE* f = fopen(argc > 1 ? argv[1] : "tests/golden/gui_layout.txt", "r");
    if (!f) { printf("golden yok\n"); return 2; }
    char l[512]; int n = 0; GuiScale sc;
    while (fgets(l, sizeof l, f)) {
        ++n; char t = l[0];
        if (t == 'S') {
            float a, sx, sy; sscanf(l + 2, "%g %g %g", &a, &sx, &sy);
            GuiScale s; s.update(a); chk("sx", n, s.sx, sx); chk("sy", n, s.sy, sy);
            if (a > 1.77f && a < 1.78f) sc = s;
        } else if (t == 'A') {
            float x, y, w, h, rx, ry, rw, rh; int a;
            sscanf(l + 2, "%g %g %g %g %d %g %g %g %g", &x, &y, &w, &h, &a, &rx, &ry, &rw, &rh);
            sc.align(x, y, w, h, a);
            chk("Ax", n, x, rx); chk("Ay", n, y, ry); chk("Aw", n, w, rw); chk("Ah", n, h, rh);
        } else if (t == 'U') {
            float x, y, rx, ry; int a; sscanf(l + 2, "%g %g %d %g %g", &x, &y, &a, &rx, &ry);
            sc.undo(x, y, a); chk("Ux", n, x, rx); chk("Uy", n, y, ry);
        } else if (t == 'P') {
            float x, y, w, h, rx, ry; int a; sscanf(l + 2, "%g %g %g %g %d %g %g", &x, &y, &w, &h, &a, &rx, &ry);
            anchorShift(x, y, w, h, a); chk("Px", n, x, rx); chk("Py", n, y, ry);
        }
    }
    printf("layout_test: %d karsilastirma, %d fark\n", total, bad);
    return bad ? 1 : 0;
}
