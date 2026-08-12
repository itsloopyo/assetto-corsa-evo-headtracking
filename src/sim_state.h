#pragma once

namespace ace_ht {

// Whether the simulation is actually running, read from the shared memory page
// the engine publishes for telemetry apps ("Local\acevo_pmf_graphics", the
// Assetto Corsa lineage's classic layout). The page opens with
// { int packetId; int status; } and the engine writes status as 1 for a
// replay, 2 for a live session and 3 while that session is paused, zeroing the
// page when no session is running.
enum class SimStatus {
    // The page is not mapped, or holds a value this mod does not know. Head
    // tracking keeps running: the mod's job is to track, and refusing to
    // because a telemetry page is missing would be worse than not gating.
    Unknown = -1,
    Off = 0,
    Replay = 1,
    Live = 2,
    Paused = 3,
};

inline SimStatus SimStatusFromRaw(int raw) {
    switch (raw) {
        case 0: return SimStatus::Off;
        case 1: return SimStatus::Replay;
        case 2: return SimStatus::Live;
        case 3: return SimStatus::Paused;
        default: return SimStatus::Unknown;
    }
}

// A paused session still renders the cockpit behind the menu, so without this
// the view kept following the player's head while they were in the pause menu.
// A replay renders a real 3D view the player is watching, so tracking belongs
// there as much as it does in a live session.
inline bool IsSimRunning(SimStatus status) {
    return status == SimStatus::Live
        || status == SimStatus::Replay
        || status == SimStatus::Unknown;
}

const char* SimStatusName(SimStatus status);

// Reads the page, mapping it on first use. The engine creates it when a
// session starts, so a failed open is retried (rate limited) rather than being
// fatal. Call from the render thread.
SimStatus ReadSimStatus();

void CloseSimState();

}  // namespace ace_ht
