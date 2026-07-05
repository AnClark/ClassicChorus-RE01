#ifndef CHORUS_HPP
#define CHORUS_HPP

#include <cstdint>

#include "BiquadFilter.hpp"
#include "../Defines.hpp"
#include "../Structures.hpp"

/**
 * @file Chorus.hpp
 * @brief Core chorus DSP class used by the Classic Chorus plugin.
 *
 * Provides stereo chorus processing with modulated delays, anti-aliasing
 * filters, and parameter conversion logic.
 */

/**
 * @class Chorus
 * @brief Stereo chorus effect processing engine.
 *
 * Maintains internal delay lines, sample-rate dependent coefficients, and LFO
 * state required to generate a classic chorus sound.
 */
class Chorus
{
private:
    // Host-controlled parameters.
    float params[NUM_PARAMS]; ///< Raw parameter values from the host in the plugin's native parameter range.

    // Derived physical control variables.
    float baseDelaySamples; ///< Base delay time in samples before modulation is applied.
    float depthSamples; ///< Depth of the LFO modulation expressed in samples.
    float lfoStep; ///< Per-sample LFO increment for the current rate and sample rate.
    float dryGain; ///< Dry signal gain coefficient.
    float wetGain; ///< Wet signal gain coefficient.
    bool spreadEnabled; ///< Enables stereo spread processing for the chorus effect.
    float sampleRate; ///< Current processing sample rate in Hz.

    // Circular delay buffers.
    float delayBufferL[DELAY_LINE_SIZE] { 0.0f }; ///< Left-channel circular delay buffer.
    float delayBufferR[DELAY_LINE_SIZE] { 0.0f }; ///< Right-channel circular delay buffer.
    int32_t writePtr; ///< Write pointer for the circular delay buffers.

    // LFO state.
    float lfoPhase; ///< Current phase of the LFO.

    // Oversampling history.
    float lastInL; ///< Previous input sample for left-channel oversampling.
    float lastInR; ///< Previous input sample for right-channel oversampling.

    // Anti-alias filters.
    BiquadFilter antiAliasL; ///< Left-channel anti-alias filter for upsampling.
    BiquadFilter antiAliasR; ///< Right-channel anti-alias filter for upsampling.
    BiquadFilter deAliasL; ///< Left-channel de-alias filter for downsampling.
    BiquadFilter deAliasR; ///< Right-channel de-alias filter for downsampling.

public:
    /**
     * @brief Constructs the Chorus processor with default audio state.
     */
    Chorus() : writePtr(0), lfoPhase(0.0f), sampleRate(44100.0f), lastInL(0.0f), lastInR(0.0f)
    {}

    /**
     * @brief Set the current processing sample rate.
     * @param newSampleRate Sample rate in Hz.
     */
    void setSampleRate(float newSampleRate)
    {
        this->sampleRate = newSampleRate;
    
        // Recalculate coefficients immediately after the sample-rate change.
        recalculateCoefficients();
    }

    /**
     * @brief Set a parameter value.
     *
     * The plugin will update internal coefficient values immediately after
     * the parameter change.
     *
     * @param index Parameter index defined by the Parameters enum.
     * @param value New parameter value.
     */
    void setParameter(Parameters index, float value)
    {
        this->params[index] = value;

        // Recalculate coefficients immediately after the parameter change.
        recalculateCoefficients();
    }

    /**
     * @brief Get a parameter value.
     * @param index Parameter index defined by the Parameters enum.
     * @return Current parameter value.
     */
    float getParameter(Parameters index) const
    {
        return this->params[index];
    }

    /**
     * @brief Recompute internal coefficients and derived values.
     *
     * This should be called after parameter or sample-rate changes.
     */
    void recalculateCoefficients();

    /**
     * @brief Process one audio block through the chorus effect.
     *
     * Reads the input samples for the current frame range, applies anti-aliasing,
     * modulation, variable delay-line interpolation, and wet/dry mixing, then
     * writes the resulting samples into the output buffers.
     *
     * @param inputs Pointer to the input channel buffers.
     * @param outputs Pointer to the output channel buffers.
     * @param sampleFrames Number of audio frames to process.
     */
    void processReplacing(const float** inputs, float** outputs, int32_t sampleFrames);

    /**
     * @brief Reset all DSP state and clear delay buffers.
     *
     * Returns the chorus engine to an initial clean state.
     */
    void resetDSP();
};

#endif // CHORUS_HPP
