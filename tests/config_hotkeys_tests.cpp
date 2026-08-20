// The nav cluster is prime real estate on a sim rig - a button box, a wheel
// plugin or the game's own binds may already be sitting on Home or Page Up - so
// both halves of every action are remappable. What matters at this boundary is
// that a value the mod cannot bind leaves the action on the key it already had
// rather than on nothing, which is the difference between one hotkey not moving
// and a hotkey silently disappearing.

#include "config.h"

#include "test_support.h"

#include <windows.h>

#include <cstdio>
#include <string>

using namespace ace_ht;
using ace_test::Check;

namespace {

std::string g_dir;

std::string IniPath() {
    return g_dir + "\\HeadTracking.ini";
}

bool MakeTempDir() {
    char temp[MAX_PATH]{};
    const DWORD n = GetTempPathA(MAX_PATH, temp);
    if (n == 0 || n >= MAX_PATH) {
        std::printf("  FAIL: GetTempPathA failed (%lu)\n", GetLastError());
        ++ace_test::g_failures;
        return false;
    }
    g_dir = std::string(temp, n) + "ace-ht-hotkey-test-"
          + std::to_string(GetCurrentProcessId());
    CreateDirectoryA(g_dir.c_str(), nullptr);
    return true;
}

void RemoveTempDir() {
    DeleteFileA(IniPath().c_str());
    RemoveDirectoryA(g_dir.c_str());
}

// Loads `body` as the whole INI over a Config seeded with the shipped defaults,
// so every check reads as "what did this file change".
Config Load(const char* body) {
    FILE* f = nullptr;
    fopen_s(&f, IniPath().c_str(), "w");
    if (!f) {
        std::printf("  FAIL: could not write %s\n", IniPath().c_str());
        ++ace_test::g_failures;
        return Config{};
    }
    std::fputs(body, f);
    std::fclose(f);

    Config cfg;
    LoadConfig(g_dir, cfg);
    return cfg;
}

void RemapTests() {
    std::printf("Remapping a hotkey\n");

    const Config both = Load("[Hotkeys]\nToggleKey=0x2D\nChordToggleKey=0x4B\n");
    Check(both.toggle_key == 0x2D && both.chord_toggle_key == 0x4B,
          "moves both halves of an action");
    Check(both.cycle_mode_key == Config{}.cycle_mode_key,
          "and leaves the actions the file did not name alone");

    const Config bare = Load("[Hotkeys]\nToggleKey=2E\n");
    Check(bare.toggle_key == 0x2E, "reads a code with no 0x prefix as hex");

    const Config cycle = Load("[Hotkeys]\nCycleModeKey=0x70\nChordCycleModeKey=0x4A\n");
    Check(cycle.cycle_mode_key == 0x70 && cycle.chord_cycle_mode_key == 0x4A,
          "reaches the cycle-mode action");
}

void RefusedValuesKeepThePreviousBindingTests() {
    std::printf("A value the mod cannot bind\n");
    const Config defaults;

    // Ctrl is what the chord guard tests, so an action bound to it would either
    // never fire or fire on every chord press.
    Check(Load("[Hotkeys]\nToggleKey=0x11\n").toggle_key == defaults.toggle_key,
          "a modifier leaves the action on its previous key");

    Check(Load("[Hotkeys]\nCycleModeKey=0\n").cycle_mode_key == defaults.cycle_mode_key,
          "0 is not a key, and leaves the action on its previous key");

    Check(Load("[Hotkeys]\nCycleModeKey=0x220\n").cycle_mode_key == defaults.cycle_mode_key,
          "a code past 0xFE leaves the action on its previous key");

    // A name is what a user reaches for first, and half of them are made of hex
    // digits: read as a code "End" is 0xE and "Delete" is 0xDE, both bindable
    // and neither the key that was asked for.
    Check(Load("[Hotkeys]\nChordToggleKey=Insert\n").chord_toggle_key
              == defaults.chord_toggle_key,
          "a key name is refused rather than read as a code");
    Check(Load("[Hotkeys]\nToggleKey=Delete\n").toggle_key == defaults.toggle_key,
          "and a name that is all hex digits is refused too");

    Check(Load("[Hotkeys]\nCycleModeKey=0x2D ; Insert\n").cycle_mode_key == 0x2D,
          "a trailing comment is not junk, and the code is still read");

    Check(Load("[Hotkeys]\nToggleKey=\n").toggle_key == defaults.toggle_key,
          "an empty value leaves the action on its previous key");
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - hotkey remap tests\n");
    std::printf("===================================================\n");
    if (!MakeTempDir()) return ace_test::Summary("hotkey remap");
    RemapTests();
    RefusedValuesKeepThePreviousBindingTests();
    RemoveTempDir();
    return ace_test::Summary("hotkey remap");
}
