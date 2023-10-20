#pragma once

#include <limits>
#include <JuceHeader.h>

#include "NotchFilter.h"
#include "PeakFilter.h"

// Substitute EQ class alias here
 using EQ = PeakFilter;
// using EQ = NotchFilter;

class VocalBox
{

	size_t bufferChannel = 1, bufferNumOfSamples = 0;	// Buffer characteristics
	NotchFilter baseFreqNotch;
	std::vector<EQ*> peakSeries;

public:
	// juce::dsp::ProcessSpec* spec;
	VocalBox(){}

	~VocalBox() {
		peakSeries.clear();
	}

	void InitEQSeries(size_t steps, juce::dsp::ProcessSpec& spec) {
		baseFreqNotch.prepare(spec);

		for (size_t i = 0; i < steps - 1; ++i) {
			EQ* new_eq = new EQ;
			new_eq->prepare(spec);
			new_eq->peakQuality = 15;
			new_eq->peakGain = juce::Decibels::decibelsToGain(-30.0f);
			peakSeries.push_back(new_eq);
		}	
	}

	void InitAll(size_t harmonicPrecision, juce::dsp::ProcessSpec& spec) {
		InitEQSeries(harmonicPrecision, spec);
	}
	
	void ApplyEQ(juce::dsp::AudioBlock<float>& in_audioBlock, double& freq) {
		if (freq <= 0) return;
		baseFreqNotch.notchFrequency = freq;
		baseFreqNotch.notchQuality = 1.88;
		baseFreqNotch.process(in_audioBlock);
		/**/
		for (size_t i = 0; i < peakSeries.size(); ++i) {
			peakSeries[i]->peakFrequency = freq * (i + 3) * 0.5;
			if (peakSeries[i]->peakFrequency >= peakSeries[i]->peakSampleRate / 2) break;
			peakSeries[i]->process(in_audioBlock);
		}
		
	}

	void prepare(juce::dsp::ProcessSpec& in_spec, size_t harmonicPrecision) {
		InitAll(harmonicPrecision, in_spec);
	}

	void process(juce::dsp::AudioBlock<float>& in_audioBlock, double& frequency) {
		if (frequency < 3000 && frequency > 0) {
			ApplyEQ(in_audioBlock, frequency);
		}
	}
};

