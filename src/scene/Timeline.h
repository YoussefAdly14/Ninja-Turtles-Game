#pragma once
#include <vector>
#include <glm/glm.hpp>

// Tiny keyframe helpers for the cutscene (req 30). Plain math, no library.
//   float k = segment(t, 3.0f, 6.0f);           // 0 before 3 s, 0..1 during, 1 after 6 s
//   float x = lerp(100.0f, 700.0f, easeInOut(k));

inline float clamp01(float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

inline float segment(float t, float start, float end)
{
    return clamp01((t - start) / (end - start));
}

inline float lerp(float a, float b, float k)
{
    return a + (b - a) * k;
}

inline glm::vec2 lerp(glm::vec2 a, glm::vec2 b, float k)
{
    return a + (b - a) * k;
}

inline float easeInOut(float k)   // smoothstep: slow start, slow end
{
    return k * k * (3.0f - 2.0f * k);
}

// ---------------- keyframe tracks ----------------
// A track is a list of keys sorted by time. Between two keys the value is interpolated;
// the ease of the LATER key says how we get there. Before the first key / after the last
// the track holds the end value. Use float tracks for rotation, scale, alpha and visibility
// (0/1 with Step), vec2 tracks for positions and the camera.
//
//   const Track<glm::vec2> leoPath = {
//       { 3.0f, { 120.0f, 257.0f } },                      // start (ease ignored on the first key)
//       { 4.5f, { 360.0f, 257.0f }, Ease::InOut },          // walk out, slow at both ends
//   };
//   glm::vec2 p = sample(leoPath, t);

enum class Ease { Linear, InOut, Step };   // Step: jump to the new value when the key is reached

template <typename T>
struct Key
{
    float time;
    T value;
    Ease ease = Ease::Linear;
};

template <typename T>
using Track = std::vector<Key<T>>;

template <typename T>
T sample(const Track<T>& keys, float t)
{
    if (keys.empty()) return T();
    if (t <= keys.front().time) return keys.front().value;
    for (size_t i = 1; i < keys.size(); ++i) {
        const Key<T>& a = keys[i - 1];
        const Key<T>& b = keys[i];
        if (t < b.time) {
            if (b.ease == Ease::Step) return a.value;
            float k = segment(t, a.time, b.time);
            if (b.ease == Ease::InOut) k = easeInOut(k);
            return lerp(a.value, b.value, k);
        }
    }
    return keys.back().value;
}

// 0..1 that fades in over `fade` seconds at `start`, holds, and fades out before `end`
// (subtitles, dust puffs, flashes).
inline float fadeWindow(float t, float start, float end, float fade)
{
    return segment(t, start, start + fade) * (1.0f - segment(t, end - fade, end));
}
