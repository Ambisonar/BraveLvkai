#pragma once

#include <limits>
#include <JuceHeader.h>

#include "NotchFilter.h"
#include "PeakFilter.h"

// Substitute EQ class alias here
using EQ = PeakFilter;

class VocalBox
{

	size_t bufferChannel = 1, bufferNumOfSamples = 0;	// Buffer characteristics
	NotchFilter baseFreqNotch;
	std::vector<EQ*> peakSeries;

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
		peakSeries.clear();
	}

	void InitEQSeries(size_t steps) {
		for (size_t i = 0; i < steps; ++i) {
			EQ* new_eq = new EQ;
			new_eq->prepare(spec);
			new_eq->peakQuality = 12;
			peakSeries.push_back(new_eq);
		}	
	}

	void InitAll(size_t harmonicPrecision) {
		InitEQSeries(harmonicPrecision);
	}
	
	void ApplyEQ(juce::dsp::AudioBlock<float>& in_audioBlock, double& freq) {
		baseFreqNotch.notchFrequency = freq;
		baseFreqNotch.notchQuality = 1.88;
		baseFreqNotch.process(in_audioBlock);
		for (size_t i = 0; i < peakSeries.size(); ++i) {
			peakSeries[i]->peakFrequency = freq * (i + 2) * 0.5;
			if (peakSeries[i]->peakFrequency >= peakSeries[i]->peakSampleRate / 2) break;
			peakSeries[i]->process(in_audioBlock);
		}
	}

	void prepare(juce::dsp::ProcessSpec& in_spec, size_t harmonicPrecision) {
		spec = in_spec;
		InitAll(harmonicPrecision);
	}

	void process(juce::dsp::AudioBlock<float>& in_audioBlock, double& frequency) {
		if (frequency < 22000) {
			ApplyEQ(in_audioBlock, frequency);
		}
	}
};

