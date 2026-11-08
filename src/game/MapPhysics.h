// MapPhysics: Box2D 2.1.2 dunyasi - haritadaki statik cokgenler (m3d npoly bolumu: konum (x,z) + yerel kose noktalari) ve arac govdesi.
// 2B eksenler: Box2D (x,y) = dunya (x,z). Box2D acisi = -yaw (arac ileri = (sin yaw, cos yaw)).
#pragma once
#include "game/MapData.h"
#include <memory>

class b2World;
class b2Body;

namespace fs12 {
class MapPhysics {
public:
    MapPhysics();
    ~MapPhysics();
    void build(const MapData& map);                       // statik cokgenler
    void createVehicle(float x, float z, float yaw, float halfW, float halfL, float centerZ);
    // istenen hiz/donus ile adim at; sonuc: konum, yaw, ileri hiz
    void step(float dt, float desiredSpeed, float yawRate);
    float x() const; float z() const; float yaw() const; float forwardSpeed() const;
    int staticCount() const { return m_static; }
    int contacts() const;
private:
    std::unique_ptr<b2World> m_world;
    b2Body* m_veh = nullptr;
    int m_static = 0;
};
} // namespace fs12
