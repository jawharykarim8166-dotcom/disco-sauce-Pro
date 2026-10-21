#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Engine.h"
#include "Params.h"

class DiscoSauceProcessor : public juce::AudioProcessor
{
public:
    DiscoSauceProcessor();
    ~DiscoSauceProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Disco Sauce 2 Pro"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // presets (message thread)
    void applyPreset(int index, float amount01);
    bool savePresetFile(const juce::File& f);
    bool loadPresetFile(const juce::File& f);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int>   presetIndex { 0 };
    std::atomic<float> presetAmount { 1.0f };

    // meters for the editor
    std::atomic<float> meterGr { 0.0f }, meterInCents { 0.0f }, meterTgtCents { 0.0f };
    std::atomic<bool>  meterVoiced { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::unique_ptr<juce::XmlElement> stateToXml();
    void xmlToState(const juce::XmlElement& xml);

    ds::Engine engine;
    std::atomic<float>* raw[dsp2::kNumParams] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiscoSauceProcessor)
};
