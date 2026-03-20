#pragma once
#include <cmath>

// ================= CONFIG =================
struct BounceConfig
{
    float bpm = 120.0f;
    float amplitude = 0.33f;
    int easingType = 0;

    bool enabled = false;

    int toggleKey = VK_F2;
    int menuKey = VK_F1;
};

// ================= EASING =================
inline float Ease(float x, int type)
{
    switch (type)
    {
    case 0: // linear
        return x;

    case 1: // sine
        return -(cosf(3.14159265f * x) - 1.0f) * 0.5f;

    case 2: // quad
        return x < 0.5f
            ? 2.0f * x * x
            : 1.0f - powf(-2.0f * x + 2.0f, 2.0f) * 0.5f;

    case 3: // cubic
        return x < 0.5f
            ? 4.0f * x * x * x
            : 1.0f - powf(-2.0f * x + 2.0f, 3.0f) * 0.5f;

    case 4: // quart
        return x < 0.5f
            ? 8.0f * x * x * x * x
            : 1.0f - powf(-2.0f * x + 2.0f, 4.0f) * 0.5f;

    case 5: // quint
        return x < 0.5f
            ? 16.0f * x * x * x * x * x
            : 1.0f - powf(-2.0f * x + 2.0f, 5.0f) * 0.5f;

    case 6: // expo
        if (x == 0.0f) return 0.0f;
        if (x == 1.0f) return 1.0f;
        return x < 0.5f
            ? powf(2.0f, 20.0f * x - 10.0f) * 0.5f
            : (2.0f - powf(2.0f, -20.0f * x + 10.0f)) * 0.5f;

    case 7: // circ
        return x < 0.5f
            ? (1.0f - sqrtf(1.0f - powf(2.0f * x, 2.0f))) * 0.5f
            : (sqrtf(1.0f - powf(-2.0f * x + 2.0f, 2.0f)) + 1.0f) * 0.5f;

    case 8: // back
    {
        const float c1 = 1.70158f;
        const float c2 = c1 * 1.525f;

        return x < 0.5f
            ? (powf(2.0f * x, 2.0f) * ((c2 + 1.0f) * 2.0f * x - c2)) * 0.5f
            : (powf(2.0f * x - 2.0f, 2.0f) * ((c2 + 1.0f) * (2.0f * x - 2.0f) + c2) + 2.0f) * 0.5f;
    }

    case 9: // elastic
    {
        const float c5 = (2.0f * 3.14159265f) / 4.5f;

        if (x == 0.0f) return 0.0f;
        if (x == 1.0f) return 1.0f;

        return x < 0.5f
            ? -(powf(2.0f, 20.0f * x - 10.0f) *
                sinf((20.0f * x - 11.125f) * c5)) * 0.5f
            : (powf(2.0f, -20.0f * x + 10.0f) *
                sinf((20.0f * x - 11.125f) * c5)) * 0.5f + 1.0f;
    }

    case 10: // bounce
    {
        auto easeOutBounce = [](float t)
            {
                const float n1 = 7.5625f;
                const float d1 = 2.75f;

                if (t < 1.0f / d1)
                    return n1 * t * t;
                else if (t < 2.0f / d1)
                    return n1 * (t -= 1.5f / d1) * t + 0.75f;
                else if (t < 2.5f / d1)
                    return n1 * (t -= 2.25f / d1) * t + 0.9375f;
                else
                    return n1 * (t -= 2.625f / d1) * t + 0.984375f;
            };

        return x < 0.5f
            ? (1.0f - easeOutBounce(1.0f - 2.0f * x)) * 0.5f
            : (1.0f + easeOutBounce(2.0f * x - 1.0f)) * 0.5f;
    }

    default:
        return x;
    }
}

// ================= CORE =================
inline float GetPhase(float globalTime, float bpm)
{
    float beatDuration = 60.0f / bpm;
    return fmodf(globalTime / beatDuration, 1.0f);
}

// ping-pong (biar naik turun)
inline float PingPong(float t)
{
    return (t < 0.5f)
        ? t * 2.0f
        : (1.0f - t) * 2.0f;
}

// ================= APPLY =================
template<typename T>
inline void ApplyBounce(T& m, float globalTime, const BounceConfig& cfg)
{
    float phase = GetPhase(globalTime, cfg.bpm);

    float t = PingPong(phase);
    float e = Ease(t, cfg.easingType);

    float centered = (e - 0.5f) * 2.0f; // range: -1 → +1

    float horiz = centered;
    float vert = -centered;

    // reset matrix
    m.x = { 1,0,0,0 };
    m.y = { 0,1,0,0 };
    m.z = { 0,0,1,0 };
    m.p = { 0,0,0,1 };

    // apply scale
    m.x.x *= 1.0f + horiz * cfg.amplitude;
    m.y.y *= 1.0f + horiz * cfg.amplitude;
    m.z.z *= 1.0f + vert * cfg.amplitude;
}