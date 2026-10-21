#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace dsp2;

juce::AudioProcessorValueTreeState::ParameterLayout DiscoSauceProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;
    for (int i = 0; i < kNumParams; ++i)
    {
        const ParamDef& d = paramDef(i);
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == KBool)
        {
            ps.push_back(std::make_unique<juce::AudioParameterBool>(pid, d.name, d.df > 0.5f));
        }
        else if (d.kind == KChoice)
        {
            juce::StringArray names;
            for (auto& s : choiceNames(i)) names.add(juce::String(s));
            ps.push_back(std::make_unique<juce::AudioParameterChoice>(pid, d.name, names, (int) d.df));
        }
        else
        {
            juce::NormalisableRange<float> range(d.mn, d.mx, d.step);
            if (d.mid > d.mn && d.mid < d.mx) range.setSkewForCentre(d.mid);
            ps.push_back(std::make_unique<juce::AudioParameterFloat>(pid, d.name, range, d.df, juce::String(d.unit)));
        }
    }
    return { ps.begin(), ps.end() };
}

DiscoSauceProcessor::DiscoSauceProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < kNumParams; ++i)
        raw[i] = apvts.getRawParameterValue(paramDef(i).id);
}

bool DiscoSauceProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DiscoSauceProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare(sampleRate, samplesPerBlock);
    setLatencySamples(engine.latency());
}

void DiscoSauceProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n <= 0) return;

    float p[kNumParams];
    for (int i = 0; i < kNumParams; ++i) p[i] = raw[i]->load();

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    engine.process(buffer.getWritePointer(0), buffer.getWritePointer(1), n, p, bpm);

    meterGr.store(engine.compGrDb());
    meterInCents.store(engine.inputCents());
    meterTgtCents.store(engine.targetCents());
    meterVoiced.store(engine.voiced());
}

juce::AudioProcessorEditor* DiscoSauceProcessor::createEditor() { return new DiscoSauceEditor(*this); }

void DiscoSauceProcessor::applyPreset(int index, float amount01)
{
    presetIndex.store(index);
    presetAmount.store(amount01);
    for (int i = 0; i < kNumParams; ++i)
    {
        float v;
        if (!presetValue(index, i, amount01, v)) continue;
        if (auto* prm = apvts.getParameter(paramDef(i).id))
            prm->setValueNotifyingHost(prm->convertTo0to1(v));
    }
}

std::unique_ptr<juce::XmlElement> DiscoSauceProcessor::stateToXml()
{
    auto state = apvts.copyState();
    state.setProperty("presetIndex", presetIndex.load(), nullptr);
    state.setProperty("presetAmount", (double) presetAmount.load(), nullptr);
    return state.createXml();
}

void DiscoSauceProcessor::xmlToState(const juce::XmlElement& xml)
{
    if (!xml.hasTagName(apvts.state.getType())) return;
    auto vt = juce::ValueTree::fromXml(xml);
    presetIndex.store((int) vt.getProperty("presetIndex", 0));
    presetAmount.store((float) (double) vt.getProperty("presetAmount", 1.0));
    apvts.replaceState(vt);
}

void DiscoSauceProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = stateToXml()) copyXmlToBinary(*xml, destData);
}

void DiscoSauceProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) xmlToState(*xml);
}

bool DiscoSauceProcessor::savePresetFile(const juce::File& f)
{
    if (auto xml = stateToXml()) return xml->writeTo(f);
    return false;
}

bool DiscoSauceProcessor::loadPresetFile(const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse(f)) { xmlToState(*xml); return true; }
    return false;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DiscoSauceProcessor(); }
