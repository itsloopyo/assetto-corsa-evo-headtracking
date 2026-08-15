#include "config.h"

#include <windows.h>

#include <cstdlib>

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
    "; Controls (all remappable, see [Hotkeys]):\n"
    ";           Home / Ctrl+Shift+T   recenter\n"
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
    "RecenterKey=0x24\n"
    "ToggleKey=0x23\n"
    "CycleModeKey=0x21\n"
    "ChordRecenterKey=0x54\n"
    "ChordToggleKey=0x59\n"
    "ChordCycleModeKey=0x47\n\n"
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

// Virtual key codes are published as hex and that is how the shipped INI writes
// them, so that is how they are read: a bare 24 is 0x24, not 36. IniReader's
// ReadHex cannot tell an absent key from an unreadable one, and both matter
// here - the first is the common case, the second is a user who typed a key
// name and needs to be told it is codes only.
static bool ParseVirtualKey(const std::string& text, int& out) {
    const char* start = text.c_str();
    if (text.size() > 2 && start[0] == '0' && (start[1] == 'x' || start[1] == 'X')) {
        start += 2;
    }
    char* end = nullptr;
    const long value = std::strtol(start, &end, 16);
    if (end == start) return false;

    // The whole value has to be the code, not just the front of it. Half the
    // key names a user would try are made of hex digits - "End" reads as 0xE
    // and "Delete" as 0xDE, both perfectly bindable keys and neither the one
    // that was asked for. Only trailing space and a comment are allowed past
    // the number.
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    if (*end != '\0' && *end != ';' && *end != '#') return false;

    out = static_cast<int>(value);
    return true;
}

// A key the mod refused leaves that action on its previous binding rather than
// on nothing, so a mistyped code costs the user one hotkey and says so.
static int ReadKey(const cameraunlock::IniReader& ini, const char* key, int fallback) {
    const std::string text = ini.ReadString("Hotkeys", key, "");
    if (text.empty()) return fallback;

    int raw = 0;
    if (!ParseVirtualKey(text, raw)) {
        Log::Line("[config] %s=%s is not a virtual key code (0x24, or 24 read as hex); "
                  "keeping 0x%X", key, text.c_str(), fallback);
        return fallback;
    }
    if (!IsBindableVirtualKey(raw)) {
        Log::Line("[config] %s=%s is not a key that can be bound; keeping 0x%X",
                  key, text.c_str(), fallback);
        return fallback;
    }
    return raw;
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

    out.recenter_key          = ReadKey(ini, "RecenterKey",       out.recenter_key);
    out.toggle_key            = ReadKey(ini, "ToggleKey",         out.toggle_key);
    out.cycle_mode_key        = ReadKey(ini, "CycleModeKey",      out.cycle_mode_key);
    out.chord_recenter_key    = ReadKey(ini, "ChordRecenterKey",  out.chord_recenter_key);
    out.chord_toggle_key      = ReadKey(ini, "ChordToggleKey",    out.chord_toggle_key);
    out.chord_cycle_mode_key  = ReadKey(ini, "ChordCycleModeKey", out.chord_cycle_mode_key);

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
