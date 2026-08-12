#pragma once

namespace ace_ht {

// Resolves the DrivableCamera family vtables by RTTI class name and patches
// the compute slot on each. Returns false when no camera vtable could be
// resolved (nothing is patched in that case).
bool InstallCameraHook();

void UninstallCameraHook();

}  // namespace ace_ht
