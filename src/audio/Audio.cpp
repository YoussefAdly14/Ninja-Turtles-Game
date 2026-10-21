#include "audio/Audio.h"

// TODO(C): STUB. Implement with irrKlang (learnopengl In-Practice/2D-Game/Audio):
//   createIrrKlangDevice(), engine->play2D("assets/sounds/<file>", loop), engine->setSoundVolume(v).
// setSoundVolume is the master volume, so it changes sounds that are already playing (req 128).
// Music/SFX must be original or CC0 and credited in the README (req 125, NF20).

static float g_volume = 0.7f;
static bool g_muted = false;
const float VOLUME_STEP = 0.1f;

bool Audio::init() { return true; }
void Audio::shutdown() {}

void Audio::playMusic() {}
void Audio::stopMusic() {}
void Audio::play(Sound /*sound*/) {}

void Audio::volumeUp()
{
    g_volume += VOLUME_STEP;
    if (g_volume > 1.0f) g_volume = 1.0f;
}

void Audio::volumeDown()
{
    g_volume -= VOLUME_STEP;
    if (g_volume < 0.0f) g_volume = 0.0f;
}

void Audio::toggleMute()
{
    g_muted = !g_muted;
}

float Audio::volume()
{
    return g_volume;
}

bool Audio::isMuted()
{
    return g_muted;
}
