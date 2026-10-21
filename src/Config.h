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
const int PIZZA_VALUE = 15;                     // per defeated villain

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

// ---------------- waves ----------------
const int   WAVE_COUNT = 3;
const int   WAVE_SIZES[WAVE_COUNT] = { 3, 5, 10 };   // spec minimums
const float FIRST_SPAWN_DELAY = 2.0f;
const float SPAWN_INTERVAL_MIN = 3.0f;
const float SPAWN_INTERVAL_MAX = 6.0f;
const float WAVE_BREAK_DURATION = 2.5f;         // pause + banner between waves

// ---------------- collectibles ----------------
const float PIZZA_SIZE = 40.0f;

// ---------------- cutscene ----------------
const float CUTSCENE_DURATION = 16.0f;          // spec: 10-20 s
