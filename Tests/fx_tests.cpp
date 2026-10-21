// Disco Sauce 2 Pro - effect block tests (pure C++). Exit code != 0 on failure.
#include "../Source/DSP/Effects.h"
#include <cstdio>
#include <random>
using namespace ds;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static const double FS = 48000.0;

static double rmsDb(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i]; return 10 * std::log10(s / (b - a) + 1e-30); }
static std::vector<float> sine(double f, double amp, size_t n) { std::vector<float> v(n); for (size_t i = 0; i < n; ++i) v[i] = (float)(amp * std::sin(2 * kPi * f * i / FS)); return v; }

static void testEq()
{
    std::printf("[eq]\n");
    auto run = [&](const EqParams& p, double f) { Eq e; e.prepare(FS); e.setParams(p); auto a = sine(f, 0.1, 48000), b = a; e.process(a.data(), b.data(), 48000); return rmsDb(a, 24000, 48000) - rmsDb(sine(f, 0.1, 48000), 24000, 48000); };
    EqParams p; p.hpFreq = 1000; CHECK(run(p, 100) < -30, "HP 1k should kill 100 Hz (%.1f)", run(p, 100)); CHECK(std::fabs(run(p, 10000)) < 0.5, "HP passes 10k");
    p = EqParams(); p.g[1] = 6; p.f[1] = 1500; double g = run(p, 1500); CHECK(std::fabs(g - 6.0) < 0.4, "bell +6 dB (%.2f)", g);
    p = EqParams(); p.hsGain = 8; p.hsFreq = 8000; g = run(p, 16000); CHECK(g > 6.5 && g < 8.5, "high shelf +8 (%.2f)", g);
    p = EqParams(); p.lsGain = -6; p.lsFreq = 200; g = run(p, 40); CHECK(g < -5.0 && g > -6.5, "low shelf -6 (%.2f)", g);
    p = EqParams(); p.lpFreq = 3000; g = run(p, 12000); CHECK(g < -20, "LP 3k kills 12k (%.1f)", g);
}
static void testComp()
{
    std::printf("[compressor]\n");
    Compressor c; c.prepare(FS); CompParams p; p.thr = -20; p.ratio = 4; p.knee = 0; p.atkMs = 2; p.relMs = 50; c.setParams(p);
    auto a = sine(500, 0.3162, 48000), b = a; c.process(a.data(), b.data(), 48000);
    double gr = c.grDb(); std::printf("  steady GR %.2f dB (expect about -7.5)\n", gr);
    CHECK(std::fabs(gr + 7.5) < 1.0, "compressor gain reduction %.2f", gr);
    p.on = false; c.setParams(p); auto x = sine(500, 0.3, 4800), y = x; auto x0 = x; c.process(x.data(), y.data(), 4800); CHECK(x == x0, "bypass is exact");
}
static void testLimiter()
{
    std::printf("[limiter]\n");
    Limiter L; L.prepare(FS); L.ceilDb = -1.0f; auto a = sine(300, 4.0, 48000), b = a; L.process(a.data(), b.data(), 48000);
    float pk = 0; for (float v : a) pk = std::max(pk, std::fabs(v)); CHECK(pk <= dbToLin(-1.0f) + 1e-6f, "limiter ceiling (%.4f)", pk);
}
static void testDelay()
{
    std::printf("[delay]\n");
    Delay d; d.prepare(FS); DelayParams p; p.timeMs = 100; p.mix = 1; p.fb = 0; p.ping = false; p.tone = 20000; p.hp = 10; d.setParams(p);
    std::vector<float> a(24000, 0.0f), b(24000, 0.0f); a[0] = 1; b[0] = 1; d.process(a.data(), b.data(), 24000);
    size_t pk = 0; for (size_t i = 100; i < a.size(); ++i) if (std::fabs(a[i]) > std::fabs(a[pk > 100 ? pk : 100]) || pk == 0) pk = i;
    std::printf("  echo at sample %zu (expect 4800)\n", pk);
    CHECK(pk >= 4790 && pk <= 4810, "delay time off (%zu)", pk);
}
static void testReverb()
{
    std::printf("[reverb]\n");
    Reverb r; r.prepare(FS); ReverbParams p; p.mix = 1; p.size = 0.6f; r.setParams(p);
    std::vector<float> a(48000 * 8, 0.0f), b(a.size(), 0.0f); a[0] = b[0] = 1;
    for (size_t i = 0; i < a.size(); i += 512) r.process(a.data() + i, b.data() + i, (int) std::min<size_t>(512, a.size() - i));
    bool fin = true; for (float v : a) if (!std::isfinite(v)) fin = false;
    double early = rmsDb(a, 2400, 24000), late = rmsDb(a, 48000 * 6, 48000 * 8);
    std::printf("  tail level %.1f dB early, %.1f dB at 6-8 s\n", early, late);
    CHECK(fin && early > -80 && late < early - 20, "reverb tail should exist and decay");
}
static void testDeEss()
{
    std::printf("[de-esser]\n");
    auto run = [&](double f) { DeEsser d; d.prepare(FS); DeEssParams p; p.freq = 6000; p.thr = -30; p.amount = 1; d.setParams(p); auto a = sine(f, 0.3, 48000), b = a; d.process(a.data(), b.data(), 48000); return rmsDb(a, 24000, 48000) - rmsDb(sine(f, 0.3, 48000), 24000, 48000); };
    double hi = run(9000), lo = run(300); std::printf("  9 kHz %.1f dB, 300 Hz %.1f dB\n", hi, lo);
    CHECK(hi < -6 && std::fabs(lo) < 0.5, "de-esser acts on sibilance only");
}
static void testDoubler()
{
    std::printf("[doubler]\n");
    Doubler d; d.prepare(FS); DblParams p; p.amount = 1; p.voices = 4; d.setParams(p);
    auto a = sine(220, 0.3, 48000), b = a; d.process(a.data(), b.data(), 48000);
    double dl = rmsDb(a, 24000, 48000), dr = rmsDb(b, 24000, 48000), orig = rmsDb(sine(220, 0.3, 48000), 24000, 48000);
    std::printf("  L %.1f dB R %.1f dB (dry %.1f)\n", dl, dr, orig);
    bool differ = false; for (size_t i = 24000; i < 48000; ++i) if (std::fabs(a[i] - b[i]) > 1e-3f) differ = true;
    CHECK(differ, "doubler should widen stereo"); CHECK(dl < orig + 9 && dr < orig + 9, "doubler gain bounded");
}
static void testFuzz()
{
    std::printf("[fuzz] all blocks, extreme random parameters, hostile signals\n");
    std::mt19937 rng(5); std::uniform_real_distribution<double> U(0, 1);
    Eq eq; Compressor cp; DeEsser de; Doubler db; Delay dl; Reverb rv; Limiter lm;
    for (double fs : {44100.0, 48000.0, 96000.0}) {
        eq.prepare(fs); cp.prepare(fs); de.prepare(fs); db.prepare(fs); dl.prepare(fs); rv.prepare(fs); lm.prepare(fs);
        bool ok = true; float worst = 0;
        for (int blk = 0; blk < 300; ++blk) {
            EqParams e; e.hpFreq = (float)(20 + U(rng) * 380); e.lsFreq = (float)(40 + U(rng) * 560); e.lsGain = (float)(U(rng) * 30 - 15); e.hsFreq = (float)(2000 + U(rng) * 14000); e.hsGain = (float)(U(rng) * 30 - 15); e.lpFreq = (float)(3000 + U(rng) * 17000);
            for (int k = 0; k < 3; ++k) { e.f[k] = (float)(100 + U(rng) * 11000); e.g[k] = (float)(U(rng) * 30 - 15); e.q[k] = (float)(0.3 + U(rng) * 7.7); }
            eq.setParams(e);
            CompParams c; c.thr = (float)(-60 + U(rng) * 60); c.ratio = (float)(1 + U(rng) * 19); c.atkMs = (float)(0.1 + U(rng) * 100); c.relMs = (float)(10 + U(rng) * 990); c.knee = (float)(U(rng) * 24); c.makeup = (float)(U(rng) * 24); c.mix = (float) U(rng); cp.setParams(c);
            DeEssParams d; d.freq = (float)(3000 + U(rng) * 9000); d.thr = (float)(-50 + U(rng) * 50); d.amount = (float) U(rng); de.setParams(d);
            DblParams b; b.amount = (float) U(rng); b.voices = 1 + (int)(U(rng) * 4); b.delayMs = (float)(5 + U(rng) * 55); b.detune = (float)(U(rng) * 30); b.width = (float) U(rng); b.tone = (float)(2000 + U(rng) * 18000); db.setParams(b);
            DelayParams y; y.timeMs = (float)(20 + U(rng) * 1480); y.fb = (float)(U(rng) * 0.95); y.tone = (float)(500 + U(rng) * 15500); y.hp = (float)(20 + U(rng) * 980); y.mix = (float) U(rng); y.ping = U(rng) < 0.5; dl.setParams(y);
            ReverbParams r; r.size = (float) U(rng); r.damp = (float) U(rng); r.preMs = (float)(U(rng) * 200); r.width = (float) U(rng); r.low = (float)(20 + U(rng) * 980); r.high = (float)(1000 + U(rng) * 19000); r.mix = (float) U(rng); rv.setParams(r);
            lm.ceilDb = (float)(-12 + U(rng) * 12);
            const int n = 256; std::vector<float> L((size_t) n), R((size_t) n);
            const int kind = (int)(U(rng) * 4);
            for (int i = 0; i < n; ++i) { float s = kind == 0 ? (float)(2 * U(rng) - 1) : kind == 1 ? (((i / 20) & 1) ? 1.0f : -1.0f) : kind == 2 ? 0.0f : (float)(0.7 * std::sin(i * 0.3)); L[(size_t) i] = s; R[(size_t) i] = s * 0.8f; }
            eq.process(L.data(), R.data(), n); cp.process(L.data(), R.data(), n); de.process(L.data(), R.data(), n);
            saturate(SatParams{true, (float) U(rng), (float) U(rng)}, L.data(), R.data(), n);
            db.process(L.data(), R.data(), n); dl.process(L.data(), R.data(), n); rv.process(L.data(), R.data(), n); lm.process(L.data(), R.data(), n);
            for (int i = 0; i < n; ++i) { if (!std::isfinite(L[(size_t) i]) || !std::isfinite(R[(size_t) i])) ok = false; worst = std::max(worst, std::max(std::fabs(L[(size_t) i]), std::fabs(R[(size_t) i]))); }
        }
        std::printf("  fs=%.0f: %s, worst output peak %.3f\n", fs, ok ? "finite" : "NON-FINITE", worst);
        CHECK(ok, "non-finite output at fs=%.0f", fs); CHECK(worst <= 1.0f + 1e-5f, "limiter let %.3f through", worst);
    }
}
int main()
{
    testEq(); testComp(); testLimiter(); testDelay(); testReverb(); testDeEss(); testDoubler(); testFuzz();
    std::printf("\n%s (%d failures)\n", failures ? "FX TESTS FAILED" : "FX TESTS PASSED", failures);
    return failures ? 1 : 0;
}
