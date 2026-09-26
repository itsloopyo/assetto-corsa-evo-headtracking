#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The pre-canonical HeadTracking.ini reader, frozen. It reads a file the way
// v1.1.3, the last build before the canonical config format, did, so a
// player's old file is carried over as that build read it. Never edit anything
// in this folder: tests/config_differential/ pins every file here by hash.
//
// Frozen from src/config.cpp, src/config.h and src/config_sanitize.h at
// 34cb1c9, which hold v1.1.3's bytes, with three changes: it fills this frozen
// copy of that commit's Config and its defaults instead of the mod's own, it
// writes nothing (the first-run WriteDefaultConfigIfMissing is not part of it),
// and it lives in namespace ace_ht::legacy. It also takes the file's full path
// where LoadConfig took the game folder and appended HeadTracking.ini, and
// reports whether the file opened. The defaults are the literals the code held
// then (cameraunlock-core's PositionSettings limits and smoothing defaults), so
// a later change to core cannot move what an old file means.
namespace ace_ht::legacy {

struct Config {
    std::uint16_t udp_port = 4242;
    bool enable_on_startup = true;

    // Virtual key codes. The nav-cluster key fires with Ctrl and Shift not both
    // held (NavGuarded), the chord key only while both are (ChordGuarded).
    int toggle_key = 0x23;
    int cycle_mode_key = 0x21;
    int chord_toggle_key = 0x59;
    int chord_cycle_mode_key = 0x47;

    float yaw_sensitivity = 1.0f;
    float pitch_sensitivity = 1.0f;
    float roll_sensitivity = 1.0f;
    bool invert_yaw = false;
    bool invert_pitch = false;
    bool invert_roll = false;

    float local_smoothing = 0.0f;
    float remote_smoothing = 0.15f;

    // Chose the startup mode, rotation and position or rotation only. The mode
    // hotkey cycled through all three modes either way.
    bool position_enabled = true;
    float position_sensitivity_x = 1.0f;
    float position_sensitivity_y = 1.0f;
    float position_sensitivity_z = 1.0f;
    bool invert_position_x = false;
    bool invert_position_y = false;
    bool invert_position_z = false;
    // LimitY set both vertical bounds, up and down (PositionSettings::Symmetric).
    float limit_x = 0.30f;
    float limit_y = 0.20f;
    float limit_z = 0.40f;
    float limit_z_back = 0.10f;
};

// Reads the file at `ini_path` over `out`. Keys that are absent, or whose value
// the boundary checks in config_sanitize.h reject, leave the corresponding
// member of `out` at whatever it already held. Returns whether the file opened;
// when it did not, `out` is untouched.
bool LoadConfig(const std::string& ini_path, Config& out);

struct Key {
    const char* section;
    const char* key;
};

// Every key LoadConfig takes a value from. The retired [Rotation] Smoothing and
// [Position] Smoothing are read only to warn that they are ignored, so they are
// not among them.
std::vector<Key> ReadKeys();

}  // namespace ace_ht::legacy
