#include "game/Projectile.h"
#include "core/Transform.h"
#include "models/PropModels.h"
#include "Config.h"

Projectile makeProjectile(ProjectileKind kind, int lane, float x, bool fromTurtle, int id)
{
    Projectile p;
    p.id = id;
    p.kind = kind;
    p.lane = lane;
    p.x = x;
    p.fromTurtle = fromTurtle;
    p.width = PROJECTILE_SIZE;
    p.height = PROJECTILE_SIZE;
    switch (kind) {
    case ProjectileKind::Sai:
        p.band = AttackBand::Mid;  p.velocity = SAI_SPEED;       p.damage = TURTLE_DAMAGE;  p.yOffset = MID_SHOT_HEIGHT;
        break;
    case ProjectileKind::Shuriken:
        p.band = AttackBand::High; p.velocity = -SHURIKEN_SPEED; p.damage = VILLAIN_DAMAGE; p.yOffset = HIGH_SHOT_HEIGHT;
        break;
    case ProjectileKind::BlasterBolt:
        p.band = AttackBand::Low;  p.velocity = -BLASTER_SPEED;  p.damage = VILLAIN_DAMAGE; p.yOffset = LOW_SHOT_HEIGHT;
        break;
    }
    return p;
}

void Projectile::update(float dt)
{
    age += dt;
    x += velocity * dt;
    spin += 720.0f * dt;

    // left the playfield: just disappear (req 50). Only a villain's BODY at the Lair loses the game.
    if (right() < LAIR_WIDTH || left() > WORLD_WIDTH) alive = false;
}

void Projectile::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom() + height * 0.5f);
    switch (kind) {
    case ProjectileKind::Sai:         PropModels::drawSai(base, spin);      break;
    case ProjectileKind::Shuriken:    PropModels::drawShuriken(base, spin); break;
    case ProjectileKind::BlasterBolt: PropModels::drawBlasterBolt(base, age); break;
    }
}
