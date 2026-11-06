#pragma once

// Sound bonus (owner C, spec 5, req 124-129). miniaudio's high-level engine (ma_engine / ma_sound),
// team decision in CLAUDE.md. If no audio device exists every call quietly does nothing.
// Adding a sound: a value before Count, a row in SOUND_TABLE (Audio.cpp), a .wav in assets/sounds/.
enum class Sound { Click, LaneSelect, Place, Attack, Hit, Death, Pickup, Win, Lose, Count };

namespace Audio
{
    bool init();
    void shutdown();

    void playMusic();               // looping background track, from the start
    void pauseMusic();              // stop, keep the position
    void resumeMusic();             // carry on from where pauseMusic() left it
    void stopMusic();               // stop and rewind
    void play(Sound sound);

    void volumeUp();
    void volumeDown();
    void toggleMute();
    float volume();                 // 0..1, for the settings screen
    bool isMuted();
}
