#pragma once

#include <chrono>
#include <exception>
#include <source_location>
#include <string>

#include <fmt/chrono.h>
#include <fmt/format.h>

#define TODO throw std::runtime_error{fmt::format("not yet implemented @{}", here())};
#define UNIMPLEMENTED throw std::runtime_error{fmt::format("not implemented @{}", here())};

namespace neszenz {

using fmt::println;
using fmt::format;

std::string here(std::source_location loc = std::source_location::current());

using Duration = std::chrono::nanoseconds;
using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;

using fMilliseconds = std::chrono::duration<double, std::milli>;
using fSeconds = std::chrono::duration<double>;

using SteadyClock = std::chrono::steady_clock;
using SteadyTimepoint = SteadyClock::time_point;

} // namespace neszenz
