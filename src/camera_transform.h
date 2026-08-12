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

// Which axis head yaw turns the view about. Pitch and roll are camera-local
// either way, and the two modes are identical whenever the car is level - the
// mode only decides what "up" means once the car is banked or pitched.
enum class YawMode {
    // Yaw about the world's up axis. Looking down at the pedals and turning the
    // head pans across the floor rather than spinning the view about the
    // direction of gaze, and a banked corner does not tilt the axis the head
    // turns about. "Up" stays a constant.
    WorldSpace,
    // Yaw about the camera's own up axis, so the pose lands the same way in the
    // car's frame however far the car is banked over. The physical truth of a
    // cockpit - the driver's head sits in the seat - at the cost of the view
    // leaning at extreme angles.
    CameraLocal,
};

// Composes `pose` into the engine's freshly computed camera-to-world transform,
// in place. `transform` is row-major with the camera position in elements
// 12..14, which is the convention the whole of camera_transform.cpp encodes.
//
// In CameraLocal mode all three axes go in one quaternion applied against the
// camera's own basis. In WorldSpace mode the yaw turns the basis about the
// world's up axis first and only pitch and roll go through the quaternion; the
// camera's position is untouched by both, so neither mode orbits the car.
//
// Pure: no engine state, no logging, no clock. The hook decides whether to
// call it and in which mode; this decides only what the matrix becomes.
void ApplyHeadPose(float transform[kCameraTransformFloats], const HeadPose& pose,
                   YawMode yaw_mode);

}  // namespace ace_ht
