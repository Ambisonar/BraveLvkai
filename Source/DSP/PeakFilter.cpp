/*
  ==============================================================================

    BellFilter.cpp
    Created: 17 Oct 2023 5:29:08pm
    Author:  sunwei06

  ==============================================================================
*/

#include "PeakFilter.h"

PeakFilter::PeakFilter() {}

void PeakFilter::updatePeakFilter()
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(peakSampleRate, peakFrequency, peakQuality, peakGain);

    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}


void PeakFilter::prepare(juce::dsp::ProcessSpec& spec)
{
    juce::dsp::ProcessSpec monoChainSpec;
    monoChainSpec.maximumBlockSize = spec.maximumBlockSize;
    monoChainSpec.numChannels = 1;
    monoChainSpec.sampleRate = spec.sampleRate;

    leftChain.prepare(monoChainSpec);
    rightChain.prepare(monoChainSpec);

    peakSampleRate = spec.sampleRate;
}

void PeakFilter::process(juce::dsp::AudioBlock<float>& block)
{
    updatePeakFilter();

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}