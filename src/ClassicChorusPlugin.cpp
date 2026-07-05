#include "ClassicChorusPlugin.h"

// ─────────────────────────────────────────────────────────────────────────────
// Classic Flanger plugin class
// ─────────────────────────────────────────────────────────────────────────────

ClassicChorusPlugin::ClassicChorusPlugin()
    : DISTRHO::Plugin(NUM_PARAMS, 0, 3)  // 3 states: preset_name, preset_modified, preset_type
{
    // Default parameter values – physical units matching initParameter() ranges
    for (uint32_t i = 0; i < NUM_PARAMS; ++i)
        setParameterValue(Parameters(i), paramInfo[i].defaultVal);

    // Request setting initial sample rate for Flanger DSP
    sampleRateChanged(getSampleRate());
}

// ── Parameters ────────────────────────────────────────────────────────
void ClassicChorusPlugin::initParameter(uint32_t index, Parameter& param)
{
    param.hints = kParameterIsAutomatable;
    param.name = paramInfo[index].name;
    param.symbol = String(paramInfo[index].name).toBasic().toLower();
    param.unit = paramInfo[index].label;
    param.ranges = DISTRHO::ParameterRanges(paramInfo[index].defaultVal, paramInfo[index].minVal, paramInfo[index].maxVal);

    if (paramInfo[index].isSwitch)
        param.hints |= kParameterIsBoolean;

    switch (index)
    {
        case pParamRange:
        case pParamRate:
            param.hints |= kParameterIsLogarithmic;
            break;
    }
}

float ClassicChorusPlugin::getParameterValue(uint32_t index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, 0.0f);

    return fChorus.getParameter(Parameters(index));
}

void ClassicChorusPlugin::setParameterValue(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < NUM_PARAMS, );

    fChorus.setParameter(Parameters(index), std::clamp(value, paramInfo[index].minVal,
                                        paramInfo[index].maxVal));
}

// ── State ──────────────────────────────────────────────────────────────
void ClassicChorusPlugin::initState(uint32_t index, State& state)
{
    state.hints = kStateIsHostWritable;

    switch (index)
    {
    case 0:
        state.key          = "preset_name";
        state.defaultValue = "";
        state.label        = "Current Preset Name";
        break;
    case 1:
        state.key          = "preset_modified";
        state.defaultValue = "false";
        state.label        = "Preset Modified";
        break;
    case 2:
        state.key          = "preset_type";
        state.defaultValue = "Factory";
        state.label        = "Preset Type";
        break;
    default:
        break;
    }
}

void ClassicChorusPlugin::setState(const char* /*key*/, const char* /*value*/)
{
    // Preset state is managed by the UI; the DSP side does not need to act on it.
    // DPF will forward state changes to the UI via stateChanged() automatically.
}

// ── Audio processing ──────────────────────────────────────────────────
void ClassicChorusPlugin::activate()
{
    fChorus.resetDSP();
}

void ClassicChorusPlugin::deactivate()
{
    fChorus.resetDSP();
}

void ClassicChorusPlugin::sampleRateChanged(double newSampleRate)
{
    fChorus.setSampleRate(static_cast<float>(newSampleRate));
}

void ClassicChorusPlugin::run(const float** inputs, float** outputs, uint32_t frames)
{
    fChorus.processReplacing(inputs, outputs, frames);
}


// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ClassicChorusPlugin();
}

END_NAMESPACE_DISTRHO
