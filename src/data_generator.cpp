
#include "data_generator.h"
#include <boost/asio.hpp>
#include <chrono>
#include <random>
#include <spdlog/spdlog.h>

//--------------------------------------------------------------------------------------------------

namespace
{
constexpr auto min_generated_value = 0;
constexpr auto max_generated_value = 9;
}  // namespace

//--------------------------------------------------------------------------------------------------

DataGenerator::DataGenerator(boost::asio::io_context &io_context, const boost::filesystem::path &output_dir,
                             std::chrono::milliseconds min_interval, std::chrono::milliseconds max_interval)
    : m_min_interval(min_interval),
      m_max_interval(max_interval),
      m_timer(io_context),
      m_last_write_timestamp(),
      m_write_count(0),
      m_file_handler(output_dir)
{
    assert(min_interval.count() > 0);
    assert(min_interval <= max_interval);

    m_timer.expires_from_now(randomInterval());
    m_timer.async_wait(std::bind(&DataGenerator::writeNewData, this));
}

//--------------------------------------------------------------------------------------------------

uint64_t DataGenerator::getWrittenValuesCount() const
{
    return m_write_count;
}

//--------------------------------------------------------------------------------------------------

void DataGenerator::writeNewData()
{
    // start new timeout
    m_timer.expires_from_now(randomInterval());
    m_timer.async_wait(std::bind(&DataGenerator::writeNewData, this));

    const auto now             = std::chrono::system_clock::now().time_since_epoch();
    const auto elapsed_minutes = std::chrono::duration_cast<std::chrono::minutes>(now);

    const auto file_name = std::to_string(elapsed_minutes.count());

    if (m_file_handler.openFile(file_name))
    {
        const auto &data = fmt::format("{} {}\n", randomValue(), now.count());
        m_file_handler.write(data);
        m_write_count++;
    }
}

//--------------------------------------------------------------------------------------------------

std::chrono::milliseconds DataGenerator::randomInterval() const
{
    static std::random_device                  random_dev;
    static std::default_random_engine          random_engine(random_dev());
    static std::uniform_int_distribution<long> distribution(m_min_interval.count(), m_max_interval.count());

    return std::chrono::milliseconds(distribution(random_engine));
}

//--------------------------------------------------------------------------------------------------

short DataGenerator::randomValue() const
{
    static std::random_device                   random_dev;
    static std::default_random_engine           random_engine(random_dev());
    static std::uniform_int_distribution<short> distribution(min_generated_value, max_generated_value);

    return distribution(random_engine);
}
