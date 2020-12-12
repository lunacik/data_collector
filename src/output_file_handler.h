#pragma once

#include <boost/filesystem/path.hpp>
#include <fstream>
#include <string>

//--------------------------------------------------------------------------------------------------

class OutputFileHandler
{
public:
    OutputFileHandler(const boost::filesystem::path &output_dir);
    ~OutputFileHandler();

    bool openFile(const std::string &file_name);
    void write(const std::string &data);
    void releaseExistingLocks() const;

private:
    void closeCurrentFile();
    void createLockFile(const boost::filesystem::path &path) const;

    const boost::filesystem::path m_output_dir;
    std::string                   m_file_name;
    std::ofstream                 m_file_stream;
};
