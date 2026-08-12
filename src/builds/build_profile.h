#pragma once

#include <cstdint>

#include "cameraunlock/memory/pe_fingerprint.h"

namespace ace_ht::builds {

// Everything this mod pins to a specific AssettoCorsaEVO.exe build.
//
// Deliberately small. The camera vtables themselves are NOT pinned: the Kunos
// engine ships full MSVC RTTI, so they are resolved at runtime by class name
// (".?AVCockpitCamera@@" and its DrivableCamera siblings), which survives a
// patch for free. What cannot be discovered at runtime is the shape of that
// interface - which vtable slot computes the camera, and where in the output
// struct the resulting transform lands - so those are the pinned surface.
struct OffsetTable {
    // Vtable slot on the DrivableCamera interface that computes the camera for
    // the frame. Signature (x64 MSVC thiscall):
    //   void slot(Camera* this, float blend /*xmm1*/, void* out /*r8*/,
    //             const float base[16] /*r9*/)
    int camera_compute_slot;

    // Byte offset into `out` where the computed camera transform is written.
    unsigned int camera_out_transform;

    // Number of floats the transform occupies at that offset.
    unsigned int camera_out_transform_floats;
};

struct BuildProfile {
    const char* Name;
    cameraunlock::memory::PeFingerprint Fingerprint;
    OffsetTable Offsets;
};

// A profile with no transform offset is a placeholder landed ahead of the
// rederive: the fingerprint routes, but the mod must stay dormant.
inline bool IsProfileComplete(const BuildProfile& p) {
    return p.Offsets.camera_out_transform != 0
        && p.Offsets.camera_out_transform_floats != 0;
}

}  // namespace ace_ht::builds
