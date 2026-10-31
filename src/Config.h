#pragma once
// Every tunable number lives here (req 12, NF10). Rebalancing never touches logic code.
// Units: world units (the screen is always WORLD_WIDTH x WORLD_HEIGHT), seconds, units/second.
// Values marked OQn are open questions for the TA (context/docs/REQUIREMENTS.md): keep them one-line changes.

// ---------------- window / world ----------------
const int   WINDOW_WIDTH = 1280;
const int   WINDOW_HEIGHT = 720;
const char* const WINDOW_TITLE = "Shell Shock: Defend the Lair";
const float WORLD_WIDTH = 1600.0f;
const float WORLD_HEIGHT = 900.0f;
const float MAX_DELTA_TIME = 0.05f;      // dt clamp so dragging the window doesn't teleport things (req 5)
const unsigned int RANDOM_SEED = 0;      // 0 = random every run, any other value = reproducible run (req 13)
const bool  DEV_KEYS_ENABLED = true;     // F2-F5 testing keys (req 17). Set false before submission.
const int   DEV_COIN_BONUS = 100;        // F2

// ---------------- layout (y goes up, lane 0 is the TOP lane) ----------------
const int   LANE_COUNT = 4;                     // spec minimum is 3
const float HUD_HEIGHT = 140.0f;                // top bar: cards, wave, pizza counter
const float PLAYFIELD_BOTTOM = 60.0f;           // below this: sewer-water channel
const float PLAYFIELD_TOP = WORLD_HEIGHT - HUD_HEIGHT;
const float LANE_HEIGHT = (PLAYFIELD_TOP - PLAYFIELD_BOTTOM) / LANE_COUNT;
const float LANE_FLOOR_OFFSET = 22.0f;          // walkway surface above the lane's bottom edge
const float LAIR_WIDTH = 170.0f;                // home base at the left edge, spans all lanes
const float TURTLE_SPAWN_X = LAIR_WIDTH + 50.0f;
const float VILLAIN_SPAWN_X = WORLD_WIDTH + 60.0f;   // just off-screen, walks in

// ---------------- economy (OQ1) ----------------
const int START_COINS = 30;                     // enough for 3 Leonardos
const int LEONARDO_COST = 10;
const int RAPHAEL_COST = 60;                    // 2 x 30
const int DONATELLO_COST = 90;                  // 3 x 30
// pizza dropped by a defeated villain: harder villains and later waves pay more (balance, req 130)
const int PIZZA_VALUE = 15;                     // Foot Soldier (easy)
const int BEBOP_PIZZA_VALUE = 25;               // medium
const int ROCKSTEADY_PIZZA_VALUE = 40;          // hard
const int PIZZA_WAVE_BONUS = 5;                 // + this per wave after wave 1 (wave 2: +5, wave 3: +10)
const bool REFUND_TURTLE_ON_EXIT = true;        // a turtle that clears its lane and walks off the right edge pays back its cost

// ---------------- turtles ----------------
const float TURTLE_WIDTH = 70.0f;               // hitbox
const float TURTLE_HEIGHT = 120.0f;
const int   LEONARDO_HP = 5;
const int   RAPHAEL_HP = 5;
const int   DONATELLO_HP = 7;                   // "7 hits" (OQ6: treated as HP)
const int   TURTLE_DAMAGE = 1;
const float TURTLE_WALK_SPEED = 45.0f;
const float LEONARDO_ATTACK_INTERVAL = 0.8f;
const float LEONARDO_REACH = 25.0f;             // katana reach past the hitbox
const float RAPHAEL_ATTACK_INTERVAL = 1.4f;
const float JUMP_DURATION = 0.6f;
const float JUMP_HEIGHT = 110.0f;
const float DUCK_MIN_DURATION = 0.25f;          // OQ5: duck while held, at least this long
const float DUCK_HEIGHT_SCALE = 0.6f;           // hitbox height while ducking

// ---------------- villains ----------------
const float FOOT_WIDTH = 65.0f,        FOOT_HEIGHT = 125.0f;
const float BEBOP_WIDTH = 90.0f,       BEBOP_HEIGHT = 135.0f;
const float ROCKSTEADY_WIDTH = 105.0f, ROCKSTEADY_HEIGHT = 155.0f;
const int   FOOT_HP = 5;
const int   BEBOP_HP = 5;
const int   ROCKSTEADY_HP = 7;
const int   VILLAIN_DAMAGE = 1;
const int   ROCKSTEADY_DAMAGE = 2;
const float VILLAIN_WALK_SPEED = 40.0f;
const float ROCKSTEADY_SPEED_MULTIPLIER = 1.8f; // "runs"
const float FOOT_ATTACK_INTERVAL = 1.6f;
const float BEBOP_ATTACK_INTERVAL = 1.8f;
const float ROCKSTEADY_ATTACK_INTERVAL = 1.2f;
const float ROCKSTEADY_REACH = 20.0f;

// ---------------- projectiles ----------------
const float SAI_SPEED = 420.0f;
const float SHURIKEN_SPEED = 360.0f;
const float BLASTER_SPEED = 400.0f;
const float PROJECTILE_SIZE = 28.0f;            // hitbox (square)
const float HIGH_SHOT_HEIGHT = 95.0f;           // above the floor: hits standing, missed when ducking
const float MID_SHOT_HEIGHT = 60.0f;
const float LOW_SHOT_HEIGHT = 18.0f;            // skims the floor: hits standing, missed when jumping
const float PROJECTILE_SPIN_SPEED = 720.0f;     // degrees/s, sai + shuriken
const float PROJECTILE_IMPACT_DURATION = 0.15f; // impact flash before a projectile that hit is removed (req 75)

// ---------------- combat animation timing (owner B) ----------------
const float ATTACK_ANIM_DURATION = 0.33f;       // one swing / throw / shot / punch (drives Pose::attack)
const float STRIKE_POINT = 0.5f;                // swing progress where the katana/punch lands and shots leave the hand (req 71)
const float HIT_FLASH_DURATION = 0.25f;         // short flash after taking a hit (req 82, 94)
const float HIT_FLASH_TINT = 0.35f;             // extra red on top of the damage tint while flashing
const float WALK_CYCLE_SPEED = 8.0f;            // walk animation rate (Pose::walkCycle per second)

// ---------------- effects (owner B: req 68, 83, 120) ----------------
const int   EFFECTS_MAX_PARTICLES = 600;        // cap, extra spawns are skipped
const float SMOKE_PUFF_DURATION = 0.5f;         // ninja smoke on placement / defeat (req 68)
const int   SMOKE_PUFF_COUNT = 9;               // puffs in the outer ring (+3 in the middle)
const float SMOKE_PUFF_SPEED = 110.0f;
const float SMOKE_PUFF_ALPHA = 0.85f;
const float HIT_SPARK_DURATION = 0.25f;         // sparks on every hit (req 120)
const int   HIT_SPARK_COUNT = 7;
const float HIT_SPARK_SPEED = 380.0f;
const float DEFEAT_EFFECT_DURATION = 0.6f;      // shell spin / dropped weapon (req 83)
const float EFFECT_GRAVITY = 900.0f;            // falling pieces and sparks

// ---------------- waves ----------------
const int   WAVE_COUNT = 3;
const int   WAVE_SIZES[WAVE_COUNT] = { 3, 5, 10 };   // spec minimums
const float FIRST_SPAWN_DELAY = 2.0f;
const float SPAWN_INTERVAL_MIN = 3.0f;
const float SPAWN_INTERVAL_MAX = 6.0f;
const float WAVE_BREAK_DURATION = 2.5f;         // pause + banner between waves

// ---------------- collectibles ----------------
const float PIZZA_SIZE = 40.0f;

// ---------------- health bars ----------------
const float HEALTH_BAR_GAP = 16.0f;                       // above the head
const float HEALTH_BAR_MAX_Y = PLAYFIELD_TOP - 12.0f;    // never under the HUD (tall villains in the top lane)

// ---------------- cutscene ----------------
const float CUTSCENE_DURATION = 16.0f;          // spec: 10-20 s
const bool  CUTSCENE_SKIP_ENABLED = true;       // Space/Enter skips (OQ10: set false if the TA doesn't allow it)
