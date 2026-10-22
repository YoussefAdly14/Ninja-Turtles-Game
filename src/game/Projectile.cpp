#include "game/Projectile.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
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

void Projectile::impact()
{
    hasHit = true;
    velocity = 0.0f;
    impactTimer = PROJECTILE_IMPACT_DURATION;
}

void Projectile::update(float dt)
{
    age += dt;

    // already hit: stay put while the flash plays, then gone (req 50, 75)
    if (hasHit) {
        impactTimer -= dt;
        if (impactTimer <= 0.0f) alive = false;
        return;
    }

    x += velocity * dt;
    spin += PROJECTILE_SPIN_SPEED * dt;

    // left the playfield in the direction it flies: just disappear (req 50). Villains spawn just
    // off-screen on the right, so their shots start out there and must not be removed at birth.
    // Only a villain's BODY at the Lair loses the game.
    if ((velocity > 0.0f && left() > WORLD_WIDTH) || (velocity < 0.0f && right() < LAIR_WIDTH)) alive = false;
}

// Impact flash: the weapon shrinks away inside an expanding, fading ring with four sparks (req 75).
static void drawImpact(const glm::mat4& base, float progress, glm::vec4 color)
{
    float fade = 1.0f - progress;
    float size = PROJECTILE_SIZE * (0.6f + progress);
    Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, size * 0.6f, size * 0.6f), withAlpha(hexColor(0xFFFFFF), 0.8f * fade));
    Renderer::draw(Shape::Ring, part(base, 0.0f, 0.0f, size * 1.4f, size * 1.4f), withAlpha(color, fade));
    for (int i = 0; i < 4; ++i) {
        glm::mat4 ray = rotate2D(base, 45.0f + 90.0f * i);
        Renderer::draw(Shape::Triangle, part(ray, 0.0f, size * 0.9f, 6.0f, size * 0.6f), withAlpha(color, fade));
    }
}

void Projectile::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom() + height * 0.5f);

    glm::mat4 weapon = base;
    if (hasHit) {
        float progress = 1.0f - impactTimer / PROJECTILE_IMPACT_DURATION;
        glm::vec4 flash = kind == ProjectileKind::BlasterBolt ? hexColor(0xFF8A3D) : hexColor(0xFFE27A);
        drawImpact(base, progress, flash);
        weapon = scale2D(base, 1.0f - progress, 1.0f - progress);
    }

    switch (kind) {
    case ProjectileKind::Sai:         PropModels::drawSai(weapon, spin);      break;
    case ProjectileKind::Shuriken:    PropModels::drawShuriken(weapon, spin); break;
    case ProjectileKind::BlasterBolt: PropModels::drawBlasterBolt(weapon, age); break;
    }
}
