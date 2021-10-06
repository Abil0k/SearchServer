#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

class LogDuration
{
public:
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string &id, std::ostream &dst_stream = std::cerr)
        : id_(id), dst_stream_(dst_stream)
    {
    }

    ~LogDuration()
    {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        dst_stream_ << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    std::ostream &dst_stream_;
    const Clock::time_point start_time_ = Clock::now();
};