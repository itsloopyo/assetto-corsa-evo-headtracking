#include "config.h"

#include <windows.h>

#include "config_sanitize.h"
#include "logging.h"

#include "cameraunlock/config/ini_reader.h"
#include "cameraunlock/protocol/port_utils.h"

namespace ace_ht {

static constexpr char kIniName[] = "HeadTracking.ini";

// The file a fresh install lands with. Values here must stay in step with the
// Config struct's member initialisers - the config_defaults test locks that by
// generating this file and loading it back over a poisoned Config.
static constexpr char kDefaultIniText[] =
    "; Assetto Corsa EVO Head Tracking - configuration\n"
    "; Edit values, restart the game to apply.\n"
    ";\n"
    "; Controls: Home / Ctrl+Shift+T   recenter\n"
    ";           End  / Ctrl+Shift+Y   toggle tracking\n"
    ";           PgUp / Ctrl+Shift+G   cycle tracking mode (rotation and position\n"
    ";                                 / rotation only / position only)\n"
    ";           PgDn / Ctrl+Shift+H   toggle yaw mode (world / camera-local)\n\n"
    "[Network]\n"
    "UdpPort=4242\n\n"
    "[General]\n"
    "EnableOnStartup=1\n"
    "; 1 turns the head about the world's up axis, so the axis stays level with\n"
    "; the horizon through a banked corner. 0 turns it about the camera's own up\n"
    "; axis, so the view leans with the car.\n"
    "WorldSpaceYaw=1\n\n"
    "[Hotkeys]\n"
    "; Virtual key code for the yaw mode toggle. 0x22 is Page Down.\n"
    "YawModeKey=0x22\n\n"
    "[Rotation]\n"
    "YawSensitivity=1.0\n"
    "PitchSensitivity=1.0\n"
    "RollSensitivity=1.0\n"
    "InvertYaw=0\n"
    "InvertPitch=0\n"
    "InvertRoll=0\n"
    "Smoothing=0.0\n\n"
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
    "LimitZBack=0.10\n"
    "Smoothing=0.15\n";

static std::string IniPath(const std::string& exe_dir) {
    return exe_dir + "\\" + kIniName;
}

// A value the mod refused is exactly what a "my INI setting does nothing" bug
// report needs to show, so every substitution is logged rather than swallowed.
// A NaN raw value compares unequal to everything, including itself, so it
// takes this branch too.
static float UseSanitized(const char* name, float raw, float clean) {
    if (raw != clean) {
        Log::Line("[config] %s=%.4f is out of range or not finite; using %.4f",
                  name, raw, clean);
    }
    return clean;
}

static float ReadSensitivity(const cameraunlock::IniReader& ini, const char* section,
                             const char* key, float fallback) {
    const float raw = ini.ReadFloat(section, key, fallback);
    return UseSanitized(key, raw, SanitizeSensitivity(raw));
}

static float ReadSmoothing(const cameraunlock::IniReader& ini, const char* section,
                           const char* key, float fallback) {
    const float raw = ini.ReadFloat(section, key, fallback);
    return UseSanitized(key, raw, SanitizeSmoothing(raw));
}

static float ReadLimit(const cameraunlock::IniReader& ini, const char* key, float fallback) {
    const float raw = ini.ReadFloat("Position", key, fallback);
    return UseSanitized(key, raw, SanitizePositionLimit(raw, fallback));
}

void LoadConfig(const std::string& exe_dir, Config& out) {
    const std::string path = IniPath(exe_dir);
    cameraunlock::IniReader ini;
    if (!ini.Open(path)) {
        Log::Line("[config] could not open %s - using built-in defaults", path.c_str());
        return;
    }

    bool portValid = false;
    out.udp_port = cameraunlock::NormalizeUdpPort(
        ini.ReadInt("Network", "UdpPort", out.udp_port), out.udp_port, portValid);
    if (!portValid) {
        Log::Line("[config] UdpPort is outside 1024-65535; using %u",
                  static_cast<unsigned>(out.udp_port));
    }

    out.enable_on_startup  = ini.ReadBool ("General",  "EnableOnStartup",  out.enable_on_startup);
    out.world_space_yaw    = ini.ReadBool ("General",  "WorldSpaceYaw",    out.world_space_yaw);

    const int yawModeKey = ini.ReadHex("Hotkeys", "YawModeKey", out.yaw_mode_key);
    out.yaw_mode_key = SanitizeVirtualKey(yawModeKey, out.yaw_mode_key);
    if (yawModeKey != out.yaw_mode_key) {
        Log::Line("[config] YawModeKey=0x%X is not a virtual key code; using 0x%X",
                  yawModeKey, out.yaw_mode_key);
    }

    out.yaw_sensitivity    = ReadSensitivity(ini, "Rotation", "YawSensitivity",   out.yaw_sensitivity);
    out.pitch_sensitivity  = ReadSensitivity(ini, "Rotation", "PitchSensitivity", out.pitch_sensitivity);
    out.roll_sensitivity   = ReadSensitivity(ini, "Rotation", "RollSensitivity",  out.roll_sensitivity);
    out.invert_yaw         = ini.ReadBool ("Rotation", "InvertYaw",        out.invert_yaw);
    out.invert_pitch       = ini.ReadBool ("Rotation", "InvertPitch",      out.invert_pitch);
    out.invert_roll        = ini.ReadBool ("Rotation", "InvertRoll",       out.invert_roll);
    out.smoothing          = ReadSmoothing(ini, "Rotation", "Smoothing",   out.smoothing);

    out.position_enabled   = ini.ReadBool ("Position", "Enabled",          out.position_enabled);
    out.position_sensitivity_x = ReadSensitivity(ini, "Position", "SensitivityX", out.position_sensitivity_x);
    out.position_sensitivity_y = ReadSensitivity(ini, "Position", "SensitivityY", out.position_sensitivity_y);
    out.position_sensitivity_z = ReadSensitivity(ini, "Position", "SensitivityZ", out.position_sensitivity_z);
    out.invert_position_x  = ini.ReadBool ("Position", "InvertX",          out.invert_position_x);
    out.invert_position_y  = ini.ReadBool ("Position", "InvertY",          out.invert_position_y);
    out.invert_position_z  = ini.ReadBool ("Position", "InvertZ",          out.invert_position_z);
    out.limit_x            = ReadLimit(ini, "LimitX",     out.limit_x);
    out.limit_y            = ReadLimit(ini, "LimitY",     out.limit_y);
    out.limit_z            = ReadLimit(ini, "LimitZ",     out.limit_z);
    out.limit_z_back       = ReadLimit(ini, "LimitZBack", out.limit_z_back);
    out.position_smoothing = ReadSmoothing(ini, "Position", "Smoothing", out.position_smoothing);
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
