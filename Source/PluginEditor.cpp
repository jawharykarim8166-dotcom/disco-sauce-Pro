#include "PluginEditor.h"
#include <algorithm>
#if DS_HAS_ARTWORK
 #include <BinaryData.h>
#endif

using namespace dsp2;

// ===================================================================== look and feel
DiscoLookAndFeel::DiscoLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, ui::kText);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, ui::kText);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a0d0d));
    setColour(juce::ComboBox::textColourId, ui::kText);
    setColour(juce::ComboBox::outlineColourId, ui::kGold2);
    setColour(juce::ComboBox::arrowColourId, ui::kGold);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff170808));
    setColour(juce::PopupMenu::textColourId, ui::kText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ui::kGold2);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a0d0d));
    setColour(juce::TextButton::textColourOffId, ui::kGold);
    setColour(juce::Slider::trackColourId, ui::kGold);
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff3a1814));
    setColour(juce::Slider::thumbColourId, ui::kText);
}

void DiscoLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(4.0f);
    const float d = std::min(b.getWidth(), b.getHeight()), r = d * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    g.setColour(juce::Colour(0xff2a0d0d));
    g.fillEllipse(c.x - r, c.y - r, d, d);
    juce::Path track; track.addCentredArc(c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, a1, true);
    g.setColour(juce::Colour(0xff4a1c18)); g.strokePath(track, juce::PathStrokeType(3.0f));
    juce::Path val; val.addCentredArc(c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, ang, true);
    g.setGradientFill(juce::ColourGradient(ui::kGold, c.x, c.y - r, ui::kGold2, c.x, c.y + r, false));
    g.strokePath(val, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path tick; tick.addRoundedRectangle(-1.5f, -r + 6.0f, 3.0f, r * 0.42f, 1.5f);
    g.setColour(ui::kText); g.fillPath(tick, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    juce::ignoreUnused(s);
}

void DiscoLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState();
    g.setColour(on ? ui::kGold : juce::Colour(0xff2a1210));
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(on ? ui::kGold2 : juce::Colour(0xff7a3a2c));
    g.drawRoundedRectangle(r, 5.0f, 1.2f);
    g.setColour(on ? juce::Colour(0xff150500) : ui::kDim);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void ArtLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& sl)
{
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float r = std::min(b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    if ((bool) sl.getProperties()["cap"])           // knobs without artwork get a lit 3D cap
    {
        const float cr = r * 0.62f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(c.x - cr + cr * 0.08f, c.y - cr + cr * 0.16f, cr * 2.0f, cr * 2.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffffa064), c.x - cr * 0.35f, c.y - cr * 0.5f, juce::Colour(0xff230505), c.x + cr * 0.7f, c.y + cr * 0.8f, true));
        g.fillEllipse(c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff7a1608), c.x - cr * 0.2f, c.y - cr * 0.2f, juce::Colour(0xff1a0303), c.x + cr * 0.6f, c.y + cr * 0.7f, true));
        g.fillEllipse(c.x - cr * 0.78f, c.y - cr * 0.78f, cr * 1.56f, cr * 1.56f);
        g.setColour(juce::Colour(0xff0a0101)); g.drawEllipse(c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.45f));
        g.fillEllipse(c.x - cr * 0.52f, c.y - cr * 0.66f, cr * 0.7f, cr * 0.34f);       // specular
    }

    juce::Path arc; arc.addCentredArc(c.x, c.y, r * 0.92f, r * 0.92f, 0.0f, a0, ang, true);
    g.setColour(ui::kGold.withAlpha(0.22f));
    g.strokePath(arc, juce::PathStrokeType(13.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));   // glow
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.strokePath(arc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ui::kCyan);
    g.strokePath(arc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto t = juce::AffineTransform::rotation(ang).translated(c.x, c.y);
    juce::Path tick; tick.addRoundedRectangle(-3.2f, -r * 0.80f, 6.4f, r * 0.30f, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.75f)); g.fillPath(tick, t);
    juce::Path tick2; tick2.addRoundedRectangle(-1.9f, -r * 0.79f, 3.8f, r * 0.28f, 1.8f);
    g.setColour(ui::kGold); g.fillPath(tick2, t);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    juce::Path hi; hi.addRoundedRectangle(-0.7f, -r * 0.78f, 1.4f, r * 0.16f, 0.7f); g.fillPath(hi, t);
}

void ArtLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int, float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const auto th = juce::Rectangle<float>((float) x + (float) w * 0.02f, sliderPos - 15.0f, (float) w * 0.96f, 30.0f);
    g.setColour(ui::kGold2);  g.fillRoundedRectangle(th, 7.0f);
    g.setColour(ui::kGold);   g.fillRoundedRectangle(th.reduced(3.0f, 5.0f), 5.0f);
    g.setColour(juce::Colours::black); g.drawRoundedRectangle(th, 7.0f, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.85f)); g.fillRect(th.getX() + 8.0f, th.getCentreY() - 1.0f, th.getWidth() - 16.0f, 2.0f);
}

// ===================================================================== small controls
static int decimalsFor(const ParamDef& d)
{
    if (d.step >= 1.0f) return 0;
    if (d.step >= 0.1f) return 1;
    return 2;
}

KnobComp::KnobComp(juce::AudioProcessorValueTreeState& s, int pi, const juce::String& caption)
{
    const ParamDef& d = paramDef(pi);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 15);
    slider.setNumDecimalPlacesToDisplay(decimalsFor(d));
    slider.setTextValueSuffix(juce::String(" ") + d.unit);
    slider.setDoubleClickReturnValue(true, d.df);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible(slider);
    label.setText(caption.isEmpty() ? juce::String(d.name) : caption, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(11.5f));
    label.setColour(juce::Label::textColourId, ui::kDim);
    addAndMakeVisible(label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, d.id, slider);
}
void KnobComp::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(15));
    slider.setBounds(r);
}

ToggleComp::ToggleComp(juce::AudioProcessorValueTreeState& s, int pi, const juce::String& caption)
{
    const ParamDef& d = paramDef(pi);
    button.setButtonText(caption.isEmpty() ? juce::String(d.name) : caption);
    addAndMakeVisible(button);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, d.id, button);
}

ComboComp::ComboComp(juce::AudioProcessorValueTreeState& s, int pi, const juce::String& caption)
{
    const ParamDef& d = paramDef(pi);
    label.setText(caption.isEmpty() ? juce::String(d.name) : caption, juce::dontSendNotification);
    label.setFont(juce::Font(11.5f));
    label.setColour(juce::Label::textColourId, ui::kDim);
    addAndMakeVisible(label);
    juce::StringArray names;
    for (auto& n : choiceNames(pi)) names.add(juce::String(n));
    box.addItemList(names, 1);
    addAndMakeVisible(box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, d.id, box);
}
void ComboComp::resized()
{
    auto r = getLocalBounds();
    label.setBounds(r.removeFromTop(14));
    box.setBounds(r);
}

// ===================================================================== panel
void Panel::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(ui::kPanel); g.fillRoundedRectangle(r, 10.0f);
    g.setGradientFill(juce::ColourGradient(ui::kGold, r.getX(), r.getY(), ui::kMagenta.withAlpha(0.6f), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle(r, 10.0f, 1.4f);
    g.setColour(ui::kGold);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(titleText, 14, 6, 200, 18, juce::Justification::centredLeft);
}

void Panel::resized()
{
    auto r = getLocalBounds().reduced(8);
    auto head = r.removeFromTop(22);
    extra.setBounds(head.removeFromRight(130));
    if (!strip.empty())
    {
        auto row = r.removeFromTop(38);
        row.removeFromTop(2);
        const int n = (int) strip.size();
        int wideCount = (int) stripWide.size();
        const int smallW = 90;
        const int wideW = wideCount > 0 ? std::max(110, (row.getWidth() - (n - wideCount) * (smallW + 6)) / wideCount - 6) : 0;
        for (int i = 0; i < n; ++i)
        {
            const bool wide = std::find(stripWide.begin(), stripWide.end(), (size_t) i) != stripWide.end();
            auto cell = row.removeFromLeft(wide ? wideW : smallW);
            strip[(size_t) i]->setBounds(wide ? cell : cell.withTrimmedTop(16).withHeight(22));
            row.removeFromLeft(6);
        }
    }
    const int n = (int) knobs.size();
    if (n == 0) return;
    const int maxPer = std::max(1, r.getWidth() / 62);
    const int rowsN = (n + maxPer - 1) / maxPer;
    const int perRow = (n + rowsN - 1) / rowsN;
    const int cellH = r.getHeight() / rowsN;
    for (int i = 0; i < n; ++i)
    {
        const int row = i / perRow, col = i % perRow;
        const int inRow = std::min(perRow, n - row * perRow);
        const int cellW = r.getWidth() / inRow;
        knobs[(size_t) i]->setBounds(r.getX() + col * cellW, r.getY() + row * cellH, cellW, cellH);
    }
}

// ===================================================================== open map (every parameter)
MapContent::MapContent(juce::AudioProcessorValueTreeState& s)
{
    juce::String lastGroup;
    for (int i = 0; i < kNumParams; ++i)
    {
        const ParamDef& d = paramDef(i);
        if (lastGroup != d.group)
        {
            auto h = std::make_unique<Row>();
            h->header = 1; h->text = d.group;
            h->label = std::make_unique<juce::Label>();
            h->label->setText(d.group, juce::dontSendNotification);
            h->label->setFont(juce::Font(15.0f, juce::Font::bold));
            h->label->setColour(juce::Label::textColourId, ui::kGold);
            addAndMakeVisible(*h->label);
            rows.push_back(std::move(h));
            lastGroup = d.group;
        }
        auto row = std::make_unique<Row>();
        row->label = std::make_unique<juce::Label>();
        row->label->setText(d.name, juce::dontSendNotification);
        row->label->setColour(juce::Label::textColourId, ui::kText);
        addAndMakeVisible(*row->label);
        if (d.kind == KBool)
        {
            row->toggle = std::make_unique<juce::ToggleButton>();
            row->toggle->setButtonText("ON");
            addAndMakeVisible(*row->toggle);
            row->ba = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, d.id, *row->toggle);
        }
        else if (d.kind == KChoice)
        {
            row->combo = std::make_unique<juce::ComboBox>();
            juce::StringArray names; for (auto& n : choiceNames(i)) names.add(juce::String(n));
            row->combo->addItemList(names, 1);
            addAndMakeVisible(*row->combo);
            row->ca = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, d.id, *row->combo);
        }
        else
        {
            row->slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
            row->slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 20);
            row->slider->setNumDecimalPlacesToDisplay(decimalsFor(d));
            row->slider->setTextValueSuffix(juce::String(" ") + d.unit);
            row->slider->setDoubleClickReturnValue(true, d.df);
            addAndMakeVisible(*row->slider);
            row->sa = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, d.id, *row->slider);
        }
        rows.push_back(std::move(row));
    }
    totalH = (int) rows.size() * 28 + 16;
}

void MapContent::resized()
{
    int y = 8;
    const int w = getWidth();
    for (auto& r : rows)
    {
        if (r->header) { r->label->setBounds(10, y + 4, w - 20, 22); y += 28; continue; }
        r->label->setBounds(10, y, 230, 24);
        const juce::Rectangle<int> c(250, y, w - 270, 24);
        if (r->slider) r->slider->setBounds(c);
        if (r->toggle) r->toggle->setBounds(c.withWidth(70));
        if (r->combo)  r->combo->setBounds(c.withWidth(260));
        y += 28;
    }
}

// ===================================================================== editor
static juce::String noteName(float cents, float& off)
{
    static const char* n[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    const int semis = (int) std::floor(cents / 100.0f + 0.5f);
    off = cents - 100.0f * (float) semis;
    const int pc = ((semis % 12) + 12) % 12;
    return juce::String(n[pc]) + juce::String(semis / 12 - 1);
}

DiscoSauceEditor::DiscoSauceEditor(DiscoSauceProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(1096.0 / 976.0);
    setResizeLimits(548, 488, 3840, 3420);   // scales cleanly up to 4K
    setSize(1096, 976);

#if DS_HAS_ARTWORK
       int artSize = 0;
   const char* artData = BinaryData::getNamedResource(BinaryData::namedResourceList[0], artSize);
   artwork = juce::ImageFileFormat::loadFrom(artData, (size_t) artSize);
#endif

    auto& s = proc.apvts;

    // ---------------- artwork screen: knob centres in artwork pixels
    struct Spot { int param; float cx, cy, size; };
    const Spot spots[] = {
        { P_CpMix,   215, 478, 205 },   // COMPRESSOR
        { P_RvMix,   520, 478, 205 },   // REVERB
        { P_DlMix,   232, 735, 205 },   // DELAY
        { P_HsGain,  448, 738, 205 },   // EQUALIZER (air)
        { P_TuneAmt, 668, 742, 205 },   // AUTO-TUNE
        { P_Dbl,     118, 300,  88 } }; // DOUBLER (new)
    for (auto& sp : spots)
    {
        auto k = std::make_unique<ArtKnob>();
        k->param = sp.param; k->cx = sp.cx; k->cy = sp.cy; k->size = sp.size;
        const ParamDef& d = paramDef(sp.param);
        k->s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k->s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        k->s.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k->s.setLookAndFeel(&artLaf);
        if (sp.param == P_Dbl) k->s.getProperties().set("cap", true);
        k->s.setPopupDisplayEnabled(true, true, this);
        k->s.setNumDecimalPlacesToDisplay(d.step >= 1.0f ? 0 : 1);
        k->s.setTextValueSuffix(juce::String(" ") + d.unit);
        k->s.setDoubleClickReturnValue(true, d.df);
        addAndMakeVisible(k->s);
        k->a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, d.id, k->s);
        artKnobs.push_back(std::move(k));
    }
    dryWet.setSliderStyle(juce::Slider::LinearVertical);
    dryWet.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dryWet.setLookAndFeel(&artLaf);
    dryWet.setPopupDisplayEnabled(true, true, this);
    dryWet.setTextValueSuffix(" %");
    dryWet.setNumDecimalPlacesToDisplay(0);
    dryWet.setDoubleClickReturnValue(true, 100.0);
    addAndMakeVisible(dryWet);
    dryWetAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(s, paramDef(P_Mix).id, dryWet);

    {
        juce::StringArray kn, sn;
        for (auto& n : choiceNames(P_Key)) kn.add(juce::String(n));
        for (auto& n : choiceNames(P_Scale)) sn.add(juce::String(n));
        keyBox.addItemList(kn, 1); scaleBox.addItemList(sn, 1);
        addAndMakeVisible(keyBox); addAndMakeVisible(scaleBox);
        keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, paramDef(P_Key).id, keyBox);
        scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(s, paramDef(P_Scale).id, scaleBox);
        addAndMakeVisible(tuneToggle);
        tuneAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(s, paramDef(P_TuneOn).id, tuneToggle);
    }

    // ---------------- detailed panels screen
    auto add = [&](const juce::String& title) -> Panel& { panels.push_back(std::make_unique<Panel>(s, title)); mainPage.addAndMakeVisible(*panels.back()); return *panels.back(); };
    { auto& a = add("AUTO-TUNE");
      a.addToggle(P_TuneOn, "TUNE ON"); a.addCombo(P_Key, "Key"); a.addCombo(P_Scale, "Scale");
      a.addKnob(P_TuneAmt, "Correction"); a.addKnob(P_Retune, "Retune"); a.addKnob(P_Humanize); a.addKnob(P_Hyst, "Stickiness"); a.addKnob(P_Transpose); a.addKnob(P_Fine); }
    { auto& d = add("DOUBLER");
      d.addKnob(P_Dbl, "DOUBLER"); d.addKnob(P_DblVoices, "Voices"); d.addKnob(P_DblDelay, "Delay"); d.addKnob(P_DblDetune, "Detune"); d.addKnob(P_DblWidth, "Width"); d.addKnob(P_DblTone, "Tone"); }
    { auto& c = add("COMPRESSOR"); compPanel = &c; c.enableExtra();
      c.addToggle(P_CpOn, "ON"); c.addKnob(P_CpThr, "Threshold"); c.addKnob(P_CpRatio, "Ratio"); c.addKnob(P_CpAtk, "Attack"); c.addKnob(P_CpRel, "Release"); c.addKnob(P_CpMake, "Makeup"); c.addKnob(P_CpMix, "Mix"); }
    { auto& e = add("EQUALIZER");
      e.addToggle(P_EqOn, "ON"); e.addKnob(P_HpFreq, "Low Cut"); e.addKnob(P_LsGain, "Low Shelf"); e.addKnob(P_B1G, "Bell 1"); e.addKnob(P_B2G, "Bell 2"); e.addKnob(P_B3G, "Bell 3"); e.addKnob(P_HsGain, "High Shelf"); }
    { auto& d = add("DE-ESSER");
      d.addToggle(P_DsOn, "ON"); d.addKnob(P_DsFreq, "Freq"); d.addKnob(P_DsThr, "Threshold"); d.addKnob(P_DsAmt, "Amount"); }
    { auto& d = add("SATURATION");
      d.addToggle(P_SatOn, "ON"); d.addKnob(P_SatDrive, "Drive"); d.addKnob(P_SatMix, "Mix"); }
    { auto& d = add("DELAY");
      d.addToggle(P_DlOn, "ON"); d.addToggle(P_DlSync, "SYNC"); d.addToggle(P_DlPing, "PING-PONG");
      d.addKnob(P_DlMix, "Mix"); d.addKnob(P_DlTime, "Time"); d.addKnob(P_DlFb, "Feedback"); d.addKnob(P_DlTone, "Tone"); }
    { auto& d = add("REVERB");
      d.addToggle(P_RvOn, "ON"); d.addKnob(P_RvMix, "Mix"); d.addKnob(P_RvSize, "Size"); d.addKnob(P_RvDamp, "Damping"); d.addKnob(P_RvPre, "Pre-Delay"); d.addKnob(P_RvWidth, "Width"); }
    { auto& d = add("OUTPUT");
      d.addToggle(P_LimOn, "LIMITER"); d.addKnob(P_InGain, "Input"); d.addKnob(P_OutGain, "Output"); d.addKnob(P_Mix, "Dry/Wet"); }
    addChildComponent(mainPage);

    mapContent = std::make_unique<MapContent>(s);
    mapViewport.setViewedComponent(mapContent.get(), false);
    mapViewport.setScrollBarsShown(true, false);
    addChildComponent(mapViewport);

    // ---------------- shared controls
    for (size_t i = 0; i < presets().size(); ++i) presetBox.addItem(presets()[i].name, (int) i + 1);
    presetBox.setSelectedId(proc.presetIndex.load() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { presetChanged(); };
    addAndMakeVisible(presetBox);

    presetAmount.setSliderStyle(juce::Slider::LinearHorizontal);
    presetAmount.setRange(0.0, 100.0, 1.0);
    presetAmount.setTextValueSuffix(" %");
    presetAmount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 18);
    presetAmount.setValue(proc.presetAmount.load() * 100.0f, juce::dontSendNotification);
    presetAmount.onValueChange = [this] { proc.applyPreset(presetBox.getSelectedId() - 1, (float) presetAmount.getValue() * 0.01f); };
    addAndMakeVisible(presetAmount);
    presetAmountLabel.setText("PRESET AMOUNT", juce::dontSendNotification);
    presetAmountLabel.setFont(juce::Font(11.0f));
    presetAmountLabel.setColour(juce::Label::textColourId, ui::kText);
    addAndMakeVisible(presetAmountLabel);

    panelsButton.onClick = [this] { setView(view == ViewPanels ? ViewArt : ViewPanels); };
    mapButton.onClick    = [this] { setView(view == ViewMap ? ViewArt : ViewMap); };
    addAndMakeVisible(panelsButton);
    addAndMakeVisible(mapButton);

    saveButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Save preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.dspreset");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser& fc) { auto f = fc.getResult(); if (f != juce::File()) proc.savePresetFile(f.withFileExtension("dspreset")); });
    };
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>("Load preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.dspreset");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile()) proc.loadPresetFile(f); });
    };
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);

    setView(ViewArt);
    startTimerHz(20);
}

DiscoSauceEditor::~DiscoSauceEditor()
{
    stopTimer();
    for (auto& k : artKnobs) k->s.setLookAndFeel(nullptr);
    dryWet.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void DiscoSauceEditor::presetChanged()
{
    proc.applyPreset(presetBox.getSelectedId() - 1, (float) presetAmount.getValue() * 0.01f);
}

void DiscoSauceEditor::setView(int v)
{
    view = v;
    const bool art = (v == ViewArt);
    for (auto& k : artKnobs) k->s.setVisible(art);
    dryWet.setVisible(art); keyBox.setVisible(art); scaleBox.setVisible(art); tuneToggle.setVisible(art);
    mainPage.setVisible(v == ViewPanels);
    mapViewport.setVisible(v == ViewMap);
    panelsButton.setButtonText(v == ViewPanels ? "BACK" : "PRO PANELS");
    mapButton.setButtonText(v == ViewMap ? "BACK" : "OPEN MAP");
    resized();
    repaint();
}

void DiscoSauceEditor::timerCallback()
{
    if (compPanel != nullptr)
        compPanel->extra.setText("GR " + juce::String(proc.meterGr.load(), 1) + " dB", juce::dontSendNotification);
    tunerVoiced = proc.meterVoiced.load();
    float off = 0.0f, off2 = 0.0f;
    tunerIn = noteName(proc.meterInCents.load(), off);
    tunerOut = noteName(proc.meterTgtCents.load(), off2);
    tunerCents = off;
    repaint(tunerBox.getSmallestIntegerContainer().expanded(2));
}

void DiscoSauceEditor::paint(juce::Graphics& g)
{
    g.fillAll(ui::kBg);
    if (view == ViewArt)
    {
        if (artwork.isValid())
        {
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(artwork, getLocalBounds().toFloat());
        }
        else
        {
            g.setColour(ui::kGold); g.setFont(juce::Font(54.0f, juce::Font::bold));
            g.drawText("DISCO SAUCE  ALL-IN-1", getLocalBounds().removeFromTop(200), juce::Justification::centred);
        }
        const float sx = (float) getWidth() / 1096.0f, sy = (float) getHeight() / 976.0f;
        g.setColour(ui::kText); g.setFont(juce::Font(11.0f * kScale, juce::Font::bold));
        g.drawText("DOUBLER", juce::Rectangle<float>(118.0f * sx - 40.0f * kScale, 300.0f * sy + 46.0f * sy, 80.0f * kScale, 14.0f * kScale), juce::Justification::centred);
    }
    else
    {
        const juce::Rectangle<int> head(0, 0, getWidth(), 116);
        g.setColour(juce::Colour(0xff120505)); g.fillRect(head);
        g.setColour(ui::kGold); g.setFont(juce::Font(48.0f, juce::Font::bold));
        g.drawText("DISCO SAUCE", 20, 6, 420, 60, juce::Justification::centredLeft);
        g.setColour(ui::kCyan); g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("ALL-IN-1  2 PRO  -  flowmotion", 24, 70, 360, 20, juce::Justification::centredLeft);
        g.setColour(ui::kDim); g.setFont(juce::Font(11.0f));
        g.drawText("Latency " + juce::String(proc.getLatencySamples()) + " samples (reported to host)", 24, 94, 360, 16, juce::Justification::centredLeft);
    }

    // tuner readout
    g.setColour(juce::Colours::black.withAlpha(view == ViewArt ? 0.72f : 1.0f)); g.fillRoundedRectangle(tunerBox, 10.0f);
    g.setColour(ui::kGold2); g.drawRoundedRectangle(tunerBox, 10.0f, 1.4f);
    g.setColour(ui::kDim); g.setFont(juce::Font(11.0f * kScale)); g.drawText("TUNER", tunerBox.withTrimmedLeft(10).withHeight(16), juce::Justification::centredLeft);
    if (tunerVoiced)
    {
        g.setColour(ui::kText); g.setFont(juce::Font(28.0f * kScale, juce::Font::bold));
        g.drawText(tunerIn, tunerBox.withTrimmedTop(12).withHeight(34), juce::Justification::centred);
        const float cx = tunerBox.getCentreX(), cy = tunerBox.getBottom() - 16.0f, span = tunerBox.getWidth() * 0.4f;
        g.setColour(juce::Colour(0xff4a1c18)); g.fillRoundedRectangle(cx - span, cy - 3.0f, span * 2, 6.0f, 3.0f);
        const float k = juce::jlimit(-50.0f, 50.0f, tunerCents) / 50.0f;
        g.setColour(std::abs(k) < 0.15f ? ui::kCyan : ui::kMagenta);
        g.fillEllipse(cx + k * span - 6.0f * kScale, cy - 6.0f * kScale, 12.0f * kScale, 12.0f * kScale);
        g.setColour(ui::kDim); g.setFont(juce::Font(10.5f * kScale));
        g.drawText("-> " + tunerOut, tunerBox.withTrimmedLeft(tunerBox.getWidth() * 0.5f).withHeight(16), juce::Justification::centredRight);
    }
    else
    {
        g.setColour(ui::kDim); g.setFont(juce::Font(18.0f));
        g.drawText("--", tunerBox, juce::Justification::centred);
    }
}

void DiscoSauceEditor::layoutArt()
{
    const float sx = (float) getWidth() / 1096.0f, sy = (float) getHeight() / 976.0f;
    for (auto& k : artKnobs)
    {
        const int w = (int) (k->size * sx), h = (int) (k->size * sy);
        k->s.setBounds((int) (k->cx * sx) - w / 2, (int) (k->cy * sy) - h / 2, w, h);
    }
    dryWet.setBounds((int) (893 * sx), (int) (385 * sy), (int) (66 * sx), (int) (435 * sy));

    const float k = (float) getWidth() / 986.0f;
    kScale = k;
    auto S = [k](float v) { return (int) (v * k); };
    presetBox.setBounds(S(24), S(22), S(230), S(26));
    presetAmountLabel.setFont(juce::Font(11.0f * k));
    presetAmountLabel.setBounds(S(24), S(50), S(120), S(14));
    presetAmount.setBounds(S(24), S(64), S(230), S(20));
    keyBox.setBounds(S(24), S(90), S(70), S(24));
    scaleBox.setBounds(S(98), S(90), S(156), S(24));
    tuneToggle.setBounds(S(24), S(118), S(90), S(22));

    tunerBox = { (float) getWidth() - 262.0f * k, 22.0f * k, 240.0f * k, 80.0f * k };
    auto bt = juce::Rectangle<int>(getWidth() - S(262), S(108), S(262), S(24));
    panelsButton.setBounds(bt.removeFromLeft(S(96))); bt.removeFromLeft(S(4));
    mapButton.setBounds(bt.removeFromLeft(S(84))); bt.removeFromLeft(S(4));
    saveButton.setBounds(bt.removeFromLeft(S(36))); bt.removeFromLeft(S(4));
    loadButton.setBounds(bt.removeFromLeft(S(36)));
}

void DiscoSauceEditor::layoutPanels()
{
    auto r = getLocalBounds();
    auto head = r.removeFromTop(116);
    auto right = head.removeFromRight(300).reduced(6, 8);
    presetBox.setBounds(right.removeFromTop(26));
    right.removeFromTop(4);
    presetAmountLabel.setBounds(right.removeFromTop(14));
    presetAmount.setBounds(right.removeFromTop(22));
    right.removeFromTop(4);
    auto btns = right.removeFromTop(24);
    panelsButton.setBounds(btns.removeFromLeft(96)); btns.removeFromLeft(4);
    mapButton.setBounds(btns.removeFromLeft(84)); btns.removeFromLeft(4);
    saveButton.setBounds(btns.removeFromLeft(36)); btns.removeFromLeft(4);
    loadButton.setBounds(btns.removeFromLeft(36));
    tunerBox = { 560.0f, 10.0f, 220.0f, 96.0f };
    kScale = 1.0f;
    presetAmountLabel.setFont(juce::Font(11.0f));

    auto body = r.reduced(6, 4);
    mapViewport.setBounds(body);
    mapContent->setSize(std::max(200, body.getWidth() - 18), mapContent->requiredHeight());
    mainPage.setBounds(body);

    const int rowH = body.getHeight() / 3;
    struct Cell { int idx; int w; };
    const std::vector<std::vector<Cell>> layout = {
        { {0, 620}, {1, 380} },
        { {2, 360}, {3, 400}, {4, 240} },
        { {5, 170}, {6, 300}, {7, 300}, {8, 230} } };
    for (int row = 0; row < 3; ++row)
    {
        int x = 0;
        for (auto& c : layout[(size_t) row])
        {
            const int w = body.getWidth() * c.w / 1000;
            panels[(size_t) c.idx]->setBounds(x, row * rowH, w, rowH);
            x += w;
        }
    }
}

void DiscoSauceEditor::resized()
{
    if (view == ViewArt) layoutArt(); else layoutPanels();
}
