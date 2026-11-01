# TMNT requirements (full digest of TMNT.xlsx)

Source of truth for Status/Owner is `TMNT.xlsx`. Read only the section you need.
**The HARD RULES in `CLAUDE.md` override any library/technique this sheet suggests** (e.g. text = FreeType only, audio = irrKlang only, RNG = rand()).


Line format: `ID [Priority·Sprint·Owner→Reviewer·effort pts] (spec §, deps) Stakeholder: I should be able to … — rules you MUST follow`
Status: ✅ = done in code and checked (build + screenshot/test). No mark = not done yet or only partly done. Marked by the lead after reviewing the work; the TMNT.xlsx Status column is still the team's official tracker.
Priority: Must = required by spec · Bonus = sound bonus (all of §5 needed) · Optional · Dev aid. 1 pt ≈ 1–2 h.


## Engine & setup
- **1** ✅ [Must·S1·A→C·2] (§1) Developer: create the game window and a modern OpenGL 3.3+ core-profile context — Use GLFW + GLAD (or GLEW) + GLM. Request the 3.3 core profile explicitly and print the GL version at startup. No fixed-function calls anywhere (glBegin/glEnd, glVertex, glColor, glMatrixMode, glPushMatrix): the spec requires modern OpenGL.
- **2** ✅ [Must·S1·A→C·3] (§7) Developer: build the whole project from a clean checkout with one documented project file — CMake or a Visual Studio solution with dependencies kept in /external. Confirm with the TA which OS/IDE they compile on. Any compilation error = zero for the project.
- **3** ✅ [Must·S1·A→C·2] (§1, deps 1) Developer: compile, link and use GLSL shaders with readable error logs — Shader class: load from file, uniform setters (mat4, vec4, float, int). One flat-color shader with u_model, u_projection, u_color, u_tintAmount and u_grayscale covers characters, damage tint and locked cards.
- **4** ✅ [Must·S1·A→C·2] (§1, deps 3) Developer: draw in a fixed logical world (e.g., 1600 × 900 units) through an orthographic projection — All gameplay coordinates are world units, never pixels. Handle window resize with letterboxing so lanes, mouse picking and the HUD stay aligned.
- **5** ✅ [Must·S1·A→C·1] (§1, deps 1) Developer: run a frame-rate-independent game loop driven by delta time — All speeds in units/second and all timers in seconds. Clamp dt (≤ 0.05 s) so dragging the window doesn't teleport entities. Enable vsync.
- **6** ✅ [Must·S1·A→C·3] (§1, deps 3) Developer: draw reusable primitive meshes: quad, circle/ellipse, triangle, rounded rectangle, arc/ring, polygon and thick line — Upload each mesh to a VAO/VBO once at startup and draw it with a model matrix + color. These are the building blocks of every model; never re-upload static geometry every frame.
- **7** ✅ [Must·S1·D→B·3] (§4.1, deps 6) Developer: compose models from parts using a transform hierarchy (matrix stack / scene-graph nodes) — Each part has a local transform relative to its parent (hand → forearm → upper arm → torso), so one rotation animates the weapon in the hand. Needed for 'weapon animates in the character's hand'.
- **8** ✅ [Must·S1·C→A·3] (§4.2, deps 3) Developer: render text and numbers on screen — Font atlas through stb_truetype or FreeType (extra libraries are allowed). Used by the coin counter, health numbers, card costs, wave banner, menu labels and cutscene subtitles.
- **9** ✅ [Must·S1·A→C·2] (§4.3, deps 5) Developer: switch between game states: Main Menu, Sound Settings, Cutscene, Playing, Paused, Win and Lose — One state machine owns update/render dispatch. Paused, Win and Lose keep drawing the frozen scene under the menu but stop all gameplay updates.
- **10** ✅ [Must·S1·A→C·3] (§3.1, deps 5) Developer: manage turtles, villains, projectiles and collectibles through one shared Entity interface — Fields: id, type, lane (const after creation), x, yOffset, width, height, hp, maxHp, state, alive. Virtual update(dt) and draw(). Dead entities are removed at the end of the frame, never while iterating. Agree this interface on day 1: everyone builds on it.
- **11** ✅ [Must·S1·C→A·2] (§3.2, deps 4) Developer: receive keyboard and mouse input and convert the cursor position to world coordinates — GLFW callbacks for W, S, Up, Down, Esc, left click and cursor movement. Screen → world conversion must account for the letterboxing.
- **12** ✅ [Must·S1·A→C·1] (§3.2) Developer: tune every gameplay number from one Config.h — START_COINS, card costs, coin drop value, HP, damage, speeds, attack intervals, jump/duck timings, wave definitions, cutscene length. Rebalancing never touches logic code.
- **13** ✅ [Must·S1·A→C·1] (§3.1) Developer: randomize lanes, enemy order and spawn timing with a seeded RNG — std::mt19937. A debug flag fixes the seed so the video can be recorded reproducibly.
- **14** ✅ [Must·S1·A→C·1] (§7) Developer: load shaders, fonts and sounds through relative paths — No absolute paths (C:\Users\...). The TA must be able to build and run straight from the submitted zip.
- **15** ✅ [Must·S1·A→C·1] (§4, deps 6) Player: see everything drawn in a consistent order — Background → environment → lanes and highlights → Lair → collectibles → characters → projectiles → health bars → HUD → menus.
- **16** ✅ [Dev aid·S1·C→A·1] (§1, deps 8) Developer: toggle a debug overlay showing FPS, lane bounds and hitboxes — F1 toggles it. Off by default in the submitted build.
- **17** ✅ [Dev aid·S2·A→C·1] (§6, deps 9) Developer: use debug keys to add coins, skip to a wave, kill all enemies or force a loss — Speeds up testing and recording the video. Disable or document them before submission.

## Menus & game flow
- **18** ✅ [Must·S1·C→A·2] (§4.3, deps 8, 11) Player: see menu buttons react when I hover over and click them — One Button component (hover = highlight/scale, press = pressed state) reused in all three menus so the UI is consistent.
- **19** ✅ [Must·S1·C→A·2] (§4.3, deps 18, 9) Player: see the Main Menu when the game launches, before the cutscene — Themed screen (sewer background, game title, turtle silhouettes), not a plain rectangle.
- **20** ✅ [Must·S1·C→A·1] (§4.3, deps 19) Player: click Start to play the cutscene and then start the game — Start → Cutscene → Playing. Every start creates a fresh game state (coins = START_COINS, wave 1, no entities).
- **21** ✅ [Must·S1·C→A·1] (§4.3, deps 19) Player: click Exit to close the application — glfwSetWindowShouldClose; free GL and audio resources; no crash on exit.
- **22** ✅ [Bonus·S3·C→A·1] (§5, deps 19) Player: open the Sound Settings from the Main Menu — Bonus. The button must be on the Main Menu to qualify (§5).
- **23** ✅ [Must·S2·C→A·2] (§4.3, deps 9) Player: press Esc during gameplay to open the Pause Menu — Only in the Playing state. While paused nothing moves: entities, projectiles, spawn and wave timers and animations all freeze.
- **24** ✅ [Must·S2·C→A·1] (§4.3, deps 23) Player: click Continue to resume exactly where I left off — Pressing Esc again may also resume (optional). Timers resume without a jump.
- **25** ✅ [Must·S2·A→C·2] (§4.3, deps 23, 10) Player: click Restart in the Pause or Win/Lose menu to start a new game — Full reset: coins, wave, spawn queue, all entities, lane selections, card selection, collectibles and music. Assumption: Restart skips the cutscene (see Open Questions).
- **26** ✅ [Must·S2·C→A·1] (§4.3, deps 23) Player: click Return to Main Menu in the Pause or Win/Lose menu — Resets the game; pressing Start again replays the cutscene.
- **27** ✅ [Must·S2·C→A·2] (§4.3, deps 9) Player: see a Win/Lose Menu when I win or lose — Themed 'Lair Defended!' / 'The Lair Has Fallen' message over the frozen scene, with Restart and Return to Main Menu.
- **28** ✅ [Optional·S3·C→A·1] (§4.3, deps 8) Player: see a short controls legend (lanes, cards, W/S, Esc) — Not required by the spec, but it helps graders and the video. Put it in the Pause Menu or as a small HUD hint.

## Cutscene
- **29** ✅ [Must·S1·D→B·1] (§2) Team member: agree a 10–20 s cutscene script and storyboard — See the 'TMNT Theme Bible' tab for beats, timing and dialogue. Lock it by the end of Sprint 1 so modeling and animation can start.
- **30** ✅ [Must·S2·D→B·3] (§2, deps 7) Developer: animate cutscene actors and the camera with a keyframe timeline — Tracks for position, rotation, scale, alpha and visibility with linear and ease-in-out interpolation. Camera pan/zoom by animating the view matrix.
- **31** ✅ [Must·S2·D→B·2] (§2, deps 30, 20) Player: watch a fully animated 10–20 second cutscene after pressing Start — Target 16 s. Log the measured duration in debug builds to prove it stays between 10 and 20 s. Transitions into gameplay automatically.
- **32** ✅ [Must·S3·D→B·3] (§2, deps 31, 69, 85) Player: see at least one turtle and one villain visibly animated and interacting on screen — Reuse the gameplay model functions (consistency, less work). Walk cycles, gestures, weapon draws and a clash. Never static images.
- **33** ✅ [Must·S2·D→B·2] (§2, deps 31, 8) Player: follow a short dialogue between the turtles and the villains — Subtitle bar or speech bubbles timed on the timeline; the speaker bobs and gestures while their line shows. The story must still read with the text hidden: no text-only storytelling.
- **34** ✅ [Must·S2·D→B·1] (§2, deps 29) Player: understand what the turtles are defending and why it matters — Show the Lair (the home base) with the last mutagen canister inside. Rocksteady and Bebop lead the Foot Clan to seize it for Shredder; the dialogue says why losing it matters.
- **35** ✅ [Must·S3·D→B·2] (§2, deps 31, 111) Player: recognize the NYC sewer setting from the cutscene — Open with an establishing pan through the sewer tunnel using the same environment elements as the game scene.
- **36** ✅ [Must·S3·D→B·1] (§2, deps 31) Player: see a smooth transition from the cutscene into gameplay — Fade or iris wipe to the playfield. The last shot frames the Lair as it looks in-game, so the hand-off is logical.
- **37** ✅ [Optional·S3·D→B·1] (§2, deps 31) Player: skip the cutscene with Space or Enter — Optional. Ask the TA first whether a skip is acceptable.
- **38** [Optional·S3·D→B·2] (§2, deps 32) Team member: include Michelangelo (and Splinter) in the cutscene — Optional cameo. The spec allows exactly three player types, so the fourth turtle appears only in the cutscene and menu. The required story works without them.

## Playfield & lanes
- **39** ✅ [Must·S1·A→C·2] (§3.1, deps 4, 6) Player: see a side-view playfield of at least 3 horizontal lanes — Default 4 lanes (configurable, minimum 3). Each lane is an independent battle line, drawn as a sewer walkway with visible separators.
- **40** ✅ [Must·S2·D→B·3] (§3.1, deps 7, 39) Player: see the turtles' Lair (home base) at the left end of every lane — Must be distinct from and clearly visible against the environment through shape, not only color: round vault door with a shell emblem, pipes and lights, spanning all lanes.
- **41** ✅ [Must·S1·B→D·1] (§3.1, deps 39, 10) Game system: place each new turtle at the leftmost position of its chosen lane — Spawn x is just right of the Lair. Donatello stays there; the others start walking right immediately. **Exception (OQ16, TA approved):** with `DONATELLO_FREE_PLACEMENT` Donatello is placed at the clicked spot in the lane instead; `false` restores the leftmost rule.
- **42** ✅ [Must·S1·B→D·2] (§3.1, deps 39, 13) Game system: spawn enemies at the right-most edge in a randomly chosen lane — Spawn x at the right edge (they may start just off-screen and walk in), lane picked by the RNG, then move left toward the Lair.
- **43** ✅ [Must·S1·B→D·1] (§3.1, deps 10) Game system: keep every turtle and villain in the lane where it was placed or spawned — Lane index is const after creation. Movement is horizontal only; the vertical offset is used just for jump/duck.
- **44** ✅ [Must·S1·B→D·2] (§3.1, deps 10) Developer: detect hits between entities, projectiles and collectibles within a lane — Overlap on x in the same lane plus a vertical band check (HIGH/LOW attack vs Standing/Jumping/Ducking).
- **45** ✅ [Must·S1·B→D·2] (§3.1, deps 41) Turtle (any): advance automatically from left to right after being placed (except Donatello) — Constant walk speed from Config. The walk animation plays while moving.
- **46** ✅ [Must·S1·B→D·1] (§3.1, deps 45) Game system: handle a turtle that reaches the right edge — Assumption: the turtle walks off-screen and is removed. Confirm with the TA (see Open Questions).
- **47** ✅ [Must·S1·B→D·2] (§3.1, deps 10) Turtle / Villain: start attacking as soon as I am placed (turtle) or appear (villain), even with no opponent in my lane — Spec snippet: lane 4's player attacks with no enemy, and lane 3's enemy attacks with no player. Attacks fire on a fixed interval from spawn.
- **48** ✅ [Must·S2·B→D·3] (§3.1, deps 44, 45) Turtle / Villain: stop moving when I meet an opponent in my lane and keep attacking until one of us is defeated — Contact = hitboxes overlap in the same lane. If several opponents overlap, fight the nearest one first. The survivor resumes moving once the other is removed.
- **49** ✅ [Must·S2·B→D·1] (§3.1, deps 44) Game system: let turtle and villain attacks overlap in the same lane — Projectiles from both sides pass through each other without cancelling; both keep attacking until one side dies (lane 2 of the spec snippet).
- **50** ✅ [Must·S2·B→D·1] (§3.1, deps 44) Game system: remove projectiles that hit something or leave the playfield — Villain shots that reach the left edge simply disappear: only a villain's body reaching the Lair causes a loss. Prevents memory growth during long waves.
- **51** ✅ [Must·S2·B→D·1] (§3.1, deps 48) Villain (any): pass through or overlap other villains in my lane — Assumption mirroring the turtle rule: a fast Rocksteady walks through a Foot Soldier that has stopped to fight. Confirm with the TA (see Open Questions).

## Lane selection & controls
- **52** ✅ [Must·S1·C→A·2] (§3.2, deps 11, 39) Player: see the entire horizontal lane highlight when my mouse hovers over it — The whole lane, not a cell. A semi-transparent overlay so characters stay visible. It follows the cursor every frame and disappears outside the playfield.
- **53** ✅ [Must·S1·C→A·2] (§3.2, deps 52) Player: left-click a lane to select it and lock its highlight, and click it again to deselect it — The selected style must differ from hover (brighter fill + border + marker by the Lair). Clicks on the HUD or cards never toggle lanes.
- **54** ✅ [Must·S1·C→A·1] (§3.2, deps 9, 11) Game system: ignore lane, card and jump/duck input outside the Playing state — Stops lanes toggling behind the Pause Menu, during the cutscene or after a win/loss. In those states only menu buttons respond.
- **55** ✅ [Must·S1·C→A·1] (§3.2, deps 53) Player: select several lanes at the same time — Store selection as a set of lane indices. It persists until toggled and is cleared on Restart/Main Menu.
- **56** ✅ [Must·S1·B→D·2] (§3.2, deps 55, 11) Player: press W or Up Arrow to make every turtle in all selected lanes jump — Jump = vertical arc of fixed duration (Config, e.g., 0.6 s). Ignored by Donatello and by turtles in unselected lanes.
- **57** ✅ [Must·S1·B→D·2] (§3.2, deps 55, 11) Player: press S or Down Arrow to make every turtle in all selected lanes duck — Duck while the key is held (spec: 'while pressing'), with a minimum duration. Crouch pose: squashed body, lowered head. Donatello is unaffected.
- **58** ✅ [Must·S2·B→D·2] (§3.2, deps 56, 57, 44) Game system: make jumping avoid low attacks and ducking avoid high attacks — Every villain projectile has a band: HIGH (Foot Soldier shuriken) or LOW (Bebop blaster). Standing turtles are hit by both.

## Player cards & pizza economy
- **59** ✅ [Must·S1·A→C·1] (§3.2, deps 12) Game system: start the game with enough pizza (coins) to place at least three Leonardos — START_COINS = 30, Leonardo cost = 10. These are assumed values; see Open Question 1 before locking them.
- **60** ✅ [Must·S1·C→A·2] (§4.2, deps 8, 59) Player: see my pizza total at the top of the screen, updated immediately — Pizza icon (the collectible model) + number. Short pulse animation whenever it changes.
- **61** ✅ [Must·S2·C→A·3] (§4.2, deps 8, 69) Player: see one card per turtle type showing its portrait, name and cost — Three cards top-left (Leonardo, Raphael, Donatello). Portraits use the same model functions, scaled down.
- **62** ✅ [Must·S2·C→A·2] (§4.2, deps 61, 3) Player: see locked cards in grayscale that ignore hover and clicks — Grayscale through the shader (luminance), not a grey box drawn on top. No hover highlight, cursor change or selection on a locked card.
- **63** ✅ [Must·S2·C→A·1] (§4.2, deps 61) Player: see unlocked cards in full color that respond to hover and can be selected — Hover highlight; tooltip with the turtle's role (optional).
- **64** ✅ [Must·S2·C→A·1] (§4.2, deps 62, 59) Game system: unlock a card automatically when my coins reach its cost — Re-checked whenever coins change. At the start only Leonardo is unlocked.
- **65** ✅ [Must·S2·C→A·1] (§4.2, deps 64) Game system: lock a card again automatically when my coins fall below its cost — e.g., right after buying a turtle.
- **66** ✅ [Must·S2·C→A·3] (§3.2, deps 53, 63, 41) Player: click an unlocked card to place that turtle on my selected lane — Flow: select lane → click card → turtle appears at the lane's leftmost position. Several lanes selected: assumption = most recently selected lane (see Open Questions). With no lane selected, show a hint and spend nothing. **Donatello (OQ16):** click his card (no lane needed), then click a spot in a lane; the lane under the cursor is used, a ghost shows green/red, right-click or the card again cancels. Refused with a hint when another Donatello or a villain is in the way; pizza is spent only when he is placed.
- **67** ✅ [Must·S2·A→C·1] (§3.2, deps 66, 59) Game system: deduct the card's cost when a turtle is placed — Atomic purchase: check coins ≥ cost, deduct, spawn, then re-evaluate every card's lock state.
- **68** ✅ [Optional·S3·B→D·1] (§4, deps 66) Player: see a ninja smoke-puff when a turtle is placed — Short scale/alpha particle animation that reinforces the theme.

## Turtles (players)
- **69** ✅ [Must·S1·D→B·5] (§4.1, deps 7) Leonardo (Melee): be drawn as a detailed turtle with a blue mask and two katanas — Parts: shell with plate pattern, plastron, head, blue mask with tails, eyes, arms, hands, legs, knee/elbow pads, belt, two katanas (blade, guard, hilt). Never a single primitive.
- **70** ✅ [Must·S2·D→B·3] (§3.2, deps 69) Leonardo (Melee): play idle, walk and katana-swing animations — The swing rotates the arm/katana joint around the shoulder: the weapon must animate in the character's hand (spec table). Also provide the jump and crouch poses used by the jump/duck requirements.
- **71** ✅ [Must·S2·B→D·3] (§3.2, deps 45, 48) Leonardo (Melee): advance, hit enemies at close range for 1 damage per attack and survive 5 hits — Damage lands on the swing's strike frame when an enemy is in melee range. Starter unit, unlocked from the start.
- **72** ✅ [Must·S1·D→B·5] (§4.1, deps 7) Raphael (Ranged): be drawn as a detailed turtle with a red mask and sai — Bulkier stance, throwing pose, sai with three prongs and a grip. Must differ from Leonardo in silhouette and equipment, not only mask color.
- **73** ✅ [Must·S3·D→B·2] (§3.2, deps 72) Raphael (Ranged): play idle, walk and throw animations — The arm winds back and releases; a new sai appears in his hand after each throw. Also provide jump and crouch poses.
- **74** ✅ [Must·S2·B→D·3] (§3.2, deps 45, 75) Raphael (Ranged): advance and throw sai that deal 1 damage, and survive 5 hits — Fires on an interval from the moment he is placed. Unlocks at 2× the Leonardo unlock value (see Open Question 1).
- **75** ✅ [Must·S2·B→D·3] (§3.2, deps 44) Raphael (Ranged): throw a sai that spins in flight, animates on impact and disappears on collision — Projectile entity modeled as a real sai (blade, prongs, grip), rotating in flight, with an impact flash on hit, then removed. It hits only the first villain it touches (no piercing) and ignores other turtles (never harms players).
- **76** ✅ [Must·S2·D→B·5] (§4.1, deps 7) Donatello (Defense): be drawn as a detailed turtle with a purple mask, bo staff and a shell barricade — Wide planted stance, bo staff braced across the body, a shield made from a manhole cover and pipes. Clearly a 'defender' silhouette.
- **77** ✅ [Must·S3·D→B·2] (§3.2, deps 76) Donatello (Defense): play an idle guard animation and a block recoil when hit — No walk or attack animation needed. A small staff twirl or shield brace on block.
- **78** ✅ [Must·S2·B→D·2] (§3.2, deps 41, 44) Donatello (Defense): stay where I am placed and absorb enemy attacks for 7 hits — Never moves, never attacks, ignores jump/duck. Enemies that reach him stop and attack him. Unlocks at 3× the Leonardo unlock value (see Open Question 1).
- **79** ✅ [Must·S2·B→D·3] (§3.2, deps 78) Donatello (Defense): protect turtles standing behind me from enemy attacks — 'Behind' = same lane, left of Donatello (between him and the Lair). Since he can stand anywhere in the lane, he only shields against an attacker on his right (`isShielded` in `Combat.cpp` takes the attacker's x). Villain projectiles hit him first. Once a turtle walks past him, or he is defeated, that turtle is vulnerable again.
- **80** ✅ [Must·S2·B→D·1] (§3.2, deps 45) Turtle (any): pass through or overlap other turtles in my lane — Leonardo and Raphael walk through Donatello and each other without blocking.
- **81** ✅ [Must·S2·C→A·2] (§4.1, deps 8, 10) Turtle (any): show my remaining health above or below me at all times — Health bar + number. Turtle bars are styled differently from villain bars (green with a shell frame vs red with a Foot frame). Bars follow jumps and stay on top of everything.
- **82** ✅ [Must·S3·B→D·2] (§3.2, deps 3, 10) Turtle (any): turn increasingly red as I lose health — tintAmount = 1 − hp/maxHp sent as a uniform, plus an optional white hit flash. The spec also allows scaling the model down per hit instead.
- **83** ✅ [Must·S2·B→D·1] (§3.2, deps 10) Turtle (any): be removed from the scene when my health reaches 0 — Optional short defeat effect (shell spin / smoke) before removal. Frees the lane so enemies resume walking.
- **84** ✅ [Must·S2·B→D·1] (§3.2, deps 71, 74) Turtle (any): deal 1 point of damage per attack (Donatello deals none) — Applies to Leonardo and Raphael only.

## Villains (enemies)
- **85** ✅ [Must·S2·D→B·5] (§4.1, deps 7) Foot Soldier (Easy): be drawn as a detailed Foot Clan ninja — Hood, face mask with eye slit, Foot emblem, armor plates, belt, shuriken pouch. Angular and dark purple/black: nothing like a turtle.
- **86** ✅ [Must·S1·B→D·3] (§3.3, deps 42, 44) Foot Soldier (Easy): walk at normal speed and throw high shuriken that turtles must duck under, starting with 5 lives — HIGH-band projectile, 1 damage, fired on an interval from spawn.
- **87** ✅ [Must·S2·A→C·5] (§4.1, deps 7) Bebop (Medium): be drawn as a detailed mutant warthog with a blaster — Mohawk, tusks, sunglasses, vest, belly, arm-mounted blaster. Round, bulky silhouette.
- **88** ✅ [Must·S2·B→D·3] (§3.3, deps 42, 44) Bebop (Medium): walk at normal speed and fire low blaster shots that turtles must jump over, starting with 5 lives — LOW-band projectile skimming the floor, 1 damage.
- **89** ✅ [Must·S3·A→C·5] (§4.1, deps 7) Rocksteady (Hard): be drawn as a detailed mutant rhino with a horn and armor — Horn, helmet, shoulder armor, heavy fists; the largest villain. Charge pose with dust puffs.
- **90** ✅ [Must·S2·B→D·3] (§3.3, deps 42, 48) Rocksteady (Hard): run toward the turtles and attack at close range for 2 damage, starting with 7 lives — Speed ≈ 1.8× normal. Assumption: melee hits can't be dodged by jump/duck (see Open Questions).
- **91** ✅ [Must·S3·A→C·3] (§3.3, deps 85, 87, 89) Villain (any): play walk and attack animations — Shuriken throw swing, blaster recoil, Rocksteady punch/charge. Uses the same hierarchy joints as the turtles.
- **92** ✅ [Must·S3·B→D·2] (§4.1, deps 86, 88) Villain (any): fire projectiles that look clearly different from the turtles' — Shuriken = 4-point spinning star; blaster bolt = glowing elongated shot. Never the same shape as the sai.
- **93** ✅ [Must·S2·C→A·1] (§4.1, deps 81) Villain (any): show my remaining health above or below me at all times — Same HealthBar component as the turtles with the villain style.
- **94** ✅ [Must·S3·B→D·1] (§3.3, deps 82) Villain (any): turn increasingly red as I lose health — Same tint uniform as the turtles.
- **95** ✅ [Must·S2·B→D·1] (§3.3, deps 10) Villain (any): be removed from the scene when defeated and drop a collectible — Removal + drop happen in the same frame.
- **96** ✅ [Must·S2·B→D·1] (§3.3, deps 86, 88, 90) Villain (any): deal 1 damage per attack (2 for Rocksteady) — Values from Config.
- **97** ✅ [Must·S2·A→C·1] (§3.4, deps 10) Villain (any): trigger the loss when I reach the Lair in any lane — x ≤ Lair edge → Lose state immediately.

## Waves
- **98** ✅ [Must·S2·A→C·2] (§3.3, deps 12, 42) Game system: run three waves of at least 3, 5 and 10 enemies — Wave 1: Foot Soldiers only. Wave 2: Foot Soldiers + Bebop. Wave 3: Foot Soldiers + Bebop + Rocksteady. Default to exactly 3/5/10 (the spec gives minimums, so more is allowed). Data-driven from Config.
- **99** ✅ [Must·S2·A→C·1] (§3.3, deps 98) Game system: guarantee each wave includes every enemy type allowed in it — Random order and lanes, but at least one of each allowed type, so graders always see the mix.
- **100** ✅ [Must·S2·A→C·1] (§3.3, deps 98) Game system: spawn enemies at random intervals during a wave — e.g., 3–6 s apart (tunable). Wave 1's first enemy appears about 2 s after gameplay starts: enemies don't wait for the player.
- **101** ✅ [Must·S2·A→C·2] (§3.3, deps 98) Game system: start a wave when its first enemy appears and end it when its last enemy appears — After the last spawn of a wave, no more enemies of that wave appear.
- **102** ✅ [Must·S2·A→C·1] (§3.3, deps 101) Game system: begin the next wave only after every remaining enemy on screen is defeated — Short pause (2–3 s) and a banner before the next wave's first spawn.
- **103** ✅ [Optional·S3·C→A·1] (§4.2, deps 102, 8) Player: see which wave I'm on — 'Wave 2/3' banner at each wave start and a small HUD indicator. Not required, but it makes the logic obvious in the video.

## Collectibles
- **104** ✅ [Must·S1·D→B·1] (§4.2, deps 7) Player: see a detailed pizza-slice model for collectibles and the coin icon — Crust, cheese, pepperoni and a cheese drip. Reused in the HUD counter.
- **105** ✅ [Must·S2·A→C·2] (§3.3, deps 95, 104) Game system: drop a pizza slice where an enemy is defeated — Bobbing idle animation; it stays until collected (no timeout).
- **106** ✅ [Must·S2·A→C·2] (§3.3, deps 105, 44) Turtle (any): collect a pizza slice by colliding with it — Any turtle overlapping the slice collects it; pop animation + coin counter increment. Drop value configurable (default 15).

## Win & lose
- **107** ✅ [Must·S2·A→C·1] (§3.4, deps 102) Game system: declare a win when every enemy of all three waves is defeated — Wave 3 spawning finished AND no villains alive.
- **108** ✅ [Must·S2·A→C·1] (§3.4, deps 97) Game system: declare a loss as soon as any villain reaches the Lair — Any lane.
- **109** ✅ [Must·S2·A→C·2] (§3.4, deps 107, 108) Game system: freeze everything in the scene on a win or a loss — No movement, attacks, projectiles, spawns or animations continue; the scene stays visible under the Win/Lose Menu.
- **110** ✅ [Must·S2·A→C·1] (§3.4, deps 107, 108) Game system: resolve a win and a loss in the same frame consistently — Check the loss first: an enemy at the Lair means the Lair fell.

## Scene & environment
- **111** ✅ [Must·S2·D→B·3] (§4.2, deps 6, 39) Player: see a sewer-tunnel background that fits the TMNT setting — Arched brick ceiling, gradient wall and a water channel at the bottom, not a flat fill.
- **112** ✅ [Must·S3·C→A·2] (§4.2, deps 111) Player: see sewer pipes with joints, valves and drips along the walls — Environment element 1 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Animated drip particles. Must not cover lanes, characters or health bars.
- **113** ✅ [Must·S3·D→B·2] (§4.2, deps 111) Player: see mutagen barrels with glowing green ooze — Environment element 2 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Barrel bands, hazard symbol, pulsing glow. Ties to the story. Must not cover lanes, characters or health bars.
- **114** ✅ [Must·S3·A→C·1] (§4.2, deps 111) Player: see manhole covers and drain grates set into the walkways — Environment element 3 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Kept flat on the lane floor so they never hide characters. Must not cover lanes, characters or health bars.
- **115** ✅ [Must·S3·A→C·2] (§4.2, deps 111) Player: see hanging work lamps with light cones — Environment element 4 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Subtle flicker animation. Must not cover lanes, characters or health bars.
- **116** ✅ [Must·S3·C→A·2] (§4.2, deps 111) Player: see brick walls with graffiti tags and a lair sign — Environment element 5 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Original graffiti shapes, not real-world logos. Must not cover lanes, characters or health bars.
- **117** ✅ [Optional·S3·A→C·1] (§4.2, deps 111) Player: see stacked pizza boxes and crates near the Lair — Environment element 6 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Sells the turtles' home. Must not cover lanes, characters or health bars.
- **118** ✅ [Optional·S3·C→A·2] (§4.2, deps 111) Player: see an animated sewer-water stream with ripples — Environment element 7 of 7 (the spec requires at least 5 distinct ones, integrated naturally). Scrolling ripple lines along the bottom channel. Must not cover lanes, characters or health bars.
- **119** ✅ [Must·S2·C→A·2] (§4.2, deps 60, 61) Player: see a themed HUD that doesn't cover the playfield — Reserved top bar: cards top-left, pizza counter top-right, wave indicator top-center (matches the spec snippet layout).
- **120** ✅ [Optional·S3·B→D·2] (§4, deps 44) Player: see hit sparks and defeat smoke effects — Small particle bursts; reinforces feedback.
- **121** ✅ [Must·S3·D→B·1] (§3.1) Team member: check that every scene component is built from several detailed parts, not one primitive — Covers characters, the Lair, environment, projectiles, pizza and cards. The spec's disclaimer says single-primitive components lead to significant grade deductions. List the part count per object next to the Theme Bible roster.
- **122** [Must·S3·D→B·1] (§4) Team member: review that every asset fits the TMNT sewer theme — End of Sprint 3: cutscene, story, environment, characters, weapons and UI all belong to the same world (§4).
- **123** ✅ [Must·S3·D→B·1] (§4.1) Team member: verify turtles and villains look completely different and each type differs from the others — Test: switch the grayscale uniform on for the whole scene. Every type must still be recognizable by shape and equipment alone (recoloring alone fails). Designs must also signal attack type and difficulty: Foot Soldier's overhead throwing pose, Bebop's low-slung blaster, Rocksteady as the biggest and toughest-looking.

## Sound (bonus)
- **124** [Bonus·S3·C→A·2] (§5, deps 14) Developer: initialize an audio library and load sounds — miniaudio (single header), SDL_mixer or irrKlang. If no audio device is found, the game still runs silently.
- **125** [Bonus·S3·C→A·1] (§5, deps 124) Player: hear looping background music — Original or royalty-free/CC0 music, not the TMNT theme song (the video may be published online). Credit sources in the README.
- **126** [Bonus·S3·C→A·2] (§5, deps 124) Player: hear sound effects for at least 4 interactions — Plan 6: lane select click, turtle placement, attack/hit, character death, pizza pickup, win/lose sting.
- **127** [Bonus·S3·C→A·2] (§5, deps 22, 124) Player: use a Sound Settings menu to raise, lower or mute the volume — Volume +, Volume −, Mute/Unmute and Back, with the current level shown as a bar or %.
- **128** [Bonus·S3·C→A·1] (§5, deps 127) Player: hear volume and mute changes applied immediately to music and effects — Master volume applied to all existing sounds ('correctly modify existing sounds'). Kept for the whole session.
- **129** [Bonus·S3·C→A·1] (§5, deps 125) Game system: pause, resume and restart the music with the game state — Music restarts on Restart; win/lose sting plays once.

## Testing & balancing
- **130** [Must·S3·B→D·2] (§3.3, deps 98, 106) Team member: balance the economy and difficulty so the game is winnable but not trivial — Tune START_COINS, drop value, HP, speeds and attack intervals in Config.h. A playtester who hasn't seen the code should win within 2–3 tries.
- **131** [Must·S4·B→D·2] (§7) Team member: survive stress cases without crashing — Spam clicks, place many turtles in one lane, select all lanes and hold S, pause/resume rapidly, restart 10× in a row, resize and minimize the window.
- **132** [Must·S4·B→D·2] (§7) Team member: run a full regression pass against this sheet — Every row marked Done is checked by its reviewer (not its owner) before it becomes Tested.

## Video
- **133** [Must·S4·D→B·1] (§6) Team member: write the video script and shot list — Voiceover covers the game description, the aim and how to play. The shot list covers every feature in the recording row.
- **134** [Must·S4·D→B·1] (§6, deps 133) Team member: open the video with the required title sequence — Exact order: GUC name & logo → DMET 502 Computer Graphics, Winter 2026 → project name → students' names → content → Thank you.
- **135** [Must·S4·C→A·3] (§6, deps 133) Team member: record 2–5 minutes of real gameplay with voiceover — Target ~4 min at 1080p with a clear mic. Show: menu, cutscene, lane hover/select, card lock/unlock, all 3 turtles, all 3 villains, jump/duck dodges, Donatello protecting, damage tint, pizza pickup, 3 waves, Pause Menu, a win AND a loss, Sound Settings.
- **136** [Must·S4·C→A·2] (§6, deps 135, 134) Team member: edit, length-check and export the video — Between 2:00 and 5:00, mp4, no copyrighted music, ends with a 'Thank you' card. It may be published online, so polish it.
- **137** [Must·S4·C→A·1] (§6, deps 136) Team member: submit the video link separately through the submission form — The spec prefers a separate link via the same form.

## Submission
- **138** [Must·S4·D→B·1] (§7) Team member: include a README with build steps, controls and asset credits — Team name and members, build steps, controls, and credits for every third-party library, font and sound.
- **139** [Must·S4·A→C·2] (§7, deps 2) Team member: build and run the game from the zip on a clean machine — Unzip → build → run a full playthrough. One compile error = zero.
- **140** [Must·S4·A→C·1] (§7, deps 139) Team member: package only the source files as [Team name]-[Name of one team member].zip — .h/.cpp plus the shader, font and sound files needed to run. No build folders, .exe, .obj or .vs. Confirm with the TA that non-code assets may be included.
- **141** [Must·S4·A→C·1] (§7, deps 140) Team member: upload the zip to Google Drive and grant the graders access — Test the link from an incognito window before submitting.
- **142** [Must·S4·A→C·1] (§7, deps 141) Team member: submit through the official form before Mon 2 Nov 2026, 11:59 PM — Internal deadline: Sun 1 Nov, 9 PM.

## Non-functional
- **NF1** ✅ Graphics API [Must·S1·A]: Use only modern OpenGL 3.3+ (shaders, VAOs/VBOs). No deprecated fixed-function calls anywhere. Check: Search the code for glBegin, glVertex, glColor3, glMatrixMode, glPushMatrix: zero hits.
- **NF2** Build [Must·S4·A]: The project compiles with zero errors on the TAs' environment from the submitted zip. Check: Clean-machine build in Sprint 1 and again in Sprint 4.
- **NF3** Performance [Must·S3·A]: Hold 60 FPS with 30+ entities on screen on a lab-grade PC. Check: Debug overlay FPS during a busy wave 3.
- **NF4** ✅ Performance [Must·S1·A]: Static meshes are uploaded once; only uniforms change per frame. Check: Code review of the draw path.
- **NF5** ✅ Frame independence [Must·S1·A]: Movement, attacks and animations run at the same real-time speed at 30, 60 and 144 Hz. Check: Toggle vsync off/on and compare a turtle crossing a lane.
- **NF6** Memory [Must·S4·B]: Entities and GL objects are freed; memory doesn't grow across restarts. Check: Restart 10× and watch memory in Task Manager.
- **NF7** Robustness [Must·S4·B]: No crash under spam clicks, key mashing, many turtles in one lane, rapid pause/resume, resize or minimize. Check: Stress checklist (functional req. 'survive stress cases').
- **NF8** Resolution [Must·S2·C]: Layout, picking and text stay correct at 1280×720, 1920×1080 and after resizing. Check: Manual check at both sizes.
- **NF9** ✅ Portability [Must·S1·A]: All asset paths are relative; nothing depends on a team member's machine. Check: Run from a freshly unzipped folder.
- **NF10** Code quality [Must·S1·All]: One class/file pair per system (Game, LaneManager, Entity, Turtle, Villain, Projectile, WaveManager, UI, Menu, Cutscene, Models, Audio); tunables only in Config.h; no magic numbers. Check: Peer review before merging to main.
- **NF11** Version control [Must·S1·All]: Git repo with one branch per member; main always builds; merge at least daily. Check: Main builds after every merge.
- **NF12** UX: Feedback [Must·S2·C]: Every click and key press gives a visible response within one frame (hover, press, highlight, jump). Check: Play-test checklist.
- **NF13** ✅ UX: Visibility [Must·S2·C]: Coins, wave, every health value and every card's lock state are always visible. Check: Screenshot review at the busiest moment.
- **NF14** ✅ UX: Learnability [Optional·S3·C]: A first-time player understands how to place, select lanes and dodge within one minute. Check: Controls legend + hallway test with a friend.
- **NF15** ✅ UX: Error design [Must·S2·C]: Invalid actions (card with no lane selected, clicking a locked card) are explained or safely ignored. Check: Try every invalid action once.
- **NF16** ✅ UI consistency [Must·S3·C]: One font, one button style and one palette across all menus and the HUD. Check: Side-by-side screenshot review.
- **NF17** ✅ Visual quality [Must·S3·D]: Every object is composed of multiple detailed parts with smooth animation; no single-primitive models. Check: Grayscale test + parts-count review per model (≥ 12 parts per character).
- **NF18** Theme consistency [Must·S3·D]: Cutscene, story, environment, characters, weapons and UI all belong to the TMNT sewer world. Check: Theme review at the end of Sprint 3.
- **NF19** ✅ Accessibility [Optional·S2·C]: Health is readable without relying on color alone (bar length + number). Check: Grayscale test.
- **NF20** Licensing [Must·S3·C]: Original or royalty-free/CC0 audio and fonts only; no TMNT theme song, official logo or ripped sprites (the video may be published online). Check: Credits list in the README.

## Open questions for the TA (build the default, keep the value in Config.h)
- **OQ1 Unlock costs**: What exact pizza values should unlock Raphael (Ranged) and Donatello (Defense)? Why unclear: Start = enough for 3 Melee, and only Melee starts unlocked. If Ranged = 2 × Melee's price (2 × 10 = 20) and Defense = 3 × 10 = 30, both are affordable with the 30 starting coins, so they'd start unlocked. That contradicts 'all locked except Melee'. **Default:** Base = the 30 starting coins (pizza) that unlock Leonardo: Leonardo 10 per placement, Raphael 60, Donatello 90; drop 15 per villain. (reqs 59, 74, 78, 64) TA answer: —
- **OQ2 Placement with several lanes selected**: If several lanes are selected when a card is clicked, where does the turtle go? Why unclear: Placement needs a selected lane, but jump/duck lets you select several lanes at once. **Default:** Place one turtle in the most recently selected lane. (reqs 66) TA answer: —
- **OQ3 Right edge**: What happens when a turtle reaches the right edge with no enemy? Why unclear: The spec says turtles move 'until they reach the right' but not what follows. **Default:** Walks off-screen and is removed. (reqs 46) TA answer: —
- **OQ4 Dodging melee**: Can jump/duck dodge Rocksteady's melee attack? Why unclear: Only HIGH (duck) and LOW (jump) ranged attacks are defined. **Default:** No: melee hits always land. (reqs 90, 58) TA answer: —
- **OQ5 Duck input**: Is duck held while S/Down is pressed, or a fixed-time action like jump? Why unclear: The spec says 'while pressing S'. **Default:** Held, with a minimum duration. (reqs 57) TA answer: —
- **OQ6 Defense durability**: Does Rocksteady's 2-damage hit remove 2 of Donatello's 7 'hits'? Why unclear: Defense durability is given in hits, other units in lives. **Default:** Treat all as HP: Rocksteady removes 2. (reqs 78) TA answer: —
- **OQ7 Ranged villains meeting turtles**: Do Foot Soldiers and Bebop stop when they collide with a turtle? Why unclear: They attack from a distance, but the general rule says both stop on contact. **Default:** Yes, the contact rule applies to every villain. (reqs 48) TA answer: —
- **OQ8 Villains overlapping**: Can villains in the same lane pass through each other (e.g., a fast Rocksteady behind a fighting Foot Soldier)? Why unclear: The spec lets players overlap but says nothing about enemies. **Default:** Yes, villains overlap like turtles do. (reqs 51) TA answer: —
- **OQ9 Restart**: Should Restart replay the cutscene? Why unclear: The spec only says Start triggers the cutscene. **Default:** No: Restart jumps straight into gameplay; Main Menu → Start replays it. (reqs 25) TA answer: —
- **OQ10 Cutscene skip**: Is a skip key allowed? Why unclear: Not mentioned. **Default:** Optional skip on Space/Enter, only if allowed. (reqs 37) TA answer: —
- **OQ11 Zip contents**: May the zip include shaders, fonts, sounds and third-party libraries? Why unclear: 'Only .h, .cpp, … files' but the game needs these to run. **Default:** Include them; exclude build outputs. (reqs 140) TA answer: —
- **OQ12 Build environment**: Which OS/IDE do the TAs compile on (e.g., Windows + Visual Studio)? Why unclear: Compile errors = zero, so the target environment matters. **Default:** Windows + Visual Studio via CMake. (reqs 2, 139) TA answer: —
- **OQ13 Collecting pizza**: Is pizza collected only by a turtle colliding with it, or also by the mouse? Why unclear: 'Collected when the player collides with them' could mean the turtle or the user. **Default:** A turtle colliding with it. (reqs 106) TA answer: —
- **OQ14 Textures**: Are textures/sprites allowed, or must models be geometry? Why unclear: The spec demands modeling detail; sprites might not count. **Default:** All characters and the Lair are geometry; no character sprites. (reqs 69, 111) TA answer: —
- **OQ15 Franchise theme**: Is a TMNT-based theme OK, given the video may be published online? Why unclear: Existing characters, not an original theme. **Default:** Use it with original models, original dialogue and royalty-free audio; no official logo or theme song. (reqs 125, 136) TA answer: —
- **OQ16 Donatello placement**: Spec says a bought player appears at the lane's leftmost position. May Donatello (stationary) be placed anywhere along the lane instead? Why unclear: Defense does not move, so a spot of the player's choosing is more useful. **Default:** Anywhere in the lane under the cursor, with checks (no stacking on another Donatello, not on a villain), switchable with `DONATELLO_FREE_PLACEMENT` in Config.h. (reqs 41, 66, 79) TA answer: fine to place him anywhere (relayed by the team, 1 Nov 2026).

## Theme bible
- Shell Shock: Defend the Lair (working title)
- Story: Shredder wants the last canister of mutagen, hidden in the turtles' sewer Lair, to build a mutant army. Rocksteady and Bebop lead the Foot Clan through the tunnels; the turtles must hold every lane. If any villain reaches the Lair, the mutagen is lost.
- Spec role | Character | Weapon / attack | Health | Damage | Speed | Cost / unlock (default) | Visual design (must differ by shape, not color) | Model parts (minimum) | Animations
- Player: Melee | Leonardo | Two katanas, close-range slash | 5 lives | 1 | Normal | 10 (unlocked at start) | Slim balanced stance, blue mask, katanas crossed on the back and drawn when attacking. | Shell + plate pattern, plastron, head, mask + tails, eyes, arms, hands, legs, pads, belt, 2 katanas (blade, guard, hilt) | Idle bob, walk cycle, katana swing (arm joint), jump, duck, hit, defeat
- Player: Ranged | Raphael | Throws spinning sai (projectile) | 5 lives | 1 | Normal | 60 = 2 × 30 (see OQ 1) | Bulkier, hunched, red mask, sai on belt, wind-up throwing pose. | Same turtle base + red mask, sai holster, sai in hand, wrist wraps, scowl brow | Idle, walk, throw (wind-up/release), sai spin in flight, impact flash
- Player: Defense | Donatello | None: blocks with a manhole-cover shield + bo staff | 7 hits | 0 | Stationary | 90 = 3 × 30 (see OQ 1) | Wide planted stance, purple mask, tall bo staff, round manhole-cover shield, goggles on head. | Turtle base + shield (rim, bolts), bo staff with wraps, goggles, knee pads | Idle guard sway, block recoil, defeat
- Enemy: Easy (upper ranged) | Foot Soldier | Throws shuriken HIGH → turtles duck | 5 lives | 1 | Normal | Drops 15 pizza | Lean human ninja: hood, eye-slit mask, Foot emblem, armor plates, dark purple/black, angular shapes. | Hood, mask, eye slit, torso armor, emblem, arms, legs, belt, pouch, shuriken | Walk, throw (overhead arm), hit, defeat
- Enemy: Medium (lower ranged) | Bebop | Blaster fires LOW shots → turtles jump | 5 lives | 1 | Normal | Drops 15 pizza | Round bulky warthog: mohawk, tusks, sunglasses, vest, arm blaster. | Head, snout, tusks, mohawk, glasses, belly, vest, arms, blaster, legs, boots | Walk, blaster recoil, hit, defeat
- Enemy: Hard (melee) | Rocksteady | Charging punch / horn butt | 7 lives | 2 | Fast (≈1.8×) | Drops 15 pizza | Largest villain: rhino head with horn, helmet, heavy shoulder armor, big fists. | Head, horn, helmet, ears, armor plates, arms, fists, belt, legs, dust puffs | Run, punch, hit, defeat
- Cutscene only (optional) | Michelangelo, Splinter | — | — | — | — | — | Mikey: orange mask, nunchaku. Splinter: robe, walking staff. Shredder is only mentioned in dialogue, so he needs no model. | Reuse the turtle base for Mikey | Gestures, talking bob
- Part count per object (req 121; draw calls per frame, counted when the models were built, 2026-10-22). Spec disclaimer: no single-primitive components.
- Object | Parts | File
- Leonardo | ~93 | src/models/TurtleModels.cpp
- Raphael | ~105 | src/models/TurtleModels.cpp
- Donatello | ~115 | src/models/TurtleModels.cpp
- Foot Soldier | ~45 | src/models/FootModel.cpp
- Bebop | ~65 | src/models/VillainModels.cpp
- Rocksteady | ~80 | src/models/VillainModels.cpp
- Lair (home base) | ~200 | src/models/LairModel.cpp
- Pizza slice (collectible + HUD icon) | ~29 | src/models/PropModels.cpp
- Sai / shuriken / blaster bolt | multi-part each (blade+prongs+grip / star+hub / glow+core+tail) | src/models/PropModels.cpp
- Mutagen barrel | ~30 each | src/scene/Environment.cpp
- Work lamp | ~17 each | src/scene/Environment.cpp
- Manhole cover / drain grate | 10 / 11 | src/scene/Environment.cpp
- HUD card | frame + name band + full turtle model + pizza icon + cost | src/ui/Hud.cpp
- Check any of them on screen: NinjaTurtlesGame.exe --gallery turtles | villains | props
- Environment elements (spec needs ≥ 5)
- # | Element | Details | Where | Owner
- 1 | Sewer pipes | Joints, valves, rust bands, animated drips | Top wall, behind lanes | C
- 2 | Mutagen barrels | Bands, hazard symbol, pulsing green ooze | Near lane ends, between lanes | D
- 3 | Manhole covers & drain grates | Bolted rings, slots | Flat on the walkways | A
- 4 | Hanging work lamps | Cage, bulb, light cone, flicker | Ceiling | A
- 5 | Brick wall with graffiti + lair sign | Brick pattern, original tags | Back wall | C
- 6 | Pizza boxes & crates | Stacked boxes, crate planks | Next to the Lair | A
- 7 | Sewer-water stream | Scrolling ripple lines | Bottom channel | C
- — | The Lair (home base) | Vault door with shell emblem, pipes, lights; spans all lanes | Left edge | D
- Cutscene storyboard (target 16 s; must stay 10–20 s)
- Time | Shot | On screen (animation) | Dialogue / subtitle | Spec point covered
- 0–3 s | Establishing pan | Camera glides through the sewer tunnel (pipes, lamps, water) to the Lair door; the mutagen canister glows inside. | — | Setting + what is defended
- 3–6 s | Inside the Lair | Leonardo and Raphael stand guard by the canister; Donatello reinforces the door. (Optional: Michelangelo grabs pizza, Splinter watches.) | Leonardo: "Last canister of mutagen. In the wrong hands, it makes monsters." | Why it matters
- 6–9 s | Invasion | Rumble, dust falls; a manhole bursts open on the right; Foot Soldiers drop in; Rocksteady and Bebop stomp out of the dust. | Rocksteady: "Shredder wants that mutagen. Smash everything!" | Enemies introduced, conflict
- 9–12 s | Standoff | Leonardo draws his katanas; Donatello slams his manhole shield down; Bebop raises his blaster. | Leonardo: "You'll have to get through us first." / Bebop: "Easy!" | Players + enemies interacting
- 12–15 s | Clash | Raphael throws a sai that knocks a Foot Soldier's shuriken out of the air; both sides fall back into their lanes. | Raphael: "Next one's not a warning." | Animated interaction
- 15–16 s | Transition | Camera pulls back to the playfield framing; iris wipe into gameplay. | Leonardo: "Turtles, defend the Lair!" | Logical lead-in to gameplay
- Dialogue lines are original placeholders; rewrite them freely, but keep them short and keep the story readable from the animation alone.
- Palette (keep turtles and villains in clearly separate families)
- Use | Hex | Swatch
- Turtle skin | #4C9A2A
- Shell | #7A5C2E
- Plastron | #D8C27A
- Leonardo mask | #1F5FBF
- Raphael mask | #C0262D
- Donatello mask | #7B3FA0
- Foot Clan | #2B1F33
- Bebop / Rocksteady | #6E6E6E
- Mutagen glow | #7CFC00
- Sewer wall | #3A4A44
- Pizza | #F2B33D

## Team split, sprints, working rules
- Team split: who owns what
- Split by system so people rarely edit the same files. Fill in names in the yellow cells; the Owner name column on the requirements sheet updates automatically.
- Member | Name (fill in) | Role | Owns | Main files | Reviews work of
- A | Engine & game rules | Window, shaders, primitive meshes, game loop, state machine, Entity system, lanes, waves, economy (coins/buying/drops), win/lose/freeze, Restart. Bebop & Rocksteady models and villain animations. Build, zip and submission. | main.cpp, Game, Shader, Mesh, LaneManager, Entity, WaveManager, Economy, Config.h, VillainModels | C
- B | Combat & character logic | Collision, movement, spawning positions, attacks and projectiles, jump/duck/dodge, Donatello protection, damage/tint/death, every turtle's and villain's behavior, effects, balancing and stress/regression testing. | Collision, Turtle, Villain, Projectile, Effects | D
- C | UI, input, menus & audio | Text rendering, input, lane hover/select, cards with lock/unlock and placement, pizza counter, health bars, HUD, all three menus, half the environment, the sound bonus, recording and editing the video. | Text, Input, UI, HealthBar, Menu, Audio, Environment (part) | A
- D | Art, models & cutscene | Transform hierarchy, Leonardo/Raphael/Donatello/Foot Soldier models and animations, the Lair, sewer background and part of the environment, pizza model, the whole cutscene, theme reviews, video script and title cards, README. | Hierarchy, TurtleModels, FootModel, LairModel, Background, Cutscene, Timeline | B
- Sprint calendar (deadline: Mon 2 Nov 2026, 11:59 PM)
- Sprint | Dates | Goal | Exit criteria (demo to the team at the end)
- 1.0 | Sat 17 – Wed 21 Oct | Foundations: OpenGL setup, shapes, hierarchy, Entity interface, lanes, input, lane hover/select, text, main menu, first turtle models, movement and jumping on placeholder boxes. | Main Menu → Start shows the lane grid; clicking selects lanes; boxes spawn, walk, jump and duck; Leonardo model renders.
- 2.0 | Thu 22 – Mon 26 Oct | Core game: combat, projectiles, every character's behavior and most models, Donatello protection, cards and coins, waves, collectibles, win/lose, pause and win/lose menus, cutscene first pass. | A full run from wave 1 to a win and to a loss is playable with real models.
- 3.0 | Tue 27 – Thu 29 Oct | Polish and bonus: environment elements, effects, damage tint, Rocksteady model and villain animations, cutscene final, sound bonus, theme and distinctness reviews, balancing. Feature freeze Thu night. | Every Must row is Done; reviewers start marking Tested.
- 4.0 | Fri 30 Oct – Mon 2 Nov | Test and ship: stress + regression testing, clean-machine build, video recording and editing, zip, Drive, form. Internal deadline Sun 1 Nov, 9 PM. | Form submitted, video link submitted, confirmation received.
- Working rules
- 1. Day 1 (Sat 17 / Sun 18 Oct): agree the Entity interface, Config.h names and the shape/hierarchy API together. Everyone builds on them.
- 2. Build gameplay with placeholder boxes first; D swaps in real models through one draw function per character, so logic never waits for art.
- 3. One Git branch per member, merge to main at least daily, and main must always build.
- 4. Pairs review each other (A↔C, B↔D). Only the reviewer moves a row from Done to Tested.
- 5. Unanswered TA questions: build the default from the 'Open Questions' tab and keep the value in Config.h so it's a one-line change.
