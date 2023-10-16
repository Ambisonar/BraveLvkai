#pragma once

#include <limits>
#include <JuceHeader.h>

#include "DSP/NotchFilter.h"
#include "../pitch_detector/pitch_detector.h"

// Substitute EQ class alias here
using EQ = NotchFilter;

using namespace adamski;

class VocalBox
{
	void* pitchMPM = nullptr;
	AudioSampleBuffer* sampleBuffer = nullptr;
	juce::dsp::AudioBlock<float>* audioBlock = nullptr;
	size_t bufferChannel = 1, bufferNumOfSamples = 0;	// Buffer characteristics
	std::vector<EQ*> notchSeries;

public:
	juce::dsp::ProcessSpec spec;

	VocalBox( 
		juce::dsp::AudioBlock<float>* blockPtr,		// Pointer to the audio block being analyzed
		juce::dsp::ProcessSpec& proc_spec,
		size_t harmonicPrecision					// Specifies how many harmonics being considered
	) : spec(proc_spec){
		InitAll(harmonicPrecision);
	}

	VocalBox(){}

	~VocalBox() {
		delete[] pitchMPM;
		delete[] sampleBuffer;
		notchSeries.clear();
	}

	void InitPitchDetector() {
		if (pitchMPM != nullptr) delete[] pitchMPM;
		if (audioBlock != nullptr)
			pitchMPM = (void*)(new PitchMPM(spec.sampleRate, audioBlock->getNumChannels() * audioBlock->getNumSamples()));
	}

	void InitEQSeries(size_t steps) {
		for (size_t i = 0; i < steps; ++i) {
			EQ* new_eq = new EQ;
			new_eq->prepare(spec);
			notchSeries.push_back(new EQ);
		}	
	}

	void InitAll(size_t harmonicPrecision) {
		InitPitchDetector();
		InitEQSeries(harmonicPrecision);
	}

	void ApplyEQ(float& freq) {
		for (size_t i = 0; i < notchSeries.size(); ++i) {
			notchSeries[i]->notchFrequency = freq * (i + 1);
			notchSeries[i]->notchQuality = 10;
			notchSeries[i]->process(*audioBlock);
		}
	}

	void RefreshAudioBuffer() {
		if (audioBlock != nullptr)
		{
			if (bufferChannel != audioBlock->getNumChannels() || bufferNumOfSamples != audioBlock->getNumSamples()) {
				delete[] sampleBuffer;
				bufferChannel = audioBlock->getNumChannels();
				bufferNumOfSamples = audioBlock->getNumSamples();
				sampleBuffer = new AudioSampleBuffer(bufferChannel, bufferNumOfSamples);
			}
			audioBlock->copyTo(*sampleBuffer);
			
		}
	}

	float GetPitch() {
		RefreshAudioBuffer();
		if (sampleBuffer->getNumSamples() == 0) return std::numeric_limits<float>::min();
		return static_cast<PitchMPM*>(pitchMPM)->getPitch(sampleBuffer->getReadPointer(0));
	}

	void prepare(juce::dsp::ProcessSpec& in_spec, size_t harmonicPrecision) {
		spec = in_spec;
		InitAll(harmonicPrecision);
	}

	void process(juce::dsp::AudioBlock<float>* in_audioBlock) {
		audioBlock = in_audioBlock;
		float baseFreq = GetPitch();
		ApplyEQ(baseFreq);
	}
};

