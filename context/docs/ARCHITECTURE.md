# Architecture

How the code is split, what each module promises the others, and who owns which file.
The HARD RULES in `CLAUDE.md` still apply to everything here (labs + learnopengl only).

## Layers (includes only point downward)

```
main.cpp                 window + loop only
  game/Game              state machine: menus, cutscene, playing, paused, won, lost
    ui/  scene/          HUD, menus, health bars, text | environment, cutscene
      game/ (rest)       World + rules: turtles, villains, projectiles, pizza, combat, waves, lanes, coins
        models/          drawing only: Pose + matrix in, triangles out
          core/          Window, Input, Renderer, Mesh, Shader, Transform, Colors, Random
Config.h                 every tunable number (included anywhere)
game/Types.h             shared enums (included anywhere)
```

Two rules keep this modular:
- **Models never read game state.** They get a `glm::mat4 base` and a `Pose`, nothing else. That's why D can build Leonardo while B builds his combat, and the HUD/cutscene can reuse the same models.
- **Menus never change state.** `Menu::update` returns a `MenuAction`; only `Game::handleMenuAction` acts on it.

## One frame

```
main loop: dt (clamped to MAX_DELTA_TIME) -> game.update(dt) -> clear -> game.render() -> Input::endFrame -> swap -> poll
Game::update  : only the current state's update runs (so nothing moves behind Pause/Win/Lose)
updatePlaying : Esc -> cards (buy) -> lane hover/toggle -> jump/duck -> World::update -> lose check -> win check
World::update : waves spawn -> turtles -> villains -> projectiles -> pizza -> Combat::resolve -> removeDead
```

Draw order (req 15), in `Game::renderWorld`:
background -> environment props -> lanes + highlights -> Lair -> pizza -> characters (top lane first) -> projectiles -> health bars -> [F1 hitboxes] -> HUD. Menus are drawn last by `Game::render`.

## Coordinates

- World is always **1600 x 900 units**, (0,0) bottom-left, y up (`glm::ortho` in `Window::projection`). Resizing letterboxes; `Input::mouseWorld()` is already in world units.
- **Lane 0 is the top lane.** `laneBottom/laneTop/laneCenterY/laneFloorY(lane)` and `laneAt(point)` in `game/LaneManager.h`.
- The Lair occupies x 0..`LAIR_WIDTH`; the HUD is the top `HUD_HEIGHT`; the water channel is below `PLAYFIELD_BOTTOM`.

## Contracts

### Renderer (`core/Renderer.h`)
One shader (`assets/shaders/basic.vert/.frag`): vertex colour x `u_Color`, then `u_DamageTint` (mix to red) and `u_Grayscale` (luminance). Blending is on.
- `Renderer::draw(Shape::Quad|Triangle|RightTriangle|Circle|Semicircle|Ring, model, color)`. Every shape is 1 x 1 (see `Mesh.h`), so `scale(w, h)` gives a w x h shape.
- `Renderer::draw(myMesh, model, color)` for your own meshes (`createPolygonMesh`, `createMesh`, `createIndexedMesh`). **Create them once in your module's `init()`, destroy in `shutdown()`**, never per frame (NF4).
- Shortcuts: `drawRect`, `drawCircle`, `drawLine` (world units).
- `setDamageTint(0..1)`, `setGrayscale(0..1)` stay on until you set them back to 0. `beginFrame` resets them.
- `setView(mat4)` is for the cutscene camera. Reset to identity before drawing UI on top.

### Hierarchy (`core/Transform.h`)
`part = parent * local`. Use `joint(parent, pivotX, pivotY, degrees)` for anything that rotates (shoulder, elbow, katana), and `part(parent, x, y, w, h, deg)` for the final visible piece. Never use a `part()` result as a parent, because its scale would stretch the children. `models/Placeholders.cpp` shows the pattern: body -> shoulder -> arm -> hand -> weapon.

### Model functions (`models/*.h`)
`void drawX(const glm::mat4& base, const Pose& pose)`
- `base` = world position of the **feet centre** (props: object centre). Model units = world units: a turtle is ~`TURTLE_HEIGHT` tall, villains use `FOOT_/BEBOP_/ROCKSTEADY_HEIGHT`.
- Turtles face **right**, villains face **left**.
- `Pose` fields: `time`, `walkCycle` (use `sin`), `attack` (0 = rest, 0..1 = swing progress), `ducking`, `inAir`, `hitFlash`. If you need another field, add it to `Pose` and fill it in `Turtle::pose()` / `Villain::pose()`.
- ≥ 12 parts per character, never one primitive, recognisable by shape in grayscale (NF17, req 123).

### Entity (`game/Entity.h`)
Shared fields: `id, lane, x (feet centre), yOffset, width, height, hp, maxHp, alive, age` + `left/right/bottom/top`, `damageTint()`, `takeDamage()`, `healthBarPosition()`, and `overlapsX(a, b, reach)` for the same-lane contact test.
Entities live in **typed vectors** in `World` (`turtles`, `villains`, `projectiles`, `collectibles`), like Breakout's `std::vector<GameObject>`, instead of a polymorphic list (req 10 asked for virtual `update/draw`; typed vectors give the same split without pointers or leaks).
- Kill something with `alive = false` (or `takeDamage`). `World::removeDead()` erases at the end of the frame. **Never erase inside a loop.**
- Spawn through `World::spawnVillain / spawnProjectile / dropPizza / placeTurtle`. Pushing into a *different* vector than the one being looped over is safe.

### World (`game/World.h`)
All state for one game. `reset()` = new game (coins, waves, lanes, entities). Gameplay code takes `World&`, never `Game&`.

## Where things go

| Need to... | Put it in |
|---|---|
| change a number | `src/Config.h` |
| add a colour | `core/Colors.h` (theme hex codes via `hexColor(0xRRGGBB)`) |
| add a mesh for a model | that model's `.cpp`, created in `init()` |
| add an entity behaviour | `Turtle.cpp` / `Villain.cpp` (per-unit) or `Combat.cpp` (between units) |
| add a new screen | a `GameState` + `Menu` factory in `ui/Menu.cpp` + a `MenuAction` |
| add a sound trigger | call `Audio::play(Sound::X)` at the event; add the enum value if missing |
| add an effect (sparks, smoke) | new `game/Effects.h/.cpp` (owner B), drawn after characters |
| add a file | anywhere under `src/`, then re-run CMake configure (sources are globbed) |

## File owners (from the team split in REQUIREMENTS.md; reviewer pairs A↔C, B↔D)

| Owner | Files |
|---|---|
| A: engine & rules | `main.cpp`, `Config.h`, `core/{Window,Renderer,Mesh,Shader,Random,glad}`, `game/{Game,World,Entity,LaneManager,Economy,WaveManager,Collectible}`, `models/VillainModels.cpp` (Bebop, Rocksteady), env props: manholes, lamps, pizza boxes |
| B: combat & characters | `game/{Turtle,Villain,Projectile,Combat}`, projectile models in `models/PropModels.cpp`, `game/Effects` (new) |
| C: UI, input, audio | `core/Input`, `ui/*`, `audio/*`, env props: pipes, graffiti wall, water stream; lane highlight look in `LaneManager::draw` |
| D: art & cutscene | `core/{Transform,Colors}`, `models/{Pose,TurtleModels,FootModel,LairModel}`, pizza in `PropModels.cpp`, `scene/{Cutscene,Timeline}`, `Environment::drawBackground` + mutagen barrels |

Every unfinished spot is marked `TODO(A|B|C|D)` with its requirement number: `grep -rn "TODO(B)" src`.
