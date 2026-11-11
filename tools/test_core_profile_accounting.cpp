// Host test with a deterministic esp_timer stub: nested work is counted once.
#define S3_CORE_PROFILE_IMPLEMENTATION
#include "../esp32s3/components/mame0220/src/s3_core_profile.h"
#include <stdexcept>

static int64_t fake_time = 1;
extern "C" int64_t esp_timer_get_time() { return fake_time; }

int main() {
    using namespace s3_core_profile;
    reset();
    fake_time = 10;
    {
        scope outer(scheduler);
        fake_time = 100;
        { scope work(cpu); fake_time = 600; }
        fake_time = 700;
        {
            scope callbacks(timers);
            fake_time = 800;
            {
                scope frame(video);
                fake_time = 900;
                { scope inputs(ports); fake_time = 1000; }
                fake_time = 1100;
                { scope output(osd); fake_time = 2100; }
                fake_time = 2200;
            }
            fake_time = 2500;
        }
        fake_time = 3000;
    }
    assert(exclusive[scheduler] == 690); // 2990 - 500 - 1800
    assert(exclusive[timers] == 400);
    assert(exclusive[cpu] == 500);
    assert(exclusive[video] == 300);
    assert(exclusive[ports] == 100);
    assert(exclusive[osd] == 1000);
    assert(exclusive[sound] == 0);
    assert(max_depth == 4 && !depth && !top);
    // Exception unwinding must restore the parent and allow a new session.
    fake_time = 3100;
    try {
        scope outer(scheduler);
        fake_time = 3200;
        scope work(cpu);
        fake_time = 3300;
        throw std::runtime_error("expected");
    } catch (const std::runtime_error &) {}
    assert(!depth && !top && exclusive[cpu] == 600);
    reset();
    fake_time += 5000000;
    { scope work(cpu); fake_time += 500; }
    assert(sequence == 1 && !top && !depth);
    for (auto value : exclusive) assert(!value);
    std::puts("PASS: nested exclusive accounting, unwind, reset and interval rollover");
}
