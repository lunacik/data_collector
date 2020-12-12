#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/optional/optional_fwd.hpp>

namespace boost
{
namespace asio
{
class io_context;
}
}  // namespace boost

//--------------------------------------------------------------------------------------------------

class HttpTransmitter
{
public:
    HttpTransmitter(boost::asio::io_context &io_context, const std::string &address, uint16_t port);

    bool transmit(const std::string &payload);

private:
    boost::optional<boost::asio::ip::tcp::socket> establishConnection();
    bool sendPayload(boost::asio::ip::tcp::socket &socket, const std::string &payload);

    boost::asio::io_context &m_io_context;
    const std::string        m_address;
    const uint16_t           m_port;
};
