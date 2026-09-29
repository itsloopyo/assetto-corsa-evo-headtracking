#include "headtracking_mod.h"

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);

        // Pinned, so nothing can unload it. Five engine vtables point into this
        // module and the render thread can be inside a detour at any moment, so
        // unmapping it would crash the game; and the clean-up an unload needs
        // (joining the receiver and hotkey threads) cannot run under the loader
        // lock without deadlocking. With the module pinned, the only detach is
        // process exit, where the OS reclaims everything.
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                reinterpret_cast<LPCWSTR>(module), &pinned)) {
            return FALSE;
        }
        ace_ht::Initialize();
    }
    return TRUE;
}
