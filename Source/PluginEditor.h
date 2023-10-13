/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "CustomStyle.h"

//==============================================================================
/**
*/
class BraveLvkaiAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BraveLvkaiAudioProcessorEditor (BraveLvkaiAudioProcessor&);
    ~BraveLvkaiAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BraveLvkaiAudioProcessor& audioProcessor;

    juce::CustomStyle customStyle;
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<float> waveformValues;
    bool shouldPaintWaveform = false;
    bool enableIRParameters = false;

    juce::TextButton openIRFileButton;
    juce::Label irFileLabel;

    using APVTS = juce::AudioProcessorValueTreeState;

    juce::Slider highPassFreqSlider;
    juce::Label highPassFreqLabel;
    std::unique_ptr<APVTS::SliderAttachment> highPassFreqSliderAttachment;
    juce::Slider lowPassFreqSlider;
    juce::Label lowPassFreqLabel;
    std::unique_ptr<APVTS::SliderAttachment> lowPassFreqSliderAttachment;
    juce::Slider driveSlider;
    juce::Label driveLabel;
    std::unique_ptr<APVTS::SliderAttachment> driveSliderAttachment;
    juce::Slider satDryWetSlider;
    juce::Label satDryWetLabel;
    std::unique_ptr<APVTS::SliderAttachment> satDryWetSliderAttachment;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    std::unique_ptr<APVTS::SliderAttachment> volumeSliderAttachment;
    juce::Slider distortionTypeSlider;
    juce::Label distortionTypeLabel;
    std::unique_ptr<APVTS::SliderAttachment> distortionTypeSliderAttachment;
    juce::Slider revDryWetSlider;
    juce::Label revDryWetLabel;
    std::unique_ptr<APVTS::SliderAttachment> revDryWetSliderAttachment;

    void openButtonClicked();
    void createSlider(juce::Slider& slider, juce::String textValueSuffix);
    void createLabel(juce::Label& label, juce::String text,
        juce::Component* slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraveLvkaiAudioProcessorEditor)
};
