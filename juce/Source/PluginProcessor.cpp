#include "PluginProcessor.h"

OctaveDividerProcessor::OctaveDividerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout OctaveDividerProcessor::createLayout()
{
    using namespace juce;
    // Perillas de 0 a 1 (posición del pote). La curva log de 100k está dentro del núcleo.
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "direct", 1 }, "Direct Level",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "oct1", 1 }, "Octave 1 Level",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.8f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "oct2", 1 }, "Octave 2 Level",
                 NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f));
    // Modo de la Octava 2: Sub (-1 oct, redondo, para bajo) o el original (-2 oct)
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "oct2mode", 1 }, "Octave 2 Mode",
                 StringArray { "Sub -1 oct (bajo)", "-2 oct (original)" }, 0));
    return { p.begin(), p.end() };
}

void OctaveDividerProcessor::prepareToPlay (double sr, int)
{
    core.prepare (sr);
    for (auto* s : { &kDirect, &kOct1, &kOct2 }) s->reset (sr, 0.02);
    kDirect.setCurrentAndTargetValue (*apvts.getRawParameterValue ("direct"));
    kOct1.setCurrentAndTargetValue   (*apvts.getRawParameterValue ("oct1"));
    kOct2.setCurrentAndTargetValue   (*apvts.getRawParameterValue ("oct2"));
    core.setSubMode (apvts.getRawParameterValue ("oct2mode")->load() < 0.5f);
}

bool OctaveDividerProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
        && (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
         || l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void OctaveDividerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    kDirect.setTargetValue (*apvts.getRawParameterValue ("direct"));
    kOct1.setTargetValue   (*apvts.getRawParameterValue ("oct1"));
    kOct2.setTargetValue   (*apvts.getRawParameterValue ("oct2"));

    core.setSubMode (apvts.getRawParameterValue ("oct2mode")->load() < 0.5f);

    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        // El pedal es mono: suma a mono, procesa y devuelve la misma señal en todos los canales
        double x = 0.0;
        for (int ch = 0; ch < numCh; ++ch) x += buffer.getReadPointer (ch)[i];
        x /= (double) numCh;

        const auto y = (float) core.process (x, kDirect.getNextValue(), kOct1.getNextValue(), kOct2.getNextValue());
        for (int ch = 0; ch < numCh; ++ch) buffer.getWritePointer (ch)[i] = y;
    }
}

juce::AudioProcessorEditor* OctaveDividerProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void OctaveDividerProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void OctaveDividerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OctaveDividerProcessor(); }
