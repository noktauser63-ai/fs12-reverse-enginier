// GameState: oyuncunun parasi, sahip oldugu araclar ve surdugu arac. Basit ikili dosyada saklanir (fs12_state.bin).
#pragma once
#include <cstdint>
#include <vector>
#include "game/VehicleCatalog.h"

namespace fs12 {
struct GameState {
    int32_t money = 450000;
    uint8_t owned[32] = {};       // katalog indeksi (vehicleSpecs sirasi) -> sahip mi
    int32_t active = -1;          // surulen aracin katalog indeksi
    GameState();                  // baslangic: K420 + 5465 H
    // dukkan (sadece gorselli araclar: traktor/biçerdover/pickup, katalog sirasi)
    static int shopCount();
    static int specIndex(int shopSlot);          // dukkan sirasi -> katalog indeksi
    bool canBuy(int slot) const;
    bool canSell(int slot) const;
    bool buy(int slot);
    bool sell(int slot);
    bool select(int slot);                       // sahip olunan araci surmek icin sec
    const VehicleSpec* activeSpec() const;
    std::vector<uint8_t> toBytes() const;
    bool fromBytes(const uint8_t* d, size_t n);
};
}
