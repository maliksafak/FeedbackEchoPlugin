#include <JuceHeader.h>
#include "Plugin.h"

AudioPlugin::AudioPlugin()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo()).withOutput("Output", AudioChannelSet::stereo()))
{
    addParameter(delay_time = new AudioParameterFloat({"delay_time", 1}, "Delay Time", 0.01f, 1.0f, 0.5f));
    addParameter(attenuation = new AudioParameterFloat({"attenuation", 1}, "Attenuation", 0.01f, 1.0f, 0.6f));
    addParameter(dry = new AudioParameterFloat({"dry", 1}, "Dry", 0.0f, 1.0f, 1.0f));
    addParameter(wet = new AudioParameterFloat({"wet", 1}, "Wet", 0.0f, 1.0f, 1.0f));
}

void AudioPlugin::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    line = new float[(size_t)sampleRate];

    for (size_t sample = 0; sample < (size_t)(sampleRate); sample++)
    {
        line[sample] = 0.0f;
    }
}

void AudioPlugin::releaseResources()
{
    delete[] line;
}

template <typename T>
void AudioPlugin::processSamples(AudioBuffer<T> &audioBuffer, MidiBuffer &midiBuffer)
{
    auto reader = audioBuffer.getArrayOfReadPointers();
    auto writer = audioBuffer.getArrayOfWritePointers();

    size_t delay_time_in_samples = (size_t)(delay_time->get() * getSampleRate());

    for (size_t sample = 0; sample < (size_t)audioBuffer.getNumSamples(); sample++)
    {
        float sample_dry = 0.0f;

        for (size_t channel = 0; channel < (size_t)audioBuffer.getNumChannels(); channel++)
        {
            sample_dry += (float)reader[channel][sample];
        }

        float sample_wet = line[line_head];
        line[line_head] = (sample_dry * (1.0f - attenuation->get())) + (sample_wet * (1.0f - attenuation->get()));

        float mixed = (sample_dry * dry->get()) + (sample_wet * wet->get());

        for (size_t channel = 0; channel < (size_t)audioBuffer.getNumChannels(); channel++)
        {
            writer[channel][sample] = mixed;
        }
        
        line_head = (line_head + 1) % delay_time_in_samples;
    }
}

void AudioPlugin::processBlock(AudioBuffer<float> &audioBuffer, MidiBuffer &midiBuffer)
{
    processSamples<float>(audioBuffer, midiBuffer);
}

void AudioPlugin::processBlock(AudioBuffer<double> &audioBuffer, MidiBuffer &midiBuffer)
{
    processSamples<double>(audioBuffer, midiBuffer);
}

void AudioPlugin::getStateInformation(MemoryBlock &destData)
{
    MemoryOutputStream *memoryOutputStream = new MemoryOutputStream(destData, true);
    memoryOutputStream->writeFloat(*delay_time);
    memoryOutputStream->writeFloat(*attenuation);
    memoryOutputStream->writeFloat(*dry);
    memoryOutputStream->writeFloat(*wet);
    delete memoryOutputStream;
}

void AudioPlugin::setStateInformation(const void *data, int sizeInBytes)
{
    MemoryInputStream *memoryInputStream = new MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    delay_time->setValueNotifyingHost(memoryInputStream->readFloat());
    attenuation->setValueNotifyingHost(memoryInputStream->readFloat());
    dry->setValueNotifyingHost(memoryInputStream->readFloat());
    wet->setValueNotifyingHost(memoryInputStream->readFloat());
    delete memoryInputStream;
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPlugin();
}