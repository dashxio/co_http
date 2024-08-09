#pragma once
#include "io_context.h"
#include "http_parser.hpp"
#include <optional>

struct HttpServer;

struct HttpState {
    virtual void update(HttpServer* hd) = 0;
    virtual ~HttpState() = default;
};

struct AcceptState : HttpState {
    void update(HttpServer* h) override;
};

struct ReadState : HttpState {
    void update(HttpServer* h) override;
};

struct HandleRequestState : HttpState {
    void update(HttpServer* h) override;
};

struct WriteState : HttpState {
    void update(HttpServer* h) override;
};

struct HttpServer : std::enable_shared_from_this<HttpServer> {
    struct HttpServerSkin {
        WSAOVERLAPPED m_overlapped = WSAOVERLAPPED{};
        std::shared_ptr<HttpServer> m_handler;

        void update(size_t io_bytes_num = 0) {
            m_handler->update(io_bytes_num);
        }

    };

    LPWSAOVERLAPPED getOverlappedSkin() {
        HttpServerSkin* handler_skin = new HttpServerSkin;
        handler_skin->m_handler = shared_from_this(); //这里增加了HttpServer的引用计数，避免了在外部在调用h->update()之后h被析构
        return &handler_skin->m_overlapped;
    }

    std::unique_ptr<HttpState> m_state;

    async_file m_listen;
    address_resolver::address m_addr;

    async_file m_conn;
    bytes_buffer m_readbuf{ 1024 };
    //size_t m_expected_complete_bytes_num = 0;
    std::optional<size_t> m_really_complete_bytes_num = std::nullopt;
    http_request_parser<> m_req_parser;
    http_response_writer<> m_res_writer;

    static std::shared_ptr<HttpServer> makeServerWithListenPort(std::string name, std::string port);

    static std::shared_ptr<HttpServer> makeServerWithConnectSocket(SOCKET socket);

    std::shared_ptr<HttpServer> setState(HttpState* state) {
        m_state = std::unique_ptr<HttpState>(state);
        return shared_from_this();
    }

    //void getCompleteBytesNumThisTime() {
    //    auto temp = m_really_complete_bytes_num;
    //    m_really_complete_bytes_num = 0;

    //}

    void update(std::optional<size_t> io_bytes_num = std::nullopt) {
        m_really_complete_bytes_num = io_bytes_num;
        m_state->update(this);
    }

    void start() {
        return update();
    }

    //~HttpServer();
};
