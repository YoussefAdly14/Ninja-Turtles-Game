#include "game/Villain.h"
#include "game/World.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "models/VillainModels.h"
#include "Config.h"

Villain makeVillain(VillainKind kind, int lane, int id)
{
    Villain v;
    v.id = id;
    v.kind = kind;
    v.lane = lane;
    v.x = VILLAIN_SPAWN_X;
    v.speed = VILLAIN_WALK_SPEED;
    switch (kind) {
    case VillainKind::FootSoldier:
        v.width = FOOT_WIDTH;  v.height = FOOT_HEIGHT;  v.maxHp = FOOT_HP;
        break;
    case VillainKind::Bebop:
        v.width = BEBOP_WIDTH; v.height = BEBOP_HEIGHT; v.maxHp = BEBOP_HP;
        break;
    case VillainKind::Rocksteady:
        v.width = ROCKSTEADY_WIDTH; v.height = ROCKSTEADY_HEIGHT; v.maxHp = ROCKSTEADY_HP;
        v.speed = VILLAIN_WALK_SPEED * ROCKSTEADY_SPEED_MULTIPLIER;
        break;
    default: break;
    }
    v.hp = v.maxHp;
    v.attackTimer = 0.0f;   // attack as soon as it appears (req 47)
    return v;
}

const char* villainName(VillainKind kind)
{
    switch (kind) {
    case VillainKind::FootSoldier: return "Foot Soldier";
    case VillainKind::Bebop:       return "Bebop";
    case VillainKind::Rocksteady:  return "Rocksteady";
    default:                       return "";
    }
}

// TODO(B): first pass so the skeleton runs. Owner B finishes reqs 86, 88, 90, 96.
void Villain::update(float dt, World& world)
{
    age += dt;
    if (hitFlash > 0.0f) hitFlash -= dt * 4.0f;
    if (attackAnim > 0.0f) attackAnim -= dt * 3.0f;

    // walk left toward the Lair unless fighting
    if (!fighting) {
        x -= speed * dt;
        walkCycle += dt * 8.0f;
    }

    attackTimer -= dt;
    if (attackTimer <= 0.0f) {
        attackAnim = 1.0f;
        switch (kind) {
        case VillainKind::FootSoldier:
            attackTimer = FOOT_ATTACK_INTERVAL;
            world.spawnProjectile(ProjectileKind::Shuriken, lane, left(), false);
            break;
        case VillainKind::Bebop:
            attackTimer = BEBOP_ATTACK_INTERVAL;
            world.spawnProjectile(ProjectileKind::BlasterBolt, lane, left(), false);
            break;
        case VillainKind::Rocksteady:
            attackTimer = ROCKSTEADY_ATTACK_INTERVAL;
            // TODO(B): ROCKSTEADY_DAMAGE to the nearest turtle within ROCKSTEADY_REACH (req 90, OQ4)
            break;
        default: break;
        }
    }
}

Pose Villain::pose() const
{
    Pose p;
    p.time = age;
    p.walkCycle = walkCycle;
    p.attack = attackAnim > 0.0f ? 1.0f - attackAnim : 0.0f;
    p.hitFlash = hitFlash > 0.0f ? hitFlash : 0.0f;
    return p;
}

void Villain::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom());
    Renderer::setDamageTint(damageTint());
    VillainModels::draw(kind, base, pose());
    Renderer::setDamageTint(0.0f);
}
