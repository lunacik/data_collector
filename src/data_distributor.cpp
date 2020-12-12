
#include "data_distributor.h"
#include "file_extensions.h"
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <fstream>
#include <spdlog/spdlog.h>

namespace fs = boost::filesystem;

//--------------------------------------------------------------------------------------------------

DataDistributor::DataDistributor(boost::asio::io_service &io_context, const fs::path &output_dir,
                                 const std::string &address, uint16_t port, std::chrono::seconds distribution_period)
    : m_io_context(io_context),
      m_output_dir(output_dir),
      m_distribution_period(distribution_period),
      m_timer(io_context),
      m_distribution_count(),
      m_transmitter(io_context, address, port)
{
    // force early initial distribution
    handlePeriodicUpdate();
}

//--------------------------------------------------------------------------------------------------

uint64_t DataDistributor::getDistributionCount() const
{
    return m_distribution_count;
}

//--------------------------------------------------------------------------------------------------

void DataDistributor::handlePeriodicUpdate()
{
    // start new timeout
    m_timer.expires_from_now(m_distribution_period);
    m_timer.async_wait(std::bind(&DataDistributor::handlePeriodicUpdate, this));

    const auto &distribution_files = collectDistributionFiles();

    if (!distributeFiles(distribution_files))
    {
        spdlog::warn("Failed to distribute all collected files, will retry on next distribution");
    }
    else
    {
        m_distribution_count += distribution_files.size() != 0 ? 1UL : 0UL;
    }
}

//--------------------------------------------------------------------------------------------------

std::vector<fs::path> DataDistributor::collectDistributionFiles() const
{
    using dir_it = fs::directory_iterator;

    std::vector<fs::path> distribution_files;

    for (auto it = dir_it(m_output_dir); it != dir_it(); it++)
    {
        auto file_name = it->path().filename();

        if (fs::is_regular_file(*it))
        {
            // skip lock files and any other unrelated file
            if (file_name.extension() == file_extensions::data &&
                file_name.extension().extension() != file_extensions::lock)
            {
                auto lock_file_path = it->path();
                lock_file_path.concat(file_extensions::lock);
                // skip if lock file for this data file exists
                if (!fs::exists(lock_file_path))
                {
                    distribution_files.push_back(it->path());
                }
                else
                {
                    spdlog::debug("Skipping distribution of {} because it's locked", it->path().string());
                }
            }
        }
        else
        {
            spdlog::warn("Output directory contains non regular file: {}", file_name.string());
        }
    }

    return distribution_files;
}

//--------------------------------------------------------------------------------------------------

bool DataDistributor::distributeFiles(const std::vector<fs::path> &files)
{
    bool distributed_all = true;

    for (const auto &path : files)
    {
        if (distributeFile(path))
        {
            // remove distributed files from filesystem
            if (fs::remove(path))
            {
                spdlog::info("Successfully removed file: {}", path.string());
            }
            else
            {
                spdlog::warn("Failed to remove file: {}", path.string());
            }
        }
        else
        {
            distributed_all = false;
        }
    }

    return distributed_all;
}

//--------------------------------------------------------------------------------------------------

bool DataDistributor::distributeFile(const fs::path &path)
{
    bool succeed = false;

    std::ifstream file;
    file.open(path.c_str());

    if (file.is_open())
    {
        // @note this is not efficient but rather simple solution for small files
        const std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (m_transmitter.transmit(file_content))
        {
            spdlog::info("Distributed file {} to server", path.string());
            succeed = true;
        }
        else
        {
            spdlog::warn("Failed to distribute contents of file: {}", path.string());
        }
    }
    else
    {
        spdlog::error("Failed to open file for reading: {}", path.string());
    }

    file.close();
    return succeed;
}
