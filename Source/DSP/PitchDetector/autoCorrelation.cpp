/*
  ==============================================================================

    autoCorrelation.cpp
    Created: 24 Apr 2023 3:46:18pm
    Author:  孫新磊

  ==============================================================================
*/

#include "autoCorrelation.h"

//FFT size = 2048 (11 power 2)
AutoCorrelation::AutoCorrelation() : forwardFFT(11)
{
    lastNote = -1;
    lastNotePos = 0;
    LNL = 3000;
}

void AutoCorrelation::prepare(double SampleRate, int SampleSize)
{
    sampleRate = SampleRate;
    sampleSize = SampleSize;
    windowNextFill = 0;
    curSample = 0;
}

void AutoCorrelation::process(const juce::dsp::AudioBlock<float> &inBlock, double* freq)
{
    auto *src = inBlock.getChannelPointer(0);

    windowSize = 1 << windowSizePower2;
    lastNotePos -= sampleSize;  //last note position is actually in previous block
    
    while(true){
        //noteOff the note which sustained more than LNL time
        /*
        if ( (lastNote != -1) && (curSample-1  - lastNotePos > LNL) ){
            auto message = juce::MidiMessage::allNotesOff(1);
            midiMessages.addEvent(message, curSample - 1);
            lastNote = -1;
        }
        */
        //window samples filled done ==> we can find note and make midi message
        if(windowNextFill >= windowSize){
            int note = -1;
            if (function == 0)
                note = findNote();
            else if (function == 1)
                note = SIMDfindNote();
            else
                note = FFTfindNote();
            // buildingMidiMessage(note, curSample - 1, midiMessages);
            
            //windowSamples left shift
            for (int i = 0; i < windowSize - hoppingSize; i++)
                windowSamples[i] = windowSamples[i + hoppingSize];
            windowNextFill -= hoppingSize;
            continue;
        }
        //window sample need next sample block to continue filling up
        if(curSample >= sampleSize){
            curSample = 0;
            break;
        }
        
        // keep copy sample from src to window
        windowSamples[windowNextFill] = src[curSample];
        curSample++;
        windowNextFill++;
    }
    
}

double AutoCorrelation::getFrequency()
{
    int T = 1;  //period represented in number of samples
    float ACF_PREV=0, ACF=0;    //ACF means correlation function value
    float thres = 0;    //to determine whether we're in the second local peak region
    bool flag = false;  //flag be true while we get into second local peak region

    //if (relaxTick < RELAX_TICK) {
    //    ++relaxTick;
    //    goto DIRECT_RETURN;
    //}
    //else relaxTick = 0;

    for (int k = 0; k < windowSize; ++k)
    {
        ACF_PREV = ACF;
        ACF = 0;

        // calculate correlation values
        for (int n = 0; n < windowSize - k; ++n) {
            if (windowSamples[n] < -2 || windowSamples[n] > 2 || windowSamples[n+k] < -2 || windowSamples[n+k] > 2) {
                break;   // Oh Fuck
            }
            ACF += windowSamples[n] * windowSamples[n + k];
        }
            

        //determine thres while k == 0
        if (!k) {
            thres = ACF * correlationThres;
            continue;
        }

        // already find the local peak
        if (flag && ACF <= ACF_PREV) {
            T = k - 1;
            break;
        }

        // already get into second local peak region
        if (ACF > ACF_PREV && ACF > thres) {
            flag = true;
        }
    }

    //calculating frequency
    if (thres <= noiseThres) {
        return -1;
    }

    //DIRECT_RETURN:
    return sampleRate / T;
}

int AutoCorrelation::findNote(){
    double freq = getFrequency();
    int note = round(log(freq / 440.0) / log(2) * 12 + 69);
    if (note > 127 || note < 0)
        return -1;
    return note;
}

int AutoCorrelation::SIMDfindNote(){
    //calculating correlation values and fill in sums[]
    for(int k = 0; k < windowSize; k++)
        sums[k] = 0;
    for(int k = 0; k < windowSize; k++){
        juce::FloatVectorOperationsBase<float, int>::addWithMultiply(sums, &windowSamples[k], windowSamples[k], windowSize - k);
    }
    
    int T = 1;  //period represented in number of samples
    float thres = correlationThres * sums[0];   //determine thres
    bool flag = false;  //flag be true while we get into second local peak region
    
    //find second local peak
    for (int k = 1; k < windowSize; ++k){
        // already get the peak
        if (flag && sums[k] <= sums[k-1]){
            T = k - 1;
            break;
        }
        
        // already get into second local peak region
        if (sums[k] > sums[k-1] && sums[k] > thres){
            flag = true;
        }
    }
    
    //calculating note
    double freq = sampleRate / T;
    int note = round(log(freq / 440.0) / log(2) * 12 + 69);
    if (note > 127 || note < 0 || thres <= noiseThres )
        return -1;
    return note;
}

int AutoCorrelation::FFTfindNote(){
    // copy samples from window to FFTdata
    std::fill (FFTdata.begin(), FFTdata.end(), 0);
    std::copy (&windowSamples[0], &windowSamples[2047], FFTdata.begin());
    
    // do FFT
    forwardFFT.performFrequencyOnlyForwardTransform(FFTdata.data());
    
    // calculate note
    auto k = std::distance(FFTdata.begin(), std::max_element(FFTdata.begin(), FFTdata.end()));
    double freq = (double)k / 2048 * sampleRate;
    int note = round(log(freq / 440.0) / log(2) * 12 + 69);
    if (note > 127 || note < 0 || FFTdata[k] <= noiseThres )
        return -1;
    return note;
}

void AutoCorrelation::buildingMidiMessage(int note, int notePos, juce::MidiBuffer& midiMessages){
    // illegal note
    if (note == -1)
        return;
    
    // lastNote == -1 means there's no ongoing note, we can make noteOn message now
    if (lastNote == -1){
        auto message = juce::MidiMessage::noteOn(1, note, (juce::uint8) 100);
        midiMessages.addEvent(message, notePos);
        lastNote = note;
        lastNotePos = notePos;
        return;
    }
    
    // we detect the same note again, so update its note position to make it sustain
    if (note == lastNote){
        lastNotePos = notePos;
        return;
    }
}
