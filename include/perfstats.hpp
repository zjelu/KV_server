#pragma once

#include <cstdint>
#include <iostream>

struct PerfStats
{
    // 累计耗时，统一使用纳秒
    std::uint64_t read_time_ns = 0;
    std::uint64_t parse_time_ns = 0;
    std::uint64_t execute_time_ns = 0;
    std::uint64_t write_time_ns = 0;

    std::uint64_t epoll_wait_time_ns = 0;
    std::uint64_t epoll_ctl_time_ns = 0;

    // 不同阶段对应的计数器
    std::uint64_t read_event_count = 0;
    std::uint64_t write_event_count = 0;
    std::uint64_t write_attempt_count = 0;

    std::uint64_t parse_count = 0;
    std::uint64_t execute_count = 0;

    std::uint64_t epoll_wait_count = 0;
    std::uint64_t epoll_ctl_count = 0;

    // 可选：帮助判断一次事件中处理了多少数据
    std::uint64_t bytes_read = 0;
    std::uint64_t bytes_written = 0;

    bool benchmark_started = false;
    bool report_printed = false;

    void reset() noexcept
    {
        *this = PerfStats{};
    }

    void print(std::ostream& os = std::cout) const
    {
        os << "\n========== Performance Stats ==========\n";

        print_average(
            os,
            "read",
            read_time_ns,
            read_event_count,
            "event"
        );

        print_average(
            os,
            "parse",
            parse_time_ns,
            parse_count,
            "command"
        );

        print_average(
            os,
            "execute",
            execute_time_ns,
            execute_count,
            "command"
        );

        print_average(
            os,
            "write",
            write_time_ns,
            write_event_count+write_attempt_count,
            "event"
        );
        print_average(
            os,
            "epoll_wait",
            epoll_wait_time_ns,
            epoll_wait_count,
            "call"
        );

        print_average(
            os,
            "epoll_ctl",
            epoll_ctl_time_ns,
            epoll_ctl_count,
            "call"
        );

        os << "read events:  "
           << read_event_count << '\n';

        os << "write events: "
           << write_event_count << '\n';

        os << "parsed commands: "
           << parse_count << '\n';

        os << "executed commands: "
           << execute_count << '\n';

        os << "bytes read:    "
           << bytes_read << '\n';

        os << "bytes written: "
           << bytes_written << '\n';

        os << "=======================================\n";
    }

private:
    static void print_average(
        std::ostream& os,
        const char* name,
        std::uint64_t total_ns,
        std::uint64_t count,
        const char* unit
    )
    {
        if (count == 0) {
            os << name << ": no samples\n";
            return;
        }

        const double average_ns =
            static_cast<double>(total_ns) /
            static_cast<double>(count);

        const double average_us =
            average_ns / 1000.0;

        os << "avg " << name << ": "
           << average_ns << " to_ns"
           << "  (" << average_us << " us/"
           << unit << ")\n";
    }
};