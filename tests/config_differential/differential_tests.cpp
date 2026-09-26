// The differential test for the conversion of HeadTracking.ini to the canonical
// config format.
//
// Two readings of every input:
//
//   Oracle   v1.1.3's reader and startup code, the newest published build
//            (oracle/oracle_reader.cpp; the repo publishes v* releases only)
//   Import   the frozen reader in src/legacy_config/, and the startup code of
//            the commit that froze it
//
// Comparison 1, oracle against import, is what a player sees change that the
// conversion did not cause: commits since v1.1.3 that change how the file is
// read. There are none. src/ was byte for byte v1.1.3's when the reader was
// frozen, the oracle compiles v1.1.3's reader from byte copies, and every core
// source either reader compiles holds the same bytes at v1.1.3's pin and at this
// repo's (CMakeLists.txt pins them), so the comparison may find no difference at
// all, floats bit for bit.
//
// Inputs: no build shipped HeadTracking.ini. Every build wrote it on its first
// start (WriteDefaultConfigIfMissing), so a player's file is one of those
// first-run outputs, edited or not: data/firstrun-*.ini holds each distinct one,
// extracted from kDefaultIniText at each tag. data/committed-*.ini holds every
// version of the HeadTracking.ini the repo keeps as the documented file, which
// the README told players to copy from. Then no file, an empty file, core's
// mutation corpus over v1.1.3's first-run output, and that file with ToggleKey
// and with ChordToggleKey set to every code from 0x01 to 0xFE.

#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "legacy_config/legacy_config.h"
#include "oracle/oracle_reader.h"

#include "cameraunlock/config/legacy_import.h"
#include "cameraunlock/config/testing/ini_mutations.h"

namespace {

namespace fs = std::filesystem;
namespace cfg = cameraunlock::config;
namespace legacy = ace_ht::legacy;
namespace testing = cameraunlock::config::testing;

int g_failures = 0;
int g_checks = 0;

void Check(bool ok, const std::string& what) {
    ++g_checks;
    if (ok) return;
    ++g_failures;
    std::printf("FAIL: %s\n", what.c_str());
}

std::string ReadFileBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path.string());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void WriteFileBytes(const fs::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out) throw std::runtime_error("cannot write " + path.string());
}

// ---- Scratch folders -----------------------------------------------------------
//
// One folder per reading: GetPrivateProfileString, which both readers sit on,
// is free to cache the file it last read. `game` stands for the folder
// AssettoCorsaEVO.exe is in. Every folder lives under one root for the run,
// removed once at the end.

void RemoveTree(const fs::path& root) {
    if (!fs::exists(root)) return;
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        SetFileAttributesW(entry.path().c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    fs::remove_all(root);
}

const fs::path& ScratchRoot() {
    static const fs::path root = [] {
        wchar_t temp[MAX_PATH + 1] = {};
        if (GetTempPathW(MAX_PATH + 1, temp) == 0) throw std::runtime_error("GetTempPathW failed");
        fs::path r = fs::path(temp) / ("ace_ht_diff_" + std::to_string(GetCurrentProcessId()));
        RemoveTree(r);
        return r;
    }();
    return root;
}

class Scratch {
public:
    Scratch() {
        static unsigned s_next = 0;
        root_ = ScratchRoot() / std::to_string(s_next++);
        fs::create_directories(root_ / "game");
    }

    fs::path game() const { return root_ / "game"; }
    fs::path legacy() const { return game() / "HeadTracking.ini"; }

    void WriteLegacy(const std::string& bytes) const { WriteFileBytes(legacy(), bytes); }

    std::set<std::string> Names() const {
        std::set<std::string> names;
        for (const auto& entry : fs::directory_iterator(game())) names.insert(entry.path().filename().string());
        return names;
    }

private:
    fs::path root_;
};

// ---- What a reading does -----------------------------------------------------------
//
// A Record names everything the running mod acts on after reading the file:
// `field.*` what the pipeline was handed, `start.*` the state the session starts
// in, `hotkey.*` the bindings that can fire, each as `modifiers:code` (Ctrl 1,
// Shift 2, as cameraunlock::input::KeyModifiers numbers them) in ascending
// order. Floats are their bits.
//
// v1.1.3 registered each nav key NavGuarded, which does not fire while Ctrl and
// Shift are both held, and each chord key ChordGuarded, which fires only while
// both are. A canonical binding with no modifiers and one naming Ctrl+Shift fire
// on exactly those conditions, so a record names a binding by its key and
// modifiers alone.

using Record = std::map<std::string, std::string>;

std::string Bits(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08X", static_cast<unsigned>(bits));
    return text;
}

std::string Flag(bool value) { return value ? "1" : "0"; }

const char* const kActionNames[] = {"Toggle", "CycleTrackingMode"};
const char* const kModeNames[] = {"RotationAndPosition", "RotationOnly", "PositionOnly"};

void AddHotkeys(Record& r, const std::vector<ace_oracle::Registration>& registrations) {
    std::map<int, std::vector<std::pair<unsigned, int>>> byAction;
    for (int action = 0; action < 2; ++action) byAction[action];
    for (const auto& [action, vk, modifiers] : registrations) byAction[action].push_back({modifiers, vk});
    for (auto& [action, items] : byAction) {
        std::sort(items.begin(), items.end());
        items.erase(std::unique(items.begin(), items.end()), items.end());
        std::string text;
        for (const auto& [modifiers, vk] : items) {
            char item[32];
            std::snprintf(item, sizeof(item), "%s%u:0x%02X", text.empty() ? "" : " ", modifiers, static_cast<unsigned>(vk));
            text += item;
        }
        r[std::string("hotkey.") + kActionNames[action]] = text;
    }
}

void AddPipeline(Record& r, const cameraunlock::SensitivitySettings& s, const cameraunlock::PositionSettings& p,
                 float local_smoothing, float remote_smoothing) {
    r["field.rot.yaw_sensitivity"] = Bits(s.yaw);
    r["field.rot.pitch_sensitivity"] = Bits(s.pitch);
    r["field.rot.roll_sensitivity"] = Bits(s.roll);
    r["field.rot.invert_yaw"] = Flag(s.invert_yaw);
    r["field.rot.invert_pitch"] = Flag(s.invert_pitch);
    r["field.rot.invert_roll"] = Flag(s.invert_roll);
    r["field.pos.sensitivity_x"] = Bits(p.sensitivity_x);
    r["field.pos.sensitivity_y"] = Bits(p.sensitivity_y);
    r["field.pos.sensitivity_z"] = Bits(p.sensitivity_z);
    r["field.pos.invert_x"] = Flag(p.invert_x);
    r["field.pos.invert_y"] = Flag(p.invert_y);
    r["field.pos.invert_z"] = Flag(p.invert_z);
    r["field.pos.limit_x"] = Bits(p.limit_x);
    r["field.pos.limit_y"] = Bits(p.limit_y);
    r["field.pos.limit_y_down"] = Bits(p.limit_y_down);
    r["field.pos.limit_z"] = Bits(p.limit_z);
    r["field.pos.limit_z_back"] = Bits(p.limit_z_back);
    r["field.pos.local_smoothing"] = Bits(p.local_smoothing);
    r["field.pos.remote_smoothing"] = Bits(p.remote_smoothing);
    r["field.local_smoothing"] = Bits(local_smoothing);
    r["field.remote_smoothing"] = Bits(remote_smoothing);
}

Record ObserveOracle(const ace_oracle::Published& p) {
    Record r;
    r["field.udp_port"] = std::to_string(p.udp_port);
    AddPipeline(r, p.sensitivity, p.position, p.local_smoothing, p.remote_smoothing);
    r["start.enabled"] = Flag(p.tracking_enabled);
    r["start.mode"] = kModeNames[p.mode];
    AddHotkeys(r, p.hotkeys);
    return r;
}

// The frozen reader's settings through the startup code of the commit that
// froze it: src/config.cpp's LoadConfig copies every field into the runtime
// Config, which ApplyConfigToPipeline and RegisterHotkeys in
// src/headtracking_mod.cpp consume exactly as v1.1.3 did.
Record ObserveImport(const legacy::Config& c) {
    Record r;
    r["field.udp_port"] = std::to_string(c.udp_port);
    cameraunlock::SensitivitySettings s;
    s.yaw = c.yaw_sensitivity;
    s.pitch = c.pitch_sensitivity;
    s.roll = c.roll_sensitivity;
    s.invert_yaw = c.invert_yaw;
    s.invert_pitch = c.invert_pitch;
    s.invert_roll = c.invert_roll;
    const cameraunlock::PositionSettings p = cameraunlock::PositionSettings::Symmetric(
        c.position_sensitivity_x, c.position_sensitivity_y, c.position_sensitivity_z,
        c.limit_x, c.limit_y, c.limit_z, c.limit_z_back,
        c.local_smoothing, c.remote_smoothing,
        c.invert_position_x, c.invert_position_y, c.invert_position_z);
    AddPipeline(r, s, p, c.local_smoothing, c.remote_smoothing);
    r["start.enabled"] = Flag(c.enable_on_startup);
    r["start.mode"] = kModeNames[c.position_enabled ? ace_oracle::kRotationAndPosition : ace_oracle::kRotationOnly];
    AddHotkeys(r, {{ace_oracle::kToggle, c.toggle_key, 0},
                   {ace_oracle::kToggle, c.chord_toggle_key, 3},
                   {ace_oracle::kCycleMode, c.cycle_mode_key, 0},
                   {ace_oracle::kCycleMode, c.chord_cycle_mode_key, 3}});
    return r;
}

std::vector<std::string> Differences(const Record& a, const Record& b) {
    std::vector<std::string> out;
    for (const auto& [name, value] : a) {
        const auto it = b.find(name);
        if (it == b.end()) {
            out.push_back(name + " only on the left");
        } else if (it->second != value) {
            out.push_back(name + ": " + value + " / " + it->second);
        }
    }
    for (const auto& [name, value] : b) {
        if (a.find(name) == a.end()) out.push_back(name + " only on the right");
    }
    return out;
}

// ---- Inputs ------------------------------------------------------------------------

fs::path DataPath(const char* name) {
    return fs::path(ACE_SOURCE_DIR) / "tests" / "config_differential" / "data" / name;
}

// What v1.1.0 to v1.1.3 wrote on a first start, the file most players hold.
std::string NewestFirstRun() { return ReadFileBytes(DataPath("firstrun-v1.1.0.ini")); }

const char* const kNewestFirstRunName = "v1.1.0 to v1.1.3 first-run file";

// Every key the frozen reader reads, and how the corpus varies each one. The
// reader refuses a port outside 1024-65535 and a hotkey code that is not a
// bindable key, and clamps a smoothing value into 0-1, a sensitivity into
// -100-100 and a limit into 0-10.
std::vector<testing::MutationKey> CorpusKeys() {
    return {
        {"Network", "UdpPort", "5252", {"80", "70000"}},
        {"General", "EnableOnStartup", "0", {}},
        {"Hotkeys", "ToggleKey", "0x2D", {"0x11"}, true},
        {"Hotkeys", "CycleModeKey", "0x70", {"0x10"}, true},
        {"Hotkeys", "ChordToggleKey", "0x4B", {"0x12"}, true},
        {"Hotkeys", "ChordCycleModeKey", "0x4A", {"0xA2"}, true},
        {"Rotation", "YawSensitivity", "0.5", {"-500", "500"}},
        {"Rotation", "PitchSensitivity", "0.5", {"-500", "500"}},
        {"Rotation", "RollSensitivity", "0.5", {"-500", "500"}},
        {"Rotation", "InvertYaw", "1", {}},
        {"Rotation", "InvertPitch", "1", {}},
        {"Rotation", "InvertRoll", "1", {}},
        {"Rotation", "LocalSmoothing", "0.3", {"-0.5", "1.5"}},
        {"Rotation", "RemoteSmoothing", "0.6", {"-0.5", "1.5"}},
        {"Position", "Enabled", "0", {}},
        {"Position", "SensitivityX", "0.5", {"-500", "500"}},
        {"Position", "SensitivityY", "0.5", {"-500", "500"}},
        {"Position", "SensitivityZ", "0.5", {"-500", "500"}},
        {"Position", "InvertX", "1", {}},
        {"Position", "InvertY", "1", {}},
        {"Position", "InvertZ", "1", {}},
        {"Position", "LimitX", "0.5", {"-0.5", "11"}},
        {"Position", "LimitY", "0.35", {"-0.5", "11"}},
        {"Position", "LimitZ", "0.6", {"-0.5", "11"}},
        {"Position", "LimitZBack", "0.15", {"-0.5", "11"}},
    };
}

// The generator refuses the call when these and the descriptors name different
// keys, so the corpus covers every key the reader reads.
std::vector<cfg::LegacyKey> CorpusReads() {
    std::vector<cfg::LegacyKey> reads;
    for (const legacy::Key& key : legacy::ReadKeys()) reads.push_back({key.section, key.key});
    return reads;
}

struct Input {
    std::string name;
    bool present;
    std::string bytes;
};

std::string WithKeyCode(const std::string& base, const std::string& line, int code) {
    const std::size_t at = base.find(line);
    if (at == std::string::npos) throw std::logic_error("no " + line + " in the first-run file");
    char to[48];
    std::snprintf(to, sizeof(to), "%.*s0x%02X", static_cast<int>(line.find('=') + 1), line.c_str(),
                  static_cast<unsigned>(code));
    std::string out = base;
    return out.replace(at, line.size(), to);
}

std::vector<Input> Inputs() {
    std::vector<Input> inputs = {
        {"v1.0.0 first-run file", true, ReadFileBytes(DataPath("firstrun-v1.0.0.ini"))},
        {"v1.0.1 first-run file", true, ReadFileBytes(DataPath("firstrun-v1.0.1.ini"))},
        {"v1.0.2 first-run file", true, ReadFileBytes(DataPath("firstrun-v1.0.2.ini"))},
        {kNewestFirstRunName, true, NewestFirstRun()},
        {"HeadTracking.ini at cf51838 (v1.0.0)", true, ReadFileBytes(DataPath("committed-cf51838.ini"))},
        {"HeadTracking.ini at 8a0797e", true, ReadFileBytes(DataPath("committed-8a0797e.ini"))},
        {"HeadTracking.ini at ee55327 (v1.0.1)", true, ReadFileBytes(DataPath("committed-ee55327.ini"))},
        {"HeadTracking.ini at ae323e4 (v1.0.2)", true, ReadFileBytes(DataPath("committed-ae323e4.ini"))},
        {"HeadTracking.ini at 4f5c0c4 (v1.1.0, v1.1.1)", true, ReadFileBytes(DataPath("committed-4f5c0c4.ini"))},
        {"HeadTracking.ini at e27005f (v1.1.2, v1.1.3)", true, ReadFileBytes(DataPath("committed-e27005f.ini"))},
        {"no file", false, {}},
        {"empty file", true, {}},
    };
    for (testing::IniMutation& m : testing::GenerateIniMutations(NewestFirstRun(), CorpusReads(), CorpusKeys())) {
        inputs.push_back({"corpus: " + m.name, true, std::move(m.bytes)});
    }
    for (const char* line : {"ToggleKey=0x23", "ChordToggleKey=0x59"}) {
        for (int code = 0x01; code <= 0xFE; ++code) {
            const std::string bytes = WithKeyCode(NewestFirstRun(), line, code);
            char name[48];
            std::snprintf(name, sizeof(name), "%.*s0x%02X", static_cast<int>(std::strchr(line, '=') - line + 1), line,
                          static_cast<unsigned>(code));
            inputs.push_back({name, true, bytes});
        }
    }
    return inputs;
}

// ---- Comparison ----------------------------------------------------------------------

void Compare(const std::vector<Input>& inputs) {
    const Record defaults = ObserveImport(legacy::Config{});
    int compared = 0;
    int changed = 0;
    for (const Input& input : inputs) {
        const std::string& name = input.name;
        Scratch s;
        if (input.present) s.WriteLegacy(input.bytes);
        const std::set<std::string> before = s.Names();

        const Record oracle = ObserveOracle(ace_oracle::Read(s.game().string()));
        legacy::Config read;
        const bool present = legacy::LoadConfig(s.legacy().string(), read);
        Check(present == input.present, name + ": the frozen reader finds the file exactly when it is there");
        if (!Differences(oracle, defaults).empty()) ++changed;
        const std::vector<std::string> diff = Differences(oracle, ObserveImport(read));
        for (const std::string& d : diff) std::printf("  comparison 1, %s: %s\n", name.c_str(), d.c_str());
        Check(diff.empty(), name + ": comparison 1, the oracle and the import agree");

        Check(s.Names() == before, name + ": neither reader writes a file");
        Check(!input.present || ReadFileBytes(s.legacy()) == input.bytes, name + ": neither reader changes the file");
        ++compared;
    }
    std::printf("comparison 1: %d inputs, %d of them read as something other than the defaults\n", compared,
                changed);
    Check(changed > 0, "the inputs reach settings other than the defaults");
}

// v1.1.3's first start wrote the file every player then held, and that is the
// input the corpus is built on.
void FirstRunOutputIsTheCorpusBase() {
    Scratch s;
    ace_oracle::WriteFirstRunFile(s.game().string());
    Check(ReadFileBytes(s.legacy()) == NewestFirstRun(),
          "data/firstrun-v1.1.0.ini is what v1.1.3 writes on a first start");
}

}  // namespace

int main() {
    // Unbuffered, so the lines before an uncaught exception reach the log.
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    FirstRunOutputIsTheCorpusBase();
    Compare(Inputs());
    RemoveTree(ScratchRoot());
    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
