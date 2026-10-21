// Disco Sauce 2 Pro - scale tables + note quantizer (pure C++, no JUCE)
#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace ds {

constexpr double kPi = 3.14159265358979323846;
constexpr int kNumScales   = 26;
constexpr int kCustomScale = 25;   // "Custom (Note Map)"

struct ScaleDef { const char* name; int n; double deg[12]; };

// degrees are CENTS above the root, so quarter-tone (maqam) scales are exact
inline const ScaleDef* scaleTable()
{
    static const ScaleDef t[kNumScales] = {
        {"Chromatic",              12, {0,100,200,300,400,500,600,700,800,900,1000,1100}},
        {"Major",                   7, {0,200,400,500,700,900,1100}},
        {"Minor (Natural)",         7, {0,200,300,500,700,800,1000}},
        {"Harmonic Minor",          7, {0,200,300,500,700,800,1100}},
        {"Melodic Minor",           7, {0,200,300,500,700,900,1100}},
        {"Dorian",                  7, {0,200,300,500,700,900,1000}},
        {"Phrygian",                7, {0,100,300,500,700,800,1000}},
        {"Lydian",                  7, {0,200,400,600,700,900,1100}},
        {"Mixolydian",              7, {0,200,400,500,700,900,1000}},
        {"Locrian",                 7, {0,100,300,500,600,800,1000}},
        {"Major Pentatonic",        5, {0,200,400,700,900}},
        {"Minor Pentatonic",        5, {0,300,500,700,1000}},
        {"Blues (Minor)",           6, {0,300,500,600,700,1000}},
        {"Blues (Major)",           6, {0,200,300,400,700,900}},
        {"Whole Tone",              6, {0,200,400,600,800,1000}},
        {"Diminished (W-H)",        8, {0,200,300,500,600,800,900,1100}},
        {"Diminished (H-W)",        8, {0,100,300,400,600,700,900,1000}},
        {"Hijaz / Phrygian Dom.",   7, {0,100,400,500,700,800,1000}},
        {"Hungarian Minor",         7, {0,200,300,600,700,800,1100}},
        {"Double Harmonic",         7, {0,100,400,500,700,800,1100}},
        {"Maqam Rast (quarter-tone)",   7, {0,200,350,500,700,900,1050}},
        {"Maqam Bayati (quarter-tone)", 7, {0,150,300,500,700,800,1000}},
        {"Maqam Saba (quarter-tone)",   7, {0,150,300,400,700,800,1000}},
        {"Maqam Hijaz Kar",         7, {0,100,400,500,700,800,1100}},
        {"Japanese (In Sen)",       5, {0,100,500,700,800}},
        {"Custom (Note Map)",      12, {0,100,200,300,400,500,600,700,800,900,1000,1100}},
    };
    return t;
}

// Absolute pitch classes (cents above C, [0,1200)) the tuner may land on.
class NoteQuantizer
{
public:
    void build(int key, int scale, const bool* noteOn, const float* noteCents)
    {
        pcs.clear();
        key = ((key % 12) + 12) % 12;
        if (scale < 0 || scale >= kNumScales) scale = 0;

        auto offsetFor = [&](double base) {
            int slot = (int) std::floor(base / 100.0 + 0.5);
            slot = ((slot % 12) + 12) % 12;
            return (double) noteCents[slot];
        };

        if (scale == kCustomScale) {
            for (int pc = 0; pc < 12; ++pc)
                if (noteOn[pc]) pcs.push_back(wrap(pc * 100.0 + offsetFor(pc * 100.0)));
        } else {
            const ScaleDef& s = scaleTable()[scale];
            for (int i = 0; i < s.n; ++i) {
                double base = wrap(key * 100.0 + s.deg[i]);
                pcs.push_back(wrap(base + offsetFor(base)));
            }
        }
        if (pcs.empty())   // custom map with nothing ticked -> never go silent, fall back to chromatic
            for (int pc = 0; pc < 12; ++pc) pcs.push_back(wrap(pc * 100.0 + offsetFor(pc * 100.0)));

        std::sort(pcs.begin(), pcs.end());
        pcs.erase(std::unique(pcs.begin(), pcs.end(),
                  [](double a, double b){ return std::fabs(a - b) < 1e-6; }), pcs.end());
    }

    // nearest allowed note (absolute cents, MIDI scale: 6900 = A4)
    double nearest(double cents) const
    {
        const double oct = std::floor(cents / 1200.0);
        const double r   = cents - oct * 1200.0;
        double best = 1e18, bestC = 0;
        for (double p : pcs)
            for (int k = -1; k <= 1; ++k) {
                double c = p + 1200.0 * k;
                double d = std::fabs(r - c);
                if (d < best) { best = d; bestC = c; }
            }
        return oct * 1200.0 + bestC;
    }

    // nearest with stickiness: only leave `last` when another note is closer by > hyst cents
    double pick(double cents, double hyst, bool& hasLast, double& last) const
    {
        double t = nearest(cents);
        if (hasLast && t != last && hyst > 0.0)
            if (std::fabs(cents - t) + hyst >= std::fabs(cents - last)) t = last;
        hasLast = true; last = t;
        return t;
    }

    const std::vector<double>& classes() const { return pcs; }

private:
    static double wrap(double c) { c = std::fmod(c, 1200.0); if (c < 0) c += 1200.0; if (c >= 1200.0 - 1e-9) c = 0.0; return c; }
    std::vector<double> pcs;
};

} // namespace ds
