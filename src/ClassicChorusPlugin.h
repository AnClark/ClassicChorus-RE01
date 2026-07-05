#ifndef CLASSIC_CHORUS_PLUGIN_H_INCLUDED
#define CLASSIC_CHORUS_PLUGIN_H_INCLUDED

#include "DistrhoPlugin.hpp"
#include "core/Chorus.hpp"
#include "config.h"

/**
 * @file ClassicChorusPlugin.h
 * @brief Main plugin class for the Classic Chorus audio effect.
 *
 * Implements a stereo chorus using a modulated delay line per channel and
 * anti-aliasing processing.
 */

/**
 * @class ClassicChorusPlugin
 * @brief DISTRHO audio plugin implementation of a classic chorus effect.
 *
 * @details The plugin processes stereo audio by applying a variable delay to
 * each channel. The delay time is modulated by an LFO, and feedback, dry/wet
 * mix, stereo phase offset, and waveform polarity are exposed through host
 * parameters.
 */
class ClassicChorusPlugin : public DISTRHO::Plugin
{
public:
    /**
     * @brief Constructs the plugin and initializes its internal state.
     */
    ClassicChorusPlugin();

protected:
    // ── Plugin metadata ────────────────────────────────────────────────────

    /** @brief Returns the plugin's short label/name. */
    const char* getLabel()   const override { return DISTRHO_PLUGIN_NAME; }

    /** @brief Returns the plugin vendor/maker string. */
    const char* getMaker()   const override { return DISTRHO_PLUGIN_BRAND; }

    /** @brief Returns the plugin license string (GPLv3+). */
    const char* getLicense() const override { return "GPLv3+"; }

    /** @brief Returns the plugin version as a packed integer (1.0.0). */
    uint32_t    getVersion() const override { return d_version(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH); }

    // ── Parameters ────────────────────────────────────────────────────────

    /**
     * @brief Initializes a plugin parameter descriptor.
     * @param index Zero-based parameter index.
     * @param param Reference to the DISTRHO parameter structure to fill.
     */
    void initParameter(uint32_t index, Parameter& param) override;

    /**
     * @brief Retrieves the current normalized value of a parameter.
     * @param index Zero-based parameter index.
     * @return Current parameter value in the range [0.0, 1.0] or the parameter's native range.
     */
    float getParameterValue(uint32_t index) const override;

    /**
     * @brief Sets the value of a parameter from the host.
     * @param index Zero-based parameter index.
     * @param value New parameter value.
     */
    void setParameterValue(uint32_t index, float value) override;

    // ── State ──────────────────────────────────────────────────────────────

    /**
     * @brief Initializes a plugin state key.
     * @param index Zero-based state index.
     * @param state Reference to the DISTRHO state structure to fill.
     */
    void initState(uint32_t index, State& state) override;

    /**
     * @brief Called when the host sets a state value.
     * @param key State key string provided by the host.
     * @param value State value string provided by the host.
     */
    void setState(const char* key, const char* value) override;

    // ── Audio processing ──────────────────────────────────────────────────

    /**
     * @brief Called when the plugin is activated.
     *
     * Prepares DSP state and resets internal processing caches before audio
     * processing begins.
     */
    void activate() override;

    /**
     * @brief Called when the plugin is deactivated.
     *
     * Cleans up DSP state and releases any resources held during processing.
     */
    void deactivate() override;

    /**
     * @brief Called when the host sample rate changes.
     * @param newSampleRate New sample rate in Hz.
     */
    void sampleRateChanged(double newSampleRate) override;

    /**
     * @brief Processes one audio buffer.
     * @param inputs  Array of input channel pointers.
     * @param outputs Array of output channel pointers.
     * @param frames  Number of sample frames to process.
     */
    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:
    /** @brief Chorus DSP engine instance. */
    Chorus fChorus;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicChorusPlugin)
};

#endif // CLASSIC_CHORUS_PLUGIN_H_INCLUDED
