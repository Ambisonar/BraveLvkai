/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BraveLvkaiAudioProcessor::BraveLvkaiAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

BraveLvkaiAudioProcessor::~BraveLvkaiAudioProcessor()
{
}

//==============================================================================
const juce::String BraveLvkaiAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BraveLvkaiAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BraveLvkaiAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BraveLvkaiAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BraveLvkaiAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BraveLvkaiAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BraveLvkaiAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BraveLvkaiAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BraveLvkaiAudioProcessor::getProgramName (int index)
{
    return {};
}

void BraveLvkaiAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BraveLvkaiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = getTotalNumOutputChannels();
    saturation.prepare(spec);
    convolution.prepare(spec);
    ac.prepare(sampleRate, samplesPerBlock);
    vocalBox = new VocalBox();
    vocalBox->prepare(spec, 50);
}

void BraveLvkaiAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BraveLvkaiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BraveLvkaiAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    double frequency = 0;

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    //define parameters in relation to the audio processor value tree state
    float highPassFreq = *apvts.getRawParameterValue("HighPassFreq");
    float lowPassFreq = *apvts.getRawParameterValue("LowPassFreq");
    float drive = *apvts.getRawParameterValue("Drive");
    float satDryWet = *apvts.getRawParameterValue("SatDryWet");
    float volume = *apvts.getRawParameterValue("Volume");
    float distortionType = *apvts.getRawParameterValue("DistortionType");
    float revDryWet = *apvts.getRawParameterValue("RevDryWet");

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);

    saturation.distortionType = distortionType;
    saturation.drive = drive;
    saturation.mix = satDryWet;
    saturation.volume = volume;
    saturation.process(block);

    convolution.mix = revDryWet;
    convolution.process(block);
    
    ac.process(juce::dsp::ProcessContextReplacing<float>(block), &frequency);

    if (vocalBox != nullptr && frequency > 0) {
        vocalBox->process(block, frequency);
    }
}

//==============================================================================
bool BraveLvkaiAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BraveLvkaiAudioProcessor::createEditor()
{
    return new BraveLvkaiAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BraveLvkaiAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BraveLvkaiAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout BraveLvkaiAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;

    using namespace juce;

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "HighPassFreq", 1 },
        "HighPassFreq",
        NormalisableRange<float>(20, 5000, 1, 1), 20));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "LowPassFreq", 1 },
        "LowPassFreq",
        NormalisableRange<float>(100, 20000, 1, 1), 20000));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Drive", 1 },
        "Drive",
        NormalisableRange<float>(1.f, 25.f, 1.f, 1.f), 1.f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "SatDryWet", 1 },
        "SatDryWet",
        NormalisableRange<float>(1.f, 100.f, 1.f, 1.f), 100.f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Volume", 1 },
        "Volume",
        NormalisableRange<float>(-20.f, 20.f, 0.1f, 1.f), 0.f));
    layout.add(std::make_unique<juce::AudioParameterInt>("DistortionType", "DistortionType", 1, 5, 1));

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "RevDryWet", 1 },
        "RevDryWet",
        NormalisableRange<float>(1.f, 100.f, 1.f, 1.f), 100.f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraveLvkaiAudioProcessor();
}
