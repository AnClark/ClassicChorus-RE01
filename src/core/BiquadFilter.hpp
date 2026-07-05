#ifndef BIQUAD_FILTER_HPP
#define BIQUAD_FILTER_HPP

#include <cmath>

/**
 * @file BiquadFilter.hpp
 * @brief Simple biquad filter used for anti-aliasing and sample processing.
 *
 * Provides coefficient calculation for a standard Butterworth lowpass
 * response and stateful sample processing.
 */
struct BiquadFilter
{
    float b0, b1, b2; ///< Feedforward coefficients.
    float a1, a2;     ///< Feedback coefficients.
    float w1, w2;     ///< Internal delay state samples.

    /**
     * @brief Reset filter state.
     *
     * Sets all internal history values to zero.
     */
    void reset() {
        w1 = w2 = 0.0f;
    }

    /**
     * @brief Compute standard lowpass anti-alias filter coefficients.
     * @param cutoff Cutoff frequency in Hz.
     * @param sampleRate Sample rate in Hz.
     *
     * Uses a Butterworth response with Q = 0.707.
     */
    void setLowpass(float cutoff, float sampleRate) {
        float omega = 2.0f * (float)M_PI * cutoff / sampleRate;
        float snake = sinf(omega);
        float alpha = snake / (2.0f * 0.70710678f); // Q = 0.707 (Butterworth)
        float cosw  = cosf(omega);

        float a0 = 1.0f + alpha;
        b0 = ((1.0f - cosw) / 2.0f) / a0;
        b1 = (1.0f - cosw) / a0;
        b2 = ((1.0f - cosw) / 2.0f) / a0;
        a1 = (-2.0f * cosw) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    /**
     * @brief Process a single input sample through the biquad filter.
     * @param x Input sample.
     * @return Filtered output sample.
     */
    inline float process(float x) {
        float w = x - a1 * w1 - a2 * w2;
        float y = b0 * w + b1 * w1 + b2 * w2;
        w2 = w1;
        w1 = w;
        return y;
    }
};

#endif // BIQUAD_FILTER_HPP
