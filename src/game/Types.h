#pragma once
// Shared vocabulary. No dependencies, so any module (logic, models, UI) can include it.

enum class TurtleKind { Leonardo, Raphael, Donatello, Count };        // Melee, Ranged, Defense

// Order matters: wave N allows the first N kinds (wave 1 Foot only, wave 2 + Bebop, wave 3 + Rocksteady).
enum class VillainKind { FootSoldier, Bebop, Rocksteady, Count };    // Easy, Medium, Hard

enum class ProjectileKind { Sai, Shuriken, BlasterBolt };

// Height band of an attack (req 58): High is dodged by ducking, Low by jumping, Mid by neither.
enum class AttackBand { High, Mid, Low };
