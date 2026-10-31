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

// Movement + attack timing for one villain (reqs 47, 86, 88, 90). Damage is applied in Combat::resolve.
void Villain::update(float dt, World& world)
{
    age += dt;
    if (hitFlash > 0.0f) hitFlash -= dt / HIT_FLASH_DURATION;
    if (attackAnim > 0.0f) attackAnim -= dt / ATTACK_ANIM_DURATION;
    meleeStrike = false;

    // walk left toward the Lair unless fighting (req 48); villains pass through villains (req 51, OQ8)
    if (!fighting) {
        x -= speed * dt;
        walkCycle += dt * WALK_CYCLE_SPEED;
    }

    // strike frame: the shuriken/bolt leaves the hand or the punch lands
    if (strikePending && 1.0f - attackAnim >= STRIKE_POINT) {
        strikePending = false;
        switch (kind) {
        case VillainKind::FootSoldier:   // HIGH shuriken, duck under it (req 86)
            world.spawnProjectile(ProjectileKind::Shuriken, lane, left(), false);
            break;
        case VillainKind::Bebop:         // LOW blaster bolt, jump over it (req 88)
            world.spawnProjectile(ProjectileKind::BlasterBolt, lane, left(), false);
            break;
        case VillainKind::Rocksteady:    // melee: Combat hits the nearest turtle in reach for 2 (req 90, OQ4)
            meleeStrike = true;
            break;
        default: break;
        }
    }

    // a new attack on an interval from the moment it appears, even with no turtle around (req 47)
    attackTimer -= dt;
    if (attackTimer <= 0.0f) {
        switch (kind) {
        case VillainKind::FootSoldier: attackTimer = FOOT_ATTACK_INTERVAL;       break;
        case VillainKind::Bebop:       attackTimer = BEBOP_ATTACK_INTERVAL;      break;
        case VillainKind::Rocksteady:  attackTimer = ROCKSTEADY_ATTACK_INTERVAL; break;
        default: break;
        }
        attackAnim = 1.0f;
        strikePending = true;
    }
}

Pose Villain::pose() const
{
    Pose p;
    p.time = age;
    p.walkCycle = fighting ? 0.0f : walkCycle;   // standing still: idle legs, no frozen stride or dust
    p.attack = attackAnim > 0.0f ? 1.0f - attackAnim : 0.0f;
    p.hitFlash = hitFlash > 0.0f ? hitFlash : 0.0f;
    return p;
}

void Villain::draw() const
{
    glm::mat4 base = translate2D(glm::mat4(1.0f), x, bottom());
    // redder as health drops + a short extra flash right after a hit (req 94)
    float tint = damageTint() + (hitFlash > 0.0f ? hitFlash * HIT_FLASH_TINT : 0.0f);
    Renderer::setDamageTint(tint > 1.0f ? 1.0f : tint);
    VillainModels::draw(kind, base, pose());
    Renderer::setDamageTint(0.0f);
}
