/*
  ==============================================================================

    WavReader.h
    Created: 19 Oct 2023 6:19:34pm
    Author:  TaroPie

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class WavReader
{
public:
    void readWav(const char* wavName, const int wavSize)
    {
        auto* reader = formatManager.createReaderFor(std::make_unique<juce::MemoryInputStream>(wavName, wavSize, false));
        if (reader != nullptr)
        {
            sampleRate = reader->sampleRate;
            audioBuffer.setSize(static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples), false, false , false);
            reader->read(&audioBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
        }
    }

    int getWavSampleRate()
	{
		return sampleRate;
	}

    juce::AudioBuffer<float> getWavAudioBuffer()
	{
		return audioBuffer;
	}

private:
    juce::AudioBuffer<float> audioBuffer;
    int sampleRate = 0;

    juce::AudioFormatManager formatManager;
}