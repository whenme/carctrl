// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_SERVER_H_INCLUDED__
#define __CLI_DETAIL_SERVER_H_INCLUDED__

#include <memory>
#include <queue>

namespace cli::detail
{

class Session : public std::enable_shared_from_this<Session>, public std::streambuf
{
public:
    ~Session() override = default;
    // disable value semantics
    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&)                 = delete;
    Session& operator=(Session&&)      = delete;

    virtual void start()
    {
        onConnect();
        read();
    }

protected:
    explicit Session(asiolib::ip::tcp::socket sock) : m_socket(std::move(sock)), m_outStream(this), m_data{0}
    {
    }

    virtual void disConnect()
    {
        m_socket.shutdown(asiolib::ip::tcp::socket::shutdown_both);
        m_socket.close();
    }

    virtual void read()
    {
        auto self(shared_from_this());
        m_socket.async_read_some(
            asiolib::buffer(m_data, k_MaxLength), [this, self](asiolibec::error_code ec, std::size_t length) {
                if (!m_socket.is_open() || (ec == asiolib::error::eof) || (ec == asiolib::error::connection_reset))
                {
                    onDisconnect();
                }
                else if (ec)
                {
                    onError();
                }
                else
                {
                    onDataReceived(std::string(m_data, length));  // NOLINT
                    read();
                }
            });
    }

    virtual void send(const std::string& msg)
    {
        asiolibec::error_code ec;
        asiolib::write(m_socket, asiolib::buffer(msg), ec);
        if ((ec == asiolib::error::eof) || (ec == asiolib::error::connection_reset))
        {
            onDisconnect();
        }
        else if (ec)
        {
            onError();
        }
    }

    virtual std::ostream& outStream()
    {
        return m_outStream;
    }

    virtual void onConnect()                             = 0;
    virtual void onDisconnect()                          = 0;
    virtual void onError()                               = 0;
    virtual void onDataReceived(const std::string& data) = 0;

    virtual std::string encode(const std::string& data) const
    {
        return data;
    }

private:
    // std::streambuf
    std::streamsize xsputn(const char* str, std::streamsize nsize) override
    {
        send(encode(std::string(str, str + nsize)));  // NOLINT
        return nsize;
    }

    int overflow(int chr) override
    {
        send(encode(std::string(1, static_cast<char>(chr))));
        return chr;
    }

    static constexpr uint32_t k_MaxLength = 1024;
    asiolib::ip::tcp::socket  m_socket;
    std::ostream              m_outStream;
    char                      m_data[k_MaxLength];  // NOLINT
};

template<typename ASIOLIB>
class Server
{
public:
    // disable value semantics
    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&)                 = delete;
    Server& operator=(Server&&)      = delete;

    Server(typename ASIOLIB::ContextType& ios, unsigned short port) :
        m_acceptor(ios, asiolib::ip::tcp::endpoint(asiolib::ip::tcp::v4(), port)),
        m_socket(ios)
    {
        accept();
    }
    Server(typename ASIOLIB::ContextType& ios, std::string address, unsigned short port) :
        m_acceptor(ios, asiolib::ip::tcp::endpoint(ASIOLIB::ipAddressFromString(address), port)),
        m_socket(ios)
    {
        accept();
    }
    virtual ~Server() = default;
    // returns shared_ptr instead of unique_ptr because Session needs to use enable_shared_from_this
    virtual std::shared_ptr<Session> createSession(asiolib::ip::tcp::socket sock) = 0;

private:
    void accept()
    {
        m_acceptor.async_accept(m_socket, [this](asiolibec::error_code ec) {
            if (!ec)
            {
                createSession(std::move(m_socket))->start();
            }
            accept();
        });
    }
    asiolib::ip::tcp::acceptor m_acceptor;
    asiolib::ip::tcp::socket   m_socket;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_SERVER_H_INCLUDED__
