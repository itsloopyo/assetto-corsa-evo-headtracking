#pragma once

#include <cameraunlock/logging/file_log.h>

// The process-wide log lives in cameraunlock-core (logging::Open/Close/Line/
// EmergencyLine). Alias it under the mod namespace so call sites read
// Log::Line(...) unqualified.
namespace ace_ht
{
    namespace Log = ::cameraunlock::logging;
}
