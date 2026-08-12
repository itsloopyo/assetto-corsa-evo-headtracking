#include "sim_state.h"

#include <windows.h>

#include "logging.h"

namespace ace_ht {

// Created by the engine's SharedMemoryWriter with a "Local\" prefix, so the
// name resolves within our own session - which is the game's, since this runs
// inside the game process.
static constexpr char kGraphicsPageName[] = "Local\\acevo_pmf_graphics";

// Only the first two ints are read (packetId, status). The page itself is
// 0x1324 bytes; mapping just the header keeps the mod out of the rest of it.
static constexpr SIZE_T kHeaderBytes = 8;
static constexpr unsigned kStatusIndex = 1;

// The page does not exist until a session starts, and the hook runs long
// before that. Retrying every frame would put an OpenFileMapping on the render
// path for the whole time the player is in the menus.
static constexpr ULONGLONG kRetryIntervalMs = 1000;

static HANDLE g_mapping = nullptr;
static const volatile int* g_header = nullptr;
static ULONGLONG g_nextRetryTick = 0;

static void TryMapPage() {
    const ULONGLONG now = GetTickCount64();
    if (now < g_nextRetryTick) return;
    g_nextRetryTick = now + kRetryIntervalMs;

    HANDLE mapping = OpenFileMappingA(FILE_MAP_READ, FALSE, kGraphicsPageName);
    if (mapping == nullptr) return;

    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, kHeaderBytes);
    if (view == nullptr) {
        Log::Line("[sim] %s opened but could not be mapped (%lu); head tracking will not "
                  "pause with the game.", kGraphicsPageName, GetLastError());
        CloseHandle(mapping);
        return;
    }

    g_mapping = mapping;
    g_header = static_cast<const volatile int*>(view);
    Log::Line("[sim] %s mapped; head tracking follows the session state.", kGraphicsPageName);
}

const char* SimStatusName(SimStatus status) {
    switch (status) {
        case SimStatus::Off:     return "off";
        case SimStatus::Replay:  return "replay";
        case SimStatus::Live:    return "live";
        case SimStatus::Paused:  return "paused";
        case SimStatus::Unknown: break;
    }
    return "unknown";
}

SimStatus ReadSimStatus() {
    if (g_header == nullptr) {
        TryMapPage();
        if (g_header == nullptr) return SimStatus::Unknown;
    }
    return SimStatusFromRaw(g_header[kStatusIndex]);
}

void CloseSimState() {
    if (g_header != nullptr) {
        UnmapViewOfFile(const_cast<int*>(g_header));
        g_header = nullptr;
    }
    if (g_mapping != nullptr) {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
}

}  // namespace ace_ht
