#pragma once

#include <memory>

#include "IOStream.h"
#include "MaaFramework/MaaPort.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MAA_UTILS_API SockIOStream;

class MAA_UTILS_API ServerSockIOFactory : public NonCopyButMovable
{
public:
    ServerSockIOFactory(const std::string& address, unsigned short port);
    ~ServerSockIOFactory();

    unsigned short port() const;

public:
    std::shared_ptr<SockIOStream> accept();

private:
    boost::asio::io_context io_ctx_;
    boost::asio::ip::tcp::acceptor server_acceptor_;
};

class MAA_UTILS_API ClientSockIOFactory : public NonCopyButMovable
{
public:
    ClientSockIOFactory(const std::string& address, unsigned short port);
    ~ClientSockIOFactory() = default;

public:
    std::shared_ptr<SockIOStream> connect();

private:
    boost::asio::io_context io_ctx_;
    boost::asio::ip::tcp::endpoint endpoint_;
};

class MAA_UTILS_API SockIOStream : public IOStream
{
public:
    SockIOStream(boost::asio::ip::tcp::iostream&& ios);
    virtual ~SockIOStream() override;

public:
    virtual bool write(std::string_view data) override;

    virtual bool release() override;
    virtual bool is_open() const override;

protected:
    virtual std::string read_once(size_t max_count) override;

private:
    boost::asio::ip::tcp::iostream ios_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
