/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BraveLvkaiAudioProcessorEditor::BraveLvkaiAudioProcessorEditor (BraveLvkaiAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (750, 300);
    juce::LookAndFeel::setDefaultLookAndFeel(&customStyle);

    // set AudioFormatManager for reading IR file
    formatManager.registerBasicFormats();

    addAndMakeVisible(openIRFileButton);
    openIRFileButton.setButtonText("Open IR File...");
    openIRFileButton.onClick = [this] { openButtonClicked(); };
    addAndMakeVisible(irFileLabel);
    irFileLabel.setText("", juce::dontSendNotification);
    irFileLabel.setJustificationType(juce::Justification::centredLeft);

    createSlider(highPassFreqSlider, " Hz");
    createLabel(highPassFreqLabel, "HighPassFreq", &highPassFreqSlider);
    highPassFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "HighPassFreq", highPassFreqSlider);
    createSlider(lowPassFreqSlider, " Hz");
    createLabel(lowPassFreqLabel, "LowFreq", &lowPassFreqSlider);
    lowPassFreqSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "LowPassFreq", lowPassFreqSlider);
    createSlider(driveSlider, "");
    createLabel(driveLabel, "Drive", &driveSlider);
    driveSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "Drive", driveSlider);
    createSlider(satDryWetSlider, " %");
    createLabel(satDryWetLabel, "SatDryWet", &satDryWetSlider);
    satDryWetSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "SatDryWet", satDryWetSlider);
    createSlider(volumeSlider, " dB");
    createLabel(volumeLabel, "Volume", &volumeSlider);
    volumeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "Volume", volumeSlider);
    createSlider(distortionTypeSlider, "");
    createLabel(distortionTypeLabel, "DistortionType", &distortionTypeSlider);
    distortionTypeSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "DistortionType", distortionTypeSlider);
    createSlider(revDryWetSlider, " %");
    createLabel(revDryWetLabel, "RevDryWet", &revDryWetSlider);
    revDryWetSliderAttachment = std::make_unique<APVTS::SliderAttachment>(audioProcessor.apvts, "RevDryWet", revDryWetSlider);
}

BraveLvkaiAudioProcessorEditor::~BraveLvkaiAudioProcessorEditor()
{
}

//==============================================================================
void BraveLvkaiAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(252, 248, 237));
    g.setFont(32.0f);
    
    if (shouldPaintWaveform == true) {
        const int waveformWidth = 80 * 3;
        const int waveformHeight = 100;

        juce::Path waveformPath;
        waveformValues.clear();
        waveformPath.startNewSubPath(15, waveformHeight + 60);

        auto buffer = audioProcessor.getModifiedIR();
        if (buffer.getNumSamples() < 1)
        {
            buffer = audioProcessor.getOriginalIR();
        }
        const float waveformResolution = 1024.0f;
        const int ratio = static_cast<int>(buffer.getNumSamples() / waveformResolution);

        auto bufferPointer = buffer.getReadPointer(0);
        for (int sample = 0; sample < buffer.getNumSamples(); sample += ratio)
        {
            waveformValues.push_back(juce::Decibels::gainToDecibels<float>(std::fabsf(bufferPointer[sample]), -72.0f));
        }
        for (int xPos = 0; xPos < waveformValues.size(); ++xPos)
        {
            auto yPos = juce::jmap<float>(waveformValues[xPos], -72.0f, 0.0f, waveformHeight + 60, 60);
            waveformPath.lineTo(15 + xPos / waveformResolution * waveformWidth, yPos);
        }

        g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

        shouldPaintWaveform = false;
    }
}

void BraveLvkaiAudioProcessorEditor::resized()
{
    const int topBottomMargin = 15;
    const int leftRightMargin = 15;

    const int dialWidth = 80;
    const int dialHeight = 90;

    openIRFileButton.setBounds(leftRightMargin, topBottomMargin, dialWidth * 3, 40);
    irFileLabel.setBounds(leftRightMargin, topBottomMargin + 45, dialWidth * 3, 20);

    revDryWetSlider.setBounds(leftRightMargin + dialWidth * 2, getHeight() - topBottomMargin - dialHeight, dialWidth, dialHeight);

    highPassFreqSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 3, topBottomMargin + dialHeight / 3 * 2, dialWidth, dialHeight);
    lowPassFreqSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 3, getHeight() - topBottomMargin - dialHeight, dialWidth, dialHeight);
    driveSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 2, topBottomMargin + dialHeight / 3 * 2, dialWidth, dialHeight);
    satDryWetSlider.setBounds(getWidth() - leftRightMargin - dialWidth * 2, getHeight() - topBottomMargin - dialHeight, dialWidth, dialHeight);
    volumeSlider.setBounds(getWidth() - leftRightMargin - dialWidth, topBottomMargin + dialHeight / 3 * 2, dialWidth, dialHeight);
    distortionTypeSlider.setBounds(getWidth() - leftRightMargin - dialWidth, getHeight() - topBottomMargin - dialHeight, dialWidth, dialHeight);
}

void BraveLvkaiAudioProcessorEditor::openButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a support IR File (WAV, AIFF, OGG)...", juce::File(),
        "*.wav;*.aif;*.aiff;*.ogg", true, false);
    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File())
        {
            // update text of IR file label
            irFileLabel.setText(file.getFileName(), juce::dontSendNotification);
            irFileLabel.repaint();

            auto* reader = formatManager.createReaderFor(file);
            if (reader != nullptr)
            {
                audioProcessor.setIRBufferSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
                reader->read(&audioProcessor.getOriginalIR(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
                audioProcessor.loadImpulseResponse();

                shouldPaintWaveform = true;
                enableIRParameters = true;
                //reverseButton.setEnabled(enableIRParameters);
                //decayTimeSlider.setEnabled(enableIRParameters);
                repaint();
            }
        }
    });
}

void BraveLvkaiAudioProcessorEditor::createSlider(juce::Slider& slider, juce::String textValueSuffix)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextValueSuffix(textValueSuffix);
    slider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 60, 15);
    // slider.setPopupDisplayEnabled(true, true, this);
}

void BraveLvkaiAudioProcessorEditor::createLabel(juce::Label& label, juce::String text, juce::Component* slider)
{
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setBorderSize(juce::BorderSize<int>(0));
    label.attachToComponent(slider, false);
}