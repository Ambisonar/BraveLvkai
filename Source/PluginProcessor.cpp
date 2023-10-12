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
    oversampling->reset();
    oversampling->initProcessing(static_cast<size_t> (samplesPerBlock));

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock * 3;
    spec.sampleRate = sampleRate * 4;
    spec.numChannels = getTotalNumOutputChannels();
    highPass.prepare(spec);
    lowPass.prepare(spec);
    compressor.prepare(spec);
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    //define parameters in relation to the audio processor value tree state
    float highPassCutoff = *apvts.getRawParameterValue("HighPassCutoff");
    float lowPassCutoff = *apvts.getRawParameterValue("LowPassCutoff");
    float drive = *apvts.getRawParameterValue("DRIVE");
    float dryWet = *apvts.getRawParameterValue("DRYWET");
    float volume = *apvts.getRawParameterValue("VOLUME");
    float distortionType = *apvts.getRawParameterValue("DISTORTIONTYPE");
    bool makeupGainEngaged = *apvts.getRawParameterValue("AUTOMAKEUPGAIN");

    //FILTER
    highPass.setCutoffFrequency(highPassCutoff);
    lowPass.setCutoffFrequency(lowPassCutoff);
    highPass.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lowPass.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    //COMPRESSOR
    compressor.setAttack(10.0f);
    compressor.setRelease(50.0f);
    compressor.setRatio(4.0f);
    compressor.setThreshold(-4.0f);

    //OVERSAMPLING
    juce::dsp::AudioBlock<float> blockInput(buffer);
    juce::dsp::AudioBlock<float> blockOuput = oversampling->processSamplesUp(blockInput);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto audioBlock = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(blockOuput);
    highPass.process(context);
    lowPass.process(context);

    //Messy ass signal processing block because it was my first coded plugin 8^/
    for (int channel = 0; channel < blockOuput.getNumChannels(); channel++)
    {
        for (int sample = 0; sample < blockOuput.getNumSamples(); sample++)
        {
            float in = blockOuput.getSample(channel, sample);
            float cleanSig = in;

            //Distortion Type
            //Input Gain (Not for Full wave and Half wave rectifier)
            if (distortionType == 1 || distortionType == 2 || distortionType == 3 || distortionType == 4 || distortionType == 5)
            {
                in *= drive;
            }
            float out;
            if (distortionType == 1)
            {
                // Simple hard clipping
                float threshold = 1.0f;
                if (in > threshold)
                    out = threshold;
                else if (in < -threshold)
                    out = -threshold;
                else
                    out = in;
            }
            else if (distortionType == 2)
            {
                // Soft clipping based on quadratic function
                float threshold1 = 1.0f / 3.0f;
                float threshold2 = 2.0f / 3.0f;
                if (in > threshold2)
                    out = 1.0f;
                else if (in > threshold1)
                    out = (3.0f - (2.0f - 3.0f * in) *
                        (2.0f - 3.0f * in)) / 3.0f;
                else if (in < -threshold2)
                    out = -1.0f;
                else if (in < -threshold1)
                    out = -(3.0f - (2.0f + 3.0f * in) *
                        (2.0f + 3.0f * in)) / 3.0f;
                else
                    out = 2.0f * in;
            }
            else if (distortionType == 3)
            {
                // Soft clipping based on exponential function
                if (in > 0)
                    out = 1.0f - expf(-in);
                else
                    out = -1.0f + expf(in);

                out = out * 1.5f;
            }
            else if (distortionType == 4)
            {
                // ArcTan
                out = (2.0f / juce::MathConstants<float>::pi) * atan(in);
            }
            else if (distortionType == 5)
            {
                // tubeIsh Distortion

                out = compressor.processSample(channel, in);

                out = juce::dsp::FastMathApproximations::tanh(out);
                float x = out * 0.25;
                float a = abs(x);
                float x2 = x * x;
                float y = 1 - 1 / (1 + a + x2 + 0.66422417311781 * x2 * a + 0.36483285408241 * x2 * x2);
                if (x >= 0)
                {
                    out = y;
                }
                else
                {
                    out = -y;
                }
                out = out * 3.0f;
            }
            out = (((out * (dryWet / 100.0f)) + (cleanSig * (1.0f - (dryWet / 100.0f)))) * juce::Decibels::decibelsToGain(volume));

            if (makeupGainEngaged)
            {
                //Automatic Gain Comp
                makeUpGain = pow(drive, 0.65);
                out /= makeUpGain;
                blockOuput.setSample(channel, sample, out);
            }

            else
            {
                blockOuput.setSample(channel, sample, out);
            }
        }
    }
    oversampling->processSamplesDown(blockInput);
}

//==============================================================================
bool BraveLvkaiAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BraveLvkaiAudioProcessor::createEditor()
{
    return new BraveLvkaiAudioProcessorEditor (*this);
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

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "HighPassCutoff", 1 },
        "HighPassCutoff",
        NormalisableRange<float>(20, 5000, 1, 1), 20));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "LowPassCutoff", 1 },
        "LowPassCutoff",
        NormalisableRange<float>(100, 20000, 1, 1), 20000));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Drive", 1 },
        "Drive",
        NormalisableRange<float>(1.f, 25.f, 1.f, 1.f), 1.f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "DryWet", 1 },
        "DryWet",
        NormalisableRange<float>(1.f, 100.f, 1.f, 1.f), 100.f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Volume", 1 },
        "Volume",
        NormalisableRange<float>(100, 20000, 1, 1), 20000));

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Threshold", 1 },
        "Threshold",
        NormalisableRange<float>(-60, 12, 1, 1), 0));


    auto attackReleaseRange = NormalisableRange<float>(0, 500, 1, 1);

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Attack", 1 }, "Attack", attackReleaseRange, 50));

    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "Release", 1 }, "Release", attackReleaseRange, 250));

    auto choices = std::vector<double>{ 1.5, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20, 50, 100 };
    juce::StringArray sa;
    for (auto choice : choices)
    {
        sa.add(juce::String(choice));
    }

    layout.add(std::make_unique<AudioParameterChoice>(ParameterID{ "Ratio", 1 }, "Ratio", sa, 3));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BraveLvkaiAudioProcessor();
}
