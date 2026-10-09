#include <cassert>
#include <array>
#include <string_view>
#include "UnlockMonitor.hpp"

// SDK-independent contract smoke test, compiled by tests/CMakeLists.txt.
int main() {
    constexpr std::array<std::string_view, 9> categories{
        "cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing", "jetpack"
    };
    static_assert(categories.size() == 9);
    assert(categories.front() == "cube");
    assert(categories.back() == "jetpack");

    iconstat::UnlockMonitorState monitor;
    assert(!monitor.observe(100, false)); // First observation establishes the baseline.
    assert(!monitor.observe(100, false));
    assert(monitor.observe(101, false));
    assert(monitor.requestInFlight());
    assert(!monitor.observe(102, false)); // Never overlap uploads.
    monitor.complete(false, 101);
    assert(monitor.retrySeconds() > 0.f);
    assert(!monitor.observe(101, false)); // Back off after a failed request.
    monitor.tick(30.f);
    assert(monitor.observe(101, false));
    monitor.complete(true, 101);
    assert(!monitor.observe(101, false));
    assert(!monitor.observe(102, true)); // Respect manual/startup account actions.
    assert(monitor.observe(102, false));
    monitor.complete(true, 102);
    monitor.reset();
    assert(!monitor.observe(999, false)); // Re-enabling starts with a fresh baseline.
}
