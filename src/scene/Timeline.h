#pragma once

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

inline float easeInOut(float k)   // smoothstep: slow start, slow end
{
    return k * k * (3.0f - 2.0f * k);
}
