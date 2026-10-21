// Disco Sauce 2 Pro - pitch corrector (pure C++, no JUCE, fully unit-tested)
//
//  * detection : decimated YIN (CMNDF) + full-rate sub-sample refinement, 2.5 ms hop,
//                look-ahead median smoothing, octave-jump guard, voicing hysteresis
//  * correction: pitch-synchronous grain overlap-add (PSOLA-style). The OUTPUT pitch is set by the
//                synthesis grain spacing, so with Retune = 0 the output lands exactly on the target
//                note no matter how the input drifts (detector noise cannot leak into the result).
//  * formants  : preserved (grains are copied, never resampled)
//  * latency   : fixed, reported by latency() so the host can compensate
#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "Scales.h"

namespace ds {

struct TuneSettings
{
    int   key = 0, scale = 0;
    bool  noteOn[12];
    float noteCents[12];
    float amount      = 1.0f;     // 0..1   how much of the correction is applied
    float retuneMs    = 15.0f;    // 0 = instant (hard tune)
    float humanize    = 0.0f;     // 0..1   slower retune on long notes (keeps natural drift/vibrato)
    float hystCents   = 10.0f;    // note stickiness
    float transpose   = 0.0f;     // semitones, applied to the target
    float fine        = 0.0f;     // cents, applied to the target
    float refA        = 440.0f;
    float rangeLow    = 80.0f;    // Hz
    float rangeHigh   = 1000.0f;  // Hz
    float sensitivity = 0.75f;    // 0..1  (1 = accept the weakest periodicity)
    float gateDb      = -80.0f;   // pitch detection gate

    TuneSettings() { for (int i = 0; i < 12; ++i) { noteOn[i] = true; noteCents[i] = 0.0f; } }
};

class PitchCorrector
{
public:
    static constexpr double kMinHz = 80.0;     // lowest pitch the engine/latency is designed for
    static constexpr double kMaxHz = 1200.0;

    void prepare(double sampleRate)
    {
        fs = sampleRate;
        D  = std::max(1, (int) std::floor(fs / 11000.0));
        fsd = fs / D;
        gd = 4 * D;

        // anti-alias FIR (windowed sinc)
        h.assign((size_t)(2 * gd + 1), 0.0f);
        const double wc = 2.0 * kPi * (0.17 * fsd) / fs;
        double sum = 0;
        for (int k = -gd; k <= gd; ++k) {
            double s = (k == 0) ? wc / kPi : std::sin(wc * k) / (kPi * k);
            double w = 0.54 + 0.46 * std::cos(kPi * k / gd);
            h[(size_t)(k + gd)] = (float)(s * w); sum += s * w;
        }
        for (auto& v : h) v = (float)(v / sum);

        tauMaxD = (int) std::ceil(fsd / kMinHz) + 2;
        tauMinD = std::max(2, (int) std::floor(fsd / kMaxHz));
        Wd      = tauMaxD + 8;
        hop     = std::max(D, (int) std::lround(0.0025 * fs));

        T0max = fs / kMinHz;
        HlMax = (int) std::ceil(T0max) + 1;

        const int detLag = gd + D * (Wd / 2 + tauMaxD);
        const int ltA = 2 * HlMax + (int)(0.5 * T0max) + (int)(0.1 * T0max) + 64;
        const int ltB = detLag + HlMax + 3 * hop + 64;
        Ltot = std::max(ltA, ltB);

        size_t n = 1; while ((int) n < 4 * Ltot + 8192) n <<= 1;
        x.assign(n, 0.0f); mask = (int64_t) n - 1;
        size_t m = 1; while ((int) m < 4 * HlMax + 64) m <<= 1;
        acc.assign(m, 0.0f); nrm.assign(m, 0.0f); amask = (int64_t) m - 1;
        xd.assign(4096, 0.0f); maskd = 4095;
        seg.assign((size_t)(Wd + tauMaxD + 4), 0.0f);
        dd.assign((size_t)(tauMaxD + 4), 0.0f);
        cm.assign((size_t)(tauMaxD + 4), 0.0f);
        cmR.assign((size_t)(tauMaxD + 4), 0.0f);
        dr.assign((size_t)(tauMaxD + 4), 0.0f);
        res.assign((size_t)(Wd + tauMaxD + 4), 0.0f);
        fTc.assign(FR, 0); fLog.assign(FR, 0.0f); fVoiced.assign(FR, 0);
        vSlew = (float)(1.0 / (0.002 * fs));
        reset();
        applySettings(TuneSettings(), true);
    }

    void reset()
    {
        std::fill(x.begin(), x.end(), 0.0f);
        std::fill(acc.begin(), acc.end(), 0.0f);
        std::fill(nrm.begin(), nrm.end(), 0.0f);
        std::fill(xd.begin(), xd.end(), 0.0f);
        nIn = 0; nd = 0; nFrames = 0; lastTc = -1;
        prevFrameVoiced = false; prevLog = 0; unvoicedRun = 99;
        tsNext = 0; hasPrev = false; segActive = false; curCorr = 0; segAge = 0;
        qHasLast = false; qLast = 0; vMix = 0; vTarget = 0; lastSpacing = 0;
        underruns = 0; dInC = 0; dTargetC = 0; dVoiced = false;
    }

    int latency() const { return Ltot; }

    void setSettings(const TuneSettings& s) { applySettings(s, false); }

    float process(float in)
    {
        x[(size_t)(nIn & mask)] = in;
        ++nIn;
        stepDecimator();
        if (nIn % hop == 0) computeFrame();

        const int64_t te = nIn - 1 - Ltot;
        if (te < 0) return 0.0f;

        while (tsNext <= (double)(te + HlMax)) placeMark(te);

        const size_t ei = (size_t)(te & amask);
        const float dry = x[(size_t)(te & mask)];
        if ((te & 15) == 0) vTarget = pitchAt(te).voiced ? 1.0f : 0.0f;
        vMix += (vTarget - vMix) * vSlew;
        float ps = dry;
        const float n = nrm[ei];
        if (n > 1e-4f) ps = acc[ei] / n;
        acc[ei] = 0.0f; nrm[ei] = 0.0f;
        return dry + (ps - dry) * vMix;
    }

#ifdef DS_TRACE
    struct TraceRec { double ts, f0, inC, tgt, T0s; };
    std::vector<TraceRec> trace;
#endif
    // --- diagnostics for the UI / tests
    float lastInputCents()  const { return (float) dInC; }
    float lastTargetCents() const { return (float) dTargetC; }
    bool  lastVoiced()      const { return dVoiced; }
    int   underrunCount()   const { return underruns; }
    // detector output at a given (absolute) sample time, for tests
    struct PInfo { bool voiced; double f0; };
    PInfo detectedAt(int64_t t) const { return pitchAt(t); }

private:
    // ------------------------------------------------------------------ settings
    void applySettings(const TuneSettings& n, bool force)
    {
        bool rebuild = force || n.key != S.key || n.scale != S.scale;
        for (int i = 0; i < 12 && !rebuild; ++i)
            if (n.noteOn[i] != S.noteOn[i] || n.noteCents[i] != S.noteCents[i]) rebuild = true;
        S = n;
        if (rebuild) { q.build(S.key, S.scale, S.noteOn, S.noteCents); qHasLast = false; }
    }

    // ------------------------------------------------------------------ decimator
    void stepDecimator()
    {
        const int64_t n = nIn - 1;
        const int64_t c = n - gd;
        if (c < 0 || (c % D) != 0) return;
        double s = 0.0;
        for (int k = -gd; k <= gd; ++k) s += (double) h[(size_t)(k + gd)] * x[(size_t)((c + k) & mask)];
        xd[(size_t)((c / D) & maskd)] = (float) s;
        nd = c / D + 1;
    }

    // ------------------------------------------------------------------ detection
    void pushFrame(int64_t tc, double log2f, bool voiced)
    {
        if (tc <= lastTc) tc = lastTc + 1;
        lastTc = tc;
        const size_t i = (size_t)(nFrames & (FR - 1));
        fTc[i] = tc; fLog[i] = (float) log2f; fVoiced[i] = voiced ? 1 : 0;
        ++nFrames;
        prevFrameVoiced = voiced;
        if (voiced) { prevLog = log2f; unvoicedRun = 0; } else ++unvoicedRun;
    }

    void computeFrame()
    {
        const int64_t jn = nd - 1;
        const int64_t js = jn - (Wd + tauMaxD) + 1;
        if (js < 0) return;
        const int total = Wd + tauMaxD;
        for (int i = 0; i < total; ++i) seg[(size_t) i] = xd[(size_t)((js + i) & maskd)];

        const double tcFallback = (double)(js * D) + 0.5 * D * (Wd + tauMaxD);
        double e = 0; for (int i = 0; i < Wd; ++i) e += (double) seg[(size_t) i] * seg[(size_t) i];
        const double rms = std::sqrt(e / Wd);
        if (rms < std::pow(10.0, S.gateDb / 20.0) || rms < 1e-9) { pushFrame((int64_t) tcFallback, 0.0, false); return; }

        dd[0] = 0.0f;
        for (int tau = 1; tau <= tauMaxD; ++tau) {
            double s = 0;
            const float* a = seg.data(); const float* b = seg.data() + tau;
            for (int i = 0; i < Wd; ++i) { double dl = (double) a[i] - b[i]; s += dl * dl; }
            dd[(size_t) tau] = (float) s;
        }
        cm[0] = 1.0f; double run = 0;
        for (int tau = 1; tau <= tauMaxD; ++tau) {
            run += dd[(size_t) tau];
            cm[(size_t) tau] = run > 1e-20 ? (float)(dd[(size_t) tau] * tau / run) : 1.0f;
        }

        const int tauLo = std::max(tauMinD, (int) std::floor(fsd / std::max(S.rangeHigh, 100.0f)));
        const int tauHi = std::min(tauMaxD - 1, (int) std::ceil(fsd / std::max(S.rangeLow, (float) kMinHz)));
        double thr = 0.08 + 0.32 * std::min(1.0f, std::max(0.0f, S.sensitivity));
        if (prevFrameVoiced) thr *= 1.25;

        // Whitened (LPC residual) copy of the segment. A strong formant can make one harmonic (e.g. the 3rd
        // harmonic of an "ah") look like the fundamental to any period detector; the glottal residual has
        // flat harmonics, so its period dips are only deep at the TRUE period. Selection uses both.
        {
            constexpr int P = 10;
            double r[P + 1];
            for (int k = 0; k <= P; ++k) {
                double a = 0;
                for (int i = k; i < total; ++i) {
                    const double w1 = 0.5 - 0.5 * std::cos(2.0 * kPi * (i + 0.5) / total), w2 = 0.5 - 0.5 * std::cos(2.0 * kPi * (i - k + 0.5) / total);
                    a += (double) seg[(size_t) i] * w1 * (double) seg[(size_t)(i - k)] * w2;
                }
                r[k] = a;
            }
            r[0] = r[0] * 1.0001 + 1e-12;
            for (int k = 1; k <= P; ++k) r[k] *= std::exp(-0.5 * std::pow(2.0 * kPi * 40.0 * k / fsd, 2.0));  // lag window
            double a[P + 1] = {1.0}, tmp[P + 1];
            double err = r[0];
            for (int m = 1; m <= P; ++m) {
                double acc2 = r[m];
                for (int j = 1; j < m; ++j) acc2 += a[j] * r[m - j];
                const double k = -acc2 / err;
                for (int j = 1; j < m; ++j) tmp[j] = a[j] + k * a[m - j];
                for (int j = 1; j < m; ++j) a[j] = tmp[j];
                a[m] = k; err *= (1.0 - k * k);
                if (err < 1e-20) break;
            }
            const int nres = total - P;
            for (int i = 0; i < nres; ++i) {
                double e0 = seg[(size_t)(i + P)];
                for (int j = 1; j <= P; ++j) e0 += a[j] * seg[(size_t)(i + P - j)];
                res[(size_t) i] = (float) e0;
            }
            const int WdR = nres - tauMaxD;
            dr[0] = 0.0f;
            for (int tau = 1; tau <= tauMaxD; ++tau) {
                double sd = 0;
                for (int i = 0; i < WdR; ++i) { double dl = (double) res[(size_t) i] - res[(size_t)(i + tau)]; sd += dl * dl; }
                dr[(size_t) tau] = (float) sd;
            }
            cmR[0] = 1.0f; double runR = 0;
            for (int tau = 1; tau <= tauMaxD; ++tau) {
                runR += dr[(size_t) tau];
                cmR[(size_t) tau] = runR > 1e-20 ? (float)(dr[(size_t) tau] * tau / runR) : 1.0f;
            }
        }

        // Candidate choice: local minima of the plain CMNDF; deepest first, then accept the SHORTEST lag that is
        // nearly as good (octave-safe). Voicing needs the CMNDF below threshold.
        // (values compared at their parabolic sub-sample minimum so a half-sample period is not penalised)
        auto cmI = [&](int t) {
            const double a = cm[(size_t)(t - 1)], b = cm[(size_t) t], c = cm[(size_t)(t + 1)], den = a - 2 * b + c;
            if (den <= 1e-12) return (float) b;
            return (float)(b - 0.125 * (a - c) * (a - c) / den);
        };
        int minima[96]; int nm = 0; float cmMin = 1e9f;
        for (int tau = std::max(tauLo, 2); tau <= tauHi && nm < 96; ++tau)
            if (cm[(size_t) tau] <= cm[(size_t)(tau - 1)] && cm[(size_t) tau] < cm[(size_t)(tau + 1)] && cm[(size_t) tau] < thr) {
                minima[nm++] = tau; cmMin = std::min(cmMin, cmI(tau));
            }
        if (nm == 0) { pushFrame((int64_t) tcFallback, 0.0, false); return; }
        int cand = -1;
        for (int i = 0; i < nm; ++i)
            if (cmI(minima[i]) <= 1.3f * cmMin + 0.04f) { cand = minima[i]; break; }
        if (cand < 0) { pushFrame((int64_t) tcFallback, 0.0, false); return; }

        // Sub-harmonic rescue: if the whitened residual repeats MUCH better at a multiple of the candidate lag,
        // the candidate was a formant-boosted harmonic (period/k), not the fundamental.
        {
            int bestT = cand; float bestR = cmR[(size_t) cand];
            for (int k = 2; k <= 6; ++k) {
                const int t = k * cand;
                if (t > tauHi) break;
                const int w = std::max(2, (int)(0.04 * t));
                int lo = std::max(tauLo, t - w), hi = std::min(tauHi, t + w), bt = -1; float bv = 1e9f;
                for (int u = lo; u <= hi; ++u)
                    if (cm[(size_t) u] < thr && cmR[(size_t) u] < bv) { bv = cmR[(size_t) u]; bt = u; }
                if (bt > 0 && bv < 0.5f && cm[(size_t) cand] > 0.12f && bv + 0.25f < bestR) { bestR = bv; bestT = bt; }
            }
            cand = bestT;
        }

        // octave-jump guard: a >0.4 octave jump in 2.5 ms is not singing; prefer a nearby valid dip
        if (unvoicedRun < 3) {
            if (std::fabs(std::log2(fsd / cand) - prevLog) > 0.4) {
                int alt = -1; double bestDist = 0.15;
                for (int i = 0; i < nm; ++i) {
                    const int tau = minima[i];
                    if (cm[(size_t) tau] <= cm[(size_t) cand] * 1.5f + 0.05f) {   // only when about as periodic as the main pick
                        double dist = std::fabs(std::log2(fsd / tau) - prevLog);
                        if (dist < bestDist) { bestDist = dist; alt = tau; }
                    }
                }
                if (alt > 0) cand = alt;
            }
        }

        // sub-sample refine (decimated)
        double off = 0.0;
        {
            double a = dd[(size_t)(cand - 1)], b = dd[(size_t) cand], c = dd[(size_t)(cand + 1)];
            double den = a - 2 * b + c;
            if (den > 1e-20) off = std::min(1.0, std::max(-1.0, 0.5 * (a - c) / den));
        }
        const double tau0 = (cand + off) * D;

        // full-rate refinement by normalised cross-correlation over the same time region
        const int64_t s0 = js * D;
        const int Wf = D * Wd;
        const int Lc = (int) std::lround(tau0);
        const int Lmin = std::max(2, Lc - D - 2), Lmax = Lc + D + 2;
        double e1 = 0;
        for (int i = 0; i < Wf; ++i) { double v = x[(size_t)((s0 + i) & mask)]; e1 += v * v; }
        std::array<double, 64> cc{};
        int nl = Lmax - Lmin + 1; if (nl > 64) nl = 64;
        int bi = 0; double bv = -2;
        for (int li = 0; li < nl; ++li) {
            const int L = Lmin + li;
            double num = 0, e2 = 0;
            for (int i = 0; i < Wf; ++i) {
                double a = x[(size_t)((s0 + i) & mask)], b = x[(size_t)((s0 + i + L) & mask)];
                num += a * b; e2 += b * b;
            }
            cc[(size_t) li] = num / std::sqrt(e1 * e2 + 1e-20);
            if (cc[(size_t) li] > bv) { bv = cc[(size_t) li]; bi = li; }
        }
        double Lref = tau0;
        if (bi > 0 && bi < nl - 1) {
            double a = cc[(size_t)(bi - 1)], b = cc[(size_t) bi], c = cc[(size_t)(bi + 1)];
            double den = a - 2 * b + c;
            double o = (den < -1e-20) ? 0.5 * (a - c) / den : 0.0;
            o = std::min(1.0, std::max(-1.0, o));
            double cand2 = Lmin + bi + o;
            if (std::fabs(cand2 - tau0) <= D + 1.5) Lref = cand2;   // trust only if consistent with the coarse estimate
        }
        const double f0 = fs / Lref;
        const double tc = (double) s0 + 0.5 * (Wf + Lref);
        pushFrame((int64_t) tc, std::log2(f0), true);
    }

    PInfo pitchAt(int64_t t) const
    {
        if (nFrames == 0) return { false, 0.0 };
        const int64_t kmin = std::max<int64_t>(0, nFrames - FR + 4);
        int64_t k = nFrames - 1;
        while (k > kmin && fTc[(size_t)(k & (FR - 1))] > t) --k;
        int64_t kn = k;
        if (k + 1 < nFrames && (fTc[(size_t)((k + 1) & (FR - 1))] - t) < (t - fTc[(size_t)(k & (FR - 1))])) kn = k + 1;

        int cnt = 0;
        for (int64_t j = kn - 1; j <= kn + 1; ++j)
            if (j >= kmin && j < nFrames && fVoiced[(size_t)(j & (FR - 1))]) ++cnt;
        if (cnt < 2) return { false, 0.0 };

        float v[5]; int nv = 0;
        for (int64_t j = kn - 2; j <= kn + 2; ++j)
            if (j >= kmin && j < nFrames && fVoiced[(size_t)(j & (FR - 1))]) v[nv++] = fLog[(size_t)(j & (FR - 1))];
        std::sort(v, v + nv);
        return { true, std::pow(2.0, (double) v[nv / 2]) };
    }

    // ------------------------------------------------------------------ synthesis
    float readX(double pos) const
    {
        const int64_t i = (int64_t) std::floor(pos);
        const float f = (float)(pos - (double) i);
        const float y0 = x[(size_t)((i - 1) & mask)], y1 = x[(size_t)(i & mask)];
        const float y2 = x[(size_t)((i + 1) & mask)], y3 = x[(size_t)((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }

    double refineCenter(double prevTa, double taC0, double T0a)
    {
        const int64_t c0 = (int64_t) std::floor(prevTa + 0.5);
        const double e0 = prevTa - (double) c0;
        const int64_t c1 = (int64_t) std::floor(taC0 + 0.5);
        const int R  = std::min(60, std::max(2, (int)(0.08 * T0a)));
        const int Lc = std::max(8, (int)(0.5 * T0a));
        double eref = 0;
        for (int i = -Lc; i <= Lc; ++i) { double v = x[(size_t)((c0 + i) & mask)]; eref += v * v; }
        std::array<double, 128> rho{};
        int bi = 0; double bv = -2;
        for (int d = -R; d <= R; ++d) {
            double num = 0, e2 = 0;
            for (int i = -Lc; i <= Lc; ++i) {
                double a = x[(size_t)((c0 + i) & mask)], b = x[(size_t)((c1 + d + i) & mask)];
                num += a * b; e2 += b * b;
            }
            const size_t idx = (size_t)(d + R);
            rho[idx] = num / std::sqrt(eref * e2 + 1e-20);
            if (rho[idx] > bv) { bv = rho[idx]; bi = d + R; }
        }
        double sub = 0.0;
        if (bi > 0 && bi < 2 * R) {
            double a = rho[(size_t)(bi - 1)], b = rho[(size_t) bi], c = rho[(size_t)(bi + 1)];
            double den = a - 2 * b + c;
            if (den < -1e-20) sub = std::min(0.5, std::max(-0.5, 0.5 * (a - c) / den));
        }
        return (double)(c1 + (bi - R)) + sub + e0;
    }

    void placeMark(int64_t te)
    {
        const double ts = tsNext;
        const PInfo p = pitchAt((int64_t) ts);
        if (!p.voiced) {
            tsNext = ts + hop; hasPrev = false; segActive = false; dVoiced = false;
            return;
        }
        if (nFrames == 0 || fTc[(size_t)((nFrames - 1) & (FR - 1))] < (int64_t) ts + hop) ++underruns;

        const double f0  = std::min((double) S.rangeHigh * 1.2, std::max((double) S.rangeLow * 0.8, p.f0));
        const double T0a = fs / f0;
        const double inC = 6900.0 + 1200.0 * std::log2(f0 / (double) S.refA);

        if (!segActive) { segActive = true; qHasLast = false; segAge = 0.0; curCorr = 1e9; lastSpacing = T0a; }

        const double tgt = q.pick(inC, (double) S.hystCents, qHasLast, qLast)
                           + 100.0 * (double) S.transpose + (double) S.fine;
        const double corrT = tgt - inC;

        double tau = (double) S.retuneMs * 0.001;
        tau *= 1.0 + 3.0 * (double) S.humanize * std::min(1.0, segAge / 0.35);
        if (curCorr > 1e8) curCorr = (tau < 0.005) ? corrT : 0.0;
        else {
            const double dt = lastSpacing / fs;
            const double a = (tau <= 1e-4) ? 1.0 : 1.0 - std::exp(-dt / tau);
            curCorr += a * (corrT - curCorr);
        }
        const double corrOut = curCorr * (double) S.amount;
        double r = std::pow(2.0, corrOut / 1200.0);
        r = std::min(2.0, std::max(0.5, r));
        double T0s = T0a / r;
        T0s = std::min(T0max, std::max(fs / 1500.0, T0s));

        // analysis grain centre: previous centre + whole number of periods (keeps phase aligned)
        double taC;
        if (!hasPrev) taC = ts;
        else {
            double m = std::floor((ts - prevTa) / T0a + 0.5);
            if (m < 0) m = 0;
            taC = prevTa + m * T0a;
            if (m > 0) taC = refineCenter(prevTa, taC, T0a);
            if (std::fabs(taC - ts) > 1.5 * T0a) { taC = ts; }
        }

        const double Hl = std::min((double) HlMax, std::max(16.0, std::max(T0a, T0s)));
        const int64_t tsInt = (int64_t) std::floor(ts);
        const double frac = ts - (double) tsInt;
        const int N = (int) std::ceil(Hl);
        for (int n = -N; n <= N; ++n) {
            const double u = (double) n - frac;
            if (std::fabs(u) >= Hl) continue;
            const int64_t t = tsInt + n;
            if (t < te) continue;                       // already emitted
            const float w = (float)(0.5 * (1.0 + std::cos(kPi * u / Hl)));
            const size_t ai = (size_t)(t & amask);
            acc[ai] += w * readX(taC + u);
            nrm[ai] += w;
        }

#ifdef DS_TRACE
        trace.push_back({ ts, f0, inC, tgt, T0s });
#endif
        prevTa = taC; hasPrev = true;
        tsNext = ts + T0s; lastSpacing = T0s;
        segAge += T0s / fs;
        dInC = inC; dTargetC = inC + corrOut; dVoiced = true;
    }

    // ------------------------------------------------------------------ state
    static constexpr int FR = 1024;
    double fs = 48000, fsd = 12000, T0max = 600;
    int D = 4, gd = 16, tauMaxD = 152, tauMinD = 10, Wd = 160, hop = 120, HlMax = 601, Ltot = 2000;
    std::vector<float> h, x, acc, nrm, xd, seg, dd, cm, fLog, res, dr, cmR;
    std::vector<int64_t> fTc; std::vector<uint8_t> fVoiced;
    int64_t mask = 0, amask = 0, maskd = 0;
    int64_t nIn = 0, nd = 0, nFrames = 0, lastTc = -1;
    bool prevFrameVoiced = false; double prevLog = 0; int unvoicedRun = 99;

    TuneSettings S;
    NoteQuantizer q;
    bool qHasLast = false; double qLast = 0;

    double tsNext = 0, prevTa = 0, curCorr = 0, segAge = 0, lastSpacing = 0;
    bool hasPrev = false, segActive = false;
    float vMix = 0, vTarget = 0, vSlew = 0.01f;
    int underruns = 0;
    double dInC = 0, dTargetC = 0; bool dVoiced = false;
};

} // namespace ds
