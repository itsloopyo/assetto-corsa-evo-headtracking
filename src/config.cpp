#include "config.h"

#include <windows.h>

#include "legacy_config/legacy_config.h"
#include "logging.h"

namespace ace_ht {

static constexpr char kIniName[] = "HeadTracking.ini";

// The file a fresh install lands with. Values here must stay in step with the
// Config struct's member initialisers - the config_defaults test locks that by
// generating this file and loading it back over a poisoned Config.
static constexpr char kDefaultIniText[] =
    "; Assetto Corsa EVO Head Tracking - configuration\n"
    "; Edit values, restart the game to apply.\n"
    ";\n"
    "; Controls (all remappable, see [Hotkeys]):\n"
    ";           End  / Ctrl+Shift+Y   toggle tracking\n"
    ";           PgUp / Ctrl+Shift+G   cycle tracking mode (rotation and position\n"
    ";                                 / rotation only / position only)\n\n"
    "[Network]\n"
    "UdpPort=4242\n\n"
    "[General]\n"
    "EnableOnStartup=1\n\n"
    "[Hotkeys]\n"
    "; Windows virtual key codes, in hex. Each action has a nav-cluster key and a\n"
    "; Ctrl+Shift+<key> chord, and both fire it - remap either or both.\n"
    "; Common codes: Home 0x24, End 0x23, Insert 0x2D, Delete 0x2E, PgUp 0x21,\n"
    "; PgDn 0x22, F1-F12 0x70-0x7B, A-Z 0x41-0x5A, numpad 0-9 0x60-0x69.\n"
    "ToggleKey=0x23\n"
    "CycleModeKey=0x21\n"
    "ChordToggleKey=0x59\n"
    "ChordCycleModeKey=0x47\n\n"
    "[Rotation]\n"
    "YawSensitivity=1.0\n"
    "PitchSensitivity=1.0\n"
    "RollSensitivity=1.0\n"
    "InvertYaw=0\n"
    "InvertPitch=0\n"
    "InvertRoll=0\n"
    "; Smoothing covers rotation and position alike, and the value used is picked\n"
    "; per connection from where the tracker sends from. 0.0 none .. 1.0 heavy.\n"
    "LocalSmoothing=0.0\n"
    "RemoteSmoothing=0.15\n\n"
    "[Position]\n"
    "Enabled=1\n"
    "SensitivityX=1.0\n"
    "SensitivityY=1.0\n"
    "SensitivityZ=1.0\n"
    "InvertX=0\n"
    "InvertY=0\n"
    "InvertZ=0\n"
    "LimitX=0.30\n"
    "LimitY=0.20\n"
    "LimitZ=0.40\n"
    "LimitZBack=0.10\n";

static std::string IniPath(const std::string& exe_dir) {
    return exe_dir + "\\" + kIniName;
}

Config LoadConfig(const std::string& exe_dir) {
    legacy::Config read;
    legacy::LoadConfig(IniPath(exe_dir), read);

    Config out;
    out.udp_port = read.udp_port;
    out.enable_on_startup = read.enable_on_startup;
    out.toggle_key = read.toggle_key;
    out.cycle_mode_key = read.cycle_mode_key;
    out.chord_toggle_key = read.chord_toggle_key;
    out.chord_cycle_mode_key = read.chord_cycle_mode_key;
    out.yaw_sensitivity = read.yaw_sensitivity;
    out.pitch_sensitivity = read.pitch_sensitivity;
    out.roll_sensitivity = read.roll_sensitivity;
    out.invert_yaw = read.invert_yaw;
    out.invert_pitch = read.invert_pitch;
    out.invert_roll = read.invert_roll;
    out.local_smoothing = read.local_smoothing;
    out.remote_smoothing = read.remote_smoothing;
    out.position_enabled = read.position_enabled;
    out.position_sensitivity_x = read.position_sensitivity_x;
    out.position_sensitivity_y = read.position_sensitivity_y;
    out.position_sensitivity_z = read.position_sensitivity_z;
    out.invert_position_x = read.invert_position_x;
    out.invert_position_y = read.invert_position_y;
    out.invert_position_z = read.invert_position_z;
    out.limit_x = read.limit_x;
    out.limit_y = read.limit_y;
    out.limit_z = read.limit_z;
    out.limit_z_back = read.limit_z_back;
    return out;
}

void WriteDefaultConfigIfMissing(const std::string& exe_dir) {
    const std::string path = IniPath(exe_dir);

    // CREATE_NEW rather than "does it exist?" followed by a truncating open: the
    // two steps can straddle a file the user (or a second launch) writes in
    // between, and never overwriting a user's config is the whole promise here.
    const HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_EXISTS) return;
        Log::Line("[config] could not create %s (%lu) - the game directory is not writable. "
                  "Built-in defaults are in use and edits there will not be read.",
                  path.c_str(), error);
        return;
    }

    // A short write leaves a file that parses as a config but is missing keys,
    // which then reads as "the mod ignores my setting". Say so instead.
    constexpr DWORD kTextBytes = static_cast<DWORD>(sizeof(kDefaultIniText) - 1);
    DWORD written = 0;
    const BOOL ok = WriteFile(file, kDefaultIniText, kTextBytes, &written, nullptr);
    const DWORD writeError = GetLastError();
    CloseHandle(file);
    if (!ok || written != kTextBytes) {
        Log::Line("[config] %s was created but only %lu of %lu bytes could be written (%lu); "
                  "delete it and restart the game for a complete default config.",
                  path.c_str(), written, kTextBytes, writeError);
    }
}

}  // namespace ace_ht
