#pragma once

#include <array>
#include "Engine.h"

namespace cj
{
enum Mode { engineMode, musicalMode };

struct Preset
{
    const char* name;
    const char* hint;      // what Jarrad would say about it
    int mode, layout;
    float idle, redline, flywheel;
    bool gearbox;
    float pulse, rough, rasp, mech;
    float pipe, reso, muffler, crackle, width;
    float intake, turbo, blower;
    float attack, decay, sustain, release, glide;  // ms, except sustain
    int octave;
    float revAmt, revTime;                          // rev-in pitch sweep (amount, ms)
    bool mono;
    float garage;
};

constexpr int numEnginePresets = 14; // the first presets are real engines: they're the quiz pool

inline const std::array<Preset, 20>& factoryPresets()
{
    //   name                    hint
    //   mode         layout     idle   redline fly   gbox  pulse rough rasp  mech   pipe  reso  muff  crack width  intake turbo blower
    //   atk    dec     sus   rel     glide  oct  rev   revT  mono  garage
    static const std::array<Preset, 20> presets {{
        { "Muscle V8 Burble", "Cross-plane crank, so each bank fires unevenly. That lumpy burble gives it away.",
          engineMode, v8Cross,   700, 6500, 0.55f, true,  0.45f, 0.45f, 0.35f, 0.20f, 1.6f, 0.55f, 0.35f, 0.35f, 0.70f, 0.30f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Blown Drag V8", "Cross-plane V8 with a supercharger whining on top, and no muffler at all.",
          engineMode, v8Cross,   950, 8000, 0.35f, true,  0.30f, 0.60f, 0.70f, 0.35f, 0.6f, 0.45f, 0.05f, 0.60f, 0.80f, 0.45f, 0.00f, 0.80f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.20f },
        { "Italian Flat-Plane V8", "Flat-plane V8: the banks take turns evenly, so it screams instead of burbling.",
          engineMode, v8Flat,   1000, 9000, 0.25f, true,  0.25f, 0.15f, 0.55f, 0.25f, 1.1f, 0.60f, 0.20f, 0.40f, 0.80f, 0.55f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Rally Boxer Turbo", "Flat-four with unequal-length headers. That off-beat rumble is the giveaway.",
          engineMode, boxer4,    850, 7200, 0.35f, true,  0.40f, 0.35f, 0.45f, 0.25f, 1.3f, 0.50f, 0.30f, 0.55f, 0.60f, 0.35f, 0.80f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Five-Pot Quattro", "Inline-five: 144 degrees between bangs gives it that odd warble.",
          engineMode, inline5,   850, 7000, 0.30f, true,  0.35f, 0.25f, 0.50f, 0.20f, 1.4f, 0.55f, 0.25f, 0.50f, 0.40f, 0.40f, 0.65f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Straight-Six Turbo", "Inline-six: perfectly balanced, silky smooth, and a big turbo spooling.",
          engineMode, inline6,   750, 7200, 0.35f, true,  0.30f, 0.12f, 0.35f, 0.20f, 1.8f, 0.60f, 0.30f, 0.30f, 0.50f, 0.40f, 0.90f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Rotary Brap", "Rotary: no valves and long exhaust ports. It goes brap-brap-brap at idle.",
          engineMode, rotary,    950, 9000, 0.20f, true,  0.55f, 0.55f, 0.75f, 0.10f, 0.9f, 0.50f, 0.15f, 0.65f, 0.50f, 0.30f, 0.30f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Big Twin Cruiser", "45-degree V-twin: an uneven potato-potato idle, because the two bangs aren't evenly spaced.",
          engineMode, vTwin,     850, 5800, 0.65f, false, 0.50f, 0.50f, 0.40f, 0.35f, 1.0f, 0.45f, 0.25f, 0.45f, 0.50f, 0.30f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "V12 Grand Tourer", "V12: twelve small bangs per cycle make one smooth, silky tear.",
          engineMode, v12,       900, 8500, 0.35f, true,  0.20f, 0.10f, 0.40f, 0.20f, 1.5f, 0.60f, 0.35f, 0.25f, 0.90f, 0.50f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.20f },
        { "V10 Screamer", "V10: a high-revving howl that sits between a V8 and a V12.",
          engineMode, v10,      1000, 8700, 0.20f, true,  0.20f, 0.15f, 0.60f, 0.25f, 0.9f, 0.60f, 0.20f, 0.35f, 0.85f, 0.60f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.20f },
        { "Hot Hatch Triple", "Three-cylinder: a thrummy, slightly off-beat growl, with a small turbo.",
          engineMode, inline3,   850, 6800, 0.30f, true,  0.35f, 0.30f, 0.45f, 0.25f, 1.2f, 0.50f, 0.40f, 0.50f, 0.40f, 0.40f, 0.70f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Diesel Workhorse", "Diesel: that clatter is compression-ignition knock, and it runs out of revs early.",
          engineMode, dieselI4,  750, 4500, 0.70f, true,  0.50f, 0.30f, 0.30f, 0.80f, 2.2f, 0.40f, 0.55f, 0.00f, 0.30f, 0.30f, 0.60f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "V6 Daily", "Even-fire V6: smooth, quiet and sensible. Jarrad clocks it straight away.",
          engineMode, v6,        750, 6600, 0.45f, true,  0.40f, 0.20f, 0.25f, 0.20f, 2.0f, 0.45f, 0.60f, 0.10f, 0.50f, 0.30f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },
        { "Single Thumper", "Single cylinder: thump... thump... You can count each bang.",
          engineMode, single,   1100, 7500, 0.30f, false, 0.45f, 0.40f, 0.55f, 0.40f, 0.8f, 0.50f, 0.30f, 0.50f, 0.00f, 0.35f, 0.00f, 0.00f,
          5, 400, 0.8f, 150, 0, 0, 0.0f, 100, false, 0.15f },

        // Musical presets: the note you play sets the firing frequency
        { "Burble Bass", "V8 burble, tuned to the keyboard. Low notes idle, high notes rev.",
          musicalMode, v8Cross,  700, 6500, 0.50f, false, 0.45f, 0.35f, 0.45f, 0.15f, 1.4f, 0.55f, 0.45f, 0.20f, 0.60f, 0.30f, 0.00f, 0.00f,
          4, 500, 0.85f, 140, 60, 0, 0.10f, 80, true, 0.10f },
        { "Redline Lead", "Flat-plane V8 lead. Each note revs up into pitch.",
          musicalMode, v8Flat,  1000, 9000, 0.30f, false, 0.25f, 0.15f, 0.70f, 0.20f, 1.0f, 0.60f, 0.10f, 0.50f, 0.80f, 0.50f, 0.30f, 0.00f,
          8, 600, 1.0f, 220, 90, 0, 0.35f, 220, true, 0.25f },
        { "Turbo Pad", "Inline-six chords with turbos spooling slowly into every note.",
          musicalMode, inline6,  750, 7200, 0.40f, false, 0.30f, 0.12f, 0.20f, 0.10f, 1.8f, 0.60f, 0.50f, 0.05f, 1.00f, 0.40f, 1.00f, 0.00f,
          600, 1500, 0.8f, 1500, 0, 0, 0.15f, 900, false, 0.40f },
        { "Rotary Pluck", "Rotary brap as a pluck, with crackle on the release.",
          musicalMode, rotary,   950, 9000, 0.20f, false, 0.55f, 0.45f, 0.60f, 0.10f, 0.9f, 0.50f, 0.20f, 0.60f, 0.50f, 0.30f, 0.00f, 0.00f,
          2, 350, 0.0f, 250, 0, 0, 0.25f, 60, false, 0.25f },
        { "V12 Choir", "Twelve cylinders per voice, and a long garage tail.",
          musicalMode, v12,      900, 8500, 0.35f, false, 0.25f, 0.10f, 0.30f, 0.10f, 2.5f, 0.70f, 0.40f, 0.05f, 1.00f, 0.40f, 0.00f, 0.00f,
          300, 800, 0.9f, 1200, 0, 0, 0.05f, 400, false, 0.50f },
        { "Diesel Sub", "Diesel clatter, an octave down. It shakes the room.",
          musicalMode, dieselI4, 750, 4500, 0.70f, false, 0.50f, 0.30f, 0.30f, 0.50f, 2.2f, 0.45f, 0.60f, 0.00f, 0.30f, 0.20f, 0.30f, 0.00f,
          5, 600, 0.9f, 120, 30, -1, 0.05f, 60, true, 0.10f },
    }};
    return presets;
}

inline EngineSettings settingsFromPreset (const Preset& p) noexcept
{
    EngineSettings s;
    s.layout = p.layout;
    s.pulse = p.pulse;
    s.rough = p.rough;
    s.rasp = p.rasp;
    s.mech = p.mech;
    s.pipe = p.pipe;
    s.reso = p.reso;
    s.muffler = p.muffler;
    s.crackle = p.crackle;
    s.width = p.width;
    s.intake = p.intake;
    s.turbo = p.turbo;
    s.blower = p.blower;
    return s;
}

} // namespace cj
