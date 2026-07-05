#include "Chorus.hpp"

#include <cstring>

void Chorus::recalculateCoefficients()
{
    float fs_internal = this->sampleRate * 2.0f; // 2x oversampling internal rate.

    // Parameter 0: Range (1.25ms ~ 320ms), convert to seconds and store in rangeTime.
    float rangeTime = this->params[pParamRange] / 1000.0f;
    
    // Parameter 1: Fine (0.5x ~ 1.0x)
    float fineFactor = this->params[pParamFine];
    
    // Combine to compute the base delay in samples.
    this->baseDelaySamples = rangeTime * fineFactor * fs_internal;

    // Parameter 2: Rate (0.1Hz ~ 10Hz)
    float rateHz = this->params[pParamRate];
    this->lfoStep = rateHz / fs_internal;

    // Parameter 3: Depth (0% ~ 100%, internally stored as 0.0f ~ 1.0f) -> bounded multiplication (safe programming).
    this->depthSamples = this->baseDelaySamples * this->params[pParamDepth];

    // Parameter 4: Mix (0% ~ 100%, internally stored as 0.0f ~ 1.0f)
    float mix = this->params[pParamMix];

    // Parameter 5: Level (-10dB ~ +10dB)
    float leveldB = this->params[pParamLevel];
    float totalGain = powf(10.0f, leveldB / 20.0f);

    // Decouple the dry/wet signal balance.
    this->wetGain = totalGain * mix;
    this->dryGain = totalGain * (1.0f - mix);

    // Parameter 6: Spread switch.
    this->spreadEnabled = (this->params[pParamSpread] >= 0.5f);

    // Dynamically update the anti-alias filter coefficients (cutoff set to the Nyquist frequency of the original host sample rate).
    this->antiAliasL.setLowpass(this->sampleRate * 0.45f, fs_internal);
    this->antiAliasR.setLowpass(this->sampleRate * 0.45f, fs_internal);
    this->deAliasL.setLowpass(this->sampleRate * 0.45f, fs_internal);
    this->deAliasR.setLowpass(this->sampleRate * 0.45f, fs_internal);
}

void Chorus::processReplacing(const float** inputs, float** outputs, int32_t sampleFrames)
{
    const float* inL = inputs[0];
    const float* inR = inputs[1];
    float* outL = outputs[0];
    float* outR = outputs[1];

    for (int32_t i = 0; i < sampleFrames; ++i) {
        const float rawL = inL[i];
        const float rawR = inR[i];

        float finalFilteredL = 0.0f;
        float finalFilteredR = 0.0f;

        // Hard-coded 2-pass loop -> 2x oversampling system.
        for (int32_t j = 0; j < 2; ++j) {
            // Linearly interpolate the oversampled input signal.
            float internalInL = (j == 0) ? 0.5f * (rawL + this->lastInL) : rawL;
            float internalInR = (j == 0) ? 0.5f * (rawR + this->lastInR) : rawR;

            // Input anti-alias filtering.
            float filteredInL = this->antiAliasL.process(internalInL);
            float filteredInR = this->antiAliasR.process(internalInR);

            // Write into the circular buffer.
            this->delayBufferL[this->writePtr] = filteredInL;
            this->delayBufferR[this->writePtr] = filteredInR;

            // Advance the low-frequency oscillator (LFO).
            this->lfoPhase += this->lfoStep;
            if (this->lfoPhase >= 1.0f) this->lfoPhase -= 1.0f;
            float lfoOut = sinf(this->lfoPhase * 2.0f * (float)M_PI);

            // Compute independent read pointer offsets for the left and right channels (core behavior: 180° phase inversion via Spread).
            float dL = this->baseDelaySamples + lfoOut * this->depthSamples;
            float dR = this->baseDelaySamples + (this->spreadEnabled ? -lfoOut : lfoOut) * this->depthSamples;

            // --- Left-channel fractional delay with linear interpolation ---
            float readPtrL = (float)this->writePtr - dL;
            while (readPtrL < 0.0f)
                readPtrL += DELAY_LINE_SIZE;
            int32_t rIdxL1 = (int32_t)readPtrL % DELAY_LINE_SIZE;
            int32_t rIdxL2 = (rIdxL1 + 1) % DELAY_LINE_SIZE;
            float fracL = readPtrL - (int32_t)readPtrL;
            float wetL = (1.0f - fracL) * this->delayBufferL[rIdxL1] + fracL * this->delayBufferL[rIdxL2];

            // --- Right-channel fractional delay with linear interpolation ---
            float readPtrR = (float)this->writePtr - dR;
            while (readPtrR < 0.0f)
                readPtrR += DELAY_LINE_SIZE;
            int32_t rIdxR1 = (int32_t)readPtrR % DELAY_LINE_SIZE;
            int32_t rIdxR2 = (rIdxR1 + 1) % DELAY_LINE_SIZE;
            float fracR = readPtrR - (int32_t)readPtrR;
            float wetR = (1.0f - fracR) * this->delayBufferR[rIdxR1] + fracR * this->delayBufferR[rIdxR2];

            // Decouple the dry/wet signal balance physically.
            float outL_internal = filteredInL * this->dryGain + wetL * this->wetGain;
            float outR_internal = filteredInR * this->dryGain + wetR * this->wetGain;

            // Output restoration filtering (downsampling / de-aliasing).
            finalFilteredL = this->deAliasL.process(outL_internal);
            finalFilteredR = this->deAliasR.process(outR_internal);

            // Advance the write pointer.
            this->writePtr = (this->writePtr + 1) % DELAY_LINE_SIZE;
        }

        // Save the history samples for the next interpolation cycle.
        this->lastInL = rawL;
        this->lastInR = rawR;

        // Extract and write the host output (one result is taken for every two oversampled passes, completing a 2:1 downsampling process).
        outL[i] = finalFilteredL;
        outR[i] = finalFilteredR;
    }
}

void Chorus::resetDSP()
{
    // Reset the biquad filters.
    this->antiAliasL.reset();
    this->antiAliasR.reset();
    this->deAliasL.reset();
    this->deAliasR.reset();

    // Clear the delay buffers.
    std::memset(this->delayBufferL, 0, sizeof(float) * DELAY_LINE_SIZE);
    std::memset(this->delayBufferR, 0, sizeof(float) * DELAY_LINE_SIZE);
}
