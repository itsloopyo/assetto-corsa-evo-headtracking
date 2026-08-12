#pragma once

namespace ace_ht {

void Initialize();
void Shutdown();

// Called from the camera detour once the engine has computed a camera for this
// frame. `transform` points at the freshly written camera transform inside the
// engine's output struct; InstallCameraHook guarantees it is
// kCameraTransformFloats long (see camera_transform.h).
void OnCameraTransformComputed(float* transform);

}  // namespace ace_ht
