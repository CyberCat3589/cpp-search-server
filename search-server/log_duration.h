#pragma once

#include <iostream>
#include <chrono>
#include <string>

#define LOG_DURATION(operation, out_stream) LogDuration profile_guard(operation, out_stream)

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