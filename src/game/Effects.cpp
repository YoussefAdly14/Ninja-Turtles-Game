#include "game/Effects.h"
#include "core/Renderer.h"
#include "core/Transform.h"
#include "core/Colors.h"
#include "core/Random.h"
#include "models/PropModels.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

// ---------------- spawning ----------------

void Effects::reset()
{
    m_particles.clear();
}

void Effects::spawn(const Particle& p)
{
    if ((int)m_particles.size() >= EFFECTS_MAX_PARTICLES) return;   // busy wave: skip extras, never grow forever
    m_particles.push_back(p);
}

static glm::vec2 fromAngle(float degrees)
{
    float r = glm::radians(degrees);
    return glm::vec2(std::cos(r), std::sin(r));
}

static glm::vec4 smokeColor(glm::vec3 tint)
{
    float shade = randomFloat(0.75f, 1.0f);   // a few darker puffs so the cloud has depth
    return glm::vec4(tint * shade, SMOKE_PUFF_ALPHA);
}

void Effects::smokePuff(glm::vec2 at, float radius, glm::vec3 tint)
{
    // quick "poof" ring first
    Particle ring;
    ring.kind = ParticleKind::Ring;
    ring.position = at;
    ring.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.7f);
    ring.size = radius * 0.8f;
    ring.endScale = 2.6f;
    ring.life = ring.maxLife = SMOKE_PUFF_DURATION * 0.55f;
    spawn(ring);

    // a ring of puffs bursting outwards, rising a little (smoke is light)
    for (int i = 0; i < SMOKE_PUFF_COUNT; ++i) {
        glm::vec2 dir = fromAngle(360.0f * i / SMOKE_PUFF_COUNT + randomFloat(-15.0f, 15.0f));
        Particle p;
        p.kind = ParticleKind::Smoke;
        p.position = at + dir * radius * 0.3f;
        p.velocity = dir * SMOKE_PUFF_SPEED * randomFloat(0.6f, 1.0f) + glm::vec2(0.0f, 30.0f);
        p.color = smokeColor(tint);
        p.size = radius * randomFloat(0.5f, 0.75f);
        p.endScale = 1.8f;
        p.life = p.maxLife = SMOKE_PUFF_DURATION * randomFloat(0.7f, 1.0f);
        spawn(p);
    }

    // a few slow puffs in the middle that hide whatever appears / vanishes there
    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.kind = ParticleKind::Smoke;
        p.position = at + glm::vec2(randomFloat(-0.25f, 0.25f), randomFloat(-0.35f, 0.35f)) * radius;
        p.velocity = glm::vec2(randomFloat(-15.0f, 15.0f), 25.0f);
        p.color = smokeColor(tint);
        p.size = radius * randomFloat(0.8f, 1.0f);
        p.endScale = 1.4f;
        p.life = p.maxLife = SMOKE_PUFF_DURATION * randomFloat(0.8f, 1.0f);
        spawn(p);
    }
}

void Effects::hitSparks(glm::vec2 at, float direction, glm::vec4 color, float strength)
{
    float baseAngle = direction >= 0.0f ? 0.0f : 180.0f;

    Particle flash;
    flash.kind = ParticleKind::Flash;
    flash.position = at;
    flash.color = color;
    flash.size = 22.0f * strength;
    flash.endScale = 1.6f;
    flash.angle = randomFloat(0.0f, 90.0f);
    flash.life = flash.maxLife = HIT_SPARK_DURATION * 0.5f;
    spawn(flash);

    int count = (int)(HIT_SPARK_COUNT * strength);
    for (int i = 0; i < count + 2; ++i) {
        // most fly on through the target, two bounce back towards the attacker
        bool back = i >= count;
        float angle = baseAngle + (back ? 180.0f : 0.0f) + randomFloat(-65.0f, 65.0f);
        Particle p;
        p.kind = ParticleKind::Spark;
        p.position = at;
        p.velocity = fromAngle(angle) * HIT_SPARK_SPEED * randomFloat(0.5f, 1.0f) * (back ? 0.5f : strength);
        p.color = glm::mix(color, WHITE, randomFloat(0.0f, 0.4f));
        p.size = randomFloat(12.0f, 22.0f) * strength;
        p.endScale = 0.2f;
        p.gravity = EFFECT_GRAVITY * 0.6f;   // slight arc down
        p.life = p.maxLife = HIT_SPARK_DURATION * randomFloat(0.6f, 1.0f);
        spawn(p);
    }
}

static glm::vec4 maskColor(TurtleKind kind)
{
    switch (kind) {
    case TurtleKind::Raphael:   return RAPHAEL_MASK;
    case TurtleKind::Donatello: return DONATELLO_MASK;
    default:                    return LEONARDO_MASK;
    }
}

// A piece that pops up, falls to the lane floor, bounces once and shrinks away.
static Particle fallingPiece(ParticleKind kind, glm::vec2 at, glm::vec2 velocity, float size, float spin, float floorY)
{
    Particle p;
    p.kind = kind;
    p.position = at;
    p.velocity = velocity;
    p.size = size;
    p.spin = spin;
    p.angle = randomFloat(0.0f, 360.0f);
    p.gravity = EFFECT_GRAVITY;
    p.floorY = floorY;
    p.life = p.maxLife = DEFEAT_EFFECT_DURATION;
    return p;
}

void Effects::turtleDefeat(TurtleKind kind, glm::vec2 at, float floorY)
{
    smokePuff(at, TURTLE_HEIGHT * 0.45f, glm::vec3(0.85f));

    // the turtle pulls into its shell, which spins up and away
    Particle shell;
    shell.kind = ParticleKind::Shell;
    shell.position = at;
    shell.velocity = glm::vec2(randomFloat(-20.0f, 20.0f), 260.0f);
    shell.gravity = EFFECT_GRAVITY * 0.6f;
    shell.floorY = floorY + 25.0f;
    shell.size = TURTLE_WIDTH;
    shell.spin = 900.0f;
    shell.life = shell.maxLife = DEFEAT_EFFECT_DURATION;
    spawn(shell);

    // the two mask tails flutter down in the turtle's colour
    for (int i = 0; i < 2; ++i) {
        glm::vec2 v(randomFloat(-90.0f, -30.0f) * (i == 0 ? 1.0f : -0.6f), randomFloat(120.0f, 200.0f));
        Particle r = fallingPiece(ParticleKind::Ribbon, at + glm::vec2(0.0f, TURTLE_HEIGHT * 0.3f), v, 26.0f,
                                  randomFloat(-300.0f, 300.0f), floorY + 4.0f);
        r.gravity = EFFECT_GRAVITY * 0.5f;   // cloth falls slowly
        r.color = maskColor(kind);
        spawn(r);
    }
}

void Effects::villainDefeat(VillainKind kind, glm::vec2 at, float height, float floorY)
{
    // Foot ninjas vanish in dusky purple-grey smoke, the mutants in sewer dust
    glm::vec3 tint = kind == VillainKind::FootSoldier ? glm::vec3(0.72f, 0.66f, 0.78f) : glm::vec3(0.78f, 0.74f, 0.66f);
    smokePuff(at, height * 0.45f, tint);

    // their weapon drops (they face left, so it's flung back to the right)
    glm::vec2 hand = at + glm::vec2(-10.0f, height * 0.1f);
    switch (kind) {
    case VillainKind::FootSoldier:
        spawn(fallingPiece(ParticleKind::Shuriken, hand, glm::vec2(randomFloat(60.0f, 120.0f), 260.0f),
                           PROJECTILE_SIZE, 720.0f, floorY + 8.0f));
        break;
    case VillainKind::Bebop:
        spawn(fallingPiece(ParticleKind::Blaster, hand, glm::vec2(randomFloat(60.0f, 120.0f), 240.0f),
                           1.0f, 360.0f, floorY + 8.0f));
        break;
    case VillainKind::Rocksteady:
        // helmet and shoulder-armour pieces burst off
        for (int i = 0; i < 4; ++i) {
            glm::vec2 v(randomFloat(-140.0f, 160.0f), randomFloat(200.0f, 330.0f));
            Particle s = fallingPiece(ParticleKind::Shard, at + glm::vec2(randomFloat(-20.0f, 20.0f), height * 0.3f),
                                      v, randomFloat(14.0f, 22.0f), randomFloat(-500.0f, 500.0f), floorY + 5.0f);
            s.color = i % 2 == 0 ? hexColor(0x8C8C8C) : MUTANT_GRAY;
            spawn(s);
        }
        break;
    default:
        break;
    }
}

// ---------------- update (Particles page: life -= dt, position += velocity * dt) ----------------

void Effects::update(float dt)
{
    for (Particle& p : m_particles) {
        p.life -= dt;
        p.velocity.y -= p.gravity * dt;
        if (p.kind == ParticleKind::Smoke) p.velocity *= 1.0f - std::min(1.0f, 4.0f * dt);   // smoke slows down fast
        p.position += p.velocity * dt;
        p.angle += p.spin * dt;

        // falling pieces land on the lane floor and bounce once, softly
        if (p.gravity > 0.0f && p.position.y < p.floorY && p.velocity.y < 0.0f) {
            p.position.y = p.floorY;
            p.velocity.y *= -0.35f;
            p.velocity.x *= 0.5f;
            p.spin *= 0.4f;
        }
    }
    // dead particles go after the loop, never inside it (ARCHITECTURE.md)
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(),
                                     [](const Particle& p) { return p.life <= 0.0f; }),
                      m_particles.end());
}

// ---------------- drawing: each particle is several shapes ----------------

// solid pieces keep their colour and shrink away at the end (alpha on overlapping parts would show seams)
static float shrinkAway(float progress)
{
    const float START = 0.65f;
    return progress < START ? 1.0f : 1.0f - (progress - START) / (1.0f - START);
}

static void drawParticle(const Particle& p)
{
    float progress = 1.0f - p.life / p.maxLife;      // 0 -> 1 over its life
    float fade = 1.0f - progress;
    float s = p.size * (1.0f + (p.endScale - 1.0f) * progress);
    glm::mat4 base = rotate2D(translate2D(glm::mat4(1.0f), p.position.x, p.position.y), p.angle);

    switch (p.kind) {
    case ParticleKind::Smoke: {
        // puff + lighter top-left highlight + darker bottom lobe = a cartoon cloud
        float a = p.color.a * fade;
        glm::vec3 c(p.color);
        Renderer::draw(Shape::Circle, part(base, 0.0f, -s * 0.12f, s * 0.8f, s * 0.6f), glm::vec4(c * 0.8f, a));
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, s, s), glm::vec4(c, a));
        Renderer::draw(Shape::Circle, part(base, -s * 0.15f, s * 0.15f, s * 0.5f, s * 0.5f),
                       glm::vec4(glm::mix(c, glm::vec3(1.0f), 0.5f), a));
        break;
    }
    case ParticleKind::Ring:
        Renderer::draw(Shape::Ring, part(base, 0.0f, 0.0f, s, s), withAlpha(p.color, p.color.a * fade));
        Renderer::draw(Shape::Ring, part(base, 0.0f, 0.0f, s * 0.7f, s * 0.7f), withAlpha(p.color, p.color.a * fade * 0.5f));
        break;
    case ParticleKind::Spark: {
        // streak trailing behind a bright head, pointing where it flies
        float deg = glm::degrees(std::atan2(p.velocity.y, p.velocity.x));
        glm::mat4 dir = rotate2D(translate2D(glm::mat4(1.0f), p.position.x, p.position.y), deg);
        Renderer::draw(Shape::Quad, part(dir, -s * 0.5f, 0.0f, s, 3.5f), withAlpha(p.color, fade));
        Renderer::draw(Shape::Circle, part(dir, 0.0f, 0.0f, 6.0f, 6.0f), withAlpha(glm::mix(p.color, WHITE, 0.6f), fade));
        break;
    }
    case ParticleKind::Flash:
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, s * 1.7f, s * 1.7f), withAlpha(p.color, 0.45f * fade));
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, s, s), withAlpha(WHITE, fade));
        for (int i = 0; i < 4; ++i) {
            glm::mat4 ray = rotate2D(base, 90.0f * i);
            Renderer::draw(Shape::Triangle, part(ray, 0.0f, s * 0.8f, s * 0.2f, s * 0.8f), withAlpha(p.color, fade));
        }
        break;
    case ParticleKind::Shell: {
        // back of a turtle shell: plastron-coloured edge, brown dome, darker plates
        float k = s * shrinkAway(progress);
        glm::vec4 plate = hexColor(0x5E4522);
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, k, k * 0.74f), PLASTRON);
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, k * 0.88f, k * 0.62f), SHELL);
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, k * 0.3f, k * 0.24f), plate);
        Renderer::draw(Shape::Circle, part(base, -k * 0.27f, 0.0f, k * 0.2f, k * 0.2f), plate);
        Renderer::draw(Shape::Circle, part(base, k * 0.27f, 0.0f, k * 0.2f, k * 0.2f), plate);
        Renderer::draw(Shape::Circle, part(base, 0.0f, k * 0.2f, k * 0.18f, k * 0.12f), plate);
        Renderer::draw(Shape::Circle, part(base, 0.0f, -k * 0.2f, k * 0.18f, k * 0.12f), plate);
        break;
    }
    case ParticleKind::Ribbon: {
        // two little tails from a knot
        float k = s * shrinkAway(progress);
        Renderer::draw(Shape::Quad, part(base, k * 0.45f, k * 0.08f, k, k * 0.22f, 12.0f), p.color);
        Renderer::draw(Shape::Quad, part(base, k * 0.4f, -k * 0.12f, k * 0.85f, k * 0.2f, -18.0f), p.color);
        Renderer::draw(Shape::Circle, part(base, 0.0f, 0.0f, k * 0.3f, k * 0.3f), p.color * glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        break;
    }
    case ParticleKind::Shuriken: {
        float k = shrinkAway(progress);
        PropModels::drawShuriken(scale2D(translate2D(glm::mat4(1.0f), p.position.x, p.position.y), k, k), p.angle);
        break;
    }
    case ParticleKind::Blaster: {
        // Bebop's arm blaster: body, barrel, glowing muzzle, grip, power cell
        float k = shrinkAway(progress);
        glm::mat4 g = scale2D(base, k, k);
        Renderer::draw(Shape::Quad, part(g, 8.0f, -10.0f, 8.0f, 14.0f, -15.0f), hexColor(0x3A3A3A));
        Renderer::draw(Shape::Quad, part(g, 0.0f, 0.0f, 34.0f, 14.0f), hexColor(0x5A5A5A));
        Renderer::draw(Shape::Quad, part(g, -24.0f, 2.0f, 18.0f, 7.0f), hexColor(0x2E2E2E));
        Renderer::draw(Shape::Circle, part(g, -34.0f, 2.0f, 9.0f, 9.0f), hexColor(0xFF8A3D));
        Renderer::draw(Shape::Quad, part(g, 6.0f, 2.0f, 10.0f, 6.0f), hexColor(0xFFB25C));
        break;
    }
    case ParticleKind::Shard: {
        float k = s * shrinkAway(progress);
        Renderer::draw(Shape::Triangle, part(base, 0.0f, 0.0f, k, k * 0.75f), p.color);
        Renderer::draw(Shape::Triangle, part(base, 0.0f, -k * 0.1f, k * 0.5f, k * 0.35f), p.color * glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
        break;
    }
    }
}

void Effects::draw() const
{
    // 1. smoke (normal alpha blending), 2. solid pieces on top so a spinning shell reads through the smoke
    for (const Particle& p : m_particles)
        if (p.kind == ParticleKind::Smoke || p.kind == ParticleKind::Ring) drawParticle(p);
    for (const Particle& p : m_particles)
        if (p.kind != ParticleKind::Smoke && p.kind != ParticleKind::Ring
            && p.kind != ParticleKind::Spark && p.kind != ParticleKind::Flash) drawParticle(p);

    // 3. sparks with additive blending so they glow, then back to normal (learnopengl 2D-Game/Particles)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const Particle& p : m_particles)
        if (p.kind == ParticleKind::Spark || p.kind == ParticleKind::Flash) drawParticle(p);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
