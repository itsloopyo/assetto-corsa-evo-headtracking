#pragma once

#include <cstdint>
#include <string>

namespace ace_ht {

struct Config {
    // Held as the socket's own type so an out-of-range INI value cannot reach
    // UdpReceiver::Start by silently truncating to a wrong 16-bit port.
    std::uint16_t udp_port = 4242;
    bool enable_on_startup = true;

    // Virtual key codes. Every action has a nav-cluster key and a
    // Ctrl+Shift+<key> chord, and both fire it - the chord is there for
    // keyboards with no nav cluster. Defaults: End, Page Up, and the Y/G
    // chord letters.
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

    // Smoothing is chosen per connection from the packet's source address, and
    // both values cover rotation and position alike. A tracker running on this
    // machine is already steady, so local_smoothing is 0.0 and nothing floors
    // it; a phone on WiFi jitters over the network, which is what
    // remote_smoothing is for.
    float local_smoothing = 0.0f;
    float remote_smoothing = 0.15f;

    bool position_enabled = true;
    float position_sensitivity_x = 1.0f;
    float position_sensitivity_y = 1.0f;
    float position_sensitivity_z = 1.0f;
    bool invert_position_x = false;
    bool invert_position_y = false;
    bool invert_position_z = false;
    float limit_x = 0.30f;
    float limit_y = 0.20f;
    float limit_z = 0.40f;
    float limit_z_back = 0.10f;
};

// Reads HeadTracking.ini from `exe_dir` over `out`. Keys that are absent, or
// whose value the boundary checks in config_sanitize.h reject, leave the
// corresponding member of `out` at whatever it already held - so passing a
// default-constructed Config yields the shipped defaults.
void LoadConfig(const std::string& exe_dir, Config& out);

// Writes the documented default HeadTracking.ini into `exe_dir`, unless one is
// already there. Never overwrites a user's file.
void WriteDefaultConfigIfMissing(const std::string& exe_dir);

}  // namespace ace_ht
