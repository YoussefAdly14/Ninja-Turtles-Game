#pragma once

// The 10-20 s animated intro (owner D, spec 2, req 29-38). Storyboard: context/docs/REQUIREMENTS.md -> Theme bible.
// Reuse the gameplay model functions (TurtleModels/VillainModels) so it matches the game.
class Cutscene
{
public:
    void start();
    void update(float dt);
    void draw() const;
    bool finished() const;
    float time() const { return m_time; }

private:
    float m_time = 0.0f;
};
