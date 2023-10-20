/*
  ==============================================================================

    Convolution.cpp
    Created: 16 Oct 2023 2:37:43pm
    Author:  TaroPie

  ==============================================================================
*/

#include "Convolution.h"

Convolution::Convolution() 
{
    originalIRBuffer.clear();
    modifiedIRBuffer.clear();
}

void Convolution::prepare(juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    convolver.prepare(spec);
    convolver.reset();
    dryWetMixer.prepare(spec);
    dryWetMixer.reset();
}

void Convolution::process(juce::dsp::AudioBlock<float>& block)
{
    dryWetMixer.setWetMixProportion(mix / 100.0f);
    dryWetMixer.pushDrySamples(block);
    convolver.process(juce::dsp::ProcessContextReplacing<float>(block));
    dryWetMixer.mixWetSamples(block);
}

void Convolution::setIRBufferSize(int newNumChannels, int newNumSamples, bool keepExistingContent, bool clearExtraSpace, bool avoidReallocating)
{
    originalIRBuffer.setSize(newNumChannels, newNumSamples, keepExistingContent, clearExtraSpace, avoidReallocating);
}

juce::AudioBuffer<float>& Convolution::getOriginalIR()
{
    return originalIRBuffer;
}

juce::AudioBuffer<float>& Convolution::getModifiedIR()
{
    return modifiedIRBuffer;
}

void Convolution::loadImpulseResponse()
{
    // Nomalize IR signal
    float globalMaxMagnitude = originalIRBuffer.getMagnitude(0, originalIRBuffer.getNumSamples());
    originalIRBuffer.applyGain(1.0f / (globalMaxMagnitude + 0.01));

    // Trim the white space before and after the signal
    int numSamples = originalIRBuffer.getNumSamples();
    int blockSize = static_cast<int>(std::floor(sampleRate) / 100);
    int startBlockNum = 0;
    int endBlockNum = numSamples / blockSize;

    // Find the first sample in the IR signal that is greater than 0.001
    float localMaxMagnitude = 0.0f;
    while ((startBlockNum + 1) * blockSize < numSamples)
    {
        localMaxMagnitude = originalIRBuffer.getMagnitude(startBlockNum * blockSize, blockSize);
        if (localMaxMagnitude > 0.001)
        {
            break;
        }
        ++startBlockNum;
    }

    // Find the last sample in the IR signal that is greater than 0.001
    localMaxMagnitude = 0.0f;
    while ((endBlockNum - 1) * blockSize > 0)
    {
        --endBlockNum;
        localMaxMagnitude = originalIRBuffer.getMagnitude(endBlockNum * blockSize, blockSize);
        if (localMaxMagnitude > 0.001)
        {
            break;
        }
    }

    // Calculate the length of the cropped IR signal
    int trimmedNumSamples;
    // If the tail has been clipped
    if (endBlockNum * blockSize < numSamples)
    {
        trimmedNumSamples = (endBlockNum - startBlockNum) * blockSize - 1;
    }
    else
    {
        trimmedNumSamples = numSamples - startBlockNum * blockSize;
    }

    // Redefine the buffer size of IR
    modifiedIRBuffer.setSize(originalIRBuffer.getNumChannels(), trimmedNumSamples, false, true, false);
    // Move samples
    for (int channel = 0; channel < originalIRBuffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < trimmedNumSamples; ++sample)
        {
            modifiedIRBuffer.setSample(channel, sample, originalIRBuffer.getSample(channel, sample + startBlockNum * blockSize));
        }
    }

    // Copy back to originalIRBuffer
    originalIRBuffer.makeCopyOf(modifiedIRBuffer);

    updateImpulseResponse(modifiedIRBuffer);
}

void Convolution::updateImpulseResponse(juce::AudioBuffer<float> irBuffer)
{
    convolver.loadImpulseResponse(std::move(irBuffer), sampleRate, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
}

int Convolution::getCurrentIRSize()
{
    return convolver.getCurrentIRSize();
}