#pragma once
#include <glm/glm.hpp>
#include "game/Types.h"
#include "models/Pose.h"

// Leonardo, Raphael, Donatello (owner D, req 69-77). Also used by the cutscene and the HUD card portraits.
//
// Contract for every model function in models/:
//   base = world matrix of the character's FEET CENTRE (the caller translates/scales it).
//   Model units = world units: a standing turtle is about TURTLE_HEIGHT tall. Turtles face RIGHT.
//   Build every part from Renderer shapes / your own meshes with core/Transform.h (parent * local).
//   Never a single primitive (spec disclaimer): >= 12 parts per character (NF17).
namespace TurtleModels
{
    void init();       // create any custom meshes ONCE here (createPolygonMesh...), never per frame
    void shutdown();   // destroyMesh them

    void drawLeonardo(const glm::mat4& base, const Pose& pose);
    void drawRaphael(const glm::mat4& base, const Pose& pose);
    void drawDonatello(const glm::mat4& base, const Pose& pose);
    void draw(TurtleKind kind, const glm::mat4& base, const Pose& pose);
}
