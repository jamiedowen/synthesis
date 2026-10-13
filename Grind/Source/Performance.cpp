#include "Performance.h"

#include <algorithm>
#include <cmath>

namespace grind
{
namespace
{
constexpr float normalVelocity = 0.75f;
constexpr double slideOverlap = 1.0e-3; // quarter notes the old note overlaps a slide
} // namespace

Performance::Performance()
{
    for (auto& s : steps)
        s.store (Step().pack());
    for (auto& l : lengths)
        l.store (16);
    pending.reserve (256);
    loadDefaultPatterns();
}

void Performance::prepare (double sampleRate)
{
    sr = sampleRate;
    reset();
}

void Performance::reset()
{
    pending.clear();
    ppq = 0.0;
    wasHostPlaying = wasSeqRunning = arpRunning = false;
    lastSeqNote = -1;
    heldOver = false;
    numArpNotes = numKeysDown = 0;
    arpIndex = 0;
    previousMode = -1;
    playingStep.store (-1);
}

//==============================================================================
Step Performance::getStep (int pattern, int step) const noexcept
{
    pattern = std::clamp (pattern, 0, numPatterns - 1);
    step = std::clamp (step, 0, numSteps - 1);
    return Step::unpack (steps[(size_t) (pattern * numSteps + step)].load (std::memory_order_relaxed));
}

void Performance::setStep (int pattern, int step, const Step& s) noexcept
{
    if (pattern < 0 || pattern >= numPatterns || step < 0 || step >= numSteps)
        return;
    steps[(size_t) (pattern * numSteps + step)].store (s.pack(), std::memory_order_relaxed);
}

int Performance::getLength (int pattern) const noexcept
{
    return lengths[(size_t) std::clamp (pattern, 0, numPatterns - 1)].load (std::memory_order_relaxed);
}

void Performance::setLength (int pattern, int length) noexcept
{
    if (pattern >= 0 && pattern < numPatterns)
        lengths[(size_t) pattern].store (std::clamp (length, 1, numSteps), std::memory_order_relaxed);
}

void Performance::clearPattern (int pattern) noexcept
{
    Step rest;
    rest.gate = false;
    for (int s = 0; s < numSteps; ++s)
        setStep (pattern, s, rest);
}

void Performance::loadDefaultPatterns() noexcept
{
    // note, flags: '.' plain, 'A' accent, 'S' slide, 'B' both, '-' rest, 'T' tie, 'R' ratchet x2
    struct Text { int length; int notes[16]; const char* flags; };
    static const Text patterns[] = {
        // Acid line
        { 16, { 36, 36, 48, 36, 39, 36, 46, 36, 36, 48, 36, 41, 36, 43, 36, 46 }, "A..S.A.-.S..A.S." },
        // Driving bass
        { 16, { 36, 36, 36, 36, 36, 36, 39, 39, 41, 41, 41, 41, 34, 34, 43, 43 }, "A...A...A...A..." },
        // Arp-like lead
        { 16, { 60, 63, 67, 70, 72, 70, 67, 63, 60, 63, 67, 72, 75, 72, 67, 63 }, "A...S...A...SS.." },
        // Percussion / drum-engine groove (play with a percussion model)
        { 16, { 48, 48, 60, 48, 48, 55, 60, 48, 48, 48, 60, 48, 55, 48, 60, 67 }, "A-.R.-A.A-.-.R.A" },
    };

    for (int p = 0; p < numPatterns; ++p)
    {
        const auto& t = patterns[p % 4];
        lengths[(size_t) p].store (t.length);
        for (int s = 0; s < numSteps; ++s)
        {
            Step st;
            const int i = s % 16;
            st.note = t.notes[i];
            const char f = t.flags[i];
            st.gate = f != '-';
            st.accent = f == 'A' || f == 'B';
            st.slide = f == 'S' || f == 'B';
            st.tie = f == 'T';
            st.ratchet = f == 'R' ? 2 : 1;
            setStep (p, s, st);
        }
    }
}

//==============================================================================
void Performance::flush (int offset, std::vector<NoteEvent>& out)
{
    NoteEvent e;
    e.offset = offset;
    e.type = NoteEvent::allOff;
    out.push_back (e);
    pending.clear();
    lastSeqNote = -1;
    heldOver = false;
}

void Performance::scheduleOff (double when, int note)
{
    if (pending.size() < pending.capacity())
        pending.push_back ({ when, note, false, 0.0f, false });
}

void Performance::scheduleOn (double when, int note, float velocity, bool accent)
{
    if (pending.size() < pending.capacity())
        pending.push_back ({ when, note, true, velocity, accent });
}

void Performance::emitPending (double ppq0, double ppq1, double ppqPerSample, int numSamples, std::vector<NoteEvent>& out)
{
    for (size_t i = 0; i < pending.size();)
    {
        if (pending[i].ppq < ppq1)
        {
            NoteEvent e;
            e.type = pending[i].on ? NoteEvent::noteOn : NoteEvent::noteOff;
            e.note = pending[i].note;
            e.velocity = pending[i].velocity;
            e.accent = pending[i].accent;
            e.offset = std::clamp ((int) ((pending[i].ppq - ppq0) / ppqPerSample), 0, numSamples - 1);
            out.push_back (e);
            pending[i] = pending.back();
            pending.pop_back();
        }
        else
        {
            ++i;
        }
    }
}

//==============================================================================
void Performance::syncMode (const Settings& s, std::vector<NoteEvent>& out)
{
    if (s.mode == previousMode)
        return;
    if (previousMode >= 0)
        flush (0, out);
    previousMode = s.mode;
    wasSeqRunning = false;
    arpRunning = false;
    numArpNotes = numKeysDown = 0;
    transpose.store (0);
    playingStep.store (-1);
}

void Performance::keyOn (const Settings& s, int offset, int note, float velocity, std::vector<NoteEvent>& out)
{
    syncMode (s, out);

    auto passThrough = [&]
    {
        NoteEvent e;
        e.offset = offset;
        e.note = note;
        e.velocity = velocity;
        out.push_back (e);
    };

    if (s.mode == modeKeys)
    {
        passThrough();
        return;
    }

    if (s.mode == modeSeq)
    {
        if (s.record)
        {
            // Step entry: write the key at the cursor and move on
            const int len = getLength (s.pattern);
            const int cursor = std::clamp (recordCursor.load(), 0, len - 1);
            Step st;
            st.note = note;
            st.accent = velocity > 0.9f;
            setStep (s.pattern, cursor, st);
            recordCursor.store ((cursor + 1) % len);
            if (! wasSeqRunning)
                passThrough();
        }
        else if (wasSeqRunning)
        {
            transpose.store (note - 60);
        }
        else
        {
            passThrough();
        }
        return;
    }

    // Arpeggiator
    if (s.arpHold && numKeysDown == 0)
        numArpNotes = 0;
    ++numKeysDown;

    for (int i = 0; i < numArpNotes; ++i)
        if (arpNotes[(size_t) i] == note)
            return;
    if (numArpNotes < (int) arpNotes.size())
    {
        arpNotes[(size_t) numArpNotes] = note;
        arpVelocities[(size_t) numArpNotes] = velocity;
        ++numArpNotes;
    }
}

void Performance::keyOff (const Settings& s, int offset, int note, std::vector<NoteEvent>& out)
{
    syncMode (s, out);
    if (s.mode == modeArp)
    {
        numKeysDown = std::max (0, numKeysDown - 1);
        if (! s.arpHold)
        {
            for (int i = 0; i < numArpNotes; ++i)
                if (arpNotes[(size_t) i] == note)
                {
                    for (int j = i; j < numArpNotes - 1; ++j)
                    {
                        arpNotes[(size_t) j] = arpNotes[(size_t) j + 1];
                        arpVelocities[(size_t) j] = arpVelocities[(size_t) j + 1];
                    }
                    --numArpNotes;
                    break;
                }
        }
        return;
    }

    if (s.mode == modeSeq && wasSeqRunning)
        return;

    NoteEvent e;
    e.offset = offset;
    e.type = NoteEvent::noteOff;
    e.note = note;
    out.push_back (e);
}

//==============================================================================
void Performance::process (const Settings& s, const Transport& t, int numSamples, std::vector<NoteEvent>& out)
{
    const bool hostPlaying = t.playing && t.hasPosition;
    const double bpm = hostPlaying ? std::max (t.bpm, 1.0) : std::clamp (s.tempo, 20.0, 300.0);
    const double ppqPerSample = bpm / (60.0 * sr);

    syncMode (s, out);

    double ppq0 = ppq;
    if (hostPlaying)
    {
        ppq0 = t.ppq;
        // Transport started, looped or jumped: drop anything still hanging
        if (! wasHostPlaying || std::abs (ppq0 - ppq) > 0.05)
        {
            if (! pending.empty() || lastSeqNote >= 0)
                flush (0, out);
            wasSeqRunning = false;
            arpRunning = false;
        }
    }
    else if (wasHostPlaying)
    {
        flush (0, out);
        wasSeqRunning = false;
        arpRunning = false;
    }
    wasHostPlaying = hostPlaying;

    const double ppq1 = ppq0 + numSamples * ppqPerSample;

    if (s.mode == modeSeq)
        runSequencer (s, ppq0, ppq1, ppqPerSample, numSamples, out);
    else if (s.mode == modeArp)
        runArpeggiator (s, ppq0, ppq1, ppqPerSample, numSamples, out);

    emitPending (ppq0, ppq1, ppqPerSample, numSamples, out);
    ppq = ppq1;

    // Order by time; at the same sample, note-offs go before note-ons
    auto earlier = [] (const NoteEvent& a, const NoteEvent& b)
    {
        if (a.offset != b.offset) return a.offset < b.offset;
        auto rank = [] (const NoteEvent& e) { return e.type == NoteEvent::noteOn ? 1 : 0; };
        return rank (a) < rank (b);
    };
    for (size_t i = 1; i < out.size(); ++i)
        for (size_t j = i; j > 0 && earlier (out[j], out[j - 1]); --j)
            std::swap (out[j], out[j - 1]);
}

void Performance::runSequencer (const Settings& s, double ppq0, double ppq1, double ppqPerSample, int numSamples,
                                std::vector<NoteEvent>& out)
{
    const bool running = wasHostPlaying || s.seqRun;
    if (! running)
    {
        if (wasSeqRunning)
            flush (0, out);
        wasSeqRunning = false;
        playingStep.store (-1);
        return;
    }

    if (! wasSeqRunning)
    {
        seqOrigin = wasHostPlaying ? 0.0 : ppq0;
        wasSeqRunning = true;
    }

    const double stepLen = rateLengths[(size_t) std::clamp (s.seqRate, 0, (int) rateLengths.size() - 1)];
    const double swingOffset = std::clamp (s.swing, 0.0f, 1.0f) * 0.5 * stepLen;
    const double gateLen = std::clamp ((double) s.seqGate, 0.05, 1.0) * stepLen;
    const int pattern = std::clamp (s.pattern, 0, numPatterns - 1);
    const int length = getLength (pattern);
    const int trans = transpose.load();

    auto toOffset = [&] (double when) { return std::clamp ((int) ((when - ppq0) / ppqPerSample), 0, numSamples - 1); };
    auto noteOn = [&] (double when, int note, float vel, bool accent, bool slide)
    {
        NoteEvent e;
        e.offset = toOffset (when);
        e.note = note;
        e.velocity = vel;
        e.accent = accent;
        e.slide = slide;
        out.push_back (e);
    };
    auto cancelOffs = [&] (int note)
    {
        for (size_t i = 0; i < pending.size();)
            if (pending[i].note == note && ! pending[i].on) { pending[i] = pending.back(); pending.pop_back(); }
            else ++i;
    };

    const auto first = (int64_t) std::floor ((ppq0 - seqOrigin) / stepLen) - 1;
    for (auto k = std::max<int64_t> (first, 0);; ++k)
    {
        const double straight = seqOrigin + (double) k * stepLen;
        if (straight >= ppq1)
            break;
        const double when = straight + ((k & 1) != 0 ? swingOffset : 0.0);
        if (when < ppq0 || when >= ppq1)
            continue;

        const int index = (int) (k % length);
        playingStep.store (index, std::memory_order_relaxed);
        const Step st = getStep (pattern, index);
        const int note = std::clamp (st.note + trans, 0, 127);

        // Rest: let go of a note held over from a slide
        if (! st.gate)
        {
            if (heldOver && lastSeqNote >= 0)
                scheduleOff (when, lastSeqNote);
            heldOver = false;
            lastSeqNote = -1;
            continue;
        }

        // Tie: the previous note keeps sounding through this step
        if (st.tie && lastSeqNote >= 0)
        {
            cancelOffs (lastSeqNote);
            if (st.slide)
                heldOver = true;
            else
            {
                scheduleOff (when + gateLen, lastSeqNote);
                heldOver = false;
            }
            continue;
        }

        const float vel = st.accent ? 1.0f : normalVelocity;
        const bool slideIn = heldOver && lastSeqNote >= 0;

        if (slideIn)
        {
            // Legato into the new note (no retrigger, glide), then release the old one
            const int old = lastSeqNote;
            noteOn (when, note, vel, st.accent, true);
            if (old != note)
                scheduleOff (when + slideOverlap, old);
            if (! st.slide)
                scheduleOff (when + gateLen, note);
        }
        else
        {
            const int r = std::clamp (st.ratchet, 1, 4);
            const double sub = stepLen / r;
            for (int j = 0; j < r; ++j)
            {
                const double tj = when + j * sub;
                if (j == 0)
                    noteOn (tj, note, vel, st.accent, false);
                else
                    scheduleOn (tj, note, vel, st.accent);
                if (j < r - 1 || ! st.slide)
                    scheduleOff (tj + std::min (gateLen / r, sub * 0.9), note);
            }
        }

        lastSeqNote = note;
        heldOver = st.slide;
    }
}

int Performance::buildArpSequence (const Settings& s, std::array<int, 128>& notes, std::array<float, 128>& vels) const
{
    // Sorted copy of the held notes
    std::array<int, 32> sorted {};
    std::array<float, 32> sortedVel {};
    for (int i = 0; i < numArpNotes; ++i)
    {
        sorted[(size_t) i] = arpNotes[(size_t) i];
        sortedVel[(size_t) i] = arpVelocities[(size_t) i];
    }
    for (int i = 1; i < numArpNotes; ++i)
        for (int j = i; j > 0 && sorted[(size_t) j] < sorted[(size_t) j - 1]; --j)
        {
            std::swap (sorted[(size_t) j], sorted[(size_t) j - 1]);
            std::swap (sortedVel[(size_t) j], sortedVel[(size_t) j - 1]);
        }

    const int octaves = std::clamp (s.arpOctaves, 1, 4);
    const bool played = s.arpPattern == 5;

    // Base run across the octave range
    int n = 0;
    std::array<int, 128> base {};
    std::array<float, 128> baseVel {};
    for (int o = 0; o < octaves; ++o)
        for (int i = 0; i < numArpNotes && n < 128; ++i, ++n)
        {
            base[(size_t) n] = (played ? arpNotes[(size_t) i] : sorted[(size_t) i]) + 12 * o;
            baseVel[(size_t) n] = played ? arpVelocities[(size_t) i] : sortedVel[(size_t) i];
        }

    int count = 0;
    auto push = [&] (int idx)
    {
        if (count < 128)
        {
            notes[(size_t) count] = base[(size_t) idx];
            vels[(size_t) count] = baseVel[(size_t) idx];
            ++count;
        }
    };

    switch (s.arpPattern)
    {
        case 0: case 5: case 6: case 11:                     // up, played, random, chord
            for (int i = 0; i < n; ++i) push (i);
            break;
        case 1:                                               // down
            for (int i = n - 1; i >= 0; --i) push (i);
            break;
        case 2:                                               // up/down
            for (int i = 0; i < n; ++i) push (i);
            for (int i = n - 2; i > 0; --i) push (i);
            break;
        case 3:                                               // down/up
            for (int i = n - 1; i >= 0; --i) push (i);
            for (int i = 1; i < n - 1; ++i) push (i);
            break;
        case 4:                                               // up + down, ends repeated
            for (int i = 0; i < n; ++i) push (i);
            for (int i = n - 1; i >= 0; --i) push (i);
            break;
        case 7:                                               // converge
            for (int lo = 0, hi = n - 1; lo <= hi; ++lo, --hi) { push (lo); if (hi != lo) push (hi); }
            break;
        case 8:                                               // diverge
        {
            int lo = (n - 1) / 2, hi = lo + 1;
            while (lo >= 0 || hi < n) { if (lo >= 0) push (lo--); if (hi < n) push (hi++); }
            break;
        }
        case 9:                                               // pinky up
            for (int i = 0; i < n - 1; ++i) { push (i); push (n - 1); }
            if (n == 1) push (0);
            break;
        case 10:                                              // thumb up
            for (int i = 1; i < n; ++i) { push (0); push (i); }
            if (n == 1) push (0);
            break;
        default:
            for (int i = 0; i < n; ++i) push (i);
            break;
    }
    return count;
}

void Performance::runArpeggiator (const Settings& s, double ppq0, double ppq1, double ppqPerSample, int numSamples,
                                  std::vector<NoteEvent>& out)
{
    if (! s.arpHold && numKeysDown == 0)
        numArpNotes = 0;

    if (numArpNotes == 0)
    {
        arpRunning = false;
        playingStep.store (-1);
        return;
    }

    if (! arpRunning)
    {
        arpRunning = true;
        arpIndex = 0;
        arpOrigin = wasHostPlaying ? 0.0 : ppq0;
    }

    const double stepLen = rateLengths[(size_t) std::clamp (s.arpRate, 0, (int) rateLengths.size() - 1)];
    const double swingOffset = std::clamp (s.swing, 0.0f, 1.0f) * 0.5 * stepLen;
    const double gateLen = std::clamp ((double) s.arpGate, 0.05, 1.0) * stepLen;

    std::array<int, 128> notes {};
    std::array<float, 128> vels {};
    const int count = buildArpSequence (s, notes, vels);
    if (count == 0)
        return;

    const auto first = (int64_t) std::floor ((ppq0 - arpOrigin) / stepLen) - 1;
    for (auto k = std::max<int64_t> (first, 0);; ++k)
    {
        const double straight = arpOrigin + (double) k * stepLen;
        if (straight >= ppq1)
            break;
        const double when = straight + ((k & 1) != 0 ? swingOffset : 0.0);
        if (when < ppq0 || when >= ppq1)
            continue;

        const int offset = std::clamp ((int) ((when - ppq0) / ppqPerSample), 0, numSamples - 1);
        auto play = [&] (int note, float vel)
        {
            note = std::clamp (note, 0, 127);
            NoteEvent e;
            e.offset = offset;
            e.note = note;
            e.velocity = vel;
            out.push_back (e);
            scheduleOff (when + gateLen, note);
        };

        if (s.arpPattern == 11)
        {
            for (int i = 0; i < count; ++i)
                play (notes[(size_t) i], vels[(size_t) i]);
        }
        else if (s.arpPattern == 6)
        {
            randomState = randomState * 1664525u + 1013904223u;
            const int i = (int) ((randomState >> 8) % (uint32_t) count);
            play (notes[(size_t) i], vels[(size_t) i]);
        }
        else
        {
            const int i = (int) (arpIndex % count);
            play (notes[(size_t) i], vels[(size_t) i]);
        }
        playingStep.store ((int) (arpIndex % count), std::memory_order_relaxed);
        ++arpIndex;
    }
}

} // namespace grind
