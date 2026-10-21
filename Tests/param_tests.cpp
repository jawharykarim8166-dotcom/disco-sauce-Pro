#include "../Source/Params.h"
#include <cstdio>
#include <set>
#include <cstring>
using namespace dsp2;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
int main()
{
    std::printf("[params] %d parameters, %d presets\n", (int) kNumParams, (int) presets().size());
    std::set<std::string> ids;
    for (int i = 0; i < kNumParams; ++i) {
        const ParamDef& d = paramDef(i);
        CHECK(ids.insert(d.id).second, "duplicate id %s", d.id);
        CHECK(std::strchr(d.id, ' ') == nullptr && std::strlen(d.id) < 20, "bad id %s", d.id);
        CHECK(d.mn < d.mx, "%s range", d.id);
        CHECK(d.df >= d.mn && d.df <= d.mx, "%s default %g outside [%g,%g]", d.id, d.df, d.mn, d.mx);
        CHECK(d.nv >= d.mn && d.nv <= d.mx, "%s neutral %g outside range", d.id, d.nv);
        if (d.mid != 0) CHECK(d.mid > d.mn && d.mid < d.mx, "%s mid %g outside range", d.id, d.mid);
        if (d.kind == KChoice) { auto c = choiceNames(i); CHECK((int) c.size() == (int) d.mx + 1, "%s choice count %d vs %g", d.id, (int) c.size(), d.mx + 1); }
        if (d.kind == KBool) CHECK(d.mn == 0 && d.mx == 1, "%s bool range", d.id);
    }
    CHECK(ds::kNumScales == (int) choiceNames(P_Scale).size(), "scale list");
    for (size_t pi = 0; pi < presets().size(); ++pi) {
        for (auto& kv : presets()[pi].v) {
            const ParamDef& d = paramDef(kv.first);
            CHECK(kv.second >= d.mn && kv.second <= d.mx, "preset %zu %s=%g outside [%g,%g]", pi, d.id, kv.second, d.mn, d.mx);
            CHECK(!d.keep, "preset %zu touches protected param %s", pi, d.id);
        }
        for (float amt : {0.0f, 0.5f, 1.0f}) for (int p = 0; p < kNumParams; ++p) { float v; if (presetValue((int) pi, p, amt, v)) CHECK(v >= paramDef(p).mn && v <= paramDef(p).mx, "preset value range"); }
    }
    CHECK(std::strstr(presets()[kProMaxPreset].name, "DISCO PRO MAX") != nullptr, "PRO MAX preset present");
    // preset amount 0 must leave every effect inert
    float v; presetValue(kProMaxPreset, P_Dbl, 0, v); CHECK(v == 0, "doubler inert at amount 0");
    presetValue(kProMaxPreset, P_RvMix, 0, v); CHECK(v == 0, "reverb inert at amount 0");
    presetValue(kProMaxPreset, P_TuneAmt, 0, v); CHECK(v == 0, "tune inert at amount 0");
    presetValue(kProMaxPreset, P_CpRatio, 0, v); CHECK(v == 1, "compressor inert at amount 0");
    presetValue(kProMaxPreset, P_Dbl, 1, v); CHECK(v == 38, "pro max doubler at amount 1");
    std::printf("%s (%d failures)\n", failures ? "PARAM TESTS FAILED" : "PARAM TESTS PASSED", failures);
    return failures ? 1 : 0;
}
