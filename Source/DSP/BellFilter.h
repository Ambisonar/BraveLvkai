/*
  ==============================================================================

    BellFilter.h
    Created: 17 Oct 2023 5:29:08pm
    Author:  sunwei06

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class PeakFilter
{
public:
    NotchFilter();
    void prepare(juce::dsp::ProcessSpec& spec);
    void process(juce::dsp::AudioBlock<float>& block);

    float peakSampleRate{ 0 }, peakFrequency{ 0 }, peakQuality{ 0 }, peakGain{ 0 };

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using MonoChain = juce::dsp::ProcessorChain<Filter>;

    MonoChain leftChain, rightChain;

    enum ChainPositions
    {
        Peak
    };

    void updatePeakFilter();
};