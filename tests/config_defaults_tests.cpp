// The defaults a user gets exist in three places: the Config struct's member
// initialisers (what the mod falls back to), the default HeadTracking.ini the
// mod writes on first run, and the reference HeadTracking.ini committed at the
// repo root. Nothing forces them to agree, and a drift between them is silent -
// the mod would behave one way and the documented file would say another.
//
// This locks all three together by round-tripping the real generator and the
// real loader.

#include "config.h"

#include "test_support.h"

#include <windows.h>

#include <cstdio>
#include <string>

using namespace ace_ht;
using ace_test::Check;
using ace_test::CheckClose;

namespace {

std::string MakeTempDir() {
    char temp[MAX_PATH]{};
    const DWORD n = GetTempPathA(MAX_PATH, temp);
    if (n == 0 || n >= MAX_PATH) {
        std::printf("  FAIL: GetTempPathA failed (%lu)\n", GetLastError());
        ++ace_test::g_failures;
        return {};
    }
    std::string dir = std::string(temp, n) + "ace-ht-config-test-"
                    + std::to_string(GetCurrentProcessId());
    CreateDirectoryA(dir.c_str(), nullptr);
    return dir;
}

void RemoveTempDir(const std::string& dir) {
    DeleteFileA((dir + "\\HeadTracking.ini").c_str());
    RemoveDirectoryA(dir.c_str());
}

// Every field of Config, compared against a default-constructed one. Loading a
// file that says exactly what the defaults say must leave the struct untouched.
void CheckMatchesDefaults(const Config& cfg, const char* source) {
    const Config defaults;
    std::printf("%s\n", source);

    Check(cfg.udp_port == defaults.udp_port, "UdpPort");
    Check(cfg.enable_on_startup == defaults.enable_on_startup, "EnableOnStartup");

    Check(cfg.recenter_key == defaults.recenter_key, "RecenterKey");
    Check(cfg.toggle_key == defaults.toggle_key, "ToggleKey");
    Check(cfg.cycle_mode_key == defaults.cycle_mode_key, "CycleModeKey");
    Check(cfg.chord_recenter_key == defaults.chord_recenter_key, "ChordRecenterKey");
    Check(cfg.chord_toggle_key == defaults.chord_toggle_key, "ChordToggleKey");
    Check(cfg.chord_cycle_mode_key == defaults.chord_cycle_mode_key, "ChordCycleModeKey");

    CheckClose(cfg.yaw_sensitivity, defaults.yaw_sensitivity, "YawSensitivity");
    CheckClose(cfg.pitch_sensitivity, defaults.pitch_sensitivity, "PitchSensitivity");
    CheckClose(cfg.roll_sensitivity, defaults.roll_sensitivity, "RollSensitivity");
    Check(cfg.invert_yaw == defaults.invert_yaw, "InvertYaw");
    Check(cfg.invert_pitch == defaults.invert_pitch, "InvertPitch");
    Check(cfg.invert_roll == defaults.invert_roll, "InvertRoll");
    CheckClose(cfg.local_smoothing, defaults.local_smoothing, "LocalSmoothing");
    CheckClose(cfg.remote_smoothing, defaults.remote_smoothing, "RemoteSmoothing");

    Check(cfg.position_enabled == defaults.position_enabled, "Position Enabled");
    CheckClose(cfg.position_sensitivity_x, defaults.position_sensitivity_x, "SensitivityX");
    CheckClose(cfg.position_sensitivity_y, defaults.position_sensitivity_y, "SensitivityY");
    CheckClose(cfg.position_sensitivity_z, defaults.position_sensitivity_z, "SensitivityZ");
    Check(cfg.invert_position_x == defaults.invert_position_x, "InvertX");
    Check(cfg.invert_position_y == defaults.invert_position_y, "InvertY");
    Check(cfg.invert_position_z == defaults.invert_position_z, "InvertZ");
    CheckClose(cfg.limit_x, defaults.limit_x, "LimitX");
    CheckClose(cfg.limit_y, defaults.limit_y, "LimitY");
    CheckClose(cfg.limit_z, defaults.limit_z, "LimitZ");
    CheckClose(cfg.limit_z_back, defaults.limit_z_back, "LimitZBack");
}

void GeneratedDefaultsTests() {
    const std::string dir = MakeTempDir();
    if (dir.empty()) return;

    WriteDefaultConfigIfMissing(dir);

    const std::string path = dir + "\\HeadTracking.ini";
    std::printf("The generated default HeadTracking.ini\n");
    Check(GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES,
          "is written when none exists");

    // Deliberately not a default Config: if a key were missing from the
    // generated file the loader would leave these poisoned values in place and
    // the comparison below would catch it.
    Config cfg;
    cfg.udp_port = 5555;
    cfg.enable_on_startup = false;
    cfg.recenter_key = cfg.toggle_key = cfg.cycle_mode_key = 0x70;
    cfg.chord_recenter_key = cfg.chord_toggle_key = cfg.chord_cycle_mode_key = 0x71;
    cfg.yaw_sensitivity = cfg.pitch_sensitivity = cfg.roll_sensitivity = 9.0f;
    cfg.invert_yaw = cfg.invert_pitch = cfg.invert_roll = true;
    cfg.local_smoothing = 0.99f;
    cfg.remote_smoothing = 0.99f;
    cfg.position_enabled = false;
    cfg.position_sensitivity_x = cfg.position_sensitivity_y = cfg.position_sensitivity_z = 9.0f;
    cfg.invert_position_x = cfg.invert_position_y = cfg.invert_position_z = true;
    cfg.limit_x = cfg.limit_y = cfg.limit_z = cfg.limit_z_back = 9.0f;

    LoadConfig(dir, cfg);
    CheckMatchesDefaults(cfg, "The generated file loads back as the built-in defaults");

    // A second call must not clobber a file the user has since edited.
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), "w");
    Check(f != nullptr, "the test can rewrite the file");
    if (f) {
        std::fputs("[Network]\nUdpPort=5000\n", f);
        std::fclose(f);
    }
    WriteDefaultConfigIfMissing(dir);
    Config edited;
    LoadConfig(dir, edited);
    Check(edited.udp_port == 5000, "an existing HeadTracking.ini is never overwritten");

    RemoveTempDir(dir);
}

void ReferenceIniTests() {
    // ACE_SOURCE_DIR is the repo root, where the reference HeadTracking.ini
    // that ships as documentation lives.
    Config cfg;
    cfg.udp_port = 5555;
    cfg.local_smoothing = 0.99f;
    cfg.remote_smoothing = 0.99f;
    cfg.limit_x = 9.0f;
    cfg.recenter_key = cfg.toggle_key = cfg.cycle_mode_key = 0x70;
    cfg.chord_recenter_key = cfg.chord_toggle_key = cfg.chord_cycle_mode_key = 0x71;

    LoadConfig(ACE_SOURCE_DIR, cfg);
    CheckMatchesDefaults(cfg, "The reference HeadTracking.ini at the repo root");
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - config default tests\n");
    std::printf("=====================================================\n");
    GeneratedDefaultsTests();
    ReferenceIniTests();
    return ace_test::Summary("config defaults");
}
