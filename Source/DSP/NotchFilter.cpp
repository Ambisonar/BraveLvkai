/*
  ==============================================================================

    Filter.cpp
    Created: 23 Mar 2023 2:43:36pm
    Author:  TaroPie

  ==============================================================================
*/

#include "NotchFilter.h"

NotchFilter::NotchFilter() {}

void NotchFilter::updateNotchFilter()
{
    auto notchCoefficients = juce::dsp::IIR::Coefficients<float>::makeNotch(notchSampleRate, notchFrequency, notchQuality);

    *leftChain.get<ChainPositions::Notch>().coefficients = *notchCoefficients;
    *rightChain.get<ChainPositions::Notch>().coefficients = *notchCoefficients;
}


void NotchFilter::prepare(juce::dsp::ProcessSpec& spec)
{
    juce::dsp::ProcessSpec monoChainSpec;
    monoChainSpec.maximumBlockSize = spec.maximumBlockSize;
    monoChainSpec.numChannels = 1;
    monoChainSpec.sampleRate = spec.sampleRate;

    leftChain.prepare(monoChainSpec);
    rightChain.prepare(monoChainSpec);

    notchSampleRate = spec.sampleRate;
}

void NotchFilter::process(juce::dsp::AudioBlock<float>& block)
{
    updateNotchFilter();

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}