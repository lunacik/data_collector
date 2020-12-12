
#include "http_transmitter.h"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <spdlog/spdlog.h>

namespace
{
constexpr auto message_version = 11;  // htpp 1.1
constexpr auto post_target     = "/";
constexpr auto content_type    = "text/plain";
}  // namespace

//--------------------------------------------------------------------------------------------------

HttpTransmitter::HttpTransmitter(boost::asio::io_service &io_context, const std::string &address, uint16_t port)
    : m_io_context(io_context), m_address(address), m_port(port)
{
}

//--------------------------------------------------------------------------------------------------

bool HttpTransmitter::transmit(const std::string &payload)
{
    try
    {
        auto socket = establishConnection();
        if (!socket)
        {
            return false;
        }

        sendPayload(socket.get(), payload);

        boost::system::error_code error_code;
        socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, error_code);

        return true;
    }
    catch (const std::exception &e)
    {
        spdlog::critical("Exception occured during message distribution: \n\t{}", e.what());
        return false;
    }
}

//--------------------------------------------------------------------------------------------------

boost::optional<boost::asio::ip::tcp::socket> HttpTransmitter::establishConnection()
{
    boost::asio::ip::tcp::resolver resolver(m_io_context);
    boost::asio::ip::tcp::socket   socket(m_io_context);

    boost::system::error_code error_code;

    const auto &results = resolver.resolve(m_address, std::to_string(m_port), error_code);

    if (error_code.failed())
    {
        spdlog::error("Failed to resolve host address: {}:{}, message: ", m_address, m_port, error_code.message());
        return {};
    }

    boost::asio::connect(socket, results.begin(), results.end(), error_code);

    if (error_code.failed())
    {
        socket.close();
        spdlog::error("Failed to connect to host at address: {}:{}, message: ", m_address, m_port,
                      error_code.message());
        return {};
    }

    return socket;
}

//--------------------------------------------------------------------------------------------------

bool HttpTransmitter::sendPayload(boost::asio::ip::tcp::socket &socket, const std::string &payload)
{
    boost::system::error_code error_code;
    namespace http = boost::beast::http;

    http::request<http::string_body> request(http::verb::post, post_target, message_version);
    request.set(http::field::host, m_address);
    request.set(http::field::content_type, content_type);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request.body() = payload;
    request.prepare_payload();

    http::write(socket, request, error_code);

    if (error_code.failed())
    {
        spdlog::error("Failed to send to host at address: {}:{}, message: ", m_address, m_port, error_code.message());
        return false;
    }

    // receive response from server
    boost::beast::flat_buffer          buffer;
    http::response<http::dynamic_body> response;

    error_code.clear();
    http::read(socket, buffer, response, error_code);

    if (error_code.failed())
    {
        spdlog::warn("Failed to receive response from the server: ", error_code.message());
        return false;
    }
    else
    {
        if (response.result() != http::status::ok)
        {
            spdlog::warn("Server responsed with error status: {}", response.result_int());
            return false;
        }
        else
        {
            return true;
        }
    }
}
