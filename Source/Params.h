#pragma once

#include <array>

namespace me
{
constexpr int numVoices   = 8;
constexpr int numSteps    = 16;
constexpr int numPatterns = 8;

enum VoiceIndex { kick, snare, clap, closedHat, openHat, lowTom, highTom, edgePerc };

struct VoiceInfo
{
    const char* id;
    const char* name;
    std::array<const char*, 4> knobNames; // labels for tune / decay / tone / snap
    int midiNote;                          // primary GM note
    unsigned int colour;
};

inline constexpr std::array<VoiceInfo, numVoices> voiceInfo {{
    { "kick",  "KICK",      { "TUNE",  "DECAY", "CLICK",  "PUNCH"  }, 36, 0xffff2a6d },
    { "snare", "SNARE",     { "TUNE",  "DECAY", "TONE",   "SNAPPY" }, 38, 0xff05d9e8 },
    { "clap",  "CLAP",      { "TUNE",  "DECAY", "TONE",   "SPREAD" }, 39, 0xfff9c80e },
    { "chat",  "CL HAT",    { "TUNE",  "DECAY", "TONE",   "METAL"  }, 42, 0xffb967ff },
    { "ohat",  "OP HAT",    { "TUNE",  "DECAY", "TONE",   "METAL"  }, 46, 0xffd29bff },
    { "ltom",  "LO TOM",    { "TUNE",  "DECAY", "STICK",  "DROP"   }, 45, 0xffff6c11 },
    { "htom",  "HI TOM",    { "TUNE",  "DECAY", "STICK",  "DROP"   }, 48, 0xffff9e45 },
    { "perc",  "EDGE PERC", { "PITCH", "DECAY", "CUTOFF", "FM"     }, 37, 0xff01ffc3 },
}};

// Per-voice parameter suffixes, in the order the kit tables below use them
inline constexpr std::array<const char*, 9> voiceParamSuffixes {
    "tune", "decay", "tone", "snap", "drive", "verb", "blend", "pan", "level"
};

// Map incoming MIDI notes to voices (GM drum map, with a few aliases)
inline int noteToVoice (int note)
{
    switch (note)
    {
        case 35: case 36:          return kick;
        case 38: case 40:          return snare;
        case 39:                   return clap;
        case 42: case 44:          return closedHat;
        case 46:                   return openHat;
        case 41: case 43: case 45: return lowTom;
        case 47: case 48: case 50: return highTom;
        case 37: case 56: case 75: return edgePerc;
        default:                   return -1;
    }
}

//==============================================================================
struct VoiceSettings { float tune, decay, tone, snap, drive, verb, blend, pan, level; };

struct Kit
{
    const char* name;
    std::array<VoiceSettings, numVoices> voices;
    // vintage, heat, punch, verbSize, verbGateMs, verbTone, verbMix
    float vintage, heat, punch, verbSize, verbGate, verbTone, verbMix;
};

inline const std::array<Kit, 5>& factoryKits()
{
    static const std::array<Kit, 5> kits {{
        { "Midnight 84",
          {{ {  0.0f, 0.45f, 0.40f, 0.55f, 0.25f, 0.00f, 0, 0.00f,  0.0f },
             {  0.0f, 0.45f, 0.50f, 0.55f, 0.15f, 0.45f, 0, 0.00f, -1.0f },
             {  0.0f, 0.40f, 0.50f, 0.50f, 0.10f, 0.40f, 0, 0.10f, -4.0f },
             {  0.0f, 0.30f, 0.60f, 0.40f, 0.00f, 0.00f, 0, 0.20f, -8.0f },
             {  0.0f, 0.45f, 0.60f, 0.40f, 0.00f, 0.05f, 0, 0.20f, -9.0f },
             { -2.0f, 0.50f, 0.30f, 0.50f, 0.15f, 0.50f, 0,-0.30f, -3.0f },
             {  0.0f, 0.45f, 0.30f, 0.50f, 0.15f, 0.50f, 0, 0.30f, -3.0f },
             {  0.0f, 0.35f, 0.50f, 0.40f, 0.10f, 0.20f, 0,-0.15f, -8.0f } }},
          0.35f, 0.25f, 0.35f, 0.75f, 350.0f, 0.50f, 0.55f },

        { "Turbo Killer",
          {{ { -1.0f, 0.55f, 0.65f, 0.85f, 0.75f, 0.00f, 0, 0.00f,  0.0f },
             { -1.0f, 0.50f, 0.60f, 0.75f, 0.55f, 0.55f, 0, 0.00f, -1.0f },
             {  0.0f, 0.45f, 0.55f, 0.60f, 0.40f, 0.50f, 0, 0.00f, -3.0f },
             {  0.0f, 0.25f, 0.70f, 0.55f, 0.20f, 0.00f, 0, 0.15f, -9.0f },
             {  0.0f, 0.40f, 0.70f, 0.55f, 0.20f, 0.05f, 0, 0.15f,-10.0f },
             { -3.0f, 0.55f, 0.45f, 0.70f, 0.50f, 0.55f, 0,-0.35f, -3.0f },
             { -1.0f, 0.50f, 0.45f, 0.70f, 0.50f, 0.55f, 0, 0.35f, -3.0f },
             { -5.0f, 0.30f, 0.65f, 0.70f, 0.50f, 0.20f, 0, 0.00f, -8.0f } }},
          0.20f, 0.60f, 0.70f, 0.70f, 260.0f, 0.55f, 0.60f },

        { "Gated '85",
          {{ {  0.0f, 0.40f, 0.35f, 0.50f, 0.15f, 0.05f, 0, 0.00f,  0.0f },
             {  1.0f, 0.40f, 0.45f, 0.50f, 0.10f, 0.85f, 0, 0.00f, -2.0f },
             {  0.0f, 0.35f, 0.45f, 0.50f, 0.05f, 0.75f, 0, 0.00f, -5.0f },
             {  0.0f, 0.25f, 0.55f, 0.35f, 0.00f, 0.00f, 0, 0.25f, -9.0f },
             {  0.0f, 0.40f, 0.55f, 0.35f, 0.00f, 0.10f, 0, 0.25f,-10.0f },
             { -2.0f, 0.45f, 0.30f, 0.60f, 0.10f, 0.85f, 0,-0.40f, -3.0f },
             {  1.0f, 0.40f, 0.30f, 0.60f, 0.10f, 0.85f, 0, 0.40f, -3.0f },
             {  2.0f, 0.30f, 0.45f, 0.30f, 0.00f, 0.40f, 0,-0.20f, -9.0f } }},
          0.30f, 0.20f, 0.45f, 0.92f, 450.0f, 0.45f, 0.75f },

        { "Dark Ghost",
          {{ { -3.0f, 0.65f, 0.35f, 0.70f, 0.60f, 0.05f, 0, 0.00f,  0.0f },
             { -3.0f, 0.60f, 0.35f, 0.60f, 0.45f, 0.65f, 0, 0.00f, -1.0f },
             { -2.0f, 0.55f, 0.35f, 0.60f, 0.35f, 0.65f, 0, 0.00f, -4.0f },
             { -2.0f, 0.30f, 0.45f, 0.50f, 0.25f, 0.05f, 0, 0.15f,-10.0f },
             { -2.0f, 0.50f, 0.45f, 0.50f, 0.25f, 0.10f, 0, 0.15f,-11.0f },
             { -5.0f, 0.65f, 0.25f, 0.70f, 0.45f, 0.70f, 0,-0.35f, -3.0f },
             { -3.0f, 0.60f, 0.25f, 0.70f, 0.45f, 0.70f, 0, 0.35f, -3.0f },
             { -7.0f, 0.45f, 0.35f, 0.80f, 0.45f, 0.40f, 0, 0.00f, -7.0f } }},
          0.50f, 0.50f, 0.60f, 0.85f, 600.0f, 0.30f, 0.60f },

        { "Edge Metal",
          {{ {  2.0f, 0.25f, 0.70f, 0.90f, 0.50f, 0.00f, 0, 0.00f,  0.0f },
             {  2.0f, 0.30f, 0.70f, 0.80f, 0.35f, 0.30f, 0, 0.00f, -2.0f },
             {  2.0f, 0.30f, 0.70f, 0.30f, 0.30f, 0.30f, 0, 0.00f, -5.0f },
             {  3.0f, 0.20f, 0.80f, 0.90f, 0.20f, 0.00f, 0, 0.25f, -9.0f },
             {  3.0f, 0.35f, 0.80f, 0.90f, 0.20f, 0.05f, 0, 0.25f,-10.0f },
             {  0.0f, 0.35f, 0.60f, 0.80f, 0.30f, 0.30f, 0,-0.30f, -4.0f },
             {  2.0f, 0.30f, 0.60f, 0.80f, 0.30f, 0.30f, 0, 0.30f, -4.0f },
             {  0.0f, 0.40f, 0.65f, 0.85f, 0.40f, 0.30f, 0, 0.00f, -5.0f } }},
          0.00f, 0.40f, 0.55f, 0.60f, 220.0f, 0.60f, 0.45f },
    }};
    return kits;
}

//==============================================================================
// Default patterns: one row per voice, '.' = off, 'x' = on, 'X' = accent
struct PatternText { std::array<const char*, numVoices> rows; };

inline const std::array<PatternText, 4>& defaultPatterns()
{
    static const std::array<PatternText, 4> patterns {{
        // A: straight synthwave
        {{ "X...x...X...x...",
           "....X.......X...",
           "....x.......x...",
           "x.x.x.x.x.x.x...",
           "..............x.",
           "................",
           "................",
           "................" }},
        // B: half-time darksynth
        {{ "X......x..x.....",
           "........X.......",
           "........x.......",
           "x.xxx.xxx.xxx.xx",
           "................",
           "................",
           "................",
           "...x.......x...." }},
        // C: driving 16ths
        {{ "X...x...X...x...",
           "....X.......X...",
           "................",
           "xxX.xxX.xxX.xxX.",
           "..x...x...x...x.",
           "................",
           "................",
           "...x..x....x..x." }},
        // D: tom fill
        {{ "X.......x.......",
           "............x.X.",
           "................",
           "x.x.x.x.........",
           "................",
           "........X.x.x...",
           "X.x.x.x.........",
           "................" }},
    }};
    return patterns;
}

} // namespace me
