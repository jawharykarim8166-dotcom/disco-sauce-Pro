// Disco Sauce 2 Pro - whole-chain tests
#include "../Source/DSP/Engine.h"
#include <cstdio>
#include <random>
using namespace ds; using namespace dsp2;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static void fill(float* p, int preset, float amt) {
    for (int i = 0; i < kNumParams; ++i) { p[i] = paramDef(i).df; float v; if (presetValue(preset, i, amt, v)) p[i] = v; }
}
static std::vector<float> voice(double fs, size_t n, double f0) {
    std::vector<float> o(n); double ph = 0, vp = 0; std::mt19937 rng(3); std::normal_distribution<double> nd(0, 1);
    for (size_t i = 0; i < n; ++i) { vp += 2 * kPi * 5.5 / fs; double f = f0 * std::pow(2.0, 25.0 * std::sin(vp) / 1200.0); ph += f / fs; ph -= std::floor(ph);
        double s = 0; for (int k = 1; k < 25; ++k) s += std::sin(2 * kPi * k * ph) / std::pow(k, 1.3); o[i] = (float)(0.25 * s + 0.002 * nd(rng)); }
    return o;
}
int main()
{
    const double fs = 48000; const int N = (int)(3 * fs);
    // 1. all effects neutral + tune off => bit-for-bit-ish delayed input (latency alignment, mid/side, gains)
    {
        Engine e; e.prepare(fs, 512); float p[kNumParams]; for (int i = 0; i < kNumParams; ++i) p[i] = paramDef(i).nv; p[P_TuneOn] = 0; p[P_LimOn] = 0;
        auto in = voice(fs, (size_t) N, 200.0); std::vector<float> L = in, R(in.size()); for (size_t i = 0; i < R.size(); ++i) R[i] = in[i] * 0.5f;
        auto L0 = L, R0 = R;
        for (int i = 0; i < N; i += 333) { int n = std::min(333, N - i); e.process(L.data() + i, R.data() + i, n, p, 120.0); }
        double err = 0, sig = 0; for (int i = e.latency(); i < N; ++i) { err += std::pow(L[(size_t) i] - L0[(size_t)(i - e.latency())], 2) + std::pow(R[(size_t) i] - R0[(size_t)(i - e.latency())], 2); sig += std::pow(L0[(size_t)(i - e.latency())], 2); }
        std::printf("[chain] neutral+tune off: error %.1f dB, latency %d samples (%.1f ms)\n", 10 * std::log10(err / sig + 1e-30), e.latency(), 1000.0 * e.latency() / fs);
        CHECK(10 * std::log10(err / sig + 1e-30) < -90, "neutral chain must be transparent");
    }
    // 2. every preset at several amounts: finite, bounded, and mix=0 returns the aligned dry signal
    for (int pr = 0; pr < (int) presets().size(); ++pr) for (float amt : {0.0f, 0.5f, 1.0f}) {
        Engine e; e.prepare(fs, 512); float p[kNumParams]; fill(p, pr, amt); p[P_Key] = 9; p[P_Scale] = 2;
        auto in = voice(fs, (size_t) N, 190.0); std::vector<float> L = in, R = in; bool ok = true; float pk = 0;
        for (int i = 0; i < N; i += 256) { int n = std::min(256, N - i); e.process(L.data() + i, R.data() + i, n, p, 100.0 + pr * 20); }
        for (size_t i = 0; i < L.size(); ++i) { if (!std::isfinite(L[i]) || !std::isfinite(R[i])) ok = false; pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i]))); }
        CHECK(ok && pk <= dbToLin(-0.3f) + 1e-4f, "preset %d amt %.1f: finite=%d peak=%.3f", pr, amt, (int) ok, pk);
    }
    std::printf("[chain] all 6 presets x 3 amounts: finite and under the limiter ceiling\n");
    // 3. PRO MAX end-to-end tuning: A minor, sung 30 cents flat on A3 => output on A3
    {
        Engine e; e.prepare(fs, 512); float p[kNumParams]; fill(p, kProMaxPreset, 1.0f); p[P_Key] = 9; p[P_Scale] = 2; p[P_Dbl] = 0; p[P_DlMix] = 0; p[P_RvMix] = 0; p[P_SatDrive] = 0; p[P_CpRatio] = 1; p[P_DsAmt] = 0; p[P_Mix] = 100; p[P_Retune] = 0;
        for (int i = 0; i < kNumParams; ++i) if (paramDef(i).group[0] == 'E') p[i] = paramDef(i).nv;
        double fIn = 220.0 * std::pow(2.0, -30.0 / 1200.0);
        auto in = voice(fs, (size_t) N, fIn); std::vector<float> L = in, R = in;
        for (int i = 0; i < N; i += 256) { int n = std::min(256, N - i); e.process(L.data() + i, R.data() + i, n, p, 120.0); }
        // measure output pitch by autocorrelation (long window)
        size_t s0 = (size_t)(2.0 * fs), len = 8192; double best = 0; int bl = 0;
        for (int Lg = 150; Lg < 400; ++Lg) { double a = 0, b1 = 0, b2 = 0; for (size_t i = 0; i < len; ++i) { a += L[s0 + i] * L[s0 + i + (size_t) Lg]; b1 += L[s0 + i] * L[s0 + i]; b2 += L[s0 + i + (size_t) Lg] * L[s0 + i + (size_t) Lg]; } double c = a / std::sqrt(b1 * b2 + 1e-20); if (c > best) { best = c; bl = Lg; } }
        double f = fs / bl; std::printf("[chain] PRO MAX, A minor, input %.2f Hz (-30c) -> output ~%.1f Hz (target 220.0, +-%.1f Hz lag resolution)\n", fIn, f, f * f / fs);
        CHECK(std::fabs(f - 220.0) < f * f / fs + 0.3, "end-to-end tuning");
    }
    // 4. host buffer sizes must not change the result (block-size independence)
    {
        float p[kNumParams]; fill(p, kProMaxPreset, 1.0f); p[P_Key] = 0; p[P_Scale] = 1;
        auto in = voice(fs, (size_t) N, 175.0); std::vector<float> A = in, B = in, A2 = in, B2 = in;
        Engine e1, e2; e1.prepare(fs, 64); e2.prepare(fs, 2048);
        for (int i = 0; i < N; i += 64) { int n = std::min(64, N - i); e1.process(A.data() + i, A2.data() + i, n, p, 120); }
        for (int i = 0; i < N; i += 2048) { int n = std::min(2048, N - i); e2.process(B.data() + i, B2.data() + i, n, p, 120); }
        double d = 0, s = 0; for (size_t i = 0; i < A.size(); ++i) { d += std::pow(A[i] - B[i], 2); s += A[i] * A[i]; }
        std::printf("[chain] block 64 vs 2048 difference: %.1f dB\n", 10 * std::log10(d / s + 1e-30));
        CHECK(10 * std::log10(d / s + 1e-30) < -60, "block size dependence");
    }
    std::printf("%s (%d failures)\n", failures ? "ENGINE TESTS FAILED" : "ENGINE TESTS PASSED", failures);
    return failures ? 1 : 0;
}
