#include "headtracking_mod.h"

#include <windows.h>

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(module);
        ace_ht::Initialize();
        break;
    case DLL_PROCESS_DETACH:
        // Only run the full shutdown on an explicit FreeLibrary (lpReserved
        // null). On process exit the kernel has already killed other threads
        // without unwinding, so joining/locking here could deadlock - let the
        // OS reclaim everything.
        if (lpReserved == nullptr) {
            ace_ht::Shutdown();
        }
        break;
    }
    return TRUE;
}
