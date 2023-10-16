/*
  ==============================================================================

    Saturation.cpp
    Created: 16 Oct 2023 2:00:11pm
    Author:  TaroPie

  ==============================================================================
*/

#include "Saturation.h"

Saturation::Saturation() {}

void Saturation::prepare(juce::dsp::ProcessSpec& spec)
{
    oversampling.reset();
    oversampling.initProcessing(static_cast<size_t> (spec.maximumBlockSize));

    juce::dsp::ProcessSpec specOverSampling;
    specOverSampling.maximumBlockSize = spec.maximumBlockSize * 3;
    specOverSampling.sampleRate = spec.sampleRate * 4;
    specOverSampling.numChannels = spec.numChannels;

    compressor.prepare(specOverSampling);
    compressor.setAttack(10.0f);
    compressor.setRelease(50.0f);
    compressor.setRatio(4.0f);
    compressor.setThreshold(-4.0f);
}

void Saturation::process(juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::AudioBlock<float> blockOuput = oversampling.processSamplesUp(block);
    for (int channel = 0; channel < blockOuput.getNumChannels(); channel++)
    {
        for (int sample = 0; sample < blockOuput.getNumSamples(); sample++)
        {
            float in = blockOuput.getSample(channel, sample);
            float cleanSig = in;

            // Distortion Type
            if (distortionType == 1 || distortionType == 2 || distortionType == 3 || distortionType == 4 || distortionType == 5)
            {
                // Input Gain (Not for Full wave and Half wave rectifier)
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
                    out = (3.0f - (2.0f - 3.0f * in) * (2.0f - 3.0f * in)) / 3.0f;
                else if (in < -threshold2)
                    out = -1.0f;
                else if (in < -threshold1)
                    out = -(3.0f - (2.0f + 3.0f * in) * (2.0f + 3.0f * in)) / 3.0f;
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
            out = (((out * (mix / 100.0f)) + (cleanSig * (1.0f - (mix / 100.0f)))) * juce::Decibels::decibelsToGain(volume));

            blockOuput.setSample(channel, sample, out);
        }
    }
    oversampling.processSamplesDown(block);
}