/*
  ==============================================================================

    Saturation.h
    Created: 16 Oct 2023 2:00:11pm
    Author:  TaroPie

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class Saturation
{
public:
    Saturation();

    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::dsp::AudioBlock<float>& block);

    float distortionType{ 0 }, drive{ 0 }, mix{ 0 }, volume{ 0 };

private:
    juce::dsp::Oversampling<float> oversampling{ 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    juce::dsp::Compressor<float> compressor;
};