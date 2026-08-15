// Behaviour-locking tests for the camera transform composition - the one piece
// of this mod that decides what the player actually sees. Pure math, no game,
// no engine, no Windows API.
//
// These characterize the SHIPPED conventions (row-major camera-to-world,
// translation in elements 12..14, engine yaw and lateral lean negated at the
// boundary). If a change here makes one fail, the view moved.

#include "camera_transform.h"

#include "test_support.h"

#include <cmath>
#include <cstring>

using namespace ace_ht;
using ace_test::Check;
using ace_test::CheckClose;

namespace {

constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

// Level camera at the world origin: right +X, up +Y, forward +Z.
constexpr float kIdentity[kCameraTransformFloats] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

// A camera yawed 90 degrees away from the identity one and parked away from the
// origin. right x up == forward, matching the left-handed basis the engine
// writes.
constexpr float kTurned[kCameraTransformFloats] = {
     0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f, 0.0f,
    10.0f, 2.0f, -5.0f, 1.0f,
};

// A car banked 30 degrees to the left, the case that separates a head yaw about
// the world's up axis from one about the camera's own: level, the two are the
// same rotation.
constexpr float kBanked[kCameraTransformFloats] = {
    0.86602540f,  0.5f,        0.0f, 0.0f,
   -0.5f,         0.86602540f, 0.0f, 0.0f,
    0.0f,         0.0f,        1.0f, 0.0f,
    3.0f,         1.5f,        7.0f, 1.0f,
};

struct Matrix {
    float m[kCameraTransformFloats];

    explicit Matrix(const float src[kCameraTransformFloats]) {
        std::memcpy(m, src, sizeof(m));
    }
};

Matrix Applied(const float src[kCameraTransformFloats], const HeadPose& pose) {
    Matrix result(src);
    ApplyHeadPose(result.m, pose);
    return result;
}

void CheckMatrixClose(const Matrix& actual, const float expected[kCameraTransformFloats],
                      const char* what) {
    for (int i = 0; i < static_cast<int>(kCameraTransformFloats); ++i) {
        if (std::fabs(actual.m[i] - expected[i]) > ace_test::kEpsilon) {
            std::printf("  FAIL: %s (element %d: expected %.6f, got %.6f)\n",
                        what, i, expected[i], actual.m[i]);
            ++ace_test::g_failures;
            return;
        }
    }
    std::printf("  ok:   %s\n", what);
}

float Dot3(const float* row_a, const float* row_b) {
    return row_a[0] * row_b[0] + row_a[1] * row_b[1] + row_a[2] * row_b[2];
}

void NeutralPoseTests() {
    std::printf("A zero pose is a no-op\n");
    const HeadPose neutral;

    CheckMatrixClose(Applied(kTurned, neutral), kTurned,
                     "a zero pose leaves the engine matrix untouched");
    CheckMatrixClose(Applied(kBanked, neutral), kBanked,
                     "and leaves a banked one untouched too");
}

void LeanTests() {
    std::printf("Lean is applied in the clean camera's own basis\n");

    HeadPose pose;
    pose.lean_x = 0.10f;
    pose.lean_y = 0.20f;
    pose.lean_z = 0.30f;

    // The engine's lateral axis runs opposite to OpenTrack's, so a positive
    // lean_x moves the camera along -X of a level basis.
    const Matrix level = Applied(kIdentity, pose);
    CheckClose(level.m[12], -0.10f, "lean_x moves along the negated right axis");
    CheckClose(level.m[13], 0.20f, "lean_y moves along the up axis");
    CheckClose(level.m[14], 0.30f, "lean_z moves along the forward axis");

    // Same pose against a turned camera: the offset must rotate with the car,
    // so lean_z (forward) now moves along world -X.
    const Matrix turned = Applied(kTurned, pose);
    CheckClose(turned.m[12], 10.0f - 0.30f, "lean follows the car's forward axis");
    CheckClose(turned.m[13], 2.0f + 0.20f, "lean up is unchanged by the car's yaw");
    CheckClose(turned.m[14], -5.0f - 0.10f, "lean lateral follows the car's right axis");
}

void YawTests() {
    // A level camera's up axis IS the world's, so this also pins the angle and
    // the direction: 90 degrees of head yaw turns the view by exactly that.
    std::printf("Yaw turns the basis about the up axis\n");

    HeadPose pose;
    pose.yaw = 90.0f;

    const Matrix turned = Applied(kIdentity, pose);
    CheckClose(turned.m[0], 0.0f, "right.x after a 90 degree yaw");
    CheckClose(turned.m[1], 0.0f, "right.y after a 90 degree yaw");
    CheckClose(turned.m[2], 1.0f, "right.z after a 90 degree yaw");
    CheckClose(turned.m[4], 0.0f, "up.x is untouched by yaw");
    CheckClose(turned.m[5], 1.0f, "up stays the up axis");
    CheckClose(turned.m[8], -1.0f, "forward.x after a 90 degree yaw");
    CheckClose(turned.m[10], 0.0f, "forward.z after a 90 degree yaw");

    // Yawing the view must not orbit the camera around the world origin.
    const Matrix moved = Applied(kTurned, pose);
    CheckClose(moved.m[12], 10.0f, "yaw leaves position.x alone");
    CheckClose(moved.m[13], 2.0f, "yaw leaves position.y alone");
    CheckClose(moved.m[14], -5.0f, "yaw leaves position.z alone");
}

void BankedCarTests() {
    // Yaw is the only axis that leaves the camera's frame. Pitch and roll are
    // the driver's head in the seat, so with the yaw at zero the pose must land
    // the same way in the car's frame however far the car is banked over.
    // Expressed in the car's own basis - each output row dotted with each clean
    // row - a banked car must give exactly what a level one does.
    std::printf("Pitch and roll stay in the car's own frame\n");

    HeadPose pose;
    pose.pitch = -12.0f;
    pose.roll = 6.0f;

    const Matrix level = Applied(kIdentity, pose);
    const Matrix banked = Applied(kBanked, pose);

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            CheckClose(Dot3(&banked.m[r * 4], &kBanked[c * 4]), level.m[r * 4 + c],
                       "banked basis row matches the level one in car coordinates");
        }
    }

    CheckClose(banked.m[12], 3.0f, "banking does not move the camera in x");
    CheckClose(banked.m[13], 1.5f, "banking does not move the camera in y");
    CheckClose(banked.m[14], 7.0f, "banking does not move the camera in z");
}

void PitchAndRollTests() {
    std::printf("Pitch and roll rotate the basis about the camera's own axes\n");

    HeadPose pitch;
    pitch.pitch = 20.0f;
    const Matrix pitched = Applied(kIdentity, pitch);
    CheckClose(pitched.m[8], 0.0f, "pitch leaves forward.x at zero");
    CheckClose(pitched.m[9], -std::sin(20.0f * kDegToRad), "pitch tilts forward.y");
    CheckClose(pitched.m[10], std::cos(20.0f * kDegToRad),
               "pitch shortens forward.z by cos(pitch)");
    CheckClose(pitched.m[0], 1.0f, "pitch leaves the right axis alone");

    HeadPose roll;
    roll.roll = 15.0f;
    const Matrix rolled = Applied(kIdentity, roll);
    CheckClose(rolled.m[0], std::cos(15.0f * kDegToRad), "roll shortens right.x");
    CheckClose(rolled.m[1], std::sin(15.0f * kDegToRad), "roll lifts right.y");
    CheckClose(rolled.m[10], 1.0f, "roll leaves the forward axis alone");
}

void CheckBasisIntegrity(const Matrix& out) {
    for (int r = 0; r < 3; ++r) {
        CheckClose(std::sqrt(Dot3(&out.m[r * 4], &out.m[r * 4])), 1.0f,
                   "basis row stays unit length");
    }
    CheckClose(Dot3(&out.m[0], &out.m[4]), 0.0f, "right is perpendicular to up");
    CheckClose(Dot3(&out.m[0], &out.m[8]), 0.0f, "right is perpendicular to forward");
    CheckClose(Dot3(&out.m[4], &out.m[8]), 0.0f, "up is perpendicular to forward");

    const bool homogeneous = out.m[3] == 0.0f && out.m[7] == 0.0f
                          && out.m[11] == 0.0f && out.m[15] == 1.0f;
    Check(homogeneous, "the homogeneous column survives the composition");
}

HeadPose CombinedPose() {
    HeadPose pose;
    pose.yaw = 25.0f;
    pose.pitch = -13.0f;
    pose.roll = 7.0f;
    pose.lean_x = -0.08f;
    pose.lean_y = 0.04f;
    pose.lean_z = 0.12f;
    return pose;
}

void BasisIntegrityTests() {
    std::printf("A combined pose leaves a well-formed transform\n");
    CheckBasisIntegrity(Applied(kTurned, CombinedPose()));
    CheckBasisIntegrity(Applied(kBanked, CombinedPose()));
}

void WorldYawTests() {
    // Yaw turns the view about the world's up axis, so "up" stays a constant
    // however the car is banked. This is what stops a banked corner tilting the
    // axis the head turns about, and what makes looking down at the pedals and
    // turning the head pan across the floor rather than spin the view about the
    // direction of gaze.
    std::printf("Yaw is about the world's up axis, not the car's\n");

    HeadPose pose;
    pose.yaw = 40.0f;

    // A rotation about the world's up axis leaves every basis vector's world Y
    // component exactly where it was, no matter what the yaw angle is. On a
    // banked car a camera-local yaw would move them, so this is the check that
    // separates the two.
    const Matrix banked = Applied(kBanked, pose);
    for (int r = 0; r < 3; ++r) {
        CheckClose(banked.m[r * 4 + 1], kBanked[r * 4 + 1],
                   "yaw on a banked car leaves the basis row's height alone");
    }

    CheckClose(banked.m[12], 3.0f, "yaw does not move the camera in x");
    CheckClose(banked.m[13], 1.5f, "yaw does not move the camera in y");
    CheckClose(banked.m[14], 7.0f, "yaw does not move the camera in z");

    CheckBasisIntegrity(banked);
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - camera transform tests\n");
    std::printf("=======================================================\n");
    NeutralPoseTests();
    LeanTests();
    YawTests();
    WorldYawTests();
    BankedCarTests();
    PitchAndRollTests();
    BasisIntegrityTests();
    return ace_test::Summary("camera transform");
}
