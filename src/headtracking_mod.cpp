#include "headtracking_mod.h"

#include <windows.h>

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>

#include "builds/build_registry.h"
#include "camera_hook.h"
#include "camera_transform.h"
#include "config.h"
#include "exe_paths.h"
#include "logging.h"
#include "sim_state.h"

#include "cameraunlock/input/chord_hotkeys.h"
#include "cameraunlock/input/hotkey_poller.h"
#include "cameraunlock/protocol/udp_receiver.h"
#include "cameraunlock/time/frame_clock.h"
#include "cameraunlock/tracking/head_tracking_session.h"

namespace ace_ht {

using Session = cameraunlock::HeadTrackingSession<cameraunlock::UdpReceiver>;
static_assert(Session::kHasRemoteRecenter,
              "UdpReceiver must forward TryConsumeRecenterRequest for tracker-app recentering");

static Config g_config;
static cameraunlock::UdpReceiver g_receiver;
static Session g_session(g_receiver);
static cameraunlock::input::HotkeyPoller g_hotkeys;
static cameraunlock::time::FrameClock g_frameClock;

static std::atomic<bool> g_trackingEnabled{false};
static std::atomic<bool> g_active{false};

// Read on the render path, written from the hotkey thread. Seeded from the INI
// at boot; the toggle moves it for this session only, so the file stays the
// authority on what the mod comes up in.
static std::atomic<bool> g_worldSpaceYaw{true};

static std::atomic<long long> g_frameCounter{0};

// Enough of the engine's camera transform to confirm in a bug report that the
// hook fires and that the matrix still looks like a camera-to-world transform
// (row 3 holding world-scale translation) after a game patch. Bounded because
// this runs on the render path.
constexpr long long kDiagnosticFrames = 3;

// Translation only, from the INI-backed Config into the core pipeline's own
// settings types. Both arguments are explicit rather than reaching for the
// file statics, so this reads as - and can be reasoned about as - a mapping
// with no other reach into the mod's state.
static void ApplyConfigToPipeline(const Config& config, Session& session) {
    cameraunlock::SensitivitySettings sensitivity;
    sensitivity.yaw = config.yaw_sensitivity;
    sensitivity.pitch = config.pitch_sensitivity;
    sensitivity.roll = config.roll_sensitivity;
    sensitivity.invert_yaw = config.invert_yaw;
    sensitivity.invert_pitch = config.invert_pitch;
    sensitivity.invert_roll = config.invert_roll;

    auto& proc = session.GetProcessor();
    proc.SetSensitivity(sensitivity);
    proc.SetSmoothing(config.smoothing);

    cameraunlock::PositionSettings position(
        config.position_sensitivity_x,
        config.position_sensitivity_y,
        config.position_sensitivity_z,
        config.limit_x, config.limit_y, config.limit_z, config.limit_z_back,
        config.position_smoothing,
        config.invert_position_x, config.invert_position_y, config.invert_position_z);
    session.GetPositionProcessor().SetSettings(position);

    session.SetMode(config.position_enabled ? cameraunlock::TrackingMode::RotationAndPosition
                                            : cameraunlock::TrackingMode::RotationOnly);
}

static void Recenter() {
    g_session.Recenter();
    Log::Line("[input] recenter");
}

static void ToggleTracking() {
    const bool on = !g_trackingEnabled.load();
    g_trackingEnabled.store(on);
    Log::Line("[input] tracking %s", on ? "enabled" : "disabled");
}

static void CycleTrackingMode() {
    const char* name = "";
    switch (g_session.CycleMode()) {
        case cameraunlock::TrackingMode::RotationAndPosition: name = "rotation and position"; break;
        case cameraunlock::TrackingMode::RotationOnly:        name = "rotation only"; break;
        case cameraunlock::TrackingMode::PositionOnly:        name = "position only"; break;
    }
    Log::Line("[input] tracking mode: %s", name);
}

static void ToggleYawMode() {
    const bool world = !g_worldSpaceYaw.load();
    g_worldSpaceYaw.store(world);
    Log::Line("[input] yaw mode: %s", world ? "world (horizon-locked)" : "camera-local");
}

// Every action is reachable two ways: its nav-cluster key, and the
// Ctrl+Shift+<letter> chord for keyboards without a nav cluster. Pairing them
// in one row is what keeps the two lists from drifting apart - a new action
// cannot pick up a nav key and silently miss its chord.
struct HotkeyBinding {
    int nav_key;
    int chord_key;
    void (*action)();
};

// GetKeyNameText wants the scan code in bits 16-23 and the extended-key flag in
// bit 24. The nav cluster, the arrows and a few others are extended keys, and
// without that bit they name their numpad twins - a recenter left on Home would
// report itself in the log as "Num 7", which is the one thing this line exists
// to get right now that the key is the user's to choose.
static bool IsExtendedKey(int vk) {
    switch (vk) {
        case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE: case VK_NUMLOCK: case VK_SNAPSHOT:
            return true;
        default:
            return false;
    }
}

static std::string HotkeyName(int vk) {
    const UINT scan = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    if (scan) {
        LONG lparam = static_cast<LONG>(scan) << 16;
        if (IsExtendedKey(vk)) lparam |= 1L << 24;
        char name[64]{};
        if (GetKeyNameTextA(lparam, name, sizeof(name)) > 0) return name;
    }
    // A key this layout has no name for still has to be identifiable, and the
    // code is what the user typed into the INI.
    char code[8]{};
    std::snprintf(code, sizeof(code), "0x%02X", vk);
    return code;
}

static void RegisterHotkeys(const Config& config) {
    using namespace cameraunlock::input;

    const HotkeyBinding bindings[] = {
        { config.recenter_key,   config.chord_recenter_key,   Recenter },
        { config.toggle_key,     config.chord_toggle_key,     ToggleTracking },
        { config.cycle_mode_key, config.chord_cycle_mode_key, CycleTrackingMode },
        { config.yaw_mode_key,   config.chord_yaw_mode_key,   ToggleYawMode },
    };

    for (const HotkeyBinding& binding : bindings) {
        g_hotkeys.AddHotkey(binding.nav_key, NavGuarded(binding.action));
        g_hotkeys.AddHotkey(binding.chord_key, ChordGuarded(binding.action));
    }

    g_hotkeys.Start();
}

static void Bootstrap() {
    std::wstring exeDirWide;
    std::string exeDir;
    const bool haveExeDir = ExeDirectory(exeDirWide, exeDir);

    // Beside the game EXE, not in the process working directory: a launcher can
    // start the game from anywhere, and a bare relative name then drops the log
    // wherever that happens to be - or fails to create it at all - exactly when
    // a user is being asked to send one. An unresolved directory still needs
    // somewhere to say so before the mod goes dormant.
    Log::Open(haveExeDir ? exeDirWide + L"\\AssettoCorsaEvoHeadTracking.log"
                         : std::wstring(L"AssettoCorsaEvoHeadTracking.log"));
    Log::Line("=== Assetto Corsa EVO Head Tracking ===");

    if (!haveExeDir) {
        Log::Line("[boot] could not resolve the game directory - mod is dormant, game runs vanilla.");
        return;
    }
    Log::Line("[boot] game directory: %s", exeDir.c_str());

    if (builds::SelectProfile(GetModuleHandleW(nullptr)) != builds::ProfileSelection::Matched) {
        Log::Line("[boot] no usable build profile - mod is dormant, game runs vanilla.");
        return;
    }

    WriteDefaultConfigIfMissing(exeDir);
    LoadConfig(exeDir, g_config);
    Log::Line("[boot] config: port=%u enableOnStartup=%d smoothing=%.2f position=%d "
              "worldSpaceYaw=%d",
              static_cast<unsigned>(g_config.udp_port), g_config.enable_on_startup ? 1 : 0,
              g_config.smoothing, g_config.position_enabled ? 1 : 0,
              g_config.world_space_yaw ? 1 : 0);

    ApplyConfigToPipeline(g_config, g_session);
    g_trackingEnabled.store(g_config.enable_on_startup);
    g_worldSpaceYaw.store(g_config.world_space_yaw);

    g_receiver.SetLog([](const std::string& msg) { Log::Line("[udp] %s", msg.c_str()); });
    if (g_receiver.Start(g_config.udp_port)) {
        Log::Line("[boot] listening for OpenTrack data on UDP %u",
                  static_cast<unsigned>(g_config.udp_port));
    }

    if (!InstallCameraHook()) {
        // Nothing will ever read the tracker now, so give the port back rather
        // than sitting on 4242 for the rest of the session and blocking
        // whatever else the user points their tracker at.
        g_receiver.Stop();
        Log::Line("[boot] no camera vtable could be patched - mod is inert.");
        return;
    }

    RegisterHotkeys(g_config);
    g_active.store(true);
    Log::Line("[boot] ready. %s/Ctrl+Shift+%s recenter, %s/Ctrl+Shift+%s toggle tracking, "
              "%s/Ctrl+Shift+%s cycle tracking mode, %s/Ctrl+Shift+%s toggle yaw mode.",
              HotkeyName(g_config.recenter_key).c_str(),
              HotkeyName(g_config.chord_recenter_key).c_str(),
              HotkeyName(g_config.toggle_key).c_str(),
              HotkeyName(g_config.chord_toggle_key).c_str(),
              HotkeyName(g_config.cycle_mode_key).c_str(),
              HotkeyName(g_config.chord_cycle_mode_key).c_str(),
              HotkeyName(g_config.yaw_mode_key).c_str(),
              HotkeyName(g_config.chord_yaw_mode_key).c_str());
}

static void LogSimStatusChange(SimStatus status) {
    static SimStatus lastLogged = SimStatus::Unknown;
    if (status == lastLogged) return;
    lastLogged = status;
    Log::Line("[sim] session %s - head tracking %s", SimStatusName(status),
              IsSimRunning(status) ? "following" : "held");
}

static void LogFirstFrames(long long frame, const float* transform, bool haveRotation,
                           const HeadPose& pose) {
    if (frame >= kDiagnosticFrames) return;

    Log::Line("[camera] frame %lld  tracker=%s  yaw=%.2f pitch=%.2f roll=%.2f  lean=%.3f %.3f %.3f",
              frame, haveRotation ? "yes" : "no", pose.yaw, pose.pitch, pose.roll,
              pose.lean_x, pose.lean_y, pose.lean_z);
    for (unsigned row = 0; row < kCameraTransformFloats / 4; ++row) {
        Log::Line("[camera]   m[%u] % 14.5f % 14.5f % 14.5f % 14.5f", row,
                  transform[row * 4 + 0], transform[row * 4 + 1],
                  transform[row * 4 + 2], transform[row * 4 + 3]);
    }
}

void OnCameraTransformComputed(float* transform) {
    if (!g_active.load(std::memory_order_relaxed)) return;

    // Several drivable cameras can compute in the same frame. Splitting a
    // frame's delta across those calls is harmless: the smoothing and
    // interpolation are both exponential in dt, so the total advance per frame
    // is the same whether it arrives in one step or several.
    const float dt = g_frameClock.Tick();

    // Freezing the pipeline rather than skipping the whole hook is what holds
    // the view where the player left it: GetRotation keeps reporting the last
    // processed pose, so the pause menu neither follows the head nor snaps back
    // to the car's own camera, and resuming smooths out of the held pose
    // instead of jumping to wherever the head wandered during the menu.
    const SimStatus status = ReadSimStatus();
    LogSimStatusChange(status);
    if (IsSimRunning(status)) {
        g_session.Update(dt);
    }

    const long long frame = g_frameCounter.fetch_add(1, std::memory_order_relaxed);

    HeadPose pose;
    const bool haveRotation = g_session.GetRotation(pose.yaw, pose.pitch, pose.roll);
    g_session.GetPositionOffset(pose.lean_x, pose.lean_y, pose.lean_z);

    LogFirstFrames(frame, transform, haveRotation, pose);

    // No tracker data, or tracking toggled off: leave the engine's camera
    // exactly as it computed it.
    if (!haveRotation || !g_trackingEnabled.load(std::memory_order_relaxed)) return;

    ApplyHeadPose(transform, pose,
                  g_worldSpaceYaw.load(std::memory_order_relaxed) ? YawMode::WorldSpace
                                                                  : YawMode::CameraLocal);
}

void Initialize() {
    // Detached: DllMain runs under the loader lock, so the bootstrap (which
    // opens a log, reads the INI and resolves RTTI) cannot run here.
    std::thread(Bootstrap).detach();
}

void Shutdown() {
    g_active.store(false);
    UninstallCameraHook();
    g_hotkeys.Stop();
    g_receiver.Stop();
    CloseSimState();
    Log::Line("[boot] shutdown");
    Log::Close();
}

}  // namespace ace_ht
