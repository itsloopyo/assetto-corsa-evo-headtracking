#include "camera_transform.h"

#include <cmath>
#include <cstring>

#include "cameraunlock/math/quat4.h"
#include "cameraunlock/math/vec3.h"

namespace ace_ht {

// Local rather than the core's angle_utils constant: including that header
// beside quat4.h makes quat4's own kDegToRad shadow it, and this build treats
// warnings as errors.
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

// The engine's layout, and the one thing every function here depends on: a
// row-major 4x4 whose rows 0..2 are the camera's right, up and forward axes and
// whose row 3 is its world position, with (0,0,0,1) down the last column. The
// engine composes it as S*Rx*Ry*Rz*T and stores the translation in elements
// 12..14, which is what fixes the row-vector (v' = v * M) convention below.
constexpr int kDimension = 4;
constexpr int kRightRow = 0;
constexpr int kUpRow = 1;
constexpr int kForwardRow = 2;
constexpr int kTranslationRow = 3;
constexpr int kBasisRows = 3;
constexpr int kXColumn = 0;
constexpr int kZColumn = 2;
constexpr int kHomogeneousColumn = 3;
// The x/y/z part of a row, i.e. everything before the homogeneous column.
constexpr int kSpatialColumns = 3;

constexpr int Element(int row, int column) { return row * kDimension + column; }

static void MultiplyRowMajor(const float a[kCameraTransformFloats],
                             const float b[kCameraTransformFloats],
                             float out[kCameraTransformFloats]) {
    for (int r = 0; r < kDimension; ++r) {
        for (int c = 0; c < kDimension; ++c) {
            out[Element(r, c)] = a[Element(r, 0)] * b[Element(0, c)]
                               + a[Element(r, 1)] * b[Element(1, c)]
                               + a[Element(r, 2)] * b[Element(2, c)]
                               + a[Element(r, 3)] * b[Element(3, c)];
        }
    }
}

// Head rotation as a camera-local transform. Left-multiplying it onto the
// camera-to-world matrix rotates the camera about its own origin.
//
// Row i of the rotation block is q.Rotate(e_i): for a row vector,
// v*H == sum_i v_i * q.Rotate(e_i) == q.Rotate(v), so the block is exactly the
// core's quaternion with no hand-rolled Euler matrix to drift out of sync with
// the pose the position processor was fed.
static void ComposeHeadRotation(const cameraunlock::math::Quat4& q,
                                float out[kCameraTransformFloats]) {
    using cameraunlock::math::Vec3;
    const Vec3 basis[kBasisRows] = {
        Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f) };
    for (int r = 0; r < kBasisRows; ++r) {
        const Vec3 rotated = q.Rotate(basis[r]);
        out[Element(r, 0)] = rotated.x;
        out[Element(r, 1)] = rotated.y;
        out[Element(r, 2)] = rotated.z;
        out[Element(r, kHomogeneousColumn)] = 0.0f;
    }
    // A pure rotation: no translation of its own, so the camera keeps the
    // position the engine computed once this is multiplied onto it.
    for (int c = 0; c < kSpatialColumns; ++c) out[Element(kTranslationRow, c)] = 0.0f;
    out[Element(kTranslationRow, kHomogeneousColumn)] = 1.0f;
}

// Yaw about the world's up axis (+Y), turning each basis row and leaving the
// translation row alone so the view turns without the camera orbiting the car.
//
// This is the camera matrix RIGHT-multiplied by a rotation about world Y, which
// is what keeps the axis in world space; left-multiplying, as the quaternion
// path below does, would express the same angle in the camera's own frame. On a
// level camera the two agree exactly, which is the invariant the world-space
// tests lock.
static void ApplyWorldYaw(float transform[kCameraTransformFloats], float yawDegrees) {
    const float radians = yawDegrees * kDegToRad;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    for (int r = 0; r < kBasisRows; ++r) {
        const float x = transform[Element(r, kXColumn)];
        const float z = transform[Element(r, kZColumn)];
        transform[Element(r, kXColumn)] =  x * c + z * s;
        transform[Element(r, kZColumn)] = -x * s + z * c;
    }
}

// Lean, carried through the CLEAN camera basis rather than the head-rotated
// one, so the offset follows the car's orientation and not where the head is
// pointing.
static void AddBodyFrameLean(const float clean[kCameraTransformFloats],
                             float dx, float dy, float dz,
                             float out[kCameraTransformFloats]) {
    for (int c = 0; c < kSpatialColumns; ++c) {
        out[Element(kTranslationRow, c)] += dx * clean[Element(kRightRow, c)]
                                          + dy * clean[Element(kUpRow, c)]
                                          + dz * clean[Element(kForwardRow, c)];
    }
}

void ApplyHeadPose(float transform[kCameraTransformFloats], const HeadPose& pose) {
    // The camera basis is left-handed X-right / Y-up / Z-forward (confirmed
    // from a live matrix: right x up == forward exactly), but the engine's yaw
    // and its lateral axis both run opposite to OpenTrack's. Negating the two
    // here, at the engine boundary, keeps the shipped defaults correct and
    // leaves the INI's Invert flags meaning "invert away from correct" rather
    // than "correct the engine".
    const float engineYaw = -pose.yaw;
    const float engineLeanX = -pose.lean_x;

    // A copy, because the multiply writes its result over the same storage it
    // would otherwise be reading the right-hand operand from, and because the
    // lean needs the basis as the engine computed it.
    float clean[kCameraTransformFloats];
    std::memcpy(clean, transform, sizeof(clean));

    // The multiply's right-hand operand: the clean basis already turned about
    // the world's up axis. Yaw is spent here, so what goes through the
    // quaternion - and stays camera-local - is exactly pitch and roll.
    float base[kCameraTransformFloats];
    std::memcpy(base, clean, sizeof(base));
    ApplyWorldYaw(base, engineYaw);

    const cameraunlock::math::Quat4 q =
        cameraunlock::math::Quat4::FromYawPitchRoll(0.0f, pose.pitch, pose.roll);

    float head[kCameraTransformFloats];
    ComposeHeadRotation(q, head);
    MultiplyRowMajor(head, base, transform);
    AddBodyFrameLean(clean, engineLeanX, pose.lean_y, pose.lean_z, transform);
}

}  // namespace ace_ht
