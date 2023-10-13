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
    oversampling.reset(new juce::dsp::Oversampling<float>(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));
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

    juce::dsp::ProcessSpec specConvolver;
    specConvolver.maximumBlockSize = samplesPerBlock;
    specConvolver.sampleRate = sampleRate;
    specConvolver.numChannels = getTotalNumOutputChannels();
    convolver.prepare(specConvolver);
    convolver.reset();
    recDryWetMixer.prepare(specConvolver);
    recDryWetMixer.reset();
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
    float highPassFreq = *apvts.getRawParameterValue("HighPassFreq");
    float lowPassFreq = *apvts.getRawParameterValue("LowPassFreq");
    float drive = *apvts.getRawParameterValue("Drive");
    float satDryWet = *apvts.getRawParameterValue("SatDryWet");
    float volume = *apvts.getRawParameterValue("Volume");
    float distortionType = *apvts.getRawParameterValue("DistortionType");
    //bool makeupGainEngaged = *apvts.getRawParameterValue("AUTOMAKEUPGAIN");
    float revDryWet = *apvts.getRawParameterValue("RevDryWet");

    // Filter
    highPass.setCutoffFrequency(highPassFreq);
    lowPass.setCutoffFrequency(lowPassFreq);
    highPass.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lowPass.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    // Compressor
    compressor.setAttack(10.0f);
    compressor.setRelease(50.0f);
    compressor.setRatio(4.0f);
    compressor.setThreshold(-4.0f);

    // OverSampling
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

            // Distortion Type
            // Input Gain (Not for Full wave and Half wave rectifier)
            if (distortionType == 1 || distortionType == 2 || distortionType == 3 || distortionType == 4 || distortionType == 5)
            {
                in *= drive;
            }
            float out = 0.0f;
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
            out = (((out * (satDryWet / 100.0f)) + (cleanSig * (1.0f - (satDryWet / 100.0f)))) * juce::Decibels::decibelsToGain(volume));

            //if (makeupGainEngaged)
            //{
            //    //Automatic Gain Comp
            //    makeUpGain = pow(drive, 0.65);
            //    out /= makeUpGain;
            //    blockOuput.setSample(channel, sample, out);
            //}

            //else
            {
                blockOuput.setSample(channel, sample, out);
            }
        }
    }
    oversampling->processSamplesDown(blockInput);

    recDryWetMixer.setWetMixProportion(revDryWet / 100.0f);
    recDryWetMixer.pushDrySamples(blockInput);
    convolver.process(juce::dsp::ProcessContextReplacing<float>(blockInput));
    recDryWetMixer.mixWetSamples(blockInput);
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

void BraveLvkaiAudioProcessor::setIRBufferSize(int newNumChannels, int newNumSamples, bool keepExistingContent, bool clearExtraSpace, bool avoidReallocating)
{
    originalIRBuffer.setSize(newNumChannels, newNumSamples, keepExistingContent, clearExtraSpace, avoidReallocating);
}

juce::AudioBuffer<float>& BraveLvkaiAudioProcessor::getOriginalIR()
{
    return originalIRBuffer;
}

juce::AudioBuffer<float>& BraveLvkaiAudioProcessor::getModifiedIR()
{
    return modifiedIRBuffer;
}

void BraveLvkaiAudioProcessor::loadImpulseResponse()
{
    // 对IR信号进行归一化
    float globalMaxMagnitude = originalIRBuffer.getMagnitude(0, originalIRBuffer.getNumSamples());
    originalIRBuffer.applyGain(1.0f / (globalMaxMagnitude + 0.01));

    // 裁剪IR前后的空白or噪声部分
    int numSamples = originalIRBuffer.getNumSamples();
    int blockSize = static_cast<int>(std::floor(this->getSampleRate()) / 100);
    int startBlockNum = 0;
    int endBlockNum = numSamples / blockSize;

    // 找到IR信号中第一个大于0.001的样本
    float localMaxMagnitude = 0.0f;
    while ((startBlockNum + 1) * blockSize < numSamples)
    {
        localMaxMagnitude = originalIRBuffer.getMagnitude(startBlockNum * blockSize, blockSize);
        if (localMaxMagnitude > 0.001)
        {
            break;
        }
        ++startBlockNum;
    }

    // 找到IR信号中最后一个大于0.001的样本
    localMaxMagnitude = 0.0f;
    while ((endBlockNum - 1) * blockSize > 0)
    {
        --endBlockNum;
        localMaxMagnitude = originalIRBuffer.getMagnitude(endBlockNum * blockSize, blockSize);
        // find the time to decay by 60 dB (T60)
        if (localMaxMagnitude > 0.001)
        {
            break;
        }
    }

    // 计算裁剪后的IR信号长度
    int trimmedNumSamples;
    // 如果尾部有裁剪
    if (endBlockNum * blockSize < numSamples)
    {
        trimmedNumSamples = (endBlockNum - startBlockNum) * blockSize - 1;
    }
    else
    {
        trimmedNumSamples = numSamples - startBlockNum * blockSize;
    }

    // 重新定义IR的Buffer大小
    modifiedIRBuffer.setSize(originalIRBuffer.getNumChannels(), trimmedNumSamples, false, true, false);
    // 平移samples
    for (int channel = 0; channel < originalIRBuffer.getNumChannels(); ++channel)
    {
        for (int sample = 0; sample < trimmedNumSamples; ++sample)
        {
            modifiedIRBuffer.setSample(channel, sample, originalIRBuffer.getSample(channel, sample + startBlockNum * blockSize));
        }
    }

    // 复制回originalIRBuffer
    originalIRBuffer.makeCopyOf(modifiedIRBuffer);

    // 设置decay time
    //auto decayTimeParam = apvts.getParameter("DecayTime");
    //double decayTime = static_cast<double>(trimmedNumSamples) / this->getSampleRate();
    //decayTimeParam->beginChangeGesture();
    //decayTimeParam->setValueNotifyingHost(
    //    decayTimeParam->convertTo0to1(decayTime));
    //decayTimeParam->endChangeGesture();

    updateImpulseResponse(modifiedIRBuffer);
}

void BraveLvkaiAudioProcessor::updateImpulseResponse(juce::AudioBuffer<float> irBuffer)
{
    convolver.loadImpulseResponse(std::move(irBuffer), this->getSampleRate(), juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
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
    //layout.add(std::make_unique<AudioParameterFloat>(ParameterID{ "DistortionType", 1 },
    //    "DistortionType",
    //    NormalisableRange<float>(1.f, 5.f, 1.f, 1.f), 1.f));

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
