#pragma once
// Deterministic host-test replacement; never on the firmware include path.
#include <cstdint>
extern "C" int64_t esp_timer_get_time();
