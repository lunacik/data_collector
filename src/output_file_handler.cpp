
#include "output_file_handler.h"
#include "file_extensions.h"
#include <boost/filesystem/operations.hpp>
#include <spdlog/spdlog.h>

namespace fs = boost::filesystem;

//--------------------------------------------------------------------------------------------------

OutputFileHandler::OutputFileHandler(const fs::path &output_dir)
    : m_output_dir(output_dir), m_file_name(), m_file_stream()
{
    // release any locks that were not cleaned up properly during shutdown
    releaseExistingLocks();
}

//--------------------------------------------------------------------------------------------------

OutputFileHandler::~OutputFileHandler()
{
    closeCurrentFile();
}

//--------------------------------------------------------------------------------------------------

bool OutputFileHandler::openFile(const std::string &file_name)
{
    if (file_name != m_file_name)
    {
        closeCurrentFile();

        const auto file_path = m_output_dir / fs::path(file_name + file_extensions::data);

        createLockFile(file_path.string() + file_extensions::lock);

        m_file_stream.open(file_path.c_str(), std::ios::out | std::ios::app);

        if (m_file_stream.good())
        {
            spdlog::info("New file {} opened for writing", file_path.c_str());
            m_file_name = file_name;
        }
        else
        {
            spdlog::error("Failed to open file: {}, err: {}", file_path.c_str(), errno);
        }
    }

    return m_file_stream.good();
}

//--------------------------------------------------------------------------------------------------

void OutputFileHandler::write(const std::string &data)
{
    assert(m_file_stream.good());

    m_file_stream << data;

    spdlog::debug("Data has been written to file: {}", m_file_name + file_extensions::data);
}

//--------------------------------------------------------------------------------------------------

void OutputFileHandler::closeCurrentFile()
{
    if (m_file_stream.is_open())
    {
        spdlog::debug("Closing file {}", (m_output_dir / (m_file_name + file_extensions::data)).string());

        m_file_stream.close();
        m_file_name.clear();

        releaseExistingLocks();
    }
}

//--------------------------------------------------------------------------------------------------

void OutputFileHandler::releaseExistingLocks() const
{
    using dir_it = fs::directory_iterator;

    const auto now                          = std::chrono::system_clock::now().time_since_epoch();
    const auto elapsed_minutes              = std::chrono::duration_cast<std::chrono::minutes>(now);
    const auto current_timestamp_in_minutes = elapsed_minutes.count();

    for (auto it = dir_it(m_output_dir); it != dir_it(); it++)
    {
        if (it->path().extension().extension() == file_extensions::lock)
        {
            auto path_without_extension = it->path().filename().replace_extension().replace_extension();
            long file_name_as_minutes   = std::strtol(path_without_extension.filename().c_str(), nullptr, 10);

            if (file_name_as_minutes > 0 && file_name_as_minutes < current_timestamp_in_minutes)
            {
                boost::system::error_code error_code;
                if (fs::remove(it->path(), error_code))
                {
                    spdlog::info("Removed lock file: {}", it->path().string());
                }
                else
                {
                    spdlog::error("Failed to remove lock file: {}", it->path().string());
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------

void OutputFileHandler::createLockFile(const fs::path &path) const
{
    std::ofstream lock_file_stream(path.string(), std::ios::out | std::ios::trunc);
    lock_file_stream.flush();
    lock_file_stream.close();

    boost::system::error_code error_code;
    if (!fs::exists(path, error_code))
    {
        spdlog::error("Failed to create lock file: {}", path.string());
    }
    else
    {
        spdlog::info("Created lock file: {}", path.string());
    }
}
