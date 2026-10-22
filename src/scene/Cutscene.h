#pragma once

// The 16 s animated intro (owner D, spec 2, req 29-38). Storyboard: context/docs/REQUIREMENTS.md -> Theme bible.
// Reuses the gameplay model functions (TurtleModels/VillainModels/PropModels/LairModel) so it matches the game.
// Every actor and the camera are driven by keyframe tracks (scene/Timeline.h), see the shot list in Cutscene.cpp.
class Cutscene
{
public:
    void start();
    void update(float dt);
    void draw() const;
    bool finished() const;
    void skip();                  // req 37 (optional): jump to the end, Game then switches to Playing
    float time() const { return m_time; }

private:
    float m_time = 0.0f;
    double m_startWallTime = 0.0;   // debug builds log the measured length (req 31)
    bool m_logged = false;
};
