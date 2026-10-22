#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "game/Types.h"

// Short visual effects: ninja smoke when a turtle is placed (req 68), hit sparks (req 120) and
// defeat effects (req 83 optional part, 120). Purely visual: entities are still removed the same
// frame they die, these just play on where they were.
// Based on learnopengl In-Practice/2D-Game/Particles: a Particle { position, velocity, color, life }
// in a std::vector, moved by dt and dropped when its life runs out. Instead of a textured quad each
// particle is drawn from our building-block shapes (circles, quads, triangles, rings).
enum class ParticleKind
{
    Smoke,      // grey puff: big circle + lighter highlight circle, grows and fades
    Ring,       // expanding "poof" ring at the centre of a smoke burst
    Spark,      // thin streak along its velocity, shrinks (additive, like the Particles page)
    Flash,      // bright core of a hit: circle + 4 tiny rays, very short (additive)
    Shell,      // defeated turtle: its shell spins up and shrinks away
    Ribbon,     // defeated turtle: mask tail fluttering down
    Shuriken,   // defeated Foot Soldier drops his shuriken
    Blaster,    // defeated Bebop drops his arm blaster
    Shard       // defeated Rocksteady: helmet / armour pieces tumble off
};

struct Particle
{
    ParticleKind kind = ParticleKind::Smoke;
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 velocity = glm::vec2(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
    float size = 10.0f;       // start size (diameter / length)
    float endScale = 1.0f;    // size multiplier reached at the end of life (smoke grows, sparks shrink)
    float angle = 0.0f;       // degrees
    float spin = 0.0f;        // degrees/s
    float gravity = 0.0f;     // units/s^2 pulling down
    float floorY = -1.0e6f;   // falling pieces stop here (the lane floor)
    float life = 0.0f;        // seconds left (Particles page: Life)
    float maxLife = 1.0f;
};

class Effects
{
public:
    void reset();                 // Restart / new game: no leftovers
    void update(float dt);        // only from World::update, so effects freeze while paused
    void draw() const;            // after projectiles, before health bars

    // req 68: ninja smoke burst, e.g. a turtle appearing in its lane
    void smokePuff(glm::vec2 at, float radius, glm::vec3 tint = glm::vec3(0.88f));
    // req 120: sparks flying in `direction` (+1 = right, -1 = left) from a hit point
    void hitSparks(glm::vec2 at, float direction, glm::vec4 color, float strength = 1.0f);
    // req 83/120: defeat effects at the character's body centre
    void turtleDefeat(TurtleKind kind, glm::vec2 at, float floorY);
    void villainDefeat(VillainKind kind, glm::vec2 at, float height, float floorY);

private:
    std::vector<Particle> m_particles;

    void spawn(const Particle& p);
};
