#pragma once
#include <JuceHeader.h>

// Compresseur feed-forward simple : seuil, ratio, attaque, release, gain de compensation.
class Compressor
{
public:
    Compressor() = default;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        updateCoefficients();
        envelope = 0.0f;
    }

    void process (float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            auto inputLevel = std::abs (samples[i]);

            // Suivi d'enveloppe (attaque/release)
            if (inputLevel > envelope)
                envelope = attackCoeff * (envelope - inputLevel) + inputLevel;
            else
                envelope = releaseCoeff * (envelope - inputLevel) + inputLevel;

            auto envelopeDb = juce::Decibels::gainToDecibels (envelope, -100.0f);

            float gainReductionDb = 0.0f;
            if (envelopeDb > thresholdDb)
                gainReductionDb = (thresholdDb - envelopeDb) * (1.0f - 1.0f / ratio);

            lastGainReductionDb = gainReductionDb;

            auto totalGainDb = gainReductionDb + makeupGainDb;
            auto gainLinear = juce::Decibels::decibelsToGain (totalGainDb);

            samples[i] *= gainLinear;
        }
    }

    void setThresholdDb (float newThreshold) { thresholdDb = newThreshold; }
    void setRatio (float newRatio)            { ratio = juce::jmax (1.0f, newRatio); }
    void setAttackMs (float newAttackMs)      { attackMs = newAttackMs; updateCoefficients(); }
    void setReleaseMs (float newReleaseMs)    { releaseMs = newReleaseMs; updateCoefficients(); }
    void setMakeupGainDb (float newMakeup)    { makeupGainDb = newMakeup; }

    float getThresholdDb() const { return thresholdDb; }
    float getRatio() const       { return ratio; }
    float getAttackMs() const    { return attackMs; }
    float getReleaseMs() const   { return releaseMs; }
    float getMakeupGainDb() const { return makeupGainDb; }
    float getLastGainReductionDb() const { return lastGainReductionDb; }

private:
    void updateCoefficients()
    {
        attackCoeff = std::exp (-1.0f / (float) (0.001 * attackMs * sampleRate));
        releaseCoeff = std::exp (-1.0f / (float) (0.001 * releaseMs * sampleRate));
    }

    double sampleRate = 44100.0;

    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupGainDb = 0.0f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
    float lastGainReductionDb = 0.0f;
};
