/*
  ==============================================================================

    autoCorrelation.h
    Created: 24 Apr 2023 3:46:03pm
    Author:  孫新磊

  ==============================================================================
*/

#pragma once
#define RELAX_TICK 1

#include <JuceHeader.h>

class AutoCorrelation
{
    size_t relaxFeed = 5;
public:

    int LNL; //least note length
    int function = 0;   // function to find note
    int windowSizePower2 = 12;
    int hoppingSize = 1024;
    float correlationThres = 0.6;
    float noiseThres = 0.05f;
    
    AutoCorrelation();
    void prepare(double SampleRate, int SampleSize);
    void process(const juce::dsp::AudioBlock<float>& inBlock, double* freq);
    
    // return frequency (Robin)
    double getFrequency();

    // return note
    int findNote(); // Modified
    int SIMDfindNote();
    int FFTfindNote();  //this FFT can process only with fixed window size (2048 here)
    
    // determine whether should we make noteOn message
    void buildingMidiMessage(int note, int notePos, juce::MidiBuffer& midiMessages);
    
private:
    double sampleRate;
    int sampleSize;
    
    int windowSize;
    float windowSamples[8192] = {0};
    int windowNextFill;
    int curSample;
    
    // for SIMD
    float sums[8192] = { 0 };
    
    // for FFT
    juce::dsp::FFT forwardFFT;
    std::array<float, 4096> FFTdata;
    
    // for building midi message
    int lastNote;   //lastNote == -1 means there's no note sustaining
    int lastNotePos;
};

