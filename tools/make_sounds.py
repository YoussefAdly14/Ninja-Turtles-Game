"""Synthesizes the game's original 8-bit style music and sound effects (req 125, 126).

Nothing is downloaded or sampled, so the sounds are free to use in the published video.
Writes mono 16-bit WAVs into assets/sounds/ (next to this repo's tools/ folder).

    pip install numpy
    python tools/make_sounds.py

Re-run it after changing a sound; the game loads the .wav files at startup (src/audio/Audio.cpp).
"""
import os
import wave

import numpy as np

FX_RATE = 44100
MUSIC_RATE = 22050          # music is long: half rate keeps the file small
FADE_MS = 3.0               # every sound and every note fades in/out over this, so nothing clicks
FX_PEAK = 0.9               # normalized peak level of each effect
MUSIC_PEAK = 0.8            # normalized peak level of the music loop

BPM = 120
BEAT = 60.0 / BPM           # 0.5 s
LOOP_BEATS = 32             # 8 bars of 4/4 = 16 s
LOOP_SECONDS = LOOP_BEATS * BEAT

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "sounds")
rng = np.random.default_rng(502)   # fixed seed: the script always writes the same files


# ---------- building blocks ----------

def midi_hz(note):
    return 440.0 * 2.0 ** ((note - 69) / 12.0)


def times(seconds, rate):
    return np.arange(int(round(seconds * rate))) / rate


def phase_of(freq, rate):
    """Phase (in cycles) for a frequency that may change every sample (sweeps)."""
    return np.cumsum(np.broadcast_to(freq, freq.shape) / rate)


def square(phase, duty=0.5):
    return np.where(phase % 1.0 < duty, 1.0, -1.0)


def triangle(phase):
    return 2.0 * np.abs(2.0 * (phase % 1.0) - 1.0) - 1.0


def noise(n):
    return rng.uniform(-1.0, 1.0, n)


def lowpass(x, cutoff, rate):
    """One-pole low-pass; cutoff may be a per-sample array (filter sweeps)."""
    cutoff = np.broadcast_to(np.asarray(cutoff, dtype=float), x.shape)
    a = 1.0 - np.exp(-2.0 * np.pi * cutoff / rate)
    y = np.zeros_like(x)
    acc = 0.0
    for i in range(len(x)):
        acc += a[i] * (x[i] - acc)
        y[i] = acc
    return y


def decay(n, rate, seconds):
    """Exponential fall to ~1/e after `seconds`."""
    return np.exp(-np.arange(n) / (rate * seconds))


def fade(x, rate, ms=FADE_MS):
    n = min(len(x) // 2, int(rate * ms / 1000.0))
    if n > 0:
        ramp = np.linspace(0.0, 1.0, n)
        x[:n] *= ramp
        x[-n:] *= ramp[::-1]
    return x


def normalize(x, peak):
    m = np.max(np.abs(x))
    return x * (peak / m) if m > 0 else x


def write_wav(name, x, rate):
    data = np.clip(x, -1.0, 1.0)
    pcm = (data * 32767.0).astype("<i2")
    path = os.path.join(OUT_DIR, name)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(pcm.tobytes())
    print(f"{name:16s} {len(x) / rate:6.2f} s  {os.path.getsize(path) // 1024:5d} KB")


def effect(name, x):
    write_wav(name, normalize(fade(x, FX_RATE), FX_PEAK), FX_RATE)


def tone(note, seconds, rate, wave_fn=square, decay_s=None, **kw):
    """One note: oscillator, optional decay, faded ends."""
    t = times(seconds, rate)
    x = wave_fn(midi_hz(note) * t, **kw)
    if decay_s:
        x *= decay(len(x), rate, decay_s)
    return fade(x, rate)


def sequence(notes, rate):
    """notes = [(start_s, length_s, midi, wave_fn, gain, decay_s)] mixed into one buffer."""
    end = max(s + l for s, l, *_ in notes)
    out = np.zeros(int(round(end * rate)) + 1)
    for start, length, note, wave_fn, gain, decay_s in notes:
        x = tone(note, length, rate, wave_fn, decay_s) * gain
        i = int(round(start * rate))
        out[i:i + len(x)] += x
    return out


# ---------- sound effects ----------

def make_click():
    t = times(0.035, FX_RATE)
    x = square(1400.0 * t, 0.5) * decay(len(t), FX_RATE, 0.010)
    effect("click.wav", x)


def make_lane_select():
    a = tone(76, 0.05, FX_RATE, square, 0.04, duty=0.25)   # E5
    b = tone(83, 0.07, FX_RATE, square, 0.05, duty=0.25)   # B5
    effect("lane_select.wav", np.concatenate([a, b]))


def make_place():
    """Smoke 'poof': noise through a closing low-pass + a soft low thump."""
    n = int(0.32 * FX_RATE)
    sweep = np.linspace(3500.0, 250.0, n)
    puff = lowpass(noise(n), sweep, FX_RATE)
    puff *= np.minimum(np.arange(n) / (0.015 * FX_RATE), 1.0) * decay(n, FX_RATE, 0.09)
    t = times(0.32, FX_RATE)
    thump = np.sin(2 * np.pi * phase_of(np.linspace(150.0, 60.0, n), FX_RATE)) * decay(n, FX_RATE, 0.05)
    effect("place.wav", puff * 1.4 + thump * 0.5)


def make_attack():
    """Blade swish: band of noise whose pitch rises, swelling then cutting off."""
    n = int(0.16 * FX_RATE)
    raw = noise(n)
    band = lowpass(raw, np.linspace(1500.0, 7000.0, n), FX_RATE) - lowpass(raw, np.linspace(400.0, 2500.0, n), FX_RATE)
    shape = np.sin(np.linspace(0.0, np.pi, n)) ** 2
    effect("attack.wav", band * shape)


def make_hit():
    """Thud: falling sine + a short noise crack."""
    n = int(0.14 * FX_RATE)
    body = np.sin(2 * np.pi * phase_of(np.linspace(190.0, 55.0, n), FX_RATE)) * decay(n, FX_RATE, 0.045)
    crack = lowpass(noise(n), 2500.0, FX_RATE) * decay(n, FX_RATE, 0.012)
    effect("hit.wav", body + crack * 0.7)


def make_death():
    """Falling square tone with vibrato, the classic 8-bit 'defeated' slide."""
    seconds = 0.65
    n = int(seconds * FX_RATE)
    t = np.arange(n) / FX_RATE
    freq = np.geomspace(700.0, 90.0, n) * (1.0 + 0.04 * np.sin(2 * np.pi * 14.0 * t))
    x = square(phase_of(freq, FX_RATE), 0.5) * np.linspace(1.0, 0.25, n)
    effect("death.wav", lowpass(x, 4000.0, FX_RATE))


def make_pickup():
    """Bright rising arpeggio C6 E6 G6 C7."""
    step = 0.055
    notes = [(i * step, step if i < 3 else 0.16, n, square, 1.0, 0.08 if i == 3 else None)
             for i, n in enumerate([84, 88, 91, 96])]
    notes = [(s, l, n, lambda p, duty=0.25: square(p, duty), g, d) for s, l, n, _, g, d in notes]
    effect("pickup.wav", sequence(notes, FX_RATE))


def make_win():
    """Fanfare: G4 C5 E5 G5 pickups, then a held C major chord."""
    sq = lambda p: square(p, 0.5)
    sq25 = lambda p: square(p, 0.25)
    notes = []
    for i, n in enumerate([67, 72, 76, 79]):
        notes.append((i * 0.12, 0.11, n, sq25, 0.8, None))
    notes.append((0.48, 0.18, 76, sq25, 0.8, None))
    notes.append((0.66, 0.9, 84, sq25, 0.8, 0.6))           # C6 held
    notes.append((0.66, 0.9, 79, sq, 0.35, 0.6))            # G5
    notes.append((0.66, 0.9, 76, sq, 0.35, 0.6))            # E5
    notes.append((0.66, 0.9, 48, triangle, 0.9, 0.8))       # C3 bass
    effect("win.wav", sequence(notes, FX_RATE))


def make_lose():
    """Falling C minor line G4 F4 Eb4 D4 -> long C4, with a low bass under it."""
    sq = lambda p: square(p, 0.5)
    notes = [(i * 0.22, 0.2, n, sq, 0.7, None) for i, n in enumerate([67, 65, 63, 62])]
    notes.append((0.88, 1.0, 60, sq, 0.7, 0.5))
    notes += [(i * 0.22, 0.2, n, triangle, 0.9, None) for i, n in enumerate([43, 41, 39, 38])]
    notes.append((0.88, 1.0, 36, triangle, 0.9, 0.6))
    effect("lose.wav", lowpass(sequence(notes, FX_RATE), 3500.0, FX_RATE))


# ---------- music: 16 s loop, 120 bpm, A minor, Am F C G | Am F G E ----------

def make_music():
    rate = MUSIC_RATE
    total = int(round(LOOP_SECONDS * rate))
    out = np.zeros(total)

    def add(start_beat, length_beats, x):
        """Mixes x in at a beat; anything that would pass the loop end is cut and re-faded."""
        i = int(round(start_beat * BEAT * rate))
        x = x[:max(0, total - i)].copy()
        if len(x) == 0:
            return
        out[i:i + len(x)] += fade(x, rate)

    def note(start_beat, length_beats, midi, wave_fn, gain, decay_s=None, gate=0.9):
        seconds = length_beats * BEAT * gate
        end_limit = LOOP_SECONDS - start_beat * BEAT    # never run past the loop end
        seconds = min(seconds, end_limit)
        add(start_beat, length_beats, tone(midi, seconds, rate, wave_fn, decay_s) * gain)

    lead = lambda p: square(p, 0.25)
    bass = triangle

    # bass: eighth notes, root and octave
    roots = [45, 41, 48, 43, 45, 41, 43, 40]   # A2 F2 C3 G2 A2 F2 G2 E2
    for bar, root in enumerate(roots):
        for e in range(8):
            midi = root + (12 if e % 2 else 0)
            note(bar * 4 + e * 0.5, 0.5, midi, bass, 0.55, gate=0.8)

    # lead melody: (beat, length in beats, midi)
    melody = [
        (0, 1, 69), (1, .5, 72), (1.5, .5, 76), (2, .5, 74), (2.5, .5, 72), (3, 1, 69),          # Am
        (4, 1, 72), (5, .5, 77), (5.5, .5, 76), (6, 1, 72), (7, 1, 69),                           # F
        (8, .5, 67), (8.5, .5, 72), (9, .5, 76), (9.5, .5, 79), (10, 1, 76), (11, 1, 72),         # C
        (12, 1, 71), (13, .5, 74), (13.5, .5, 79), (14, 2, 74),                                   # G
        (16, 1, 81), (17, .5, 84), (17.5, .5, 88), (18, .5, 86), (18.5, .5, 84), (19, 1, 81),     # Am (up)
        (20, 1, 84), (21, .5, 89), (21.5, .5, 88), (22, 1, 84), (23, 1, 81),                      # F (up)
        (24, .5, 79), (24.5, .5, 77), (25, .5, 74), (25.5, .5, 71), (26, 1, 74), (27, 1, 79),     # G
        (28, 1, 76), (29, .5, 80), (29.5, .5, 83), (30, 2, 76),                                   # E -> back to Am
    ]
    for beat, length, midi in melody:
        note(beat, length, midi, lead, 0.32, decay_s=0.5)

    # drums: kick on 1 and 3, snare on 2 and 4, closed hats on every eighth
    kick_n = int(0.16 * rate)
    kick = np.sin(2 * np.pi * phase_of(np.linspace(140.0, 45.0, kick_n), rate)) * decay(kick_n, rate, 0.05)
    snare_n = int(0.14 * rate)
    snare = (lowpass(noise(snare_n), 5000.0, rate) * decay(snare_n, rate, 0.04)
             + np.sin(2 * np.pi * 190.0 * np.arange(snare_n) / rate) * decay(snare_n, rate, 0.02) * 0.5)
    hat_n = int(0.04 * rate)
    hat_raw = noise(hat_n)
    hat = (hat_raw - lowpass(hat_raw, 6000.0, rate)) * decay(hat_n, rate, 0.01)

    for beat in range(LOOP_BEATS):
        add(beat, 1, (kick if beat % 2 == 0 else snare) * (0.9 if beat % 2 == 0 else 0.45))
        add(beat, 0.5, hat * 0.35)
        add(beat + 0.5, 0.5, hat * 0.25)

    assert len(out) == total
    write_wav("music.wav", normalize(out, MUSIC_PEAK), rate)


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    make_click()
    make_lane_select()
    make_place()
    make_attack()
    make_hit()
    make_death()
    make_pickup()
    make_win()
    make_lose()
    make_music()
