#pragma once

#include <string>
#include <tuple>
#include <vector>

#include "cameraunlock/data/position_settings.h"
#include "cameraunlock/data/tracking_pose.h"

// The oracle: what v1.1.3, the newest published build, ran on after reading
// HeadTracking.ini. oracle_reader.cpp compiles v1.1.3's reader from byte copies
// and transcribes the startup code that consumed it.
namespace ace_oracle {

enum Action { kToggle = 0, kCycleMode = 1 };

// One HotkeyPoller registration: the action, the code, and 3 where the callback
// is ChordGuarded (fires only while Ctrl and Shift are both held), 0 where it is
// NavGuarded (fires unless Ctrl and Shift are both held).
using Registration = std::tuple<int, int, unsigned>;

// TrackingMode's numbers.
enum Mode { kRotationAndPosition = 0, kRotationOnly = 1, kPositionOnly = 2 };

struct Published {
    unsigned udp_port = 0;
    bool tracking_enabled = false;
    // What ApplyConfigToPipeline handed the processor, the position processor
    // and the session.
    cameraunlock::SensitivitySettings sensitivity;
    cameraunlock::PositionSettings position;
    float local_smoothing = 0;
    float remote_smoothing = 0;
    int mode = -1;
    std::vector<Registration> hotkeys;
};

// `exe_dir` is the game folder, as ExeDirectory gave it, with no trailing
// backslash.
Published Read(const std::string& exe_dir);

// What v1.1.3's WriteDefaultConfigIfMissing writes into `exe_dir` on a first
// start with no HeadTracking.ini there.
void WriteFirstRunFile(const std::string& exe_dir);

}  // namespace ace_oracle
