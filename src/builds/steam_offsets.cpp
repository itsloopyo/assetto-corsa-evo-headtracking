#include "builds/build_profile.h"

// Every Steam build profile lives here, append-only. Never edit an existing
// profile's numbers to "fix" a patch and never delete one: a user who has held
// back on an older build must keep matching their old profile from the same
// mod binary. Adding a profile is the only correct response to a patch.

namespace ace_ht::builds {

// Assetto Corsa EVO, Steam app 3058630, EXE built 2026-08-27 15:26:28 UTC
// (Steam build 24989348). SizeOfImage shrank by 0x13000 and every camera
// compute body moved again, so the fingerprint stopped matching. Re-verified
// against this EXE: all five concrete DrivableCamera vtables still hold the
// compute method at slot 2 and still copy 16 floats to out+0x2C.
extern const BuildProfile kSteamProfile_20260827 = {
    "steam-win64-20260827",
    { 0x6A905724, 0x06CDD000, 0x00000000 },
    {
        /* camera_compute_slot        */ 2,
        /* camera_out_transform       */ 0x2C,
        /* camera_out_transform_floats*/ 16,
    },
};

// Assetto Corsa EVO, Steam app 3058630, EXE built 2026-08-25 12:36:36 UTC
// (Steam build 24928989). The patch moved every camera compute body and grew
// SizeOfImage by 0x9F0000, so the fingerprint stopped matching and the mod
// went dormant. The pinned interface itself did not move: all five concrete
// DrivableCamera vtables still expose six slots with the compute method at
// slot 2, and each still copies 16 floats to out+0x2C.
extern const BuildProfile kSteamProfile_20260825 = {
    "steam-win64-20260825",
    { 0x6A8D8C54, 0x06CF0000, 0x00000000 },
    {
        /* camera_compute_slot        */ 2,
        /* camera_out_transform       */ 0x2C,
        /* camera_out_transform_floats*/ 16,
    },
};

// Assetto Corsa EVO, Steam app 3058630, EXE built 2026-07-22 09:02:23 UTC.
// CheckSum is 0 in the shipped EXE (the linker did not stamp one), so the
// fingerprint leans on TimeDateStamp + SizeOfImage.
extern const BuildProfile kSteamProfile_20260722 = {
    "steam-win64-20260722",
    { 0x6A60871F, 0x06300000, 0x00000000 },
    {
        /* camera_compute_slot        */ 2,
        /* camera_out_transform       */ 0x2C,
        /* camera_out_transform_floats*/ 16,
    },
};

}  // namespace ace_ht::builds
