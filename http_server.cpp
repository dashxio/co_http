#include "http_server.h"
#include "fmt/format.h"

std::shared_ptr<HttpServer> HttpServer::makeServerWithListenPort(std::string name, std::string port)
{
    std::shared_ptr<HttpServer> h = std::make_shared<HttpServer>();

    address_resolver resolver;
    fmt::println("正在监听：{}:{}", name, port);
    auto entry = resolver.resolve(name, port);
    SOCKET listenfd = entry.create_socket_and_bind();

    h->m_listen = async_file{ listenfd };
    h->setState(std::make_unique<AcceptState>().release());
    return h;
}

std::shared_ptr<HttpServer> HttpServer::makeServerWithConnectSocket(SOCKET socket)
{
	std::shared_ptr<HttpServer> h = std::make_shared<HttpServer>();;
	h->m_conn = async_file{ socket };
	h->setState(std::make_unique<ReadState>().release());
	return h;
}

//HttpServer::~HttpServer()
//{
//	fmt::println("一个连接断开了：{}", m_conn.m_fd);
//}

void AcceptState::update(HttpServer* h) {

    while (true) {
        auto conn_socket = h->m_listen.async_accept(h->m_addr, h->getOverlappedSkin());
        if (conn_socket == INVALID_SOCKET) {
            return;
        }
        //fmt::println("接受了一个连接：{}", conn_socket);
        HttpServer::makeServerWithConnectSocket(conn_socket)->start();
    }
}

void ReadState::update(HttpServer* h) {

    if (h->m_really_complete_bytes_num) {
        if (*h->m_really_complete_bytes_num == 0) {
            //fmt::println("一个连接断开了：{}", h->m_conn.m_fd);
            return;
        }
        h->m_req_parser.push_chunk(h->m_readbuf.subspan(0, *h->m_really_complete_bytes_num));
    }

    if (h->m_req_parser.request_finished()) {
        h->setState(std::make_unique<HandleRequestState>().release())->update();
    }
    else {
        h->m_conn.async_read(h->m_readbuf, h->getOverlappedSkin());
    }

}

void HandleRequestState::update(HttpServer* h) {
    std::string body = std::move(h->m_req_parser.body());
    h->m_req_parser.reset_state();

    if (body.empty()) {
        body = "你好，你的请求正文为空哦";
    }
    else {
        body = fmt::format("你好，你的请求是: [{}]，共 {} 字节", body, body.size());
    }

    h->m_res_writer.begin_header(200);
    h->m_res_writer.write_header("Server", "co_http");
    h->m_res_writer.write_header("Content-type", "text/html;charset=utf-8");
    h->m_res_writer.write_header("Connection", "keep-alive");
    h->m_res_writer.write_header("Content-length", std::to_string(body.size()));
    h->m_res_writer.end_header();
    h->m_res_writer.write_body(body);

    // fmt::println("我的响应头: {}", buffer);
    // fmt::println("我的响应正文: {}", body);
    // fmt::println("正在响应");

    h->setState(std::make_unique<WriteState>().release())->update();
}

void WriteState::update(HttpServer* h) {

    if (h->m_really_complete_bytes_num && *h->m_really_complete_bytes_num == 0) {
        //fmt::println("一个连接断开了：{}", h->m_conn.m_fd);
		return;
    }

    if (!h->m_res_writer.write_finished(h->m_really_complete_bytes_num.value_or(0))) {
        h->m_conn.async_write(h->m_res_writer.buffer_need_to_write(), h->getOverlappedSkin());
    }
    else {
        h->m_res_writer.reset_state();
        h->setState(std::make_unique<ReadState>().release())->update();
    }

}

//struct HttpServer : std::enable_shared_from_this<HttpServer> {
//    struct HttpServerSkin {
//        WSAOVERLAPPED m_overlapped;
//        std::shared_ptr<HttpServer> m_handler;
//
//        void update(size_t io_bytes_num = 0) {
//            m_handler->update(io_bytes_num);
//        }
//
//    };
//
//    LPWSAOVERLAPPED getOverlappedSkin() {
//        HttpServerSkin* handler_skin = new HttpServerSkin;
//        handler_skin->m_handler = shared_from_this(); //这里增加了HttpServer的引用计数，避免了在外部在调用h->update()之后h被析构
//        return &handler_skin->m_overlapped;
//    }
//
//    std::unique_ptr<HttpState> m_state;
//
//    async_file m_listen;
//    address_resolver::address m_addr;
//
//    async_file m_conn;
//    bytes_buffer m_readbuf{ 1024 };
//    size_t m_expected_complete_bytes_num = 0;
//    size_t m_really_complete_bytes_num = 0;
//    http_request_parser<> m_req_parser;
//    http_response_writer<> m_res_writer;
//
//    static std::shared_ptr<HttpServer> makeServerWithListenPort(std::string name, std::string port) {
//        std::shared_ptr<HttpServer> h = std::make_shared<HttpServer>();
//
//        address_resolver resolver;
//        fmt::println("正在监听：{}:{}", name, port);
//        auto entry = resolver.resolve(name, port);
//        int listenfd = entry.create_socket_and_bind();
//
//        h->m_listen = async_file{ listenfd };
//        h->setState(std::make_unique<AcceptState>().release());
//        return h;
//    }
//
//    static std::shared_ptr<HttpServer> makeServerWithConnectSocket(SOCKET socket) {
//        std::shared_ptr<HttpServer> h = std::make_shared<HttpServer>();;
//        h->m_conn = async_file{ socket };
//        h->setState(std::make_unique<ReadState>().release());
//        return h;
//    }
//
//    std::shared_ptr<HttpServer> setState(HttpState* state) {
//        m_state = std::unique_ptr<HttpState>(state);
//        return shared_from_this();
//    }
//
//    void setReallyCompleteBytesNum(size_t io_complete_bytes_num) {
//        m_really_complete_bytes_num = io_complete_bytes_num;
//    }
//
//    void update(size_t io_bytes_num = 0) {
//        setReallyCompleteBytesNum(io_bytes_num);
//        m_state->update(this);
//    }
//
//    void start() {
//        return update();
//    }
//};



//struct http_connection_handler : std::enable_shared_from_this<http_connection_handler> {
//    async_file m_conn;
//    bytes_buffer m_readbuf{ 1024 };
//    http_request_parser<> m_req_parser;
//    http_response_writer<> m_res_writer;
//
//    using pointer = std::shared_ptr<http_connection_handler>;
//
//    static pointer make() {
//        return std::make_shared<pointer::element_type>();
//    }
//
//    void do_start(int connfd) {
//        m_conn = async_file{ connfd };
//        return do_read();
//    }
//
//    void do_read() {
//        // fmt::println("开始读取...");
//        // 注意：TCP 基于流，可能粘包
//        return m_conn.async_read(m_readbuf, [self = this->shared_from_this()](expected<size_t> ret) {
//            if (ret.error()) {
//                return;
//            }
//            size_t n = ret.value();
//            // 如果读到 EOF，说明对面，关闭了连接
//            if (n == 0) {
//                // fmt::println("收到对面关闭了连接");
//                return;
//            }
//            // fmt::println("读取到了 {} 个字节: {}", n, std::string_view{m_buf.data(), n});
//            // 成功读取，则推入解析
//            self->m_req_parser.push_chunk(self->m_readbuf.subspan(0, n));
//            if (!self->m_req_parser.request_finished()) {
//                return self->do_read();
//            }
//            else {
//                return self->do_handle();
//            }
//            });
//    }
//
//    void do_handle() {
//        std::string body = std::move(m_req_parser.body());
//        m_req_parser.reset_state();
//
//        if (body.empty()) {
//            body = "你好，你的请求正文为空哦";
//        }
//        else {
//            body = fmt::format("你好，你的请求是: [{}]，共 {} 字节", body, body.size());
//        }
//
//        m_res_writer.begin_header(200);
//        m_res_writer.write_header("Server", "co_http");
//        m_res_writer.write_header("Content-type", "text/html;charset=utf-8");
//        m_res_writer.write_header("Connection", "keep-alive");
//        m_res_writer.write_header("Content-length", std::to_string(body.size()));
//        m_res_writer.end_header();
//
//        // fmt::println("我的响应头: {}", buffer);
//        // fmt::println("我的响应正文: {}", body);
//        // fmt::println("正在响应");
//
//        m_res_writer.write_body(body);
//        return do_write(m_res_writer.buffer());
//    }
//
//    void do_write(bytes_const_view buffer) {
//        return m_conn.async_write(buffer, [self = shared_from_this(), buffer](expected<size_t> ret) {
//            if (ret.error())
//                return;
//            auto n = ret.value();
//
//            if (buffer.size() == n) {
//                self->m_res_writer.reset_state();
//                return self->do_read();
//            }
//            return self->do_write(buffer.subspan(n));
//            });
//    }
//};
//
//struct http_acceptor : std::enable_shared_from_this<http_acceptor> {
//    async_file m_listen;
//    address_resolver::address m_addr;
//
//    using pointer = std::shared_ptr<http_acceptor>;
//
//    static pointer make() {
//        return std::make_shared<pointer::element_type>();
//    }
//
//    void do_start(std::string name, std::string port) {
//        address_resolver resolver;
//        fmt::println("正在监听：{}:{}", name, port);
//        auto entry = resolver.resolve(name, port);
//        int listenfd = entry.create_socket_and_bind();
//
//        m_listen = async_file{ listenfd };
//        return do_accept();
//    }
//
//    void do_accept() {
//        return m_listen.async_accept(m_addr, [self = shared_from_this()](expected<int> ret) {
//            auto connfd = ret.expect("accept");
//
//            // fmt::println("接受了一个连接: {}", connfd);
//            http_connection_handler::make()->do_start(connfd);
//            return self->do_accept();
//            });
//    }
//};