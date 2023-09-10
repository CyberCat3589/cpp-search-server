#pragma once

#include <iostream>
#include <chrono>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

class LogDuration
{
    using Clock = std::chrono::steady_clock;

public:

    LogDuration(std::string operation, std::ostream& stream) : operation_name_(operation), out_stream_(stream) {}

    ~LogDuration()
    {
        using namespace std::chrono;

        const Clock::time_point end_time = Clock::now();
        const Clock::duration duration = end_time - start_time_;
        out_stream_ << operation_name_ << ": " << duration_cast<milliseconds>(duration).count() << " ms" << std::endl;
    }

private:
    std::string operation_name_;
    const Clock::time_point start_time_ = Clock::now();
    std::ostream& out_stream_ = std::cerr;
};