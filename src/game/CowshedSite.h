// Ahir yeri (map01.m3d, FS12). Silolarin (3 adet, x=43.1, z=193.0/204.1/215.3) karsisindaki kapali ambar
// kaldirilip yerine ahir konur. Koordinatlar harita nesnelerinin mesh koseleri + matrislerinden hesaplandi
// (tools/map_objects.cpp).
#pragma once
#include <cstddef>
namespace fs12 { namespace cowshed_site {
constexpr size_t kBarnObject      = 92;      // kapali ambar: mapShared02 mesh 33, 39.8 x 25.0 m, yukseklik 11.8 m
constexpr float  kBarnX = 102.3f, kBarnZ = 199.8f;
constexpr size_t kConveyorObject  = 93;      // ambara giren 12.6 x 2.6 m, 5.5 m yuksek konveyor/rampa (ambarla birlikte kaldirilir)
constexpr size_t kDropPadObject   = 79;      // yerde 5 x 8 m isaret (x 71-76, z 196-204): eski saman satis noktasi -> ahirin balya birakma noktasi (gorunur kalir)
constexpr float  kDropPadX = 73.6f, kDropPadZ = 199.9f;
constexpr float  kSiloX = 43.1f;             // silo nesneleri #64, #65, #66
constexpr float  kDepotTipSiteX = 40.8f, kDepotTipSiteZ = 182.4f;   // FS12 TipSite 0 (depo)
constexpr float  kHerdX = 92.f, kHerdZ = 200.f;   // suru, balya birakma noktasina yakin
constexpr float  kHerdY = 0.f;               // TODO: arazi yuksekligi (zemin); haritadaki yukseklik verisinden okunacak
constexpr float  kHerdYaw = 0.f;
}}
