#pragma once

// Sound bonus (owner C, spec 5, req 124-129). irrKlang only (CLAUDE.md, learnopengl Breakout "Audio").
// The whole game calls these; while this is a stub they do nothing, and if no audio device
// exists they must keep doing nothing instead of crashing.
enum class Sound { LaneSelect, Place, Attack, Hit, Death, Pickup, Win, Lose };

namespace Audio
{
    bool init();
    void shutdown();

    void playMusic();               // looping background track
    void stopMusic();
    void play(Sound sound);

    void volumeUp();
    void volumeDown();
    void toggleMute();
    float volume();                 // 0..1, for the settings screen
    bool isMuted();
}
