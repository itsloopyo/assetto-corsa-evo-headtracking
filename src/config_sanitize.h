#pragma once

#include <cmath>

namespace ace_ht {

// Boundary validation for values read from the user-editable HeadTracking.ini.
// IniReader parses floats with strtod, which accepts "nan" and "inf" and
// overflows a literal like 1e400 to +inf, so a typo or a corrupted file feeds
// those straight into the smoothing math, the quaternion, and from there into
// the camera transform this mod writes back into the engine. Every float that
// crosses that boundary goes through one of these first.

inline float SanitizeFinite(float v, float fallback) {
    return std::isfinite(v) ? v : fallback;
}

inline float ClampRange(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Smoothing must be finite and within [0,1]. Above 1 the speed lerp in
// CalculateSmoothingFactor (Lerp(50, 0.1, smoothing)) goes negative, so the
// per-frame factor 1-exp(-speed*dt) turns negative and the view extrapolates
// away from the tracker instead of settling on it.
inline float SanitizeSmoothing(float v) {
    return ClampRange(SanitizeFinite(v, 0.0f), 0.0f, 1.0f);
}

// Sensitivity: sign and magnitude are legitimate tuning choices (boost, or
// invert without touching the Invert flags), so the only values refused are the
// two that reach the camera matrix as garbage - NaN/Inf, and a magnitude large
// enough that TrackingProcessor's `angle * sensitivity` overflows to +/-Inf.
// sin/cos of an infinite angle is NaN, so that lands a NaN camera transform in
// the engine every frame: a black screen with nothing in the log to explain it.
// The angle being multiplied is a quaternion decomposition and so never exceeds
// 180 degrees, which this bound keeps finite with room to spare while sitting
// orders of magnitude beyond any usable setting (documented range is 0.1-3.0).
constexpr float kMaxSensitivity = 100.0f;

inline float SanitizeSensitivity(float v) {
    return ClampRange(SanitizeFinite(v, 1.0f), -kMaxSensitivity, kMaxSensitivity);
}

// A virtual key code the hotkey poller can actually watch. GetAsyncKeyState
// only defines 0x01..0xFE, so a typo like YawModeKey=0x220 registers a hotkey
// that can never fire and the toggle silently does nothing.
//
// The modifiers are refused for a second reason: Ctrl and Shift are what the
// chord guard tests, so an action bound to one either never fires (a nav
// binding is suppressed while the chord is held) or fires on every press of
// any chord. Alt sits with them because it is the same class of key and a
// binding on it reads as a modifier the user expects to combine, not press.
inline bool IsBindableVirtualKey(int v) {
    if (v < 0x01 || v > 0xFE) return false;
    if (v >= 0x10 && v <= 0x12) return false;  // Shift, Control, Alt
    if (v >= 0xA0 && v <= 0xA5) return false;  // and their left/right halves
    return true;
}

// Travel limits in metres. PositionProcessor clamps each axis to
// [-limit, +limit], so a negative limit inverts the clamp bounds and a
// non-finite one propagates NaN into the camera translation. The upper bound
// catches a mistyped limit (10000 for 0.10) that would translate the camera out
// of the world, and at the top of the float range would overflow the transform's
// translation row to Inf once the lean is carried through the camera basis.
// A cockpit head has centimetres of travel, so metres of headroom is generous.
constexpr float kMaxPositionLimit = 10.0f;

inline float SanitizePositionLimit(float v, float fallback) {
    return ClampRange(SanitizeFinite(v, fallback), 0.0f, kMaxPositionLimit);
}

}  // namespace ace_ht
