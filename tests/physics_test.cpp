// Box2D 2.1.2 duman testi: kutu zemine dusup durmali.
#include <Box2D/Box2D.h>
#include <stdio.h>

int main() {
    b2World world(b2Vec2(0.f, -10.f), true);
    b2BodyDef gd; gd.position.Set(0.f, -10.f);
    b2Body* ground = world.CreateBody(&gd);
    b2PolygonShape gs; gs.SetAsBox(50.f, 10.f);
    ground->CreateFixture(&gs, 0.f);
    b2BodyDef bd; bd.type = b2_dynamicBody; bd.position.Set(0.f, 4.f);
    b2Body* body = world.CreateBody(&bd);
    b2PolygonShape bs; bs.SetAsBox(1.f, 1.f);
    b2FixtureDef fd; fd.shape = &bs; fd.density = 1.f; fd.friction = 0.3f;
    body->CreateFixture(&fd);
    for (int i = 0; i < 240; ++i) world.Step(1.f / 60.f, 8, 3);
    b2Vec2 p = body->GetPosition();
    printf("y=%.4f awake=%d version=%d.%d.%d\n", p.y, (int)body->IsAwake(), b2_version.major, b2_version.minor, b2_version.revision);
    return (p.y > 0.95f && p.y < 1.05f) ? 0 : 1;
}
