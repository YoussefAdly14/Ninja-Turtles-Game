#pragma once

// The contract between game logic and models: the logic (Turtle/Villain/Cutscene) fills a Pose,
// the model function reads it to place its joints. Logic never touches parts; models never touch HP.
struct Pose
{
    float time = 0.0f;        // seconds alive: idle bob, blinking, glow pulses
    float walkCycle = 0.0f;   // grows while walking; use sin(walkCycle) for legs/arms swing
    float attack = 0.0f;      // 0 = rest, 0..1 = progress through the current swing/throw/shot/punch
    bool  ducking = false;
    bool  inAir = false;
    float hitFlash = 0.0f;    // 1 right after being hit, fades to 0 (block recoil, flinch)
};
