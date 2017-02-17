#include "AudioLooper.h"
#include <QDebug>

#include <memory>
#include <vector>
#include <QThread>

using namespace Audio;

/**
 * @brief The Looper::Layer class is an internal class to represent Looper layers.
 */

class Looper::Layer
{
public:

    Layer()
        : availableSamples(0)
    {
        //
    }

    void zero()
    {
        std::fill(leftChannel.begin(), leftChannel.end(), static_cast<float>(0));
        std::fill(rightChannel.begin(), rightChannel.end(), static_cast<float>(0));

        availableSamples = 0;
    }

    void append(const SamplesBuffer &samples, uint samplesToAppend)
    {
        const uint sizeInBytes = samplesToAppend * sizeof(float);

        std::memcpy(&(leftChannel[0]) + availableSamples, samples.getSamplesArray(0), sizeInBytes);
        const int secondChannelIndex = samples.isMono() ? 0 : 1;
        std::memcpy(&(rightChannel[0]) + availableSamples, samples.getSamplesArray(secondChannelIndex), sizeInBytes);

        availableSamples += samplesToAppend;
    }

    std::vector<float> getSamplesPeaks(uint samplesPerPeak) const
    {
        std::vector<float> maxPeaks;

        if (samplesPerPeak) {
            float lastMaxPeak = 0;
            for (uint i = 0; i < availableSamples; ++i) {
                float peak = qMax(qAbs(leftChannel[i]), qAbs(rightChannel[i]));
                if (peak > lastMaxPeak)
                    lastMaxPeak = peak;

                if (i % samplesPerPeak == 0) {
                    maxPeaks.push_back(lastMaxPeak);
                    lastMaxPeak = 0;
                }
            }
        }

        return maxPeaks;
    }

    void mixTo(SamplesBuffer &outBuffer, uint samplesToMix, uint intervalPosition)
    {
        if (samplesToMix > 0) {
            float* internalChannels[] = {&(leftChannel[0]), &(rightChannel[0])};
            const uint channels = outBuffer.getChannels();
            for (uint c = 0; c < channels; ++c) {
                float *out = outBuffer.getSamplesArray(c);
                for (uint s = 0; s < samplesToMix; ++s) {
                    out[s] += internalChannels[c][s + intervalPosition];
                }
            }
        }
    }

    void resize(quint32 samples)
    {
        if (samples > leftChannel.capacity())
            leftChannel.resize(samples);

        if (samples > rightChannel.capacity())
            rightChannel.resize(samples);
    }

    uint availableSamples;

private:
    std::vector<float> leftChannel;
    std::vector<float> rightChannel;

};

// ------------------------------------------------------------------------------------

Looper::Looper()
    : currentLayerIndex(0),
      intervalLenght(0),
      intervalPosition(0),
      maxLayers(4),
      state(LooperState::STOPPED),
      playMode(PlayMode::SEQUENCE)
{
    for (int l = 0; l < MAX_LOOP_LAYERS; ++l) { // create all possible layers
        layers[l] = new Looper::Layer();
    }
}

Looper::~Looper()
{
    for (int l = 0; l < MAX_LOOP_LAYERS; ++l) {
        delete layers[l];
    }
}

void Looper::setMaxLayers(quint8 maxLayers)
{
    if (maxLayers < 1) {
        maxLayers = 1;
    }
    else if (maxLayers > MAX_LOOP_LAYERS) {
        maxLayers = MAX_LOOP_LAYERS;
    }

    this->maxLayers = maxLayers;
}

void Looper::startNewCycle(uint samplesInCycle)
{
    if (samplesInCycle != intervalLenght) {
        intervalLenght = samplesInCycle;
        for (Looper::Layer *layer : layers) {
            layer->resize(intervalLenght);
        }
    }

    intervalPosition = 0;

    LooperState previousState = state;

    if (state == LooperState::WAITING) {
        setState(LooperState::RECORDING);
    }

    if (state == LooperState::RECORDING || state == LooperState::PLAYING) {
        if (previousState == LooperState::WAITING) {
            currentLayerIndex = 0; // start recording in first layer
        }
        else {
            if (state == LooperState::PLAYING && playMode == PlayMode::RANDOM_LAYERS)
                currentLayerIndex = qrand() % maxLayers;
            else
                currentLayerIndex = (currentLayerIndex + 1) % maxLayers;
        }

        // need stop the recording?
        if (state == LooperState::RECORDING) {
            bool needStopRecording = previousState == LooperState::RECORDING && currentLayerIndex == 0; // stop recording when backing to first layer
            if (needStopRecording)
                setState(LooperState::PLAYING);
            else
                layers[currentLayerIndex]->zero(); // zero current layer if keep recording
        }
    }
}

void Looper::setState(LooperState state)
{
    if (this->state != state) {
        this->state = state;
        emit stateChanged();
    }
}

void Looper::setActivated(bool activated)
{
    if (activated) {
        if (state == LooperState::STOPPED) {
            setState(LooperState::WAITING);
        }
    }
    else {
        setState(LooperState::STOPPED);
    }
}

const std::vector<float> Looper::getLayerPeaks(quint8 layerIndex, uint samplesPerPeak) const
{
    if (layerIndex < maxLayers && (state == LooperState::RECORDING || state == LooperState::PLAYING))
    {
        Audio::Looper::Layer *layer = layers[layerIndex];
        return layer->getSamplesPeaks(samplesPerPeak);
    }

    return std::vector<float>(); // empty vector
}

void Looper::process(SamplesBuffer &samples)
{
    if (state == LooperState::STOPPED || state == LooperState::WAITING)
        return;

    uint samplesToProcess = qMin(samples.getFrameLenght(), intervalLenght - intervalPosition);

    if (state == LooperState::RECORDING) { // store/rec current samples
        layers[currentLayerIndex]->append(samples, samplesToProcess);
    }
    else if (state == LooperState::PLAYING) {
        switch (playMode) {
        case PlayMode::SEQUENCE:
        case PlayMode::RANDOM_LAYERS: // layer index in randomized in startNewCycle() function
            mixLayer(currentLayerIndex, samples, samplesToProcess);
            break;
        case PlayMode::ALL_LAYERS:
            playAllLayers(samples, samplesToProcess);
            break;
        default:
            qCritical() << playMode << " not implemented yet!";
        }
    }

    intervalPosition = (intervalPosition + samplesToProcess) % intervalLenght;
}

/**
 Play one layer per interval
 */
void Looper::mixLayer(quint8 layerIndex, SamplesBuffer &samples, uint samplesToMix)
{
    if (layerIndex >= maxLayers)
        return;

    Looper::Layer *loopLayer = layers[layerIndex];
    samplesToMix = qMin(samplesToMix, loopLayer->availableSamples);
    if (samplesToMix) {
        loopLayer->mixTo(samples, samplesToMix, intervalPosition); // mix buffered samples
    }
}

void Looper::playAllLayers(SamplesBuffer &samples, uint samplesToMix)
{
    for (int l = 0; l < maxLayers; ++l) {
        mixLayer(l, samples, samplesToMix);
    }
}

QString Looper::getPlayModeString(PlayMode playMode)
{
    switch (playMode) {
        case PlayMode::SEQUENCE:        return tr("Sequence");
        case PlayMode::ALL_LAYERS:      return tr("All Layers");
        case PlayMode::RANDOM_LAYERS:   return tr("Random");
    }

    return "Error";
}

