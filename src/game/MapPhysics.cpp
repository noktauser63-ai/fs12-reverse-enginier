#include "game/MapPhysics.h"
#include <Box2D/Box2D.h>
#include <cmath>
#include <cstring>

namespace fs12 {
MapPhysics::MapPhysics() : m_world(new b2World(b2Vec2(0.f, 0.f), true)) {}
MapPhysics::~MapPhysics() {}

void MapPhysics::build(const MapData& map) {
    for (const MapPolygon& g : map.polygons) {
        const int n = (int)(g.pts.size() / 2);
        if (n < 3 || n > b2_maxPolygonVertices) continue;
        float px, pz; memcpy(&px, &g.a, 4); memcpy(&pz, &g.b, 4);
        b2Vec2 v[b2_maxPolygonVertices];
        for (int i = 0; i < n; ++i) v[i].Set(g.pts[2 * i], g.pts[2 * i + 1]);   // orijinal Map::load gibi ham sira
        b2BodyDef bd; bd.position.Set(px, pz);
        b2Body* b = m_world->CreateBody(&bd);
        b2PolygonShape s; s.Set(v, n);
        b->CreateFixture(&s, 0.f);
        ++m_static;
    }
}

void MapPhysics::createVehicle(float x, float z, float yaw, float halfW, float halfL, float centerZ) {
    if (m_veh) { m_world->DestroyBody(m_veh); m_veh = nullptr; }
    b2BodyDef bd; bd.type = b2_dynamicBody; bd.position.Set(x, z); bd.angle = -yaw; bd.linearDamping = 0.f; bd.angularDamping = 0.f;
    m_veh = m_world->CreateBody(&bd);
    b2PolygonShape s; s.SetAsBox(halfW, halfL, b2Vec2(0.f, centerZ), 0.f);
    b2FixtureDef fd; fd.shape = &s; fd.density = 1.f; fd.friction = 0.3f; fd.restitution = 0.f;
    m_veh->CreateFixture(&fd);
}

void MapPhysics::step(float dt, float desiredSpeed, float yawRate) {
    if (!m_veh) return;
    const float th = m_veh->GetAngle();
    const b2Vec2 fwd(-std::sin(th), std::cos(th));      // yerel +y
    m_veh->SetLinearVelocity(b2Vec2(fwd.x * desiredSpeed, fwd.y * desiredSpeed));
    m_veh->SetAngularVelocity(-yawRate);
    m_world->Step(dt, 8, 3);
}
float MapPhysics::x() const { return m_veh ? m_veh->GetPosition().x : 0.f; }
float MapPhysics::z() const { return m_veh ? m_veh->GetPosition().y : 0.f; }
float MapPhysics::yaw() const { return m_veh ? -m_veh->GetAngle() : 0.f; }
float MapPhysics::forwardSpeed() const {
    if (!m_veh) return 0.f;
    const float th = m_veh->GetAngle(); const b2Vec2 v = m_veh->GetLinearVelocity();
    return v.x * -std::sin(th) + v.y * std::cos(th);
}
} // namespace fs12
int fs12::MapPhysics::contacts() const { return m_world->GetContactCount(); }
