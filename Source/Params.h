// Disco Sauce 2 Pro - single source of truth for every parameter (pure C++, no JUCE).
// The plugin parameters, the "Open Map" editor and the presets are all generated from this one table.
#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include "DSP/Scales.h"

namespace dsp2 {

enum Kind { KFloat = 0, KBool = 1, KChoice = 2 };

//  X(Enum,        id,          "Name",               kind, min,   max,    def,    step, mid,   neutral, "unit", "GROUP", keep)
#define DS_NOTE_ON(n, nm)   X(P_NoteOn##n, "nOn" #n, "Note " nm, KBool, 0, 1, 1, 1, 0, 1, "", "NOTE MAP", 1)
#define DS_NOTE_CT(n, nm)   X(P_NoteCt##n, "nCt" #n, "Tune " nm, KFloat, -50, 50, 0, 1, 0, 0, "c", "NOTE MAP", 1)
#define DS_PARAM_LIST(X) \
 X(P_InGain,  "inGain",  "Input Gain",   KFloat, -24, 24, 0, 0.1f, 0, 0, "dB", "GLOBAL", 1) \
 X(P_OutGain, "outGain", "Output Gain",  KFloat, -24, 24, 0, 0.1f, 0, 0, "dB", "GLOBAL", 1) \
 X(P_Mix,     "mix",     "Dry/Wet",      KFloat, 0, 100, 100, 1, 0, 100, "%", "GLOBAL", 1) \
 X(P_LimOn,   "limOn",   "Limiter",      KBool, 0, 1, 1, 1, 0, 1, "", "GLOBAL", 1) \
 X(P_LimCeil, "limCeil", "Ceiling",      KFloat, -12, 0, -0.3f, 0.1f, 0, -0.3f, "dB", "GLOBAL", 1) \
 X(P_TuneOn,  "tuneOn",  "Tune On",      KBool, 0, 1, 1, 1, 0, 1, "", "AUTO-TUNE", 0) \
 X(P_Key,     "key",     "Key",          KChoice, 0, 11, 0, 1, 0, 0, "", "AUTO-TUNE", 1) \
 X(P_Scale,   "scale",   "Scale",        KChoice, 0, 25, 0, 1, 0, 0, "", "AUTO-TUNE", 1) \
 X(P_TuneAmt, "tuneAmt", "Correction",   KFloat, 0, 100, 100, 1, 0, 0, "%", "AUTO-TUNE", 0) \
 X(P_Retune,  "retune",  "Retune Speed", KFloat, 0, 500, 15, 0.1f, 60, 15, "ms", "AUTO-TUNE", 0) \
 X(P_Humanize,"humanize","Humanize",     KFloat, 0, 100, 0, 1, 0, 0, "%", "AUTO-TUNE", 0) \
 X(P_Hyst,    "hyst",    "Note Stickiness", KFloat, 0, 100, 10, 1, 0, 10, "c", "AUTO-TUNE", 0) \
 X(P_Transpose,"transpose","Transpose",  KFloat, -12, 12, 0, 1, 0, 0, "st", "AUTO-TUNE", 0) \
 X(P_Fine,    "fine",    "Fine Tune",    KFloat, -100, 100, 0, 1, 0, 0, "c", "AUTO-TUNE", 0) \
 X(P_RefA,    "refA",    "A4 Reference", KFloat, 400, 480, 440, 0.1f, 0, 440, "Hz", "AUTO-TUNE", 1) \
 X(P_RangeLow,"rangeLow","Range Low",    KFloat, 80, 400, 80, 1, 150, 80, "Hz", "AUTO-TUNE", 1) \
 X(P_RangeHigh,"rangeHigh","Range High", KFloat, 300, 1200, 1000, 1, 700, 1000, "Hz", "AUTO-TUNE", 1) \
 X(P_Sens,    "sens",    "Sensitivity",  KFloat, 0, 100, 75, 1, 0, 75, "%", "AUTO-TUNE", 1) \
 X(P_GateDb,  "gateDb",  "Pitch Gate",   KFloat, -90, -30, -80, 1, 0, -80, "dB", "AUTO-TUNE", 1) \
 DS_NOTE_ON(0,"C") DS_NOTE_ON(1,"C#") DS_NOTE_ON(2,"D") DS_NOTE_ON(3,"D#") DS_NOTE_ON(4,"E") DS_NOTE_ON(5,"F") \
 DS_NOTE_ON(6,"F#") DS_NOTE_ON(7,"G") DS_NOTE_ON(8,"G#") DS_NOTE_ON(9,"A") DS_NOTE_ON(10,"A#") DS_NOTE_ON(11,"B") \
 DS_NOTE_CT(0,"C") DS_NOTE_CT(1,"C#") DS_NOTE_CT(2,"D") DS_NOTE_CT(3,"D#") DS_NOTE_CT(4,"E") DS_NOTE_CT(5,"F") \
 DS_NOTE_CT(6,"F#") DS_NOTE_CT(7,"G") DS_NOTE_CT(8,"G#") DS_NOTE_CT(9,"A") DS_NOTE_CT(10,"A#") DS_NOTE_CT(11,"B") \
 X(P_EqOn,    "eqOn",    "EQ On",        KBool, 0, 1, 1, 1, 0, 1, "", "EQUALIZER", 0) \
 X(P_HpFreq,  "hpFreq",  "Low Cut",      KFloat, 20, 400, 80, 1, 80, 20, "Hz", "EQUALIZER", 0) \
 X(P_LsFreq,  "lsFreq",  "Low Shelf Freq", KFloat, 40, 600, 150, 1, 150, 150, "Hz", "EQUALIZER", 0) \
 X(P_LsGain,  "lsGain",  "Low Shelf Gain", KFloat, -15, 15, 0, 0.1f, 0, 0, "dB", "EQUALIZER", 0) \
 X(P_B1F,     "b1f",     "Bell 1 Freq",  KFloat, 100, 1000, 300, 1, 300, 300, "Hz", "EQUALIZER", 0) \
 X(P_B1G,     "b1g",     "Bell 1 Gain",  KFloat, -15, 15, 0, 0.1f, 0, 0, "dB", "EQUALIZER", 0) \
 X(P_B1Q,     "b1q",     "Bell 1 Q",     KFloat, 0.3f, 8, 1.2f, 0.01f, 1.5f, 1.2f, "", "EQUALIZER", 0) \
 X(P_B2F,     "b2f",     "Bell 2 Freq",  KFloat, 400, 5000, 1500, 1, 1500, 1500, "Hz", "EQUALIZER", 0) \
 X(P_B2G,     "b2g",     "Bell 2 Gain",  KFloat, -15, 15, 0, 0.1f, 0, 0, "dB", "EQUALIZER", 0) \
 X(P_B2Q,     "b2q",     "Bell 2 Q",     KFloat, 0.3f, 8, 1.0f, 0.01f, 1.5f, 1.0f, "", "EQUALIZER", 0) \
 X(P_B3F,     "b3f",     "Bell 3 Freq",  KFloat, 1500, 12000, 4000, 1, 4000, 4000, "Hz", "EQUALIZER", 0) \
 X(P_B3G,     "b3g",     "Bell 3 Gain",  KFloat, -15, 15, 0, 0.1f, 0, 0, "dB", "EQUALIZER", 0) \
 X(P_B3Q,     "b3q",     "Bell 3 Q",     KFloat, 0.3f, 8, 1.0f, 0.01f, 1.5f, 1.0f, "", "EQUALIZER", 0) \
 X(P_HsFreq,  "hsFreq",  "High Shelf Freq", KFloat, 2000, 16000, 10000, 1, 8000, 10000, "Hz", "EQUALIZER", 0) \
 X(P_HsGain,  "hsGain",  "High Shelf Gain", KFloat, -15, 15, 0, 0.1f, 0, 0, "dB", "EQUALIZER", 0) \
 X(P_LpFreq,  "lpFreq",  "High Cut",     KFloat, 3000, 20000, 20000, 1, 12000, 20000, "Hz", "EQUALIZER", 0) \
 X(P_DsOn,    "dsOn",    "De-Ess On",    KBool, 0, 1, 1, 1, 0, 1, "", "DE-ESSER", 0) \
 X(P_DsFreq,  "dsFreq",  "De-Ess Freq",  KFloat, 3000, 12000, 7000, 1, 6500, 7000, "Hz", "DE-ESSER", 0) \
 X(P_DsThr,   "dsThr",   "De-Ess Threshold", KFloat, -50, 0, -30, 0.1f, 0, 0, "dB", "DE-ESSER", 0) \
 X(P_DsAmt,   "dsAmt",   "De-Ess Amount", KFloat, 0, 100, 50, 1, 0, 0, "%", "DE-ESSER", 0) \
 X(P_CpOn,    "cpOn",    "Comp On",      KBool, 0, 1, 1, 1, 0, 1, "", "COMPRESSOR", 0) \
 X(P_CpThr,   "cpThr",   "Threshold",    KFloat, -60, 0, -18, 0.1f, 0, 0, "dB", "COMPRESSOR", 0) \
 X(P_CpRatio, "cpRatio", "Ratio",        KFloat, 1, 20, 3, 0.1f, 4, 1, ":1", "COMPRESSOR", 0) \
 X(P_CpAtk,   "cpAtk",   "Attack",       KFloat, 0.1f, 100, 8, 0.1f, 10, 8, "ms", "COMPRESSOR", 0) \
 X(P_CpRel,   "cpRel",   "Release",      KFloat, 10, 1000, 120, 1, 150, 120, "ms", "COMPRESSOR", 0) \
 X(P_CpKnee,  "cpKnee",  "Knee",         KFloat, 0, 24, 6, 0.1f, 0, 6, "dB", "COMPRESSOR", 0) \
 X(P_CpMake,  "cpMake",  "Makeup",       KFloat, 0, 24, 0, 0.1f, 0, 0, "dB", "COMPRESSOR", 0) \
 X(P_CpMix,   "cpMix",   "Comp Mix",     KFloat, 0, 100, 100, 1, 0, 100, "%", "COMPRESSOR", 0) \
 X(P_SatOn,   "satOn",   "Sat On",       KBool, 0, 1, 1, 1, 0, 1, "", "SATURATION", 0) \
 X(P_SatDrive,"satDrive","Drive",        KFloat, 0, 100, 0, 1, 0, 0, "%", "SATURATION", 0) \
 X(P_SatMix,  "satMix",  "Sat Mix",      KFloat, 0, 100, 100, 1, 0, 100, "%", "SATURATION", 0) \
 X(P_Dbl,     "dbl",     "Doubler",      KFloat, 0, 100, 0, 1, 0, 0, "%", "DOUBLER", 0) \
 X(P_DblVoices,"dblVoices","Doubler Voices", KFloat, 1, 4, 2, 1, 0, 2, "", "DOUBLER", 0) \
 X(P_DblDelay,"dblDelay","Doubler Delay", KFloat, 5, 60, 22, 0.1f, 0, 22, "ms", "DOUBLER", 0) \
 X(P_DblDetune,"dblDetune","Doubler Detune", KFloat, 0, 30, 10, 0.1f, 0, 10, "c", "DOUBLER", 0) \
 X(P_DblWidth,"dblWidth","Doubler Width", KFloat, 0, 100, 100, 1, 0, 100, "%", "DOUBLER", 0) \
 X(P_DblTone, "dblTone", "Doubler Tone", KFloat, 2000, 20000, 9000, 1, 7000, 9000, "Hz", "DOUBLER", 0) \
 X(P_DlOn,    "dlOn",    "Delay On",     KBool, 0, 1, 1, 1, 0, 1, "", "DELAY", 0) \
 X(P_DlMix,   "dlMix",   "Delay Mix",    KFloat, 0, 100, 0, 1, 0, 0, "%", "DELAY", 0) \
 X(P_DlTime,  "dlTime",  "Delay Time",   KFloat, 20, 1500, 350, 1, 350, 350, "ms", "DELAY", 0) \
 X(P_DlSync,  "dlSync",  "Tempo Sync",   KBool, 0, 1, 0, 1, 0, 0, "", "DELAY", 0) \
 X(P_DlDiv,   "dlDiv",   "Sync Division", KChoice, 0, 7, 3, 1, 0, 3, "", "DELAY", 0) \
 X(P_DlFb,    "dlFb",    "Feedback",     KFloat, 0, 95, 30, 1, 0, 30, "%", "DELAY", 0) \
 X(P_DlTone,  "dlTone",  "Delay Tone",   KFloat, 500, 16000, 5000, 1, 4000, 5000, "Hz", "DELAY", 0) \
 X(P_DlHp,    "dlHp",    "Delay Low Cut", KFloat, 20, 1000, 150, 1, 200, 150, "Hz", "DELAY", 0) \
 X(P_DlPing,  "dlPing",  "Ping-Pong",    KBool, 0, 1, 1, 1, 0, 1, "", "DELAY", 0) \
 X(P_RvOn,    "rvOn",    "Reverb On",    KBool, 0, 1, 1, 1, 0, 1, "", "REVERB", 0) \
 X(P_RvMix,   "rvMix",   "Reverb Mix",   KFloat, 0, 100, 0, 1, 0, 0, "%", "REVERB", 0) \
 X(P_RvSize,  "rvSize",  "Size",         KFloat, 0, 100, 55, 1, 0, 55, "%", "REVERB", 0) \
 X(P_RvDamp,  "rvDamp",  "Damping",      KFloat, 0, 100, 50, 1, 0, 50, "%", "REVERB", 0) \
 X(P_RvPre,   "rvPre",   "Pre-Delay",    KFloat, 0, 200, 20, 1, 0, 20, "ms", "REVERB", 0) \
 X(P_RvWidth, "rvWidth", "Reverb Width", KFloat, 0, 100, 100, 1, 0, 100, "%", "REVERB", 0) \
 X(P_RvLow,   "rvLow",   "Reverb Low Cut", KFloat, 20, 1000, 200, 1, 200, 200, "Hz", "REVERB", 0) \
 X(P_RvHigh,  "rvHigh",  "Reverb High Cut", KFloat, 1000, 20000, 10000, 1, 8000, 10000, "Hz", "REVERB", 0)

enum Param {
#define X(E, id, nm, kind, mn, mx, df, st, mid, nv, unit, grp, keep) E,
    DS_PARAM_LIST(X)
#undef X
    kNumParams
};

struct ParamDef { const char* id; const char* name; int kind; float mn, mx, df, step, mid, nv; const char* unit; const char* group; bool keep; };

inline const ParamDef& paramDef(int i)
{
    static const ParamDef t[] = {
#define X(E, id, nm, kind, mn, mx, df, st, mid, nv, unit, grp, keep) { id, nm, kind, (float)(mn), (float)(mx), (float)(df), (float)(st), (float)(mid), (float)(nv), unit, grp, keep != 0 },
        DS_PARAM_LIST(X)
#undef X
    };
    return t[i];
}

inline std::vector<std::string> choiceNames(int p)
{
    std::vector<std::string> v;
    if (p == P_Key) { for (const char* n : {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}) v.push_back(n); }
    else if (p == P_Scale) { for (int i = 0; i < ds::kNumScales; ++i) v.push_back(ds::scaleTable()[i].name); }
    else if (p == P_DlDiv) { for (const char* n : {"1/2","1/4","1/4 dotted","1/8","1/8 dotted","1/8 triplet","1/16","1/16 triplet"}) v.push_back(n); }
    return v;
}
// delay division -> length in quarter notes
inline double divisionQuarterNotes(int d) { static const double q[8] = {2.0, 1.0, 1.5, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.0 / 6.0}; return q[d < 0 ? 0 : (d > 7 ? 7 : d)]; }

// ------------------------------------------------------------------ presets
struct PresetDef { const char* name; std::vector<std::pair<int, float>> v; };

inline const std::vector<PresetDef>& presets()
{
    static const std::vector<PresetDef> t = {
        { "Init (Clean)", {} },
        { "1 - Crisp Studio", {
            {P_TuneAmt,100},{P_Retune,28},{P_Humanize,25},{P_Hyst,15},
            {P_HpFreq,90},{P_B1F,300},{P_B1G,-2.5f},{P_B3F,4000},{P_B3G,2},{P_HsGain,3},
            {P_DsThr,-28},{P_DsAmt,55},{P_CpThr,-20},{P_CpRatio,3},{P_CpAtk,12},{P_CpRel,140},{P_CpKnee,8},{P_CpMake,2},
            {P_SatDrive,10},{P_Dbl,12},{P_DlMix,8},{P_DlSync,1},{P_DlDiv,3},{P_DlFb,25},{P_RvMix,10},{P_RvSize,45} } },
        { "2 - T-Pain", {
            {P_TuneAmt,100},{P_Retune,0},{P_Humanize,0},{P_Hyst,8},
            {P_HpFreq,100},{P_B2G,1.5f},{P_HsGain,2},
            {P_DsThr,-30},{P_DsAmt,50},{P_CpThr,-24},{P_CpRatio,4},{P_CpAtk,6},{P_CpRel,100},{P_CpMake,3},
            {P_SatDrive,18},{P_Dbl,20},{P_DlMix,14},{P_DlFb,35},{P_DlTime,320},{P_RvMix,14},{P_RvSize,50} } },
        { "3 - Dark Trap", {
            {P_TuneAmt,100},{P_Retune,6},{P_Humanize,5},{P_Hyst,12},
            {P_HpFreq,70},{P_LsFreq,120},{P_LsGain,3},{P_B1G,-3},{P_B3F,3500},{P_B3G,-1.5f},{P_HsFreq,9000},{P_HsGain,-2.5f},{P_LpFreq,14000},
            {P_DsFreq,6500},{P_DsThr,-26},{P_DsAmt,60},{P_CpThr,-22},{P_CpRatio,4.5f},{P_CpAtk,5},{P_CpRel,90},{P_CpMake,4},
            {P_SatDrive,30},{P_Dbl,25},{P_DblVoices,3},{P_DblDelay,28},{P_DlMix,20},{P_DlSync,1},{P_DlDiv,2},{P_DlFb,40},{P_DlTone,3500},
            {P_RvMix,22},{P_RvSize,70},{P_RvDamp,70},{P_RvPre,35} } },
        { "4 - Disco Pro", {
            {P_TuneAmt,100},{P_Retune,12},{P_Humanize,15},{P_Hyst,12},
            {P_HpFreq,85},{P_B1G,-2},{P_B2G,1},{P_B3G,2.5f},{P_HsFreq,11000},{P_HsGain,4},
            {P_DsThr,-30},{P_DsAmt,55},{P_CpThr,-22},{P_CpRatio,3.5f},{P_CpAtk,9},{P_CpRel,120},{P_CpMake,3},
            {P_SatDrive,14},{P_Dbl,30},{P_DlMix,12},{P_DlSync,1},{P_DlDiv,3},{P_RvMix,16},{P_RvSize,55},{P_RvPre,25} } },
        { "5 - DISCO PRO MAX", {
            {P_TuneAmt,100},{P_Retune,4},{P_Humanize,10},{P_Hyst,12},
            {P_HpFreq,95},{P_LsFreq,120},{P_LsGain,2.5f},{P_B1F,280},{P_B1G,-3},{P_B1Q,1.4f},{P_B2F,2000},{P_B2G,1.5f},{P_B2Q,0.9f},
            {P_B3F,4500},{P_B3G,2.5f},{P_B3Q,0.8f},{P_HsFreq,12000},{P_HsGain,4.5f},
            {P_DsFreq,7200},{P_DsThr,-32},{P_DsAmt,65},
            {P_CpThr,-24},{P_CpRatio,4},{P_CpAtk,8},{P_CpRel,110},{P_CpKnee,10},{P_CpMake,4},{P_CpMix,85},
            {P_SatDrive,22},{P_SatMix,70},
            {P_Dbl,38},{P_DblVoices,4},{P_DblDelay,24},{P_DblDetune,12},{P_DblWidth,100},{P_DblTone,10000},
            {P_DlMix,18},{P_DlSync,1},{P_DlDiv,4},{P_DlFb,32},{P_DlTone,4800},{P_DlHp,180},{P_DlPing,1},
            {P_RvMix,20},{P_RvSize,62},{P_RvDamp,55},{P_RvPre,28},{P_RvWidth,100},{P_RvLow,220},{P_RvHigh,11000} } },
    };
    return t;
}
constexpr int kProMaxPreset = 5;

// value for parameter p when preset `idx` is applied at `amount` (0..1). Returns false for params presets never touch.
inline bool presetValue(int idx, int p, float amount, float& out)
{
    const ParamDef& d = paramDef(p);
    if (d.keep) return false;
    float target = d.df;
    if (idx >= 0 && idx < (int) presets().size())
        for (auto& kv : presets()[(size_t) idx].v) if (kv.first == p) target = kv.second;
    if (d.kind == KFloat) out = d.nv + (target - d.nv) * std::min(1.0f, std::max(0.0f, amount));
    else                  out = amount >= 0.5f ? target : d.nv;
    out = std::min(d.mx, std::max(d.mn, out));
    return true;
}

} // namespace dsp2
