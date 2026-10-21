// Disco Sauce 2 Pro - tuning tests (pure C++). Exit code != 0 on any failure.
#include "../Source/DSP/PitchCorrector.h"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>
#include <string>

using namespace ds;
static int failures = 0;
static bool gVerbose = false;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static double centsOf(double f, double refA = 440.0) { return 6900.0 + 1200.0 * std::log2(f / refA); }
static double freqOf(double c, double refA = 440.0) { return refA * std::pow(2.0, (c - 6900.0) / 1200.0); }

// ---------------------------------------------------------------- synthetic voice
struct Voice
{
    double fs; std::mt19937 rng{1234}; std::normal_distribution<double> nd{0.0, 1.0};
    double phase = 0, vibPh = 0, jit = 0;
    int vowel = 0;
    double amp = 0.3, breath = 0.01, vibCents = 0, vibHz = 5.5;

    explicit Voice(double sr) : fs(sr) {}
    static double formantGain(double f, int vowel)
    {
        static const double F[3][3] = { {700,1220,2600}, {300,2300,3000}, {450,800,2830} };
        static const double B[3] = {90, 110, 170};
        double g = 0.03;
        for (int i = 0; i < 3; ++i) { double d = (f - F[vowel][i]) / B[i]; g += (i == 0 ? 1.0 : 0.6 / (i + 0.5)) / (1.0 + d * d); }
        return g;
    }
    // f0 in Hz (centre), returns next sample
    float next(double f0)
    {
        vibPh += 2 * kPi * vibHz / fs;
        jit = 0.995 * jit + 0.0002 * nd(rng);   // ~0.3 % wander (~2 cents), realistic
        double f = f0 * std::pow(2.0, (vibCents * std::sin(vibPh)) / 1200.0) * (1.0 + jit);
        phase += f / fs; phase -= std::floor(phase);
        double s = 0;
        for (int k = 1; k * f < 0.45 * fs && k < 40; ++k)
            s += std::pow((double) k, -1.1) * formantGain(k * f, vowel) * std::sin(2 * kPi * k * phase);
        return (float)(amp * s * 0.9 + breath * amp * nd(rng));
    }
};

// independent high-resolution pitch measurement (normalised ACF, parabolic) on a window
static double measureF0(const std::vector<float>& y, size_t start, size_t len, double fs, double fGuess)
{
    int Lc = (int)(fs / fGuess);
    int Lmin = (int)(Lc * 0.7), Lmax = (int)(Lc * 1.4) + 2;
    std::vector<double> c((size_t)(Lmax + 2), 0.0);
    double e1 = 0; for (size_t i = 0; i < len; ++i) e1 += (double) y[start + i] * y[start + i];
    int best = Lmin; double bv = -2;
    for (int L = Lmin - 1; L <= Lmax; ++L) {
        double num = 0, e2 = 0;
        for (size_t i = 0; i < len; ++i) { num += (double) y[start + i] * y[start + i + L]; e2 += (double) y[start + i + L] * y[start + i + L]; }
        c[(size_t) L] = num / std::sqrt(e1 * e2 + 1e-20);
    }
    for (int L = Lmin; L <= Lmax - 1; ++L) if (c[(size_t) L] > bv) { bv = c[(size_t) L]; best = L; }
    double a = c[(size_t)(best - 1)], b = c[(size_t) best], cc = c[(size_t)(best + 1)];
    double den = a - 2 * b + cc;
    double off = den < -1e-20 ? 0.5 * (a - cc) / den : 0.0;
    return fs / (best + off);
}

// ---------------------------------------------------------------- 1. quantizer, exhaustive
static void testQuantizer()
{
    std::printf("[quantizer] all keys x all scales vs brute force\n");
    std::mt19937 rng(7); std::uniform_real_distribution<double> U(0.0, 1.0);
    long checks = 0;
    for (int scale = 0; scale < kNumScales; ++scale)
        for (int key = 0; key < 12; ++key) {
            bool on[12]; float off[12];
            for (int i = 0; i < 12; ++i) { on[i] = (scale != kCustomScale) || U(rng) < 0.4; off[i] = (float)(U(rng) * 20.0 - 10.0); if (scale != kCustomScale && (key + scale) % 3 == 0) off[i] = 0; }
            NoteQuantizer q; q.build(key, scale, on, off);
            // brute force: enumerate every allowed note over a wide range
            std::vector<double> all;
            for (int o = -3; o < 14; ++o) for (double p : q.classes()) all.push_back(p + 1200.0 * o);
            for (int n = 0; n < 4000; ++n) {
                double x = 2000.0 + U(rng) * 9000.0;
                double bf = all[0]; for (double a : all) if (std::fabs(a - x) < std::fabs(bf - x)) bf = a;
                double r = q.nearest(x);
                ++checks;
                CHECK(std::fabs(std::fabs(r - x) - std::fabs(bf - x)) < 1e-6, "scale %d key %d x=%.2f got %.2f brute %.2f", scale, key, x, r, bf);
            }
            if (scale != kCustomScale) {
                // every degree must be present (no silent loss of notes)
                const ScaleDef& s = scaleTable()[scale];
                CHECK((int) q.classes().size() == s.n || (int) q.classes().size() <= s.n, "scale %d class count", scale);
                if ((key + scale) % 3 == 0)
                    for (int i = 0; i < s.n; ++i) {
                        double want = std::fmod(key * 100.0 + s.deg[i] + 1200.0, 1200.0), found = 1e9;
                        for (double p : q.classes()) found = std::min(found, std::min(std::fabs(p - want), 1200.0 - std::fabs(p - want)));
                        CHECK(found < 1e-6, "scale %d key %d missing degree %g", scale, key, s.deg[i]);
                    }
            }
        }
    // custom with nothing ticked must fall back, never be empty
    bool none[12] = {}; float z[12] = {};
    NoteQuantizer q; q.build(0, kCustomScale, none, z);
    CHECK(q.classes().size() == 12, "empty custom map fallback");
    // hysteresis: never farther than half-gap + hyst
    bool on[12]; for (auto& b : on) b = true;
    q.build(0, 1, on, z);
    bool has = false; double last = 0; double worst = 0;
    for (int n = 0; n < 20000; ++n) { double x = 6000 + 400 * std::sin(n * 0.01) + 700 * std::sin(n * 0.0013); double t = q.pick(x, 20.0, has, last); worst = std::max(worst, std::fabs(x - t)); }
    CHECK(worst <= 100.0 + 20.0 + 1e-6, "hysteresis bound (worst %.1f)", worst);
    std::printf("  %ld comparisons done\n", checks);
}

// ---------------------------------------------------------------- 2/3. detector + corrector on voices
struct RunResult { double medAbs, maxAbs, p95; int n; int underruns; bool finite; };

static RunResult runTuneTest(double fs, int scale, int key, double f0lo, double f0hi, double ampDb, int seed, double vibCents, double retuneMs, int vowel, double detuneMax = 22.0)
{
    PitchCorrector pc; pc.prepare(fs);
    TuneSettings S; S.key = key; S.scale = scale; S.amount = 1.0f; S.retuneMs = (float) retuneMs; S.hystCents = 10.0f;
    S.rangeLow = 80; S.rangeHigh = 1000;
    bool on[12];
    std::mt19937 rng((unsigned) seed);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    for (int i = 0; i < 12; ++i) { on[i] = (i % 2 == 0) || (i == 7); S.noteOn[i] = on[i]; S.noteCents[i] = 0; }
    NoteQuantizer q; q.build(key, scale, S.noteOn, S.noteCents);
    pc.setSettings(S);

    Voice v(fs); v.vowel = vowel; v.amp = std::pow(10.0, ampDb / 20.0) * 3.0; v.vibCents = vibCents;
    std::vector<float> in, out;
    struct Note { size_t s, e; double targetHz; };
    std::vector<Note> notes;
    const int nNotes = 14;
    // 150 ms of silence first
    for (size_t i = 0; i < (size_t)(0.15 * fs); ++i) { in.push_back(0.0f); }
    for (int n = 0; n < nNotes; ++n) {
        // pick a target scale note in range
        double lo = centsOf(f0lo), hi = centsOf(f0hi), tgt = 0;
        for (int tries = 0; tries < 200; ++tries) {
            double x = lo + U(rng) * (hi - lo);
            tgt = q.nearest(x);
            if (tgt >= lo && tgt <= hi) break;
        }
        double detune = (U(rng) * 2 - 1) * detuneMax;      // off-pitch singer (peak deviation incl. vibrato stays < 50c)
        double fIn = freqOf(tgt + detune);
        size_t len = (size_t)((0.45 + 0.3 * U(rng)) * fs);
        size_t startNote = in.size();
        for (size_t i = 0; i < len; ++i) {
            // short voiced fade-in/out
            double env = std::min(1.0, std::min((double) i / (0.01 * fs), (double)(len - i) / (0.02 * fs)));
            in.push_back((float)(v.next(fIn) * env));
        }
        notes.push_back({ startNote, startNote + len, freqOf(tgt) });
        // consonant-like noise burst between notes
        size_t gap = (size_t)(0.08 * fs);
        for (size_t i = 0; i < gap; ++i) in.push_back((float)(0.05 * v.amp * v.nd(v.rng) * std::sin(kPi * i / gap)));
    }
    for (size_t i = 0; i < (size_t)(0.3 * fs); ++i) in.push_back(0.0f);
    const int L = pc.latency();
    for (size_t i = 0; i < in.size() + (size_t) L + 256; ++i) out.push_back(pc.process(i < in.size() ? in[i] : 0.0f));

    RunResult R{0, 0, 0, 0, pc.underrunCount(), true};
    for (float s : out) if (!std::isfinite(s) || std::fabs(s) > 20.0f) R.finite = false;

    std::vector<double> errs;
    for (auto& nt : notes) {
        // output time = input time + L; skip first 120 ms (detection onset) and last 40 ms
        size_t a = nt.s + (size_t)(0.13 * fs) + L, b = nt.e - (size_t)(0.04 * fs) + L;
        for (size_t w = a; w + 4096 + 1200 < b && w + 4096 + 1200 < out.size(); w += (size_t)(0.05 * fs)) {
            double f = measureF0(out, w, 4096 > (size_t)(0.09 * fs) ? 4096 : (size_t)(0.09 * fs), fs, nt.targetHz);
            double e = std::fabs(1200.0 * std::log2(f / nt.targetHz));
            if (gVerbose && e > 8.0) std::printf("    outlier: note %.1f Hz (in-note t=%.3fs of %.3fs) measured %.2f Hz err %.1f c\n", nt.targetHz, (double)(w - L - nt.s) / fs, (double)(nt.e - nt.s) / fs, f, e);
            errs.push_back(e);
        }
    }
    if (!errs.empty()) {
        std::sort(errs.begin(), errs.end());
        R.n = (int) errs.size(); R.medAbs = errs[errs.size() / 2]; R.maxAbs = errs.back(); R.p95 = errs[(size_t)(errs.size() * 0.95)];
    }
    return R;
}

static void testAllScalesTuning()
{
    std::printf("[tuning] every scale, hard tune (retune 0): output pitch vs target note\n");
    const double rates[3] = {44100.0, 48000.0, 96000.0};
    double globalWorst = 0;
    for (int scale = 0; scale < kNumScales; ++scale) {
        double sWorst = 0, sMed = 0; int cnt = 0; int under = 0;
        for (int ri = 0; ri < 3; ++ri) {
            if (scale % 3 != ri && scale != 0 && scale != 20 && scale != kCustomScale) continue;   // spread rates over scales, all 3 on key scales
            double fs = rates[ri];
            int key = (scale * 5 + ri * 3) % 12;
            // two registers: male low-mid and female/high
            double lo[2] = {95, 220}, hi[2] = {260, 800};
            for (int reg = 0; reg < 2; ++reg) {
                RunResult R = runTuneTest(fs, scale, key, lo[reg], hi[reg], -20 - 10 * reg, 100 + scale * 7 + reg, 20.0, 0.0, (scale + reg) % 3);
                CHECK(R.finite, "scale %d non-finite output", scale);
                CHECK(R.n > 10, "scale %d: too few measurements (%d)", scale, R.n);
                sWorst = std::max(sWorst, R.maxAbs); sMed = std::max(sMed, R.medAbs); under += R.underruns; ++cnt;
            }
        }
        std::printf("  %-30s runs=%d  worst median %.2f c  worst max %.2f c  underruns %d\n", scaleTable()[scale].name, cnt, sMed, sWorst, under);
        CHECK(sMed < 2.0, "scale %d median error %.2f cents too high", scale, sMed);
        CHECK(sWorst < 6.0, "scale %d max error %.2f cents too high", scale, sWorst);
        CHECK(under == 0, "scale %d detector starved %d times", scale, under);
        globalWorst = std::max(globalWorst, sWorst);
    }
    std::printf("  global worst error: %.2f cents\n", globalWorst);
}

static void testExtremes()
{
    std::printf("[extremes] quiet vocal, wide vibrato, loud, range edges\n");
    struct C { const char* name; double lo, hi, db, vib; };
    C cs[] = { {"very quiet -60 dBFS", 120, 400, -60, 15}, {"quiet -50 dBFS", 150, 500, -50, 20}, {"loud -3 dBFS", 120, 500, -6, 15},
               {"wide vibrato 35c", 150, 450, -20, 35}, {"high falsetto 600-900", 600, 900, -25, 10}, {"low bass 85-130", 85, 130, -20, 10} };
    for (auto& c : cs) {
        RunResult R = runTuneTest(48000.0, 1, 2, c.lo, c.hi, c.db, 555, c.vib, 0.0, 0, c.vib > 30 ? 6.0 : 22.0);
        std::printf("  %-26s median %.2f c  max %.2f c  (%d meas)\n", c.name, R.medAbs, R.maxAbs, R.n);
        CHECK(R.finite && R.n > 8, "%s: invalid", c.name);
        CHECK(R.medAbs < 2.0 && R.maxAbs < 8.0, "%s: error too high", c.name);
    }
}

static void testTransparencyAndSafety()
{
    std::printf("[safety] amount=0 transparency, silence, noise, clipping, settings abuse\n");
    const double fs = 48000.0;
    // transparency: amount 0 -> output equals delayed input closely (voiced steady vowel)
    {
        PitchCorrector pc; pc.prepare(fs);
        TuneSettings S; S.amount = 0.0f; S.retuneMs = 0; pc.setSettings(S);
        Voice v(fs); v.amp = 0.5; v.vibCents = 0;
        const int N = (int)(1.6 * fs), L = pc.latency();
        std::vector<float> in((size_t) N), out((size_t) N);
        for (int i = 0; i < N; ++i) { in[(size_t) i] = v.next(190.0); out[(size_t) i] = pc.process(in[(size_t) i]); }
        // phase-insensitive fidelity: compare magnitude spectra (PSOLA may shift waveform phase by a fraction of a period)
        double snr = 0, nerr = 0, nsig = 0;
        for (int w = (int)(0.6 * fs); w + 4096 < N - L; w += 2048) {
            for (int b = 1; b < 300; ++b) {
                double fb = b * fs / 4096.0, r1 = 0, i1 = 0, r2 = 0, i2 = 0;
                for (int n = 0; n < 4096; ++n) {
                    double win = 0.5 - 0.5 * std::cos(2 * kPi * n / 4096.0), c = std::cos(2 * kPi * fb * n / fs), sn = std::sin(2 * kPi * fb * n / fs);
                    r1 += win * in[(size_t)(w - L + n)] * c; i1 -= win * in[(size_t)(w - L + n)] * sn;
                    r2 += win * out[(size_t)(w + n)] * c;    i2 -= win * out[(size_t)(w + n)] * sn;
                }
                double m1 = std::sqrt(r1 * r1 + i1 * i1), m2 = std::sqrt(r2 * r2 + i2 * i2);
                nsig += m1 * m1; nerr += (m1 - m2) * (m1 - m2);
            }
        }
        snr = 10 * std::log10(nsig / (nerr + 1e-30));
        std::printf("  amount=0 magnitude-spectrum fidelity vs delayed dry: %.1f dB\n", snr);
        CHECK(snr > 18.0, "transparency fidelity %.1f dB", snr);
    }
    // abuse: random settings, noise, silence, full-scale square, DC, denormals
    {
        PitchCorrector pc; pc.prepare(fs);
        std::mt19937 rng(99); std::uniform_real_distribution<double> U(0, 1);
        bool ok = true;
        for (int blk = 0; blk < 400; ++blk) {
            TuneSettings S;
            S.key = (int)(U(rng) * 12); S.scale = (int)(U(rng) * kNumScales);
            for (int i = 0; i < 12; ++i) { S.noteOn[i] = U(rng) < 0.3; S.noteCents[i] = (float)(U(rng) * 100 - 50); }
            S.amount = (float) U(rng); S.retuneMs = (float)(U(rng) < 0.3 ? 0 : U(rng) * 500); S.humanize = (float) U(rng);
            S.hystCents = (float)(U(rng) * 100); S.transpose = (float)(U(rng) * 24 - 12); S.fine = (float)(U(rng) * 200 - 100);
            S.refA = (float)(400 + U(rng) * 80); S.rangeLow = (float)(80 + U(rng) * 300); S.rangeHigh = (float)(S.rangeLow + 50 + U(rng) * 800);
            S.sensitivity = (float) U(rng); S.gateDb = (float)(-90 + U(rng) * 60);
            pc.setSettings(S);
            int kind = (int)(U(rng) * 6);
            Voice v(fs); v.amp = 0.4;
            for (int i = 0; i < 960; ++i) {
                float s = 0;
                switch (kind) {
                    case 0: s = 0; break;
                    case 1: s = (float)(2 * U(rng) - 1); break;
                    case 2: s = ((i / 60) & 1) ? 1.0f : -1.0f; break;
                    case 3: s = 0.5f; break;
                    case 4: s = 1e-30f * (float)(U(rng) - 0.5); break;
                    default: s = v.next(100 + 700 * U(rng)); break;
                }
                float y = pc.process(s);
                if (!std::isfinite(y) || std::fabs(y) > 12.0f) ok = false;
            }
        }
        CHECK(ok, "non-finite / exploding output under abuse");
        std::printf("  random settings + hostile signals: %s\n", ok ? "stable" : "UNSTABLE");
    }
}

static void testDetectorAccuracy()
{
    std::printf("[detector] raw detection accuracy (no correction), 80..1000 Hz\n");
    for (double fs : {44100.0, 48000.0, 96000.0}) {
        double worst = 0; int bad = 0, n = 0;
        for (double f = 85; f <= 950; f *= 1.07) {
            PitchCorrector pc; pc.prepare(fs);
            TuneSettings S; S.amount = 0; pc.setSettings(S);
            Voice v(fs); v.amp = 0.3; v.vibCents = 0; v.vowel = (int) f % 3; v.breath = 0.02;
            int N = (int)(0.8 * fs);
            for (int i = 0; i < N; ++i) pc.process(v.next(f));
            int64_t t = (int64_t)(0.45 * fs);
            auto p = pc.detectedAt(t);
            ++n;
            if (!p.voiced) { ++bad; continue; }
            double err = std::fabs(1200 * std::log2(p.f0 / f));
            worst = std::max(worst, err);
            if (err > (f < 100 ? 14.0 : 8.0)) ++bad;
        }
        std::printf("  fs=%.0f: %d pitches, worst error %.2f cents, failures %d\n", fs, n, worst, bad);
        CHECK(bad == 0, "detector failures at fs=%.0f", fs);
    }
}

int main()
{
    testQuantizer();
    testDetectorAccuracy();
    testTransparencyAndSafety();
    testAllScalesTuning();
    testExtremes();
    std::printf("\n%s (%d failures)\n", failures ? "TUNING TESTS FAILED" : "TUNING TESTS PASSED", failures);
    return failures ? 1 : 0;
}
