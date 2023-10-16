/*
  ==============================================================================

    Filter.h
    Created: 23 Mar 2023 2:43:36pm
    Author:  TaroPie

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class NotchFilter
{
public:
    NotchFilter();
    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::dsp::AudioBlock<float>& block);

    float notchSampleRate{ 0 }, notchFrequency{ 0 }, notchQuality{ 0 };
    
private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using MonoChain = juce::dsp::ProcessorChain<Filter>;

    MonoChain leftChain, rightChain;

    enum ChainPositions
    {
        Notch
    };

    void updateNotchFilter();   
};