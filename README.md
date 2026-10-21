# Shell Shock: Defend the Lair

TMNT lane tower defense for DMET 502 Computer Graphics (GUC, Winter 2026), Project 1.
Modern OpenGL 3.3 core, C++17, GLFW + GLAD + GLM + FreeType, built with CMake.

**Before writing any code, read `CLAUDE.md` (hard rules: labs + learnopengl.com only, triangles only, no fixed-function GL).** One compile error = zero for the whole project.

## Setup (once)

1. Install **VS Code** and **Python**.
2. Install the compiler tools with the course script (MinGW + Ninja + CMake, adds them to PATH):
   `python context/reference/starter/install_tools.py`, then open a **new** terminal.
3. Open this folder in VS Code and accept the recommended extensions (CMake Tools, C/C++).
4. CMake Tools picks the **Debug (Ninja)** preset by default. Click **Build**, then **Launch** in the CMake side panel, as in `context/reference/Installation Guidelines - VS Code.pdf`.

From a terminal instead:

```
cmake --preset debug
cmake --build --preset debug
build/debug/NinjaTurtlesGame.exe
```

The first build compiles GLFW and FreeType and takes a minute; after that only changed files rebuild. `assets/` is copied next to the exe, so the game runs from either the repo root or `build/debug/`.

## Controls (current skeleton)

| Input | Action |
|---|---|
| Mouse hover / left click on a lane | highlight / select-deselect (several lanes allowed) |
| Click an unlocked card (top-left) | buy that turtle into the most recently selected lane |
| W / Up | selected lanes' turtles jump |
| S / Down (hold) | selected lanes' turtles duck |
| Esc | pause / resume |
| F1 | show hitboxes (debug) |

## Project layout

```
src/
  main.cpp          window + game loop
  Config.h          EVERY gameplay number (costs, HP, speeds, waves, layout)
  core/             engine: Window, Input, Renderer, Mesh, Shader, Transform, Colors, Random
  game/             rules: Game (state machine), World, Entity, Turtle, Villain, Projectile,
                    Collectible, Combat, LaneManager, WaveManager, Economy, Types
  models/           drawing only: TurtleModels, VillainModels, FootModel, PropModels, LairModel, Pose
  scene/            Environment, Cutscene, Timeline
  ui/               Text, Button, Menu, Hud, HealthBar
  audio/            Audio
assets/             shaders/ (+ fonts/, sounds/ as they are added)
context/docs/       REQUIREMENTS.md (all reqs, theme bible, team split), LABS.md, ARCHITECTURE.md
context/reference/  original spec PDF, TMNT.xlsx, lab code, course starter
context/external/   third-party libraries (GLAD, GLFW, GLM, FreeType)
```

**`context/docs/ARCHITECTURE.md` explains how the modules talk to each other and who owns which file.** Read it before your first change.

## What works / what's a placeholder

Working: window with letterboxing, 2D renderer + building-block shapes, input, state machine (Main Menu → Cutscene → Playing ↔ Paused → Won/Lost → Restart/Main Menu), 4 lanes with hover + multi-select, cards with grayscale lock/unlock, buying turtles, jump/duck, three waves spawning in random lanes, projectiles flying, health bars, contact stop.

Placeholder or stub (search `TODO(A)`, `TODO(B)`, `TODO(C)`, `TODO(D)`):
- **all models** are simple stand-ins (`models/Placeholders.cpp`), to be replaced with detailed ones;
- **text draws nothing yet** (`ui/Text.cpp`): FreeType is built and linked, but the learnopengl Text-Rendering code and a font still need adding. Menus show unlabeled buttons until then: Main = Start / Sound / Exit, top to bottom;
- **audio is a stub** (`audio/Audio.cpp`);
- combat damage, pizza drops/pickup, and environment props are TODOs.

## Team workflow

- One branch per member (`git checkout -b <name>/<feature>`), merge into `main` **at least daily**, and **`main` must always build**. Build before you push.
- Stay in the files you own (table in ARCHITECTURE.md). Shared files (`Config.h`, `Types.h`, `Entity.h`, `Pose.h`, `World.h`): add things, don't rename or delete without telling the team.
- Reviews in pairs A↔C, B↔D; only the reviewer moves a row in `TMNT.xlsx` from Done to Tested.
- Never commit `build/`. Never use absolute paths (`C:\Users\...`) in code.

## Credits (third-party; final list goes in the submission README, req 138)

GLFW 3.5.1 (zlib license) · GLAD 2 generated loader · GLM (MIT) · FreeType 2.13.3 (FreeType License). Course starter files from the DMET 502 labs. Any font, music or sound added must be original or free (CC0/OFL) and credited here.
