#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <utility>

namespace cj
{
constexpr float pi = 3.14159265358979f;
constexpr float twoPi = 2.0f * pi;
constexpr int maxCylinders = 12;

inline float tauCoef (float seconds, float sampleRate) noexcept
{
    return std::exp (-1.0f / std::max (1.0e-6f, seconds * sampleRate));
}

inline float semitonesToRatio (float st) noexcept { return std::exp2 (st / 12.0f); }

//==============================================================================
// Cylinder layouts. Angles are in crank degrees within one engine cycle
// (720 for a four-stroke, 360 for the rotary), listed in firing order.
// Each cylinder feeds one of two exhaust banks, which become left / right.
struct Layout
{
    const char* name;
    int numCylinders;
    float cycleRevs;                              // crank revolutions per cycle
    std::array<float, maxCylinders> angle;
    std::array<uint8_t, maxCylinders> bank;
    std::array<float, maxCylinders> gain;         // per-cylinder strength: uneven = lumpy
    float pulseScale;                             // exhaust port open time
    float bankLengthRatio;                        // unequal-length headers
    bool diesel;
    float idle, redline;
};

enum LayoutIndex { single, vTwin, inline3, inline4, boxer4, inline5, inline6, v6, v8Cross, v8Flat, v10, v12, rotary, dieselI4, numLayouts };

inline const std::array<Layout, numLayouts>& layouts()
{
    static const std::array<Layout, numLayouts> table {{
        { "Single", 1, 2.0f, { 0 }, { 0 }, { 1.0f }, 0.75f, 1.0f, false, 1100.0f, 7500.0f },
        { "V-Twin 45", 2, 2.0f, { 0, 315 }, { 0, 1 }, { 1.0f, 0.88f }, 0.85f, 1.05f, false, 850.0f, 5800.0f },
        { "Inline-3", 3, 2.0f, { 0, 240, 480 }, { 0, 0, 0 }, { 1.0f, 0.94f, 1.04f }, 0.9f, 1.0f, false, 850.0f, 6800.0f },
        { "Inline-4", 4, 2.0f, { 0, 180, 360, 540 }, { 0, 0, 0, 0 }, { 1.0f, 0.97f, 1.02f, 0.98f }, 1.0f, 1.0f, false, 850.0f, 7200.0f },
        { "Flat-4 Boxer", 4, 2.0f, { 0, 180, 360, 540 }, { 0, 0, 1, 1 }, { 1.0f, 0.88f, 1.06f, 0.9f }, 1.0f, 1.28f, false, 800.0f, 7000.0f },
        { "Inline-5", 5, 2.0f, { 0, 144, 288, 432, 576 }, { 0, 0, 0, 0, 0 }, { 1.0f, 0.95f, 1.03f, 0.97f, 1.02f }, 0.95f, 1.0f, false, 850.0f, 6800.0f },
        { "Inline-6", 6, 2.0f, { 0, 120, 240, 360, 480, 600 }, { 0, 1, 0, 1, 0, 1 }, { 1.0f, 0.98f, 1.01f, 0.99f, 1.0f, 0.98f }, 0.9f, 1.04f, false, 750.0f, 7000.0f },
        { "V6", 6, 2.0f, { 0, 120, 240, 360, 480, 600 }, { 0, 1, 0, 1, 0, 1 }, { 1.0f, 0.96f, 1.03f, 0.97f, 1.02f, 0.95f }, 0.95f, 1.06f, false, 750.0f, 6600.0f },
        { "V8 Cross-Plane", 8, 2.0f, { 0, 90, 180, 270, 360, 450, 540, 630 }, { 0, 1, 0, 1, 1, 0, 1, 0 },
          { 1.0f, 0.92f, 1.07f, 0.95f, 1.03f, 0.89f, 1.05f, 0.96f }, 1.0f, 1.12f, false, 700.0f, 6500.0f },
        { "V8 Flat-Plane", 8, 2.0f, { 0, 90, 180, 270, 360, 450, 540, 630 }, { 0, 1, 0, 1, 0, 1, 0, 1 },
          { 1.0f, 0.98f, 1.01f, 0.99f, 1.0f, 0.97f, 1.02f, 0.99f }, 0.75f, 1.03f, false, 1000.0f, 9000.0f },
        { "V10", 10, 2.0f, { 0, 72, 144, 216, 288, 360, 432, 504, 576, 648 }, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
          { 1.0f, 0.97f, 1.02f, 0.98f, 1.01f, 0.99f, 1.0f, 0.97f, 1.02f, 0.98f }, 0.75f, 1.04f, false, 1000.0f, 8700.0f },
        { "V12", 12, 2.0f, { 0, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660 }, { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 },
          { 1.0f, 0.99f, 1.01f, 0.98f, 1.0f, 0.99f, 1.01f, 0.99f, 1.0f, 0.98f, 1.01f, 0.99f }, 0.7f, 1.03f, false, 900.0f, 8500.0f },
        { "Rotary", 2, 1.0f, { 0, 180 }, { 0, 0 }, { 1.0f, 0.95f }, 1.7f, 1.0f, false, 950.0f, 9000.0f },
        { "Diesel I4", 4, 2.0f, { 0, 180, 360, 540 }, { 0, 0, 0, 0 }, { 1.0f, 0.95f, 1.04f, 0.97f }, 1.2f, 1.0f, true, 750.0f, 4500.0f },
    }};
    return table;
}

// Firing pulses per second <-> crank RPM for a layout
inline float rpmToFiringHz (const Layout& l, float rpm) noexcept { return rpm / 60.0f / l.cycleRevs * (float) l.numCylinders; }
inline float firingHzToRpm (const Layout& l, float hz) noexcept { return hz / (float) l.numCylinders * l.cycleRevs * 60.0f; }

//==============================================================================
struct Random
{
    uint32_t state = 0x9e3779b9u;
    float next() noexcept // [0, 1)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (state >> 8) * (1.0f / 16777216.0f);
    }
    float bipolar() noexcept { return next() * 2.0f - 1.0f; }
};

struct OnePole
{
    float a = 0.0f, z = 0.0f;
    void setCutoff (float hz, float sr) noexcept { a = std::exp (-twoPi * std::min (hz, 0.45f * sr) / sr); }
    float lowpass (float x) noexcept { z = x + a * (z - x); return z; }
    float highpass (float x) noexcept { return x - lowpass (x); }
    void reset() noexcept { z = 0.0f; }
};

// Topology-preserving state variable filter
struct Svf
{
    float a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
    void set (float hz, float q, float sr) noexcept
    {
        const float g = std::tan (pi * std::clamp (hz, 10.0f, 0.45f * sr) / sr);
        const float k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void process (float x, float& lp, float& bp) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
    }
    float lowpass (float x) noexcept { float lp, bp; process (x, lp, bp); return lp; }
    float bandpass (float x) noexcept { float lp, bp; process (x, lp, bp); return bp; }
    void reset() noexcept { ic1 = ic2 = 0.0f; }
};

//==============================================================================
// Everything about the sound that isn't "how fast" or "how hard": snapshot once per block
struct EngineSettings
{
    int layout = v8Cross;
    float pulse = 0.4f, rough = 0.4f, rasp = 0.4f, mech = 0.2f;
    float pipe = 1.5f, reso = 0.5f, muffler = 0.3f, crackle = 0.3f, width = 0.7f;
    float intake = 0.3f, turbo = 0.0f, blower = 0.0f;
};

//==============================================================================
// Crank speed model for ENGINE mode: throttle in, RPM out, with flywheel
// inertia, a rev limiter that cuts the ignition and an optional gearbox.
class Crank
{
public:
    void prepare (float sampleRate) noexcept { sr = sampleRate; }

    void reset (float idleRpm) noexcept
    {
        rpm = idleRpm;
        gear = 0;
        cutSamples = 0;
    }

    void setRpm (float newRpm) noexcept { rpm = newRpm; }

    void setParams (float idleRpm, float redlineRpm, float flywheel, bool useGearbox) noexcept
    {
        idle = idleRpm;
        redline = std::max (redlineRpm, idleRpm * 2.0f);
        gearbox = useGearbox;
        if (! gearbox)
            gear = 0;
        baseUp = 0.15f + 1.1f * flywheel;
        baseDown = 0.4f + 1.9f * flywheel;
        updateCoefs();
    }

    // One sample. Returns true when the ignition is firing.
    bool step (float throttle, bool engineOn) noexcept
    {
        if (! engineOn)
        {
            rpm += (0.0f - rpm) * downK;
            gear = 0;
            return false;
        }

        const bool ignition = cutSamples <= 0;
        if (! ignition)
            --cutSamples;

        const float target = idle + throttle * (redline * 1.12f - idle);
        if (ignition && target > rpm)
            rpm += (target - rpm) * upK * (0.25f + 0.75f * throttle);
        else
            rpm += (std::max (idle, ignition ? target : idle) - rpm) * downK;

        if (rpm >= redline && cutSamples <= 0)
        {
            if (gearbox && gear < numGears - 1 && throttle > 0.5f)
            {
                ++gear;
                rpm *= ratios[(size_t) gear] / ratios[(size_t) gear - 1];
                cutSamples = (int) (0.07f * sr); // the "brap" of a flat-foot shift
            }
            else
            {
                cutSamples = (int) (0.035f * sr);
            }
            updateCoefs();
        }
        else if (gearbox && gear > 0 && throttle < 0.2f && rpm < 0.42f * redline)
        {
            // Downshift: the revs jump up, like a rev-matched blip
            --gear;
            rpm *= ratios[(size_t) gear] / ratios[(size_t) gear + 1];
            updateCoefs();
        }

        return ignition;
    }

    float getRpm() const noexcept { return rpm; }
    int getGear() const noexcept { return gearbox ? gear : -1; }
    float getRedline() const noexcept { return redline; }

private:
    void updateCoefs() noexcept
    {
        const float gf = gearbox ? gearFactor[(size_t) gear] : 1.0f;
        upK = 1.0f - tauCoef (baseUp * gf, sr);
        downK = 1.0f - tauCoef (baseDown * (gearbox && gear > 0 ? gf * 1.6f : 1.0f), sr);
    }

    static constexpr int numGears = 6;
    static constexpr std::array<float, numGears> ratios { 3.6f, 2.2f, 1.52f, 1.16f, 0.94f, 0.78f };
    static constexpr std::array<float, numGears> gearFactor { 1.0f, 1.6f, 2.3f, 3.1f, 4.0f, 5.0f };

    float sr = 44100.0f, rpm = 800.0f, idle = 800.0f, redline = 6500.0f;
    float baseUp = 0.5f, baseDown = 1.0f, upK = 0.0f, downK = 0.0f;
    int gear = 0, cutSamples = 0;
    bool gearbox = false;
};

//==============================================================================
// One engine. Each cylinder firing drops a pressure pulse into its bank's
// exhaust: a waveguide pipe, then a muffler. Intake roar, valvetrain tick,
// turbo, supercharger and overrun crackle sit on top.
class EngineVoice
{
public:
    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        clickA = tauCoef (0.00018f, sr);
        popDecay = tauCoef (0.012f, sr);
        bovDecay = tauCoef (0.32f, sr);
        boostUp = 1.0f - tauCoef (0.7f, sr);
        boostDown = 1.0f - tauCoef (0.22f, sr);
        throttleK = 1.0f - tauCoef (0.03f, sr);
        slowThrottleK = 1.0f - tauCoef (0.25f, sr);
        wobbleK = 1.0f - tauCoef (0.04f, sr);
        envK = 1.0f - tauCoef (0.006f, sr);
        for (auto& b : banks)
        {
            b.dc.setCutoff (22.0f, sr);
            b.loopLp.setCutoff (3200.0f, sr);
        }
        mechHp.setCutoff (2500.0f, sr);
        reset();
    }

    void reset() noexcept
    {
        phase = 0.9999f;
        for (auto& b : banks)
        {
            b.y1 = b.y2 = b.click = b.inject = b.clickInject = b.popEnv = 0.0f;
            b.a = 0.9f;
            b.dc.reset();
            b.loopLp.reset();
            b.muffler.reset();
            b.delay.fill (0.0f);
            b.writePos = 0;
        }
        intakeBp.reset();
        turboBp.reset();
        bovBp.reset();
        mechHp.reset();
        mechBp.reset();
        throttleSm = slowThrottle = boost = bovEnv = mechEnv = pulseEnv = wobble = wobbleTarget = 0.0f;
        turboPhase = blowerPhase = bovPhase = 0.0f;
        bovArmed = false;
        ctrlCounter = 0;
        firedMask = 0;
    }

    // rpmNorm is the RPM as a fraction of the redline (drives turbo spool and overrun)
    void process (const EngineSettings& s, float firingHz, float throttle, float rpmNorm, bool ignition,
                  float& outL, float& outR) noexcept
    {
        const auto& lay = layouts()[(size_t) std::clamp (s.layout, 0, numLayouts - 1)];
        firingHz = std::clamp (firingHz, 0.0f, 0.2f * sr);

        throttleSm += (throttle - throttleSm) * throttleK;
        slowThrottle += (throttle - slowThrottle) * slowThrottleK;

        if (--ctrlCounter <= 0)
            updateControl (s, lay, firingHz);

        //== Crank: walk through the cycle and fire each cylinder as its angle passes
        wobble += (wobbleTarget - wobble) * wobbleK;
        const float cycleHz = firingHz / (float) lay.numCylinders;
        const float prev = phase;
        phase += cycleHz / sr * (1.0f + s.rough * 0.06f * wobble);
        const bool wrapped = phase >= 1.0f;
        if (wrapped)
            phase -= std::floor (phase);

        const float cycleDegrees = 360.0f * lay.cycleRevs;
        for (int i = 0; i < lay.numCylinders; ++i)
        {
            const float a = lay.angle[(size_t) i] / cycleDegrees;
            if (wrapped ? (a > prev || a <= phase) : (a > prev && a <= phase))
                fire (s, lay, i, ignition, rpmNorm);
        }

        //== Exhaust banks
        float bankOut[2] {};
        const int usedBanks = bankCount (lay);
        for (int b = 0; b < usedBanks; ++b)
        {
            auto& bk = banks[(size_t) b];
            bk.y1 = bk.a * bk.y1 + bk.inject;
            bk.y2 = bk.a * bk.y2 + bk.y1;
            bk.click = clickA * bk.click + bk.clickInject;
            bk.inject = bk.clickInject = 0.0f;

            float exc = bk.y2 + bk.click;
            if (bk.popEnv > 1.0e-4f)
            {
                exc += noise.bipolar() * bk.popEnv;
                bk.popEnv *= popDecay;
            }
            exc = bk.dc.highpass (exc);

            // Waveguide pipe: the open end reflects inverted, losing top end each trip
            const float delayed = readDelay (bk, bk.delaySamples);
            const float v = exc * pipeInGain + pipeFeedback * bk.loopLp.lowpass (delayed);
            bk.delay[(size_t) bk.writePos] = v;
            bk.writePos = (bk.writePos + 1) & (delaySize - 1);

            float y = bk.muffler.lowpass (v) * mufflerGain;
            if (raspDrive > 1.0f)
                y = std::tanh (y * raspDrive) * raspNorm;
            bankOut[b] = y;
        }

        float l, r;
        if (usedBanks == 1)
        {
            l = r = bankOut[0];
        }
        else
        {
            const float w = 0.5f + 0.5f * s.width;
            l = bankOut[0] * w + bankOut[1] * (1.0f - w);
            r = bankOut[1] * w + bankOut[0] * (1.0f - w);
        }

        //== Mono extras: intake, valvetrain, turbo, supercharger
        pulseEnv += (std::abs (banks[0].y2) - pulseEnv) * envK;
        float extra = 0.0f;
        if (s.intake > 0.0f)
            extra += intakeBp.bandpass (noise.bipolar()) * s.intake * (0.1f + throttleSm) * (0.25f + std::min (1.5f, pulseEnv)) * 0.6f;

        if (s.mech > 0.0f && mechEnv > 1.0e-4f)
        {
            const float n = noise.bipolar() * mechEnv;
            extra += (lay.diesel ? mechBp.bandpass (n) * 2.2f : mechHp.highpass (n)) * s.mech * 0.35f;
            mechEnv *= mechDecay;
        }

        if (s.turbo > 0.0f)
        {
            const float target = throttleSm * std::clamp (rpmNorm * 1.5f - 0.25f, 0.0f, 1.0f);
            boost += (target - boost) * (target > boost ? boostUp : boostDown);

            turboPhase += turboHz / sr;
            turboPhase -= std::floor (turboPhase);
            const float whine = std::sin (twoPi * turboPhase) * 0.6f + std::sin (2.0f * twoPi * turboPhase) * 0.15f;
            extra += whine * s.turbo * boost * boost * 0.05f;
            extra += turboBp.bandpass (noise.bipolar()) * s.turbo * boost * (0.3f + throttleSm) * 0.12f;

            // Blow-off valve: lift off the throttle with boost up and it vents
            if (throttle > 0.5f && boost > 0.25f)
                bovArmed = true;
            if (bovArmed && throttle < slowThrottle - 0.3f)
            {
                bovEnv = std::max (bovEnv, boost);
                bovArmed = false;
            }
            if (bovEnv > 1.0e-4f)
            {
                bovPhase += 21.0f / sr;
                bovPhase -= std::floor (bovPhase);
                const float flutter = 0.65f + 0.35f * std::sin (twoPi * bovPhase);
                extra += bovBp.bandpass (noise.bipolar()) * bovEnv * flutter * s.turbo * 0.5f;
                bovEnv *= bovDecay;
            }
        }
        else
        {
            boost = 0.0f;
        }

        if (s.blower > 0.0f)
        {
            blowerPhase += blowerHz / sr;
            blowerPhase -= std::floor (blowerPhase);
            const float spin = std::min (1.0f, blowerHz / 900.0f);
            extra += (std::sin (twoPi * blowerPhase) + 0.3f * std::sin (3.0f * twoPi * blowerPhase))
                     * s.blower * (0.25f + 0.75f * throttleSm) * spin * 0.045f;
        }

        outL = l + extra;
        outR = r + extra;
    }

    uint32_t takeFiredMask() noexcept { return std::exchange (firedMask, 0u); }
    float getBoost() const noexcept { return boost; }

private:
    static constexpr int delaySize = 8192;

    struct Bank
    {
        float y1 = 0, y2 = 0, a = 0.9f, inject = 0, click = 0, clickInject = 0, popEnv = 0;
        float delaySamples = 100.0f;
        OnePole dc, loopLp;
        Svf muffler;
        std::array<float, delaySize> delay {};
        int writePos = 0;
    };

    static int bankCount (const Layout& lay) noexcept
    {
        for (int i = 0; i < lay.numCylinders; ++i)
            if (lay.bank[(size_t) i] != 0)
                return 2;
        return 1;
    }

    static float readDelay (const Bank& b, float d) noexcept
    {
        const float pos = (float) b.writePos - d;
        const float wrapped = pos < 0.0f ? pos + (float) delaySize : pos;
        const int i0 = (int) wrapped;
        const float frac = wrapped - (float) i0;
        const float x0 = b.delay[(size_t) (i0 & (delaySize - 1))];
        const float x1 = b.delay[(size_t) ((i0 + 1) & (delaySize - 1))];
        return x0 + frac * (x1 - x0);
    }

    void updateControl (const EngineSettings& s, const Layout& lay, float firingHz) noexcept
    {
        ctrlCounter = 16;

        // ROUGH scales how far each cylinder and bank strays from perfect. At zero the
        // engine is perfectly even, so a note is exactly in tune.
        unevenness = std::min (1.5f, 2.0f * s.rough);

        // Pulse length follows the port-open time, so it shrinks as the revs climb
        const float period = 1.0f / std::max (firingHz, 1.0f);
        pulseSeconds = std::clamp ((0.06f + 0.55f * s.pulse) * lay.pulseScale * period, 0.00012f, 0.012f);

        // Hot exhaust gas: speed of sound about 480 m/s; there and back again
        const float baseDelay = 2.0f * s.pipe / 480.0f * sr;
        banks[0].delaySamples = std::clamp (baseDelay, 2.0f, (float) delaySize - 4.0f);
        banks[1].delaySamples = std::clamp (baseDelay * (1.0f + (lay.bankLengthRatio - 1.0f) * unevenness), 2.0f, (float) delaySize - 4.0f);
        pipeFeedback = -0.93f * s.reso;
        pipeInGain = 0.55f * (1.0f - 0.55f * s.reso);

        const float cutoff = (9000.0f - 8200.0f * std::sqrt (s.muffler)) * (0.45f + 0.75f * throttleSm);
        for (auto& b : banks)
            b.muffler.set (cutoff, 0.6f + 0.5f * s.reso, sr);
        mufflerGain = 1.0f - 0.45f * s.muffler;

        raspDrive = 1.0f + 6.0f * s.rasp * s.rasp;
        raspNorm = 1.0f / std::tanh (raspDrive) / (1.0f + 0.9f * s.rasp);

        intakeBp.set (280.0f + 900.0f * throttleSm + 0.6f * firingHz, 1.4f, sr);
        turboHz = 2200.0f + 7500.0f * boost;
        turboBp.set (1800.0f + 4000.0f * boost, 2.0f, sr);
        bovBp.set (2600.0f, 0.8f, sr);
        mechBp.set (2400.0f, 3.0f, sr);
        mechDecay = tauCoef (lay.diesel ? 0.006f : 0.0015f, sr);
        blowerHz = std::min (0.4f * sr, firingHz / (float) lay.numCylinders * lay.cycleRevs * 30.0f);
    }

    void fire (const EngineSettings& s, const Layout& lay, int cyl, bool ignition, float rpmNorm) noexcept
    {
        firedMask |= 1u << cyl;
        wobbleTarget = noise.bipolar();

        auto& bk = banks[(size_t) lay.bank[(size_t) cyl]];
        const bool overrun = throttleSm < 0.1f && rpmNorm > 0.3f;

        float amp;
        if (! ignition || overrun)
        {
            // No combustion (rev limiter or closed throttle at speed): weak pumping
            // pulses, and sometimes unburnt fuel goes bang in the pipe
            amp = ignition ? 0.25f : 0.08f;
            if (rpmNorm > 0.3f && noise.next() < s.crackle * (ignition ? 0.16f : 0.4f))
            {
                const float bang = 0.6f + 0.8f * noise.next();
                bk.popEnv = std::max (bk.popEnv, bang * 0.5f);
                bk.a = tauCoef (0.0012f, sr);
                bk.inject += bang * 2.4f * eulerE / (0.0012f * sr);
                firedMask |= 1u << 31; // tell the UI there was a pop
                return;
            }
        }
        else
        {
            amp = 0.3f + 0.7f * throttleSm;
        }

        amp *= (1.0f + (lay.gain[(size_t) cyl] - 1.0f) * unevenness) * (1.0f + s.rough * 0.4f * noise.bipolar());
        const float t = pulseSeconds * (1.0f + s.rough * 0.25f * noise.bipolar());
        bk.a = tauCoef (t, sr);
        bk.inject += amp * eulerE / (t * sr);
        bk.clickInject += amp * s.rasp * 0.9f;

        mechEnv = std::max (mechEnv, (lay.diesel ? 1.0f : 0.45f) * (0.5f + 0.5f * noise.next()));
    }

    static constexpr float eulerE = 2.718281828f;

    std::array<Bank, 2> banks;
    Random noise;
    Svf intakeBp, turboBp, bovBp, mechBp;
    OnePole mechHp;

    float sr = 44100.0f;
    float phase = 0.9999f;
    float clickA = 0, popDecay = 0, bovDecay = 0, boostUp = 0, boostDown = 0;
    float throttleK = 0, slowThrottleK = 0, wobbleK = 0, envK = 0;
    float throttleSm = 0, slowThrottle = 0, boost = 0, bovEnv = 0, mechEnv = 0, mechDecay = 0, pulseEnv = 0;
    float wobble = 0, wobbleTarget = 0, unevenness = 1;
    float pulseSeconds = 0.002f, pipeFeedback = 0, pipeInGain = 0.5f, mufflerGain = 1, raspDrive = 1, raspNorm = 1;
    float turboHz = 2000, blowerHz = 1000, turboPhase = 0, blowerPhase = 0, bovPhase = 0;
    bool bovArmed = false;
    int ctrlCounter = 0;
    uint32_t firedMask = 0;
};

} // namespace cj
