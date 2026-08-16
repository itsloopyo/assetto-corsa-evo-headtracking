// Boundary tests for the values HeadTracking.ini feeds into the camera path.
// Pure functions only - no game, no sockets, no Windows API - so this runs
// anywhere `pixi run test` runs.

#include "config_sanitize.h"

#include "test_support.h"

#include "cameraunlock/protocol/port_utils.h"

#include <cmath>
#include <cstdio>
#include <limits>

using namespace ace_ht;
using ace_test::Check;

namespace {

const float kNan = std::numeric_limits<float>::quiet_NaN();
const float kInf = std::numeric_limits<float>::infinity();
const float kFloatMax = std::numeric_limits<float>::max();

// The shipped default of each smoothing key. They are not the same number, and
// that is the whole point of the fallback argument.
const float kLocalDefault  = 0.0f;
const float kRemoteDefault = 0.15f;

void SmoothingTests() {
    std::printf("SanitizeSmoothing\n");
    Check(SanitizeSmoothing(0.0f, kLocalDefault) == 0.0f, "0 passes through");
    Check(SanitizeSmoothing(0.15f, kLocalDefault) == 0.15f, "in-range passes through");
    Check(SanitizeSmoothing(1.0f, kLocalDefault) == 1.0f, "1 passes through");

    // A configured zero is a real setting - track me with no added latency -
    // and it has to reach the processor as written. Nothing here may raise it,
    // least of all on the remote key, whose fallback is 0.15.
    Check(SanitizeSmoothing(0.0f, kRemoteDefault) == 0.0f,
          "a configured 0 survives verbatim on the remote key, never floored to 0.15");

    // Out of range saturates. Not because the math breaks - the core clamps its
    // own interpolation speed to [0.1, 50], so a smoothing above 1 no longer
    // drives the per-frame factor negative - but so the value the mod acts on
    // and the value the INI advertises stay the same number.
    Check(SanitizeSmoothing(5.0f, kLocalDefault) == 1.0f, "above 1 clamps to 1");
    Check(SanitizeSmoothing(-2.0f, kRemoteDefault) == 0.0f,
          "below 0 clamps to the bound, not to the fallback");

    // Non-finite values take the fallback, and it is the fallback of the key
    // that was read. A malformed RemoteSmoothing answered with the LOCAL
    // default would hand a phone on WiFi zero smoothing on raw network jitter,
    // which is the one case RemoteSmoothing exists to cover.
    Check(SanitizeSmoothing(kNan, kLocalDefault) == 0.0f,
          "a NaN LocalSmoothing falls back to the local default 0.0");
    Check(SanitizeSmoothing(kNan, kRemoteDefault) == 0.15f,
          "a NaN RemoteSmoothing falls back to the remote default 0.15, not to 0.0");
    Check(SanitizeSmoothing(kInf, kRemoteDefault) == 0.15f,
          "an Inf RemoteSmoothing falls back to the remote default 0.15");
    Check(SanitizeSmoothing(-kInf, kRemoteDefault) == 0.15f,
          "a -Inf RemoteSmoothing falls back to the remote default 0.15");
    Check(SanitizeSmoothing(kInf, kLocalDefault) == 0.0f,
          "an Inf LocalSmoothing falls back to the local default 0.0");
    Check(SanitizeSmoothing(-kInf, kLocalDefault) == 0.0f,
          "a -Inf LocalSmoothing falls back to the local default 0.0");
}

void SensitivityTests() {
    std::printf("SanitizeSensitivity\n");
    Check(SanitizeSensitivity(1.0f) == 1.0f, "default passes through");
    Check(SanitizeSensitivity(2.5f) == 2.5f, "boost preserved");
    Check(SanitizeSensitivity(-1.0f) == -1.0f, "negative (invert) preserved");
    Check(SanitizeSensitivity(0.0f) == 0.0f, "zero preserved");
    Check(SanitizeSensitivity(kNan) == 1.0f, "NaN -> 1");
    Check(SanitizeSensitivity(kInf) == 1.0f, "Inf -> 1");
    Check(SanitizeSensitivity(-kInf) == 1.0f, "-Inf -> 1");

    // A finite but enormous multiplier overflows the processor's
    // `angle * sensitivity` to Inf, and sin/cos of that is NaN - which is a NaN
    // camera transform handed to the engine on every frame.
    Check(SanitizeSensitivity(kFloatMax) == 100.0f, "absurd magnitude clamps");
    Check(SanitizeSensitivity(-kFloatMax) == -100.0f, "and clamps when inverted");
    Check(std::isfinite(SanitizeSensitivity(kFloatMax) * 180.0f),
          "the clamp keeps a full-range angle finite once scaled");
}

void PositionLimitTests() {
    std::printf("SanitizePositionLimit\n");
    Check(SanitizePositionLimit(0.30f, 0.30f) == 0.30f, "default passes through");
    Check(SanitizePositionLimit(1.25f, 0.30f) == 1.25f, "large but finite limit preserved");
    Check(SanitizePositionLimit(0.0f, 0.30f) == 0.0f, "zero (axis pinned) preserved");
    // A negative limit inverts PositionProcessor's clamp bounds.
    Check(SanitizePositionLimit(-0.5f, 0.30f) == 0.0f, "negative -> 0");
    Check(SanitizePositionLimit(kNan, 0.30f) == 0.30f, "NaN -> fallback");
    Check(SanitizePositionLimit(kInf, 0.20f) == 0.20f, "Inf -> fallback");
    Check(SanitizePositionLimit(-kInf, 0.40f) == 0.40f, "-Inf -> fallback");

    // A mistyped limit puts the camera outside the world; at the top of the
    // float range it overflows the transform's translation row to Inf.
    Check(SanitizePositionLimit(10000.0f, 0.30f) == 10.0f, "absurd limit clamps");
    Check(SanitizePositionLimit(kFloatMax, 0.30f) == 10.0f, "float max clamps");
}

void VirtualKeyTests() {
    std::printf("IsBindableVirtualKey\n");
    Check(IsBindableVirtualKey(0x22), "Page Down can be bound");
    Check(IsBindableVirtualKey(0x01), "low bound can be bound");
    Check(IsBindableVirtualKey(0xFE), "high bound can be bound");
    Check(IsBindableVirtualKey(0x54), "a chord letter can be bound");
    Check(IsBindableVirtualKey(0x7B), "F12 can be bound");

    // A key the poller can never see makes the hotkey silently do nothing.
    Check(!IsBindableVirtualKey(0x220), "a code above 0xFE is not a key");
    Check(!IsBindableVirtualKey(0), "0 is not a key");
    Check(!IsBindableVirtualKey(-1), "a negative code is not a key");

    // Bound to a modifier, a nav binding is suppressed by the chord guard and a
    // chord binding fires the moment the chord itself is held.
    Check(!IsBindableVirtualKey(0x10), "Shift cannot be bound");
    Check(!IsBindableVirtualKey(0x11), "Control cannot be bound");
    Check(!IsBindableVirtualKey(0x12), "Alt cannot be bound");
    Check(!IsBindableVirtualKey(0xA2), "left Control cannot be bound");
    Check(!IsBindableVirtualKey(0xA5), "right Alt cannot be bound");
}

void UdpPortTests() {
    std::printf("NormalizeUdpPort (the INI port boundary this mod relies on)\n");
    bool valid = false;

    Check(cameraunlock::NormalizeUdpPort(4242, 4242, valid) == 4242 && valid,
          "default port is valid");
    Check(cameraunlock::NormalizeUdpPort(1024, 4242, valid) == 1024 && valid,
          "low bound is valid");
    Check(cameraunlock::NormalizeUdpPort(65535, 4242, valid) == 65535 && valid,
          "high bound is valid");

    // 70000 & 0xFFFF == 4464: a raw cast to uint16_t would bind a wrong port
    // and the mod would sit there reporting it was listening.
    Check(cameraunlock::NormalizeUdpPort(70000, 4242, valid) == 4242 && !valid,
          "above 65535 -> fallback, not truncated to 4464");
    Check(cameraunlock::NormalizeUdpPort(0, 4242, valid) == 4242 && !valid,
          "0 (ephemeral bind) -> fallback");
    Check(cameraunlock::NormalizeUdpPort(-1, 4242, valid) == 4242 && !valid,
          "negative -> fallback");
    Check(cameraunlock::NormalizeUdpPort(80, 4242, valid) == 4242 && !valid,
          "privileged port -> fallback");
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - config boundary tests\n");
    std::printf("======================================================\n");
    SmoothingTests();
    SensitivityTests();
    PositionLimitTests();
    VirtualKeyTests();
    UdpPortTests();
    return ace_test::Summary("config boundary");
}
