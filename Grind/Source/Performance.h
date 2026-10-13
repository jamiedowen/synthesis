// Keyboard / sequencer / arpeggiator front end. Turns MIDI keys and the clock
// into note events for the synth.
//
//  - KEYS: notes go straight through.
//  - SEQ:  32-step sequencer, 64 pattern slots (8 banks x 8). Keys transpose
//          the pattern, or enter notes step by step when REC is on.
//  - ARP:  arpeggiator over the held keys, 12 patterns, 1-4 octaves, HOLD.
//
// Follows the host transport when it is playing, otherwise runs from its own
// clock at the TEMPO setting.

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace grind
{
constexpr int numSteps = 32;
constexpr int numPatterns = 64;

enum PlayMode { modeKeys, modeSeq, modeArp };

struct NoteEvent
{
    enum Type : uint8_t { noteOn, noteOff, allOff, pitchBend, modWheel, aftertouch };
    int offset = 0;
    Type type = noteOn;
    int note = 60;
    float velocity = 0.8f;   // or the controller value
    bool accent = false, slide = false;
};

// One sequencer step, packed into 32 bits so the editor and the audio thread
// can share it lock-free.
struct Step
{
    int note = 48;
    bool gate = true, accent = false, slide = false, tie = false;
    int ratchet = 1; // 1..4

    uint32_t pack() const noexcept
    {
        return (uint32_t) (note & 0x7f) | (gate ? 1u << 8 : 0u) | (accent ? 1u << 9 : 0u)
             | (slide ? 1u << 10 : 0u) | (tie ? 1u << 11 : 0u) | ((uint32_t) (ratchet - 1) & 3u) << 12;
    }
    static Step unpack (uint32_t v) noexcept
    {
        Step s;
        s.note = (int) (v & 0x7f);
        s.gate = (v >> 8) & 1u;
        s.accent = (v >> 9) & 1u;
        s.slide = (v >> 10) & 1u;
        s.tie = (v >> 11) & 1u;
        s.ratchet = (int) ((v >> 12) & 3u) + 1;
        return s;
    }
};

inline constexpr std::array<const char*, 6> rateNames { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
inline constexpr std::array<double, 6> rateLengths { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 }; // quarter notes

inline constexpr std::array<const char*, 12> arpPatternNames {
    "Up", "Down", "Up/Down", "Down/Up", "Up+Down", "Played",
    "Random", "Converge", "Diverge", "Pinky Up", "Thumb Up", "Chord"
};

class Performance
{
public:
    struct Settings
    {
        int mode = modeKeys;
        int pattern = 0;
        int seqRate = 3, arpRate = 3;
        float swing = 0.0f, seqGate = 0.5f, arpGate = 0.5f;
        int arpPattern = 0, arpOctaves = 1;
        bool arpHold = false, seqRun = false, record = false;
        double tempo = 120.0;
    };

    struct Transport
    {
        bool playing = false;
        bool hasPosition = false;
        double ppq = 0.0, bpm = 120.0;
    };

    Performance();

    void prepare (double sampleRate);
    void reset();

    // Keyboard input, already filtered by MIDI channel
    void keyOn (const Settings&, int offset, int note, float velocity, std::vector<NoteEvent>& out);
    void keyOff (const Settings&, int offset, int note, std::vector<NoteEvent>& out);

    // Clock: appends sequencer / arpeggiator events for this block
    void process (const Settings&, const Transport&, int numSamples, std::vector<NoteEvent>& out);

    //== Pattern memory (shared with the editor)
    Step getStep (int pattern, int step) const noexcept;
    void setStep (int pattern, int step, const Step&) noexcept;
    int getLength (int pattern) const noexcept;
    void setLength (int pattern, int length) noexcept;
    void clearPattern (int pattern) noexcept;
    void loadDefaultPatterns() noexcept;

    int getPlayingStep() const noexcept { return playingStep.load (std::memory_order_relaxed); }
    int getRecordCursor() const noexcept { return recordCursor.load (std::memory_order_relaxed); }
    void setRecordCursor (int s) noexcept { recordCursor.store (s, std::memory_order_relaxed); }
    int getTranspose() const noexcept { return transpose.load (std::memory_order_relaxed); }

private:
    struct PendingEvent { double ppq; int note; bool on; float velocity; bool accent; };

    void syncMode (const Settings&, std::vector<NoteEvent>& out);
    void flush (int offset, std::vector<NoteEvent>& out);
    void scheduleOff (double ppq, int note);
    void scheduleOn (double ppq, int note, float velocity, bool accent);
    void emitPending (double ppq0, double ppq1, double ppqPerSample, int numSamples, std::vector<NoteEvent>& out);
    void runSequencer (const Settings&, double ppq0, double ppq1, double ppqPerSample, int numSamples,
                       std::vector<NoteEvent>& out);
    void runArpeggiator (const Settings&, double ppq0, double ppq1, double ppqPerSample, int numSamples,
                         std::vector<NoteEvent>& out);
    int buildArpSequence (const Settings&, std::array<int, 128>& notes, std::array<float, 128>& vels) const;

    std::array<std::atomic<uint32_t>, numPatterns * numSteps> steps;
    std::array<std::atomic<int>, numPatterns> lengths;

    double sr = 48000.0;
    double ppq = 0.0;            // running timeline (host or internal)
    double seqOrigin = 0.0, arpOrigin = 0.0;
    bool wasHostPlaying = false, wasSeqRunning = false;
    int previousMode = -1;

    std::vector<PendingEvent> pending;
    int lastSeqNote = -1;        // last note the sequencer played
    bool heldOver = false;       // ...and it is still held (slide into the next step)

    // Arpeggiator note set (in played order)
    std::array<int, 32> arpNotes {};
    std::array<float, 32> arpVelocities {};
    int numArpNotes = 0, numKeysDown = 0;
    int64_t arpIndex = 0;
    bool arpRunning = false;
    uint32_t randomState = 12345;

    std::atomic<int> playingStep { -1 };
    std::atomic<int> recordCursor { 0 };
    std::atomic<int> transpose { 0 };
};

} // namespace grind
