#ifndef STRUCTURES_H_INCLUDED
#define STRUCTURES_H_INCLUDED

/**
 * @file Structures.h
 * @brief Parameter enumerations and metadata for the Classic Chorus plugin.
 */

/**
 * @enum Parameters
 * @brief Unique identifiers for each plugin parameter.
 *
 * The values are used as indices into the parameter value array and into
 * the @ref paramInfo metadata table.
 */
enum Parameters
{
    pParamRange,    ///< Base delay range (1.25 ms ~ 320 ms)
    pParamFine,     ///< Fine delay multiplier (0.5x ~ 1.0x)
    pParamRate,     ///< LFO rate (0.1 Hz ~ 10 Hz)
    pParamDepth,    ///< LFO modulation depth (0% ~ 100%)
    pParamMix,      ///< Dry/Wet crossfade mix (0% ~ 100%)
    pParamLevel,    ///< Output gain (-10 dB ~ +10 dB)
    pParamSpread,   ///< Stereo 180° phase spread toggle (Off/On)
    NUM_PARAMS      ///< Total number of parameters.
};

/**
 * @struct ParamInfo
 * @brief Static metadata describing one plugin parameter.
 *
 * Holds the display name, unit label, value range, default value and a
 * flag indicating whether the parameter behaves as a switch.
 */
struct ParamInfo
{
    const char *name;       ///< Parameter display name.
    const char *label;      ///< Parameter unit label (e.g. "Hz", "ms", "%").
    float minVal;           ///< Minimum allowed value.
    float maxVal;           ///< Maximum allowed value.
    float defaultVal;       ///< Default value used on initialization.
    int isSwitch;           ///< 0 = continuous parameter, 1 = switch/enum parameter.
};

/**
 * @var paramInfo
 * @brief Static table of metadata for all plugin parameters.
 *
 * The table order matches the @ref Parameters enumeration. It is used by
 * the plugin class to initialize DISTRHO parameter descriptors.
 */
static const ParamInfo paramInfo[NUM_PARAMS] = {
    /* name,        label,    min,     max,      default, switch */
    { "Range",       "ms",     1.25f,   320.0f,   40.0f,   0 },  ///< pParamRange
    { "Fine",        "x",      0.5f,    1.0f,     0.76f,    0 },  ///< pParamFine
    { "Rate",        "Hz",     0.1f,    10.0f,    0.166f,    0 },  ///< pParamRate
    { "Depth",       "",      0.0f,    1.0f,     0.21f,   0 },  ///< pParamDepth
    { "Mix",         "",      0.0f,    1.0f,     0.5f,   0 },  ///< pParamMix
    { "Level",       "dB",     -10.0f,  10.0f,    4.6f,    0 },  ///< pParamLevel
    { "Spread",      "",       0.0f,    1.0f,     1.0f,    1 }   ///< pParamSpread (Toggle)
};

#endif // STRUCTURES_H_INCLUDED