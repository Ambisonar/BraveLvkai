#pragma once

#include <limits>
#include <JuceHeader.h>

#include "DSP/NotchFilter.h"

// Substitute EQ class alias here
using EQ = NotchFilter;

class VocalBox
{

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
		notchSeries.clear();
	}

	void InitEQSeries(size_t steps) {
		for (size_t i = 0; i < steps; ++i) {
			EQ* new_eq = new EQ;
			new_eq->prepare(spec);
			notchSeries.push_back(new_eq);
		}	
	}

	void InitAll(size_t harmonicPrecision) {
		InitEQSeries(harmonicPrecision);
	}
	
	void ApplyEQ(juce::dsp::AudioBlock<float>& in_audioBlock, double& freq) {
		for (size_t i = 0; i < notchSeries.size(); ++i) {
			notchSeries[i]->notchFrequency = freq * (i + 1) * 0.5;
			if (notchSeries[i]->notchFrequency >= notchSeries[i]->notchSampleRate / 2) break;
			notchSeries[i]->notchQuality = 1;
			notchSeries[i]->process(in_audioBlock);
		}
	}

	void prepare(juce::dsp::ProcessSpec& in_spec, size_t harmonicPrecision) {
		spec = in_spec;
		InitAll(harmonicPrecision);
	}

	void process(juce::dsp::AudioBlock<float>& in_audioBlock, double& frequency) {
		if (frequency < 8000) {
			ApplyEQ(in_audioBlock, frequency);
		}
	}
};

