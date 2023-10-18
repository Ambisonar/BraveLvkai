/*
  ==============================================================================

    FreqVisual.h
    Created: 18 Oct 2023 10:55:39am
    Author:  sunwei06

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

//==============================================================================
/*
*/
class FreqVisual  : public juce::Component
{
public:
    FreqVisual(BraveLvkaiAudioProcessor&);
    ~FreqVisual() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BraveLvkaiAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqVisual)
};
