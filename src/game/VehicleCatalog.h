// VehicleCatalog: oyundaki araclarin veri tablosu (segmentli). Orijinal FS12 degerleri (loadShopItems / VehicleDesc+0xe0) ile
// FS12_Entegrasyon.zip'ten gelen yeni araclar. "FS12" = orijinal veri; "EXT" = entegrasyon paketi (degerler segmente gore ayarlandi).
#pragma once
#include <cstddef>

namespace fs12 {
enum class VKind { Tractor, Combine, Planter, Cultivator, Pickup };
enum class VSegment { Starter, Mid, Top };
struct WheelSpec {
    const char* mesh; float x, y, z, radius;
    bool steer;          // on tekerlek: direksiyonla doner
    const char* fender;  // tekerlek merkezinde duran camurluk (donmez, direksiyonla doner) veya nullptr
};
struct PartSpec { const char* mesh; float x, y, z; float sx = 1.f; const char* tex = nullptr; };   // sx: yatay (x) olcek (tabla genisligi icin)   // govdeye gore ofsetli sabit parca (boru vb.)
struct VehicleSpec {
    const char* id;          // dahili ad
    const char* name;        // gosterilen ad
    const char* brand;
    VKind kind; VSegment segment;
    bool original;           // true: orijinal FS12 araci, false: entegrasyon paketi
    int price;               // oyun ici para
    float ps, kw, torqueNm;  // guc / tork (tractor, combine)
    float topKmh;            // azami hiz
    float tankL;             // biçerdoverde tane deposu (L)
    float headerM;           // biçerdoverde takili tabla genisligi / ekipman calisma genisligi (m)
    const char* body;        // govde mesh
    const char* bodyTex;
    const char* wheelTex;
    WheelSpec wheels[6]; int nWheels;
    const char* fixedParts[4];   // govdeye sabit parcalar (ayni doku) - pencere/isik vb. alfa gerektirenler haric
    const char* audio;       // ses on eki (audio/<on ek>_idle.p1d ...)
    PartSpec offsetParts[6];     // ofsetli sabit parcalar (govde dokusu; ornegin bosaltma borusu)
};
const VehicleSpec* vehicleSpecs(size_t& n);
const VehicleSpec* findVehicle(const char* id);
}
