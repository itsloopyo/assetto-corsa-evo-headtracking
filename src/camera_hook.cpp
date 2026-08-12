#include "camera_hook.h"

#include <windows.h>

#include <cstdint>

#include "builds/build_registry.h"
#include "camera_transform.h"
#include "headtracking_mod.h"
#include "logging.h"

#include "cameraunlock/memory/rtti_vtable.h"

namespace ace_ht {

// void slot(Camera* this, float blend /*xmm1*/, void* out /*r8*/,
//           const void* base /*r9*/)
using ComputeFn = void(__fastcall*)(void*, float, void*, const void*);

// Read once at install time rather than per frame: the active profile cannot
// change while the hook is installed, and this sits on the render path.
static unsigned g_transformByteOffset = 0;

static void ApplyToOutput(void* out) {
    float* transform = reinterpret_cast<float*>(
        reinterpret_cast<std::uint8_t*>(out) + g_transformByteOffset);
    OnCameraTransformComputed(transform);
}

// One detour instantiation per hooked class, each holding its own original.
// A single shared detour would have to map `this`'s vtable back to the right
// original on every call, which puts a lookup - and an unreachable "not found"
// branch - in a per-frame path. This has neither.
template <int Slot>
struct CameraDetour {
    static ComputeFn original;
    static void __fastcall Thunk(void* self, float blend, void* out, const void* base) {
        original(self, blend, out, base);
        ApplyToOutput(out);
    }
};
template <int Slot> ComputeFn CameraDetour<Slot>::original = nullptr;

struct CameraClass {
    const char* name;
    void* detour;
    ComputeFn* original;
};

// The concrete driving views the engine derives from DrivableCamera. All five
// share the interface; the abstract base itself is skipped (its compute slot
// is _purecall).
//
// ExternalDampedCamera is the chase cam. It is easy to miss when enumerating
// the family by vtable shape: unlike its siblings it also overrides slots 4
// and 5, so a sweep keyed on the shared vtable tail skips it. Slot 2 is the
// same compute method and writes the same matrix to the same output offset.
static const CameraClass kCameraClasses[] = {
    { "CockpitCamera",        &CameraDetour<0>::Thunk, &CameraDetour<0>::original },
    { "DashCamera",           &CameraDetour<1>::Thunk, &CameraDetour<1>::original },
    { "BonnetCamera",         &CameraDetour<2>::Thunk, &CameraDetour<2>::original },
    { "ExternalFixedCamera",  &CameraDetour<3>::Thunk, &CameraDetour<3>::original },
    { "ExternalDampedCamera", &CameraDetour<4>::Thunk, &CameraDetour<4>::original },
};
static constexpr int kCameraClassCount =
    static_cast<int>(sizeof(kCameraClasses) / sizeof(kCameraClasses[0]));

// What uninstall needs to put the engine back exactly as it found it.
struct PatchedSlot {
    void** address;
    ComputeFn original;
    const char* className;
};
static PatchedSlot g_patched[kCameraClassCount];
static int g_patchedCount = 0;

// Vtables live in a read-only section, so both patching and restoring go
// through the same unprotect / write / reprotect. Returns false if the page
// could not be made writable, in which case nothing was written.
static bool WriteSlot(void** slot, void* value, const char* className) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
        Log::Line("[camera] %s: VirtualProtect failed (%lu)", className, GetLastError());
        return false;
    }
    *slot = value;
    VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
    return true;
}

static bool PatchSlot(const CameraClass& cls, int slotIndex) {
    cameraunlock::memory::VtableInfo info{};
    const int wanted = slotIndex + 1;
    if (!cameraunlock::memory::FindVtableFromRTTI(GetModuleHandleW(nullptr), cls.name,
                                                  info, wanted)) {
        Log::Line("[camera] %s: RTTI vtable not found", cls.name);
        return false;
    }
    if (info.vfunc_count < wanted) {
        Log::Line("[camera] %s: vtable exposes %d slots, need at least %d (slot index %d)",
                  cls.name, info.vfunc_count, wanted, slotIndex);
        return false;
    }

    void** slot = reinterpret_cast<void**>(info.vtable_address) + slotIndex;
    ComputeFn original = reinterpret_cast<ComputeFn>(*slot);

    // The detour calls through `original`, so that has to be published before
    // the slot points at the detour. The game renders while this runs: install
    // them the other way round and a camera computed in the window between the
    // two stores calls a null pointer.
    *cls.original = original;
    if (!WriteSlot(slot, cls.detour, cls.name)) return false;

    g_patched[g_patchedCount] = { slot, original, cls.name };
    ++g_patchedCount;

    Log::Line("[camera] %s: vtable 0x%p slot %d patched (original 0x%p)",
              cls.name, reinterpret_cast<void*>(info.vtable_address), slotIndex,
              reinterpret_cast<void*>(original));
    return true;
}

bool InstallCameraHook() {
    const builds::BuildProfile& profile = builds::ActiveProfile();
    if (profile.Offsets.camera_out_transform_floats != kCameraTransformFloats) {
        Log::Line("[camera] profile %s declares a %u-float camera transform, but this mod "
                  "composes a 4x4 (%u floats). Not patching.",
                  profile.Name, profile.Offsets.camera_out_transform_floats,
                  kCameraTransformFloats);
        return false;
    }
    g_transformByteOffset = profile.Offsets.camera_out_transform;

    const int slotIndex = profile.Offsets.camera_compute_slot;
    for (int i = 0; i < kCameraClassCount; ++i) {
        PatchSlot(kCameraClasses[i], slotIndex);
    }
    Log::Line("[camera] %d of %d camera vtables patched", g_patchedCount, kCameraClassCount);
    return g_patchedCount > 0;
}

void UninstallCameraHook() {
    for (int i = 0; i < g_patchedCount; ++i) {
        WriteSlot(g_patched[i].address, reinterpret_cast<void*>(g_patched[i].original),
                  g_patched[i].className);
    }
    g_patchedCount = 0;
}

}  // namespace ace_ht
