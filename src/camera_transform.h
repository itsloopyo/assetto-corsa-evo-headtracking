#pragma once

namespace ace_ht {

// The camera transform this mod reads and writes back is a full 4x4. Nothing
// discovers that at runtime - it comes from the build profile - so the hook
// installer checks the active profile against it and refuses to patch on a
// mismatch, rather than letting the per-frame path compose 16 floats into a
// shorter transform and run off the end of the engine's output struct.
constexpr unsigned kCameraTransformFloats = 16;

// A processed head pose in OpenTrack's convention: degrees, and metres of
// lean. Everything the engine's own conventions differ on (its yaw and its
// lateral axis both run opposite) is handled inside ApplyHeadPose.
struct HeadPose {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float lean_x = 0.0f;
    float lean_y = 0.0f;
    float lean_z = 0.0f;
};

// Composes `pose` into the engine's freshly computed camera-to-world transform,
// in place. `transform` is row-major with the camera position in elements
// 12..14, which is the convention the whole of camera_transform.cpp encodes.
//
// Head yaw turns the basis about the WORLD's up axis; pitch and roll are
// camera-local. So looking down at the pedals and turning the head pans across
// the floor rather than spinning the view about the direction of gaze, and a
// banked corner does not tilt the axis the head turns about - "up" stays a
// constant. On a level car that is indistinguishable from yawing about the
// camera's own up axis; it only diverges once the car is banked or pitched.
// The camera's position is untouched, so the view never orbits the car.
//
// Pure: no engine state, no logging, no clock. The hook decides whether to
// call it; this decides only what the matrix becomes.
void ApplyHeadPose(float transform[kCameraTransformFloats], const HeadPose& pose);

}  // namespace ace_ht
