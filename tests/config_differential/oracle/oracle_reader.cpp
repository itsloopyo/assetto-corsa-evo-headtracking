// v1.1.3's reader and startup code, the newest published build (tag 5f4ec7b).
//
// The reader is compiled from byte copies: src/config.cpp, src/config.h,
// src/config_sanitize.h and src/logging.h beside this file are v1.1.3's files
// (git show v1.1.3:src/<file>), which CMakeLists.txt pins by hash. They are
// included inside namespace ace_oracle, after every header they include, so the
// published ace_ht::Config and ace_ht::LoadConfig become ace_oracle::ace_ht's
// and cannot collide with the mod's own. Every cameraunlock-core source they
// include holds the same bytes at v1.1.3's pin (f92be69) and at this repo's
// (CMakeLists.txt pins those too).
//
// The startup code is transcribed from v1.1.3:src/headtracking_mod.cpp, which
// hooks the game and cannot be compiled into a test:
//
//   lines 59-89     ApplyConfigToPipeline, recording what it handed on
//   lines 167-181   RegisterHotkeys, recording each AddHotkey
//   lines 216-217   the pipeline and the enabled flag at startup

#include "oracle_reader.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "cameraunlock/config/ini_reader.h"
#include "cameraunlock/data/position_settings.h"
#include "cameraunlock/logging/file_log.h"
#include "cameraunlock/math/smoothing_utils.h"
#include "cameraunlock/protocol/port_utils.h"

namespace ace_oracle {
#include "src/config.cpp"
}  // namespace ace_oracle

namespace ace_oracle {

Published Read(const std::string& exe_dir) {
    ace_ht::Config config;
    ace_ht::LoadConfig(exe_dir, config);

    Published p;
    p.udp_port = config.udp_port;

    // ApplyConfigToPipeline.
    p.sensitivity.yaw = config.yaw_sensitivity;
    p.sensitivity.pitch = config.pitch_sensitivity;
    p.sensitivity.roll = config.roll_sensitivity;
    p.sensitivity.invert_yaw = config.invert_yaw;
    p.sensitivity.invert_pitch = config.invert_pitch;
    p.sensitivity.invert_roll = config.invert_roll;
    p.position = cameraunlock::PositionSettings::Symmetric(
        config.position_sensitivity_x,
        config.position_sensitivity_y,
        config.position_sensitivity_z,
        config.limit_x, config.limit_y, config.limit_z, config.limit_z_back,
        config.local_smoothing, config.remote_smoothing,
        config.invert_position_x, config.invert_position_y, config.invert_position_z);
    p.local_smoothing = config.local_smoothing;
    p.remote_smoothing = config.remote_smoothing;
    p.mode = config.position_enabled ? kRotationAndPosition : kRotationOnly;

    p.tracking_enabled = config.enable_on_startup;

    // RegisterHotkeys: each action's nav key NavGuarded, its chord key
    // ChordGuarded.
    const struct { int nav_key; int chord_key; Action action; } bindings[] = {
        { config.toggle_key,     config.chord_toggle_key,     kToggle },
        { config.cycle_mode_key, config.chord_cycle_mode_key, kCycleMode },
    };
    for (const auto& binding : bindings) {
        p.hotkeys.emplace_back(binding.action, binding.nav_key, 0u);
        p.hotkeys.emplace_back(binding.action, binding.chord_key, 3u);
    }
    return p;
}

void WriteFirstRunFile(const std::string& exe_dir) {
    ace_ht::WriteDefaultConfigIfMissing(exe_dir);
}

}  // namespace ace_oracle
