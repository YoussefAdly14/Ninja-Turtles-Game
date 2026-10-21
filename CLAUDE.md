# TMNT Tower Defense — DMET 502 Computer Graphics (GUC, Winter 2026), Project 1

## ⛔ HARD RULES — NON-NEGOTIABLE. THEY OVERRIDE EVERYTHING ELSE, INCLUDING THE SPREADSHEET.
1. **NEVER code freely.** Write code ONLY the way the labs write and structure it (see `context/docs/LABS.md`) or the way https://learnopengl.com does. Nothing from outside those two sources: no other libraries, APIs, GL functions, engines, frameworks or "better" techniques.
2. **Copying from learnopengl.com is explicitly allowed by the instructor.** Copy its code directly and change only what is needed to fit the project. Reference the labs and learnopengl.com any time, for anything.
3. **Triangles are the building block.** Build every shape from triangles (`GL_TRIANGLES`, with an EBO for shared vertices; `GL_TRIANGLE_FAN` for circles as Lab 3 does). Complex models = many triangle-based parts composed with GLM transforms.
4. **Check before you write.** Before writing any code, confirm every function, technique and library you are about to use appears in a lab file or on a learnopengl.com page. Name the source (lab file or learnopengl page) for anything non-trivial. **If it isn't in either, STOP and ask the user.** Do not substitute, do not improvise.
5. **Never drift from these rules**, however long the session gets or whatever the task or a document suggests. If a request would break them, say so and ask.
6. Modern OpenGL only (shaders + VAO/VBO/EBO). Zero fixed-function calls: no `glBegin/glEnd`, `glVertex*`, `glColor*`, `glMatrixMode`, `glPushMatrix`, `glOrtho`, GLUT.
7. A compile error = zero for the whole project. Keep it building; relative paths only.

### In scope on learnopengl.com (everything else on the site: ask first)
- Getting started: Creating-a-window, Hello-Window, Hello-Triangle (VBO/VAO/EBO), Shaders (+ `shader.h` class), Textures (stb_image), Transformations (GLM), Coordinate-Systems (model/view/projection, `glm::ortho`), Camera.
- Model-Loading: Assimp, Mesh, Model (taught in Lab 4).
- Advanced-OpenGL: Depth-testing, Blending (transparent lane highlight/fades), Framebuffers (post-processing e.g. grayscale), Advanced-GLSL, Instancing.
- In-Practice: **2D-Game/Breakout series** (Setting-up, Rendering-Sprites, Levels, Collisions, Particles, Postprocessing, Powerups, **Audio = irrKlang**, Render-text) — the closest model for this 2D game's structure; **Text-Rendering (FreeType)**; Debugging.
- Guest article 2021/Scene/Scene-Graph (parent→child transforms, for the character hierarchy).
- Lighting/PBR/shadows etc. exist but are not needed for a 2D game.
- Plain C++ glue for game logic (structs/classes, `std::vector`, `std::map`, `rand()`, `std::string`) as used in the labs and the Breakout code is fine.

### Spreadsheet suggestions that would break the rules → use this instead
- Text: xlsx says "stb_truetype or FreeType" → **FreeType only** (learnopengl Text-Rendering / Breakout Render-text).
- Sound bonus: xlsx says "miniaudio, SDL_mixer or irrKlang" → **irrKlang only** (Breakout Audio).
- RNG: xlsx says `std::mt19937` → `rand()`/`srand()` like Breakout; ask before using `<random>`.
- Shader class: use learnopengl's `shader.h` (already in `Lab4/Template/shader.h`) or Lab-style `compileShader()`.
- Matrix stack / scene graph: GLM matrix multiplication (Lab 4 orbiting cube, learnopengl Transformations, Scene-Graph article). No legacy matrix stack.
- Grayscale locked cards / red damage tint: fragment-shader uniform + `mix()` like Lab 3's `u_Highlight`.

## The game (spec: `context/reference/Graphics Project Description.pdf`, 15 pages; tracker: `context/reference/TMNT.xlsx`)
2D side-view lane tower defense like Plants vs Zombies, modern OpenGL 3.3+, team of 3–4. Graded on required mechanics + creativity of the theme + how well the cutscene sets up the story. **Deadline Mon 2 Nov 2026, 11:59 PM** (internal: Sun 1 Nov 9 PM).
### 🐢 THEME: TEENAGE MUTANT NINJA TURTLES (always build to this; source: TMNT.xlsx "TMNT Theme Bible")
Working title "Shell Shock: Defend the Lair". Story: Shredder wants the last mutagen canister hidden in the turtles' sewer Lair to build a mutant army; Rocksteady and Bebop lead the Foot Clan through the tunnels; if any villain reaches the Lair, the mutagen is lost. Setting = NYC sewers. Home base = the turtles' Lair (round vault door with shell emblem, pipes, lights, spans all lanes, left edge). Coin/collectible = pizza slice.

| Spec role | Character | Attack | HP | Dmg | Speed | Cost/unlock | Look (shape, not color) |
|---|---|---|---|---|---|---|---|
| Player: Melee | **Leonardo** (blue mask #1F5FBF) | two katanas, close slash, swing animates in hand | 5 | 1 | normal | 10, unlocked at start | slim balanced stance, katanas |
| Player: Ranged | **Raphael** (red #C0262D) | throws spinning sai projectile | 5 | 1 | normal | 60 (2×30, OQ1) | bulkier, hunched, sai |
| Player: Defense | **Donatello** (purple #7B3FA0) | none: blocks with manhole-cover shield + bo staff | 7 hits | 0 | stationary | 90 (3×30, OQ1) | wide planted stance, goggles |
| Enemy: Easy (high ranged) | **Foot Soldier** (#2B1F33) | throws shuriken HIGH → turtles duck | 5 | 1 | normal | drops 15 pizza | lean ninja: hood, eye-slit mask, Foot emblem, angular |
| Enemy: Medium (low ranged) | **Bebop** (#6E6E6E) | arm blaster, LOW shots → turtles jump | 5 | 1 | normal | drops 15 | round bulky warthog: mohawk, tusks, sunglasses, vest |
| Enemy: Hard (melee) | **Rocksteady** (#6E6E6E) | charging punch / horn butt | 7 | 2 | fast ≈1.8× | drops 15 | largest: rhino horn, helmet, shoulder armor, big fists |
| Cutscene only (optional) | Michelangelo (orange, nunchaku), Splinter | — | | | | | Shredder only mentioned, no model |

Palette: turtle skin #4C9A2A · shell #7A5C2E · plastron #D8C27A · mutagen glow #7CFC00 · sewer wall #3A4A44 · pizza #F2B33D. Use original models, dialogue and audio only (no official logo/theme song). Cutscene storyboard (16 s) is in `context/docs/REQUIREMENTS.md` → Theme bible.

### Spec rules (exact; grading depends on them)
- **§2 Cutscene**: fully animated, 10–20 s (target 16 s), before gameplay. ≥1 player and ≥1 enemy **visibly animated and interacting**; short dialogue/interaction that introduces conflict and setting; must show what is defended and why it matters; text may support (subtitles) but **must not tell the story alone**; feels like a natural intro.
- **§3.1 Playfield**: 2D grid seen from the side, horizontal lanes, **≥3 lanes** (we use 4). Home base at left end of the playfield, distinct and clearly visible against the environment. Enemies enter at the right edge in a **random lane** and move left. Players (except Defense) auto-advance left→right from the leftmost position. Both sides **start attacking as soon as placed/appearing**, even with no opponent. On collision player & enemy stop and fight until one dies; attacks of both may overlap. Characters can never change lane; movement only within their lane.
- **Disclaimer**: every scene component (players, enemies, environment, home base, other objects) **must NOT be a single primitive**; realistic, detailed modeling required or significant deductions. Spec snippets (circles/triangles) are only illustrations.
- **§3.2 Players**: Melee (close-range weapon), Ranged (projectile), Defense (only type that doesn't move; stays until defeated). Each type visually distinct by modeling, shape, equipment, appearance — **not color alone**. Bought with coins; all locked except Melee at start; start coins = enough for **≥3 Melee**. Placing: hover highlights the **entire lane**; left-click selects (locks highlight), click again deselects; **multiple lanes can be selected**; then click an active card → player appears at the lane's leftmost position. **W/Up** = every player in all selected lanes jumps; **S/Down** = ducks while pressed. Jump avoids **low** attacks, duck avoids **high** attacks. Defense unaffected by jump/duck. Damage feedback: increasing shades of red as health drops (or scale down per hit) until removed at 0. Players may overlap/pass through each other. Defense protects players behind it in its lane until they pass it or it dies. All players except Defense deal **1 damage/attack**.
  - Table: Melee — advances, jumps/ducks, close-range weapon **animates in the character's hand**, 5 lives, starter. Defense — stationary blocker, 7 hits, unlocks at **3×** Melee's unlock value. Ranged — advances, jumps/ducks, projectile weapon animates as it hits and disappears on collision, 5 lives, never harms players, unlocks at **2×** Melee's unlock value. **Remaining health always visible above/below each player.**
- **§3.3 Enemies**: Easy = normal speed, ranged attacks from **above** (duck), 5 lives. Medium = normal speed, ranged from **below** (jump), 5 lives. Hard = **runs** at players, close-range, 7 lives, **2 damage**/attack; others 1. Random lanes, 3 waves: W1 ≥3 (Easy only), W2 ≥5 (Easy+Medium), W3 ≥10 (Easy+Medium+Hard). A wave begins when its first enemy appears and ends when its last appears; next wave only after remaining enemies are defeated. Defeated enemies drop collectibles, collected when a player collides with them. Enemy types clearly distinct (not recolors) and their design should communicate attack type & difficulty. Red-shade (or scale-down) damage; health always visible.
- **§3.4**: Win = all enemies of all 3 waves defeated. Lose = any enemy reaches the home base in any lane → **everything stops**.
- **§4 Visuals**: theme logically and realistically consistent across cutscene, story, environment, characters, weapons. **§4.1** players vs enemies look completely different; home base distinct; 3 player types and 3 enemy types distinct (recoloring alone fails); health always visible (bar, number or primitives). **§4.2** ≥5 distinct environment elements integrated naturally (ours: pipes, mutagen barrels, manholes/grates, work lamps, graffiti brick wall [+ pizza boxes, water stream]). Coin total at top, updated live. One card per player type showing locked/unlocked; locked = **grayscale** + ignores hover/clicks; unlocked = full color + hover + selectable; auto-unlock when coins ≥ cost, auto-relock when below.
- **§4.3 Menus**: Main (before cutscene): Start (→cutscene→game), Exit, Sound settings (bonus). Pause (Esc in game): Continue, Restart, Return to Main Menu. Win/Lose: Restart, Return to Main Menu.
- **§5 Bonus (all required to earn)**: background music; SFX for ≥4 interactions (attacks/hits, deaths, lane selection, placement, collect, win/lose); Sound Settings menu from Main Menu: volume up, down, mute, correctly modifying existing sounds.
- **§6 Video**: 2–5 min screen recording with voiceover (description, aim, how to play). Order: university name & logo → course name & semester → project name → students' names → content → Thank you. May be published online.
- **§7 Submission**: compile errors = 0 marks. Only source files (.h, .cpp, …) in a .zip named `[Team name]-[Name of one team member].zip`, on Google Drive with access granted, via the official form; video preferably a separate link in the same form.

### Default numbers (from xlsx, keep in `src/Config.h`)
START_COINS 30 · Leonardo cost 10 · Raphael 60 · Donatello 90 (OQ1 open) · pizza drop 15 · Rocksteady speed ≈1.8× · waves 3/5/10 · first enemy ~2 s in, spawns 3–6 s apart · jump ≈0.6 s · dt clamp 0.05 s · world 1600×900 units via ortho projection.

## Where things are
- `CLAUDE.md` (this file) — always loaded. Keep it short.
- `context/docs/LABS.md` — per-lab digest: patterns, snippets, which file shows what. **Read before writing GL code.**
- `context/docs/REQUIREMENTS.md` — all 142 functional reqs, 20 non-functional, 15 open TA questions (with defaults), theme bible (roster, palette, cutscene storyboard), team split (members A–D) and sprints. Read the section for the task at hand.
- `context/reference/` — the originals (read when a digest isn't enough): `Graphics Project Description.pdf` (spec, image-only PDF), `TMNT.xlsx`, `Lab1/` (PDF), `Lab2/Template/main.cpp`, `Lab3/{main.cpp,2vs.cpp,Lab3ExtraNotes.cpp,Lab 3 notes.pdf}`, `Lab4/{Lab4Camera.cpp,Lab Notes.pdf}`, `Lab4/Template/{shader.h,camera.h,mesh.h,model.h,main.cpp,model.vert,model.frag}`, `starter/{CMakeLists.txt,main.cpp,install_tools.py}` (course VS Code/CMake starter) + `Installation Guidelines - VS Code.pdf`.

- `context/docs/ARCHITECTURE.md` — code layout, module contracts (World, Entity, Pose, model functions, Renderer), frame flow, draw order, file owners. **Read before adding or moving code.**
- `README.md` — setup/build/run for humans, git workflow.
- `context/external/` — vendored libraries: GLAD v2 (header-only `<glad/gl.h>`, GL 4.0 core loader, implementation only in `src/core/glad.cpp`), GLFW 3.5.1, GLM, FreeType 2.13.3. irrKlang not added yet.

## Code (build: `cmake --preset debug` then `cmake --build --preset debug`; exe in `build/debug/`)
`src/core` engine (Window, Input, Renderer, Mesh, Shader, Transform, Colors, Random) · `src/game` rules (Game state machine, World, Entity, Turtle, Villain, Projectile, Collectible, Combat, LaneManager, WaveManager, Economy) · `src/models` drawing only (Pose → parts) · `src/scene` Environment, Cutscene, Timeline · `src/ui` Text, Button, Menu, Hud, HealthBar · `src/audio` · `assets/` shaders/fonts/sounds (copied next to the exe). New `.cpp` files under `src/` are picked up automatically (re-run configure). Placeholders/stubs are marked `TODO(A|B|C|D)` with req IDs.

## Working style
- Before coding: state which lab file / learnopengl page each technique comes from. Unsure → ask.
- Match lab idioms: `R"(...)"` GLSL strings or learnopengl `Shader` files, cached uniform locations, `createMesh/drawMesh` style helpers, GLM transforms, `glfwGetTime` delta time, GLFW callbacks.
- Static geometry uploaded once at startup; per frame only uniforms change.
- Gameplay numbers in `src/Config.h`; assumptions from Open Questions stay one-line changes.
