// Cowshed (ahir): FS14 libfs14.so'daki Cows::update mantiginin FS12 yeniden insasi icin bagimsiz modulu.
// Game/Map sinifi henuz yok; bu sinif saf mantik (GL, fizik, Game bagimliligi yok). Game gelince:
//   - her kare: cowshed.update(oyunSaniyesi, saat)
//   - balya ahira birakilinca: cowshed.addBales(...)
//   - sut tanki / gubre noktasi: takeMilk / takeManure
// Kaynak: Cows::update @ 0024e268 (decompile). "KOD" = decompile'da gorulen sabit,
// "TODO(constants.txt)" = .so veri bolumunden okunacak, simdilik yer tutucu, "ONERI" = ekonomi onerisi.
#pragma once
#include <cstdint>
#include <vector>

namespace fs12 {

enum class BaleType : uint8_t { Hay = 0, Straw = 1 };

struct CowshedParams {
    // --- KOD: Cows::update icinde literal olarak gorulen degerler ---
    float feedPerDay = 6000.f;            // yem (ot balyasi) tuketimi, litre/gun
    float strawPerDay = 4000.f;           // altlik (saman) tuketimi, litre/gun
    float manurePerDay = 1000.f;          // gubre uretimi, litre/gun (saman varken)
    float milkPotentialDrainPerHour = 3000.f;  // sagim saatinde potansiyelden dusen miktar
    float milkOutPerHour = 1000.f;        // sagim saatinde tanka giren azami sut, litre/saat
    float milkTankMax = 14000.f;          // 0x465ac000 float sabiti
    float morningStart = 6.f, morningEnd = 7.f;     // sabah sagimi
    float eveningStart = 17.f, eveningEnd = 18.f;   // aksam sagimi
    int maxHayBales = 32, maxStrawBales = 32;       // oyun ici yardim metni: "up to 32 bales"

    // --- TODO(constants.txt): .so veri bolumunden (FruitUtil::BALE_VOLUME, Map::MAX_MANURE_FILL_LEVEL,
    //     DAY_START_HOUR / NIGHT_START_HOUR). Asagidakiler YER TUTUCU. ---
    float baleVolume = 1000.f;            // bir balyanin litre karsiligi (FruitUtil::BALE_VOLUME)
    float maxManure = 20000.f;            // Map::MAX_MANURE_FILL_LEVEL
    float daySeconds = 86400.f;           // "gunluk" oranlarin bolundugu sure. Orijinal:
                                          // (NIGHT_START_HOUR - DAY_START_HOUR) * 3600 olabilir.

    // --- ONERI: ekonomi (FS12 listesine gore; oynayarak ayarlanir) ---
    int   cowshedPrice = 120000;          // ahir satin alma (Agrotron K 420 = 92.000 ile X 720 = 280.000 arasi)
    float milkPricePerLiter = 0.60f;
    float strawBalePrice = 0.f;           // TODO: FruitUtil::BALE_PRICE
};

class Cowshed {
public:
    explicit Cowshed(const CowshedParams& p = CowshedParams()) : prm(p) {}

    // Ahira balya birak. Kabul edilen sayiyi doner (kapasite doluysa fazlasi reddedilir).
    int addBales(BaleType t, int count);

    // dtGameSec: gecen OYUN saniyesi, hour: gunun saati [0,24) (kesirli).
    void update(float dtGameSec, float hour);

    // Tanktan sut al (litre); alinan miktari doner.
    float takeMilk(float liters);
    // Tum sutu sat; kazanci doner.
    float sellAllMilk() { float l = takeMilk(milkTank); return l * prm.milkPricePerLiter; }
    // Gubre al (gubre dagitici dolumu icin).
    float takeManure(float liters);

    int hayBales() const { return hay; }
    int strawBales() const { return straw; }
    float milk() const { return milkTank; }
    float manure() const { return manureLevel; }
    float feedInTrough() const { return hayTrough; }
    float bedding() const { return strawBed; }
    const CowshedParams& params() const { return prm; }

    // Kayit: surumlu, kucuk-endian sabit duzen.
    std::vector<uint8_t> serialize() const;
    bool deserialize(const uint8_t* data, size_t size);

private:
    CowshedParams prm;
    int hay = 0, straw = 0;                 // depodaki balya sayisi
    float hayTrough = 0.f, strawBed = 0.f;  // yemlik / altlikta kalan litre
    float milkPotential = 0.f;              // yenen yemden birikmis, henuz sagilmamis (Cows+0x478)
    float milkTank = 0.f;                   // Game+0x11b8
    float manureLevel = 0.f;                // Game+0x11c8
};

} // namespace fs12
