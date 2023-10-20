/*
  ==============================================================================

    Convolution.h
    Created: 16 Oct 2023 2:37:43pm
    Author:  TaroPie

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Convolution
{
public:
	Convolution();

    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::dsp::AudioBlock<float>& block);

    void setIRBufferSize(int newNumChannels, int newNumSamples, bool keepExistingContent = false, bool clearExtraSpace = false, bool avoidReallocating = false);
    juce::AudioBuffer<float>& getOriginalIR();
    juce::AudioBuffer<float>& getModifiedIR();
    void loadImpulseResponse();
    void updateImpulseResponse(juce::AudioBuffer<float> irBuffer);

    int getCurrentIRSize();

    float mix{ 0 };

private:
    int sampleRate = 48000;

    juce::dsp::Convolution convolver;
    juce::AudioBuffer<float> originalIRBuffer;
    juce::AudioBuffer<float> modifiedIRBuffer;
    juce::dsp::DryWetMixer<float> dryWetMixer;
};