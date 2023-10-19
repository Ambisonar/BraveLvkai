/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/NotchFilter.h"
#include "DSP/Saturation.h"
#include "DSP/Convolution.h"
#include "DSP/VocalBox.h"
#include "DSP/PitchDetector/autoCorrelation.h"
#include "DSP/PitchDetector/Yin.h"

//==============================================================================
/**
*/
class BraveLvkaiAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    BraveLvkaiAudioProcessor();
    ~BraveLvkaiAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

    Convolution convolution;

    double frequency = 0;

private:
    juce::dsp::Convolution convolver;
    juce::AudioBuffer<float> originalIRBuffer;
    juce::AudioBuffer<float> modifiedIRBuffer;
    juce::dsp::DryWetMixer<float> recDryWetMixer;

    double makeUpGain;

    Saturation saturation;
    AutoCorrelation ac;
    VocalBox vocalBox;

    PeakFilter peakFilter;

    Yin::Yin_Pitch yin;

    // juce::AudioBuffer<float> pitchDetectionBuffer{ 1, 512 };
    float* pitchDetectionBuffer = nullptr;
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BraveLvkaiAudioProcessor)
};
