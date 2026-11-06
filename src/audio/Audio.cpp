#include "audio/Audio.h"
#include <miniaudio.h>
#include <cmath>
#include <iostream>

// miniaudio high-level API (miniaud.io/docs, "Engine" section): one ma_engine mixes everything,
// each effect is a few pre-decoded ma_sound voices, the music is one looping ma_sound.
// The music and effects are original, synthesized by tools/make_sounds.py (req 125, NF20).

struct SoundInfo
{
    const char* file;
    float volume;    // per-effect level, evens out the loudness of the files
    float minGap;    // seconds before the same effect may start again (stops machine-gun stacking)
};

// one row per Sound, in enum order
static const SoundInfo SOUND_TABLE[] = {
    { "assets/sounds/click.wav",       0.50f, 0.03f },   // Click
    { "assets/sounds/lane_select.wav", 0.45f, 0.03f },   // LaneSelect
    { "assets/sounds/place.wav",       0.90f, 0.05f },   // Place
    { "assets/sounds/attack.wav",      0.45f, 0.12f },   // Attack
    { "assets/sounds/hit.wav",         0.80f, 0.06f },   // Hit
    { "assets/sounds/death.wav",       0.45f, 0.08f },   // Death
    { "assets/sounds/pickup.wav",      0.40f, 0.05f },   // Pickup
    { "assets/sounds/win.wav",         0.70f, 0.50f },   // Win
    { "assets/sounds/lose.wav",        0.70f, 0.50f },   // Lose
};
const int SOUND_COUNT = static_cast<int>(Sound::Count);
static_assert(sizeof(SOUND_TABLE) / sizeof(SOUND_TABLE[0]) == SOUND_COUNT, "one SOUND_TABLE row per Sound");

const char* const MUSIC_FILE = "assets/sounds/music.wav";
const float MUSIC_VOLUME = 0.45f;
const int VOICES_PER_SOUND = 6;      // round-robin, so the same effect can overlap itself
const float VOLUME_STEP = 0.1f;
const float VOLUME_STEPS = 10.0f;    // volume is kept on whole 10% steps

static ma_engine g_engine;
static bool g_ready = false;         // engine started (false = no audio device, stay silent)

static ma_sound g_voices[SOUND_COUNT][VOICES_PER_SOUND];
static bool g_loaded[SOUND_COUNT] = {};
static int g_nextVoice[SOUND_COUNT] = {};
static double g_lastPlay[SOUND_COUNT];

static ma_sound g_music;
static bool g_musicLoaded = false;

static float g_volume = 0.7f;
static bool g_muted = false;

const ma_uint32 SOUND_FLAGS = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;

// master volume on the engine itself, so it also changes sounds already playing (req 128)
static void applyVolume()
{
    if (g_ready) ma_engine_set_volume(&g_engine, g_muted ? 0.0f : g_volume);
}

static double engineSeconds()
{
    return ma_engine_get_time_in_milliseconds(&g_engine) / 1000.0;
}

// req 124: a missing device or file must never stop the game
bool Audio::init()
{
    if (ma_engine_init(NULL, &g_engine) != MA_SUCCESS) {
        std::cout << "Audio: no audio device, the game runs without sound" << std::endl;
        return false;
    }
    g_ready = true;

    for (int i = 0; i < SOUND_COUNT; ++i) {
        g_lastPlay[i] = -1000.0;
        int made = 0;
        for (; made < VOICES_PER_SOUND; ++made) {
            if (ma_sound_init_from_file(&g_engine, SOUND_TABLE[i].file, SOUND_FLAGS, NULL, NULL, &g_voices[i][made]) != MA_SUCCESS) break;
            ma_sound_set_volume(&g_voices[i][made], SOUND_TABLE[i].volume);
        }
        if (made < VOICES_PER_SOUND) {   // keep it all-or-nothing, so play() never touches an unmade voice
            for (int v = 0; v < made; ++v) ma_sound_uninit(&g_voices[i][v]);
            std::cout << "Audio: could not load " << SOUND_TABLE[i].file << std::endl;
            continue;
        }
        g_loaded[i] = true;
    }

    if (ma_sound_init_from_file(&g_engine, MUSIC_FILE, SOUND_FLAGS, NULL, NULL, &g_music) == MA_SUCCESS) {
        ma_sound_set_looping(&g_music, MA_TRUE);
        ma_sound_set_volume(&g_music, MUSIC_VOLUME);
        g_musicLoaded = true;
    }
    else {
        std::cout << "Audio: could not load " << MUSIC_FILE << std::endl;
    }

    applyVolume();
    return true;
}

void Audio::shutdown()
{
    if (!g_ready) return;
    for (int i = 0; i < SOUND_COUNT; ++i) {
        if (!g_loaded[i]) continue;
        for (int v = 0; v < VOICES_PER_SOUND; ++v) ma_sound_uninit(&g_voices[i][v]);
        g_loaded[i] = false;
    }
    if (g_musicLoaded) ma_sound_uninit(&g_music);
    g_musicLoaded = false;
    ma_engine_uninit(&g_engine);
    g_ready = false;
}

// req 129: music follows the game state (Game.cpp calls these)
void Audio::playMusic()
{
    if (!g_musicLoaded) return;
    ma_sound_seek_to_pcm_frame(&g_music, 0);
    ma_sound_start(&g_music);
}

void Audio::pauseMusic()
{
    if (g_musicLoaded) ma_sound_stop(&g_music);   // ma_sound_stop keeps the position
}

void Audio::resumeMusic()
{
    if (g_musicLoaded) ma_sound_start(&g_music);
}

void Audio::stopMusic()
{
    if (!g_musicLoaded) return;
    ma_sound_stop(&g_music);
    ma_sound_seek_to_pcm_frame(&g_music, 0);
}

// req 126: next voice of that effect, from its start, unless it was started less than minGap ago
void Audio::play(Sound sound)
{
    int i = static_cast<int>(sound);
    if (!g_ready || i < 0 || i >= SOUND_COUNT || !g_loaded[i]) return;

    double now = engineSeconds();
    if (now - g_lastPlay[i] < SOUND_TABLE[i].minGap) return;
    g_lastPlay[i] = now;

    ma_sound* voice = &g_voices[i][g_nextVoice[i]];
    g_nextVoice[i] = (g_nextVoice[i] + 1) % VOICES_PER_SOUND;
    ma_sound_seek_to_pcm_frame(voice, 0);
    ma_sound_start(voice);
}

// req 127, 128: whole 10% steps, applied at once to everything playing
void Audio::volumeUp()
{
    g_volume = std::round((g_volume + VOLUME_STEP) * VOLUME_STEPS) / VOLUME_STEPS;
    if (g_volume > 1.0f) g_volume = 1.0f;
    applyVolume();
}

void Audio::volumeDown()
{
    g_volume = std::round((g_volume - VOLUME_STEP) * VOLUME_STEPS) / VOLUME_STEPS;
    if (g_volume < 0.0f) g_volume = 0.0f;
    applyVolume();
}

void Audio::toggleMute()
{
    g_muted = !g_muted;
    applyVolume();
}

float Audio::volume()
{
    return g_volume;
}

bool Audio::isMuted()
{
    return g_muted;
}
