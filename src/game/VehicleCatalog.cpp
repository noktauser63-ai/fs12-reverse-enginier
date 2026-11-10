#include "game/VehicleCatalog.h"
#include <cstring>
namespace fs12 {
// Segmentler: Starter / Mid / Top. Orijinal degerler: K420 100 PS 50 km/h, X720 262 PS 60 km/h (cikarildi), 5465H 28 km/h, 7545 30 km/h (cikarildi).
// EXT araclarinin guc/tork/fiyat degerleri segmente gore bu projede ayarlandi (tork ~ PS*3.9 Nm, Starter < Mid < Top).
static const VehicleSpec kSpecs[] = {
 // ---- traktorler ----
 {"agrotronK420", "Agrotron K 420", "Deutz-Fahr", VKind::Tractor, VSegment::Starter, true, 92000, 100, 73, 380, 50, 0, 0,
  "objects/deutzAgrotronK420_frame.p3d", "objects/objectsShared_diffuse.p2d", "objects/objectsShared_diffuse.p2d",
  {{"objects/deutzAgrotronK420_wheelFront.p3d", -0.9862f, 0.7539f, 1.4231f, 0.7539f, true, nullptr},
   {"objects/deutzAgrotronK420_wheelFront.p3d", 0.9862f, 0.7539f, 1.4231f, 0.7539f, true, nullptr},
   {"objects/deutzAgrotronK420_wheelBack.p3d", -0.9643f, 0.9661f, -1.4394f, 0.9661f, false, nullptr},
   {"objects/deutzAgrotronK420_wheelBack.p3d", 0.9643f, 0.9661f, -1.4394f, 0.9661f, false, nullptr}}, 4,
  {"objects/deutzAgrotronK420_frame2.p3d", "objects/deutzAgrotronK420_fenderLeft.p3d", "objects/deutzAgrotronK420_fenderRight.p3d", nullptr}, "deutzAgrotronK420"},
 {"ursus11024", "Ursus 11024", "Ursus", VKind::Tractor, VSegment::Mid, false, 165000, 160, 118, 640, 45, 0, 0,
  "objects/ursus11024.p3d", "objects/ursus11024_diffuse.p2d", "objects/ursus11024Wheel_diffuse.p2d",
  {{"objects/ursus11024_leftWheelFront.p3d", 1.05f, 0.76f, 1.30f, 0.76f, true, "objects/ursus11024_leftFender.p3d"},
   {"objects/ursus11024_rightWheelFront.p3d", -1.05f, 0.76f, 1.30f, 0.76f, true, "objects/ursus11024_rightFender.p3d"},
   
   {"objects/ursus11024_rightWheelBack.p3d", 0.f, 0.82f, -1.25f, 0.82f, false, nullptr}}, 3, {}, "ursus_11024_traktor"},
 {"magnum", "Magnum", "Case IH", VKind::Tractor, VSegment::Top, false, 345000, 310, 228, 1250, 60, 0, 0,
  "objects/magnum.p3d", "objects/magnum_diffuse.p2d", "objects/magnumWheel_diffuse.p2d",
  {{"objects/magnum_leftWheelFront.p3d", 1.1f, 0.79f, 2.35f, 0.79f, true, "objects/magnum_leftFender.p3d"},
   {"objects/magnum_rightWheelFront.p3d", -1.1f, 0.79f, 2.35f, 0.79f, true, "objects/magnum_rightFender.p3d"},
   {"objects/magnum_leftWheelBack.p3d", 1.2f, 1.02f, -0.6f, 1.02f, false, nullptr},
   {"objects/magnum_rightWheelBack.p3d", -1.2f, 1.02f, -0.6f, 1.02f, false, nullptr}}, 4, {}, "case_magnum_traktor"},
 // ---- bicerdoverler (7545 RTS/RTSC yerine; tablalar ayri parca: takilip cikarilir) ----
 {"deutz5465H", "5465 H", "Deutz-Fahr", VKind::Combine, VSegment::Starter, true, 220000, 150, 110, 0, 28, 4200, 3.0f,
  "objects/deutz5465_frame.p3d", "objects/objectsShared_diffuse.p2d", "objects/objectsShared_diffuse.p2d",
  {{"objects/deutz5465_wheelFrontRight.p3d", -1.2358f, 0.9243f, 2.1171f, 0.9243f, false, nullptr},
   {"objects/deutz5465_wheelFrontLeft.p3d", 1.2358f, 0.9243f, 2.1171f, 0.9243f, false, nullptr},
   {"objects/deutz5465_wheelBackRight.p3d", -0.8546f, 0.5624f, -1.4079f, 0.5624f, false, nullptr},
   {"objects/deutz5465_wheelBackLeft.p3d", 0.8546f, 0.5624f, -1.4079f, 0.5624f, false, nullptr}}, 4,
  {}, "deutz5465", {{"objects/deutz5465_pipe.p3d", 1.2417f, 2.8945f, 1.2983f}, {"objects/deutz5465_pipeUnloading.p3d", 1.2417f, 2.8945f, 1.2983f}, {"objects/deutz5465_cutter.p3d", 0.f, 1.3f, 3.6f}, {"objects/deutz5465_cutterReel.p3d", 0.f, 1.7f, 6.0f}}},
 {"deutz6095", "6095", "Deutz-Fahr", VKind::Combine, VSegment::Mid, false, 245000, 230, 169, 0, 30, 7000, 4.5f,
  "objects/deutz6095.p3d", "objects/deutz6095_diffuse.p2d", "objects/deutz6095Wheel_diffuse.p2d",
  {{"objects/deutz6095_rightWheelFront.p3d", 0.f, 0.86f, 0.95f, 0.86f, false, nullptr},
   {"objects/deutz6095_leftWheelBack.p3d", -1.15f, 0.67f, -4.2f, 0.67f, false, nullptr},
   {"objects/deutz6095_rightWheelBack.p3d", 1.15f, 0.67f, -4.2f, 0.67f, false, nullptr}}, 3, {}, "deutz_6095_bicerdover",
  {{"objects/deutz6095_cutterAttacher.p3d", 0.f, 1.2f, 1.0f}, {"objects/deutz7545_cutter.p3d", 0.f, 1.3f, 3.1f, 0.534f, "objects/objectsShared_diffuse.p2d"}, {"objects/deutz7545_cutterReel.p3d", 0.f, 1.7f, 5.5f, 0.534f, "objects/objectsShared_diffuse.p2d"}}},
 {"case9230", "Axial-Flow 9230", "Case IH", VKind::Combine, VSegment::Top, false, 395000, 430, 316, 0, 32, 11500, 7.5f,
  "objects/case9230.p3d", "objects/case9230_diffuse.p2d", "objects/case9230Wheel_diffuse.p2d",
  {{"objects/case9230_rightWheelFront.p3d", 0.f, 0.93f, 0.9f, 0.93f, false, nullptr},
   {"objects/case9230_leftWheelBack.p3d", -1.4f, 0.81f, -2.8f, 0.81f, false, nullptr},
   {"objects/case9230_rightWheelBack.p3d", 1.4f, 0.81f, -2.8f, 0.81f, false, nullptr}}, 3, {}, "case_9230_bicerdover",
  {{"objects/case9230_cutterAttacher.p3d", 0.f, 1.1f, 1.8f}, {"objects/deutz7545_cutter.p3d", 0.f, 1.3f, 3.9f, 0.89f, "objects/objectsShared_diffuse.p2d"}, {"objects/deutz7545_cutterReel.p3d", 0.f, 1.7f, 6.3f, 0.89f, "objects/objectsShared_diffuse.p2d"}}},
 // ---- ekipman ----
 {"condor", "Condor", "Amazone", VKind::Planter, VSegment::Mid, false, 85000, 0, 0, 0, 0, 0, 12.0f,
  "objects/amazoneCondor_frame.p3d", "objects/amazoneCondor_diffuse.p2d", "objects/amazoneCondor_diffuse.p2d",
  {{"objects/amazoneCondor_rightWheelBack.p3d", 0.f, 0.65f, -3.5f, 0.65f, false, nullptr}}, 1, {}, nullptr,
  {{"objects/amazoneCondor_frame3.p3d", 0.f, 1.0f, -1.5f}, {"objects/amazoneCondor_frame4.p3d", 0.f, 1.0f, -1.5f}, {"objects/amazoneCondor_frame2.p3d", 0.f, 0.9f, 1.5f}}},
 {"vaederstad", "Vaederstad", "Vaederstad", VKind::Cultivator, VSegment::Mid, false, 68000, 0, 0, 0, 0, 0, 7.3f,
  "objects/vaederstad_frame.p3d", "objects/vaederstad_diffuse.p2d", "objects/vaederstadWheel_diffuse.p2d",
  {{"objects/vaederstad_leftWheelFront.p3d", 0.9f, 0.4f, -2.0f, 0.4f, false, nullptr},
   {"objects/vaederstad_rightWheelFront.p3d", -0.9f, 0.4f, -2.0f, 0.4f, false, nullptr},
   {"objects/vaederstad_leftWheelBack.p3d", 0.9f, 0.4f, -3.4f, 0.4f, false, nullptr},
   {"objects/vaederstad_rightWheelBack.p3d", -0.9f, 0.4f, -3.4f, 0.4f, false, nullptr}}, 4, {}, nullptr,
  {{"objects/vaederstad_frame3.p3d", 0.f, 0.75f, -1.0f}, {"objects/vaederstad_frame4.p3d", 0.f, 0.75f, -1.0f}, {"objects/vaederstad_frame2.p3d", 0.f, 0.5f, 0.8f}}},
 {"pickup", "Pickup", "Lizard", VKind::Pickup, VSegment::Starter, false, 20000, 120, 88, 300, 90, 0, 0,
  "objects/pickupTruck.p3d", "objects/pickupTruck_diffuse.p2d", "objects/pickupTruck_diffuse.p2d",
  {{"objects/pickupTruck_leftWheelFront.p3d", 0.95f, 0.43f, 1.55f, 0.43f, true, nullptr},
   {"objects/pickupTruck_rightWheelFront.p3d", -0.95f, 0.43f, 1.55f, 0.43f, true, nullptr},
   {"objects/pickupTruck_rightWheelBack.p3d", 0.f, 0.43f, -1.95f, 0.43f, false, nullptr}}, 3, {}, "pickup_kamyonet"},
};
const VehicleSpec* vehicleSpecs(size_t& n) { n = sizeof(kSpecs) / sizeof(kSpecs[0]); return kSpecs; }
const VehicleSpec* findVehicle(const char* id) {
    size_t n; const VehicleSpec* v = vehicleSpecs(n);
    for (size_t i = 0; i < n; ++i) if (!strcmp(v[i].id, id)) return &v[i];
    return nullptr;
}
}
