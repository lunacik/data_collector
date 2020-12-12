#pragma once

#include "http_transmitter.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional/optional_fwd.hpp>
#include <chrono>
#include <string>
#include <vector>

namespace boost
{
namespace asio
{
class io_context;
}
}  // namespace boost

//--------------------------------------------------------------------------------------------------

class DataDistributor
{
public:
    DataDistributor(boost::asio::io_context &io_context, const boost::filesystem::path &output_dir,
                    const std::string &address, uint16_t port, std::chrono::seconds distribution_period);

    uint64_t getDistributionCount() const;

private:
    void                                 handlePeriodicUpdate();
    std::vector<boost::filesystem::path> collectDistributionFiles() const;
    bool                                 distributeFiles(const std::vector<boost::filesystem::path> &files);
    bool                                 distributeFile(const boost::filesystem::path &path);

    boost::asio::io_context &     m_io_context;
    const boost::filesystem::path m_output_dir;
    const std::chrono::seconds    m_distribution_period;
    boost::asio::steady_timer     m_timer;
    uint64_t                      m_distribution_count;
    HttpTransmitter               m_transmitter;
};
