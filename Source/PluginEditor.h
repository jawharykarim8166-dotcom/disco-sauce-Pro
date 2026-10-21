#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui {
// volcano palette: black basalt + lava red/orange
const juce::Colour kBg    (0xff080303);
const juce::Colour kPanel (0xff170808);
const juce::Colour kGold  (0xffff6a1f);   // lava orange (main accent)
const juce::Colour kGold2 (0xffb3200f);   // deep magma red
const juce::Colour kText  (0xfff4e6de);
const juce::Colour kDim   (0xffa58378);
const juce::Colour kCyan  (0xffffb347);   // ember (readouts)
const juce::Colour kMagenta(0xffff2a1a);  // hot red (warnings / off-pitch)
}

class DiscoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DiscoLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// look used on the artwork screen: only a glowing pointer / thumb is drawn, the artwork provides the knob itself
class ArtLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

// rotary knob + caption, bound to one parameter
class KnobComp : public juce::Component
{
public:
    KnobComp(juce::AudioProcessorValueTreeState& s, int paramIndex, const juce::String& caption = {});
    void resized() override;
private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;   // keep last
};

class ToggleComp : public juce::Component
{
public:
    ToggleComp(juce::AudioProcessorValueTreeState& s, int paramIndex, const juce::String& caption = {});
    void resized() override { button.setBounds(getLocalBounds()); }
private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

class ComboComp : public juce::Component
{
public:
    ComboComp(juce::AudioProcessorValueTreeState& s, int paramIndex, const juce::String& caption = {});
    void resized() override;
private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// a titled section of the main page
class Panel : public juce::Component
{
public:
    Panel(juce::AudioProcessorValueTreeState& s, const juce::String& title) : state(s), titleText(title) {}
    void addToggle(int p, const juce::String& cap = {}) { strip.push_back(std::make_unique<ToggleComp>(state, p, cap)); addAndMakeVisible(*strip.back()); }
    void addCombo(int p, const juce::String& cap = {})  { strip.push_back(std::make_unique<ComboComp>(state, p, cap)); addAndMakeVisible(*strip.back()); stripWide.push_back(strip.size() - 1); }
    void addKnob(int p, const juce::String& cap = {})   { knobs.push_back(std::make_unique<KnobComp>(state, p, cap)); addAndMakeVisible(*knobs.back()); }
    juce::Label extra;                      // optional readout (e.g. gain reduction)
    void enableExtra() { extra.setJustificationType(juce::Justification::centredRight); extra.setColour(juce::Label::textColourId, ui::kCyan); extra.setFont(juce::Font(12.0f)); addAndMakeVisible(extra); }
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    juce::AudioProcessorValueTreeState& state;
    juce::String titleText;
    std::vector<std::unique_ptr<juce::Component>> strip;
    std::vector<size_t> stripWide;
    std::vector<std::unique_ptr<KnobComp>> knobs;
};

// "OPEN MAP": every single parameter, generated from the parameter table
class MapContent : public juce::Component
{
public:
    explicit MapContent(juce::AudioProcessorValueTreeState& s);
    void resized() override;
    int requiredHeight() const { return totalH; }
private:
    struct Row { int header = 0; juce::String text; std::unique_ptr<juce::Label> label; std::unique_ptr<juce::Component> control; std::unique_ptr<juce::Slider> slider; std::unique_ptr<juce::ToggleButton> toggle; std::unique_ptr<juce::ComboBox> combo;
                 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa; std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba; std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca; };
    std::vector<std::unique_ptr<Row>> rows;
    int totalH = 0;
};

class DiscoSauceEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DiscoSauceEditor(DiscoSauceProcessor&);
    ~DiscoSauceEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum View { ViewArt = 0, ViewPanels = 1, ViewMap = 2 };
    struct ArtKnob { juce::Slider s; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a; int param = 0; float cx = 0, cy = 0, size = 0; };

    void timerCallback() override;
    void setView(int v);
    void presetChanged();
    void layoutArt();
    void layoutPanels();

    DiscoSauceProcessor& proc;
    DiscoLookAndFeel laf;
    ArtLookAndFeel artLaf;
    int view = ViewArt;

    // artwork screen
    std::vector<std::unique_ptr<ArtKnob>> artKnobs;
    juce::Slider dryWet;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAtt;
    juce::ComboBox keyBox, scaleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt;
    juce::ToggleButton tuneToggle { "TUNE ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tuneAtt;

    // detailed panels screen
    juce::Component mainPage;
    std::vector<std::unique_ptr<Panel>> panels;
    Panel* compPanel = nullptr;

    // open map screen
    juce::Viewport mapViewport;
    std::unique_ptr<MapContent> mapContent;

    // shared
    juce::ComboBox presetBox;
    juce::Slider presetAmount;
    juce::Label presetAmountLabel;
    juce::TextButton panelsButton { "PRO PANELS" }, mapButton { "OPEN MAP" }, saveButton { "SAVE" }, loadButton { "LOAD" };
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image artwork;
    juce::Rectangle<float> tunerBox;
    juce::String tunerIn, tunerOut;
    bool tunerVoiced = false;
    float tunerCents = 0.0f;
    float kScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiscoSauceEditor)
};
