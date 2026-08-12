// The gate that stops head tracking following the player's head while the
// game is not running the sim. Pure classification only - the shared memory
// page itself needs the game.

#include "sim_state.h"

#include "test_support.h"

#include <cstdio>
#include <string>

using namespace ace_ht;
using ace_test::Check;

namespace {

void StatusDecodeTests() {
    std::printf("SimStatusFromRaw decodes what the engine writes\n");
    Check(SimStatusFromRaw(0) == SimStatus::Off, "0 is off");
    Check(SimStatusFromRaw(1) == SimStatus::Replay, "1 is replay");
    Check(SimStatusFromRaw(2) == SimStatus::Live, "2 is live");
    Check(SimStatusFromRaw(3) == SimStatus::Paused, "3 is paused");

    // A page this mod cannot interpret must not be read as a state it knows.
    Check(SimStatusFromRaw(4) == SimStatus::Unknown, "an unexpected value is unknown");
    Check(SimStatusFromRaw(-1) == SimStatus::Unknown, "a negative value is unknown");
}

void GateTests() {
    std::printf("Tracking follows the head only while the sim runs\n");
    Check(IsSimRunning(SimStatus::Live), "a live session tracks");
    Check(IsSimRunning(SimStatus::Replay), "a replay tracks");

    // The reason this gate exists: the cockpit still renders behind the menu.
    Check(!IsSimRunning(SimStatus::Paused), "a paused session holds the view");
    Check(!IsSimRunning(SimStatus::Off), "no session holds the view");

    // No page, or a page this mod cannot read, must not disable head tracking.
    Check(IsSimRunning(SimStatus::Unknown), "an unreadable page keeps tracking");
}

void NameTests() {
    std::printf("Every status has a log name\n");
    Check(std::string(SimStatusName(SimStatus::Off)) == "off", "off");
    Check(std::string(SimStatusName(SimStatus::Replay)) == "replay", "replay");
    Check(std::string(SimStatusName(SimStatus::Live)) == "live", "live");
    Check(std::string(SimStatusName(SimStatus::Paused)) == "paused", "paused");
    Check(std::string(SimStatusName(SimStatus::Unknown)) == "unknown", "unknown");
}

}  // namespace

int main() {
    std::printf("Assetto Corsa EVO head tracking - sim state tests\n");
    std::printf("================================================\n");
    StatusDecodeTests();
    GateTests();
    NameTests();
    return ace_test::Summary("sim state");
}
