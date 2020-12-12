
#include "data_distributor.h"
#include "data_generator.h"
#include "spdlog/sinks/daily_file_sink.h"
#include <boost/asio/io_service.hpp>
#include <boost/filesystem/path.hpp>
#include <csignal>
#include <functional>
#include <iostream>
#include <thread>
#include <docopt/docopt.h>
#include <spdlog/spdlog.h>

//--------------------------------------------------------------------------------------------------

namespace
{
volatile std::sig_atomic_t g_signal_status;

constexpr std::chrono::milliseconds MIN_GENERATION_INTERVAL(500);
constexpr std::chrono::seconds      MAX_GENERATION_INTERVAL(15);
constexpr std::chrono::milliseconds SIGNAL_CHECK_INTERVAL(100);
constexpr auto                      LOG_FILE_NAME_PREFIX("dc");

//--------------------------------------------------------------------------------------------------

void handleSignal(int signal)
{
    g_signal_status = signal;
}

//--------------------------------------------------------------------------------------------------

void installSignalHandlers()
{
    std::signal(SIGINT, handleSignal);
    std::signal(SIGABRT, handleSignal);
    std::signal(SIGSTOP, handleSignal);
    std::signal(SIGKILL, handleSignal);
}

//--------------------------------------------------------------------------------------------------

std::map<std::string, docopt::value> parseOptions(int argc, const char **argv)
{
    constexpr auto USAGE =
        // clang-format off
R"(Data collection and distribution simulation process.

    Usage:
          data_collector --tmp-persist-dir=<path> --host=<host> --port=<port> [--dist-interval=<sec>] [--log-dir=<path>]
          data_collector (-h | --help)

    Options:
          -h --help                 Show this screen.
          --tmp-persist-dir=<path>  Path to directory where data will be temporarily stored.
          --host=<host>             Name or ip address of host server.
          --port=<port>             Port of host server.
          --dist-interval=<sec>     Data distribution to server interval in seconds [default: 60].
          --log-dir=<path>          Optional path to a directory where all logs will be written with rotation interval of 7days [default: ].
)";
    // clang-format on

    return docopt::docopt(USAGE, {std::next(argv), std::next(argv, argc)});
}

//--------------------------------------------------------------------------------------------------

void setupLogger(const std::string &log_dir_path)
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_every(std::chrono::seconds(1));

    if (!log_dir_path.empty())
    {
        boost::filesystem::path dir_path(log_dir_path);
        dir_path.append(LOG_FILE_NAME_PREFIX);

        // rotation happens every day at 4:00AM, keeping logs of one week.
        constexpr auto rotation_hour = 4, rotation_minute = 0, keep_days = 7;

        auto file_sink(std::make_shared<spdlog::sinks::daily_file_sink_mt>(dir_path.string(), rotation_hour,
                                                                           rotation_minute, false, keep_days));
        spdlog::default_logger()->sinks().push_back(file_sink);
    }
}

}  // namespace

//--------------------------------------------------------------------------------------------------

int main(int argc, const char **argv)
{
    installSignalHandlers();

    auto args = parseOptions(argc, argv);

    setupLogger(args["--log-dir"].asString());

    const auto &output_dir    = args["--tmp-persist-dir"].asString();
    const auto &address       = args["--host"].asString();
    const auto  port          = std::stoull(args["--port"].asString());
    const auto  dist_interval = std::stoull(args["--dist-interval"].asString());

    constexpr auto concurrency_hint = 1;

    boost::asio::io_context generator_context(concurrency_hint);
    boost::asio::io_context distributor_context(concurrency_hint);

    std::thread generator_thread([&generator_context, &output_dir] {
        try
        {
            DataGenerator generator(generator_context, output_dir, MIN_GENERATION_INTERVAL, MAX_GENERATION_INTERVAL);
            generator_context.run();

            spdlog::info("Total amount of written values is {}", generator.getWrittenValuesCount());
        }
        catch (const std::exception &e)
        {
            spdlog::critical("Exception occured in generator thread: {}", e.what());
            throw;
        }
    });

    std::thread distributor_thread([&distributor_context, &output_dir, &address, port, dist_interval] {
        try
        {
            DataDistributor distributor(distributor_context, output_dir, address, static_cast<uint16_t>(port),
                                        std::chrono::seconds(dist_interval));
            distributor_context.run();

            spdlog::info("Total amount of file distributions is {}", distributor.getDistributionCount());
        }
        catch (const std::exception &e)
        {
            spdlog::critical("Exception occured in distributor thread: {}", e.what());
            throw;
        }
    });

    while (!g_signal_status)
    {
        std::this_thread::sleep_for(SIGNAL_CHECK_INTERVAL);
    }

    spdlog::info("-------------------------------------------------------------------");
    spdlog::info("Signal {} received, stopping threads...", g_signal_status);

    generator_context.stop();
    generator_thread.join();

    spdlog::info("Generator thread stopped.");

    distributor_context.stop();
    distributor_thread.join();

    spdlog::info("Distributor thread stopped.");
}
