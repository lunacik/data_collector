#pragma once

#include "output_file_handler.h"
#include <boost/asio/steady_timer.hpp>
#include <boost/filesystem/path.hpp>
#include <chrono>
#include <string>

namespace boost
{
namespace asio
{
class io_context;
}
}  // namespace boost

//--------------------------------------------------------------------------------------------------

class DataGenerator
{
public:
    DataGenerator(boost::asio::io_context &io_context, const boost::filesystem::path &output_dir,
                  std::chrono::milliseconds min_interval, std::chrono::milliseconds max_interval);

    uint64_t getWrittenValuesCount() const;

private:
    void                      writeNewData();
    std::chrono::milliseconds randomInterval() const;
    short                     randomValue() const;

    const std::chrono::milliseconds m_min_interval;
    const std::chrono::milliseconds m_max_interval;
    boost::asio::steady_timer       m_timer;
    int64_t                         m_last_write_timestamp;  // rounded to minutes
    uint64_t                        m_write_count;
    OutputFileHandler               m_file_handler;
};
