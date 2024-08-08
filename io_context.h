#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "expected.hpp"
#include "call_back.hpp"
#include "bytes_buffer.hpp"
#include "address_resolver.hpp"

struct CompletionKey {};

struct io_context {
    //int m_epfd;
    HANDLE m_cphd;

    inline static thread_local io_context* g_instance = nullptr;

    io_context() {
        WSADATA wsaData;
        convert_error(WSAStartup(MAKEWORD(2, 2), &wsaData)).expect("WSAStrartup");

        /*To create an I / O completion port without associating it,
          set the FileHandle parameter to INVALID_HANDLE_VALUE,
          the ExistingCompletionPort parameter to NULL,
          and the CompletionKey parameter to zero(which is ignored in this case).
          Set the NumberOfConcurrentThreads parameter to the desired concurrency value for the new I / O completion port,
          or zero for the default (the number of processors in the system).*/
        m_cphd = convert_error(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)).expect("create complete port");
        g_instance = this;
    }

    callback<> _getQueuedCompletionPackage();

    void join() {
        //std::array<struct epoll_event, 128> events;
        //while (true) {
        //    int ret = epoll_wait(m_epfd, events.data(), events.size(), -1);
        //    if (ret < 0)
        //        throw;
        //    for (int i = 0; i < ret; ++i) {
        //        auto cb = callback<>::from_address(events[i].data.ptr);
        //        cb();
        //    }
        //}
        while (true) {
            auto cb = _getQueuedCompletionPackage();
            cb();
        }
    }

    ~io_context() {
        CloseHandle(m_cphd);
        WSACleanup();
        g_instance = nullptr;
    }

    static io_context& get() {
        assert(g_instance);
        return *g_instance;
    }
};

struct file_descriptor {
    //int m_fd = -1;
    SOCKET m_fd = INVALID_SOCKET;

    file_descriptor() = default;

    explicit file_descriptor(SOCKET fd) : m_fd(fd) {}

    file_descriptor(file_descriptor&& that) noexcept : m_fd(that.m_fd) {
        that.m_fd = INVALID_SOCKET;
    }

    file_descriptor& operator=(file_descriptor&& that) noexcept {
        std::swap(m_fd, that.m_fd);
        return *this;
    }

    ~file_descriptor() {
        if (m_fd == INVALID_SOCKET)
            return;
        closesocket(m_fd);
    }
};

struct async_file : file_descriptor {
    CompletionKey m_com_key;

    async_file() = default;

    explicit async_file(SOCKET fd) : file_descriptor(fd) {
        convert_error(CreateIoCompletionPort((HANDLE)m_fd, io_context::get().m_cphd, (ULONG_PTR)&m_com_key, 0)).expect("Associate file handle with I/O completion port");
    }

    void async_read(bytes_view buf, LPWSAOVERLAPPED lp) {
        WSABUF wsa_buf = buf;
        DWORD dw_flags = 0;
        auto ret = convert_error(WSARecv(m_fd, &wsa_buf, 1, NULL, &dw_flags, lp, NULL));
        if (ret.error() && !ret.is_error(WSA_IO_PENDING)) {
            fmt::println("连接错误地断开了。。。");
            return;
        }
    }

    void async_write(bytes_view buf, LPWSAOVERLAPPED lp) {
        WSABUF wsa_buf = buf;
        DWORD dw_flags = 0;
        auto ret = convert_error(WSASend(m_fd, &wsa_buf, 1, NULL, dw_flags, lp, NULL));
        if (ret.error() && !ret.is_error(WSA_IO_PENDING)) {
            fmt::println("连接错误地断开了。。。");
            return;
        }
    }

    decltype(auto) async_accept(address_resolver::address& addr, LPWSAOVERLAPPED lp) {
        auto ret = convert_error(WSAAccept(m_fd, &addr.m_addr, (int*)&addr.m_addrlen, NULL, 0));
        if (ret.is_error(WSAEWOULDBLOCK)) {
            convert_error<FALSE, ForGetLastError>(PostQueuedCompletionStatus(io_context::get().m_cphd, 0, (ULONG_PTR)&m_com_key, lp)).expect("Post Status");
        }
        return ret.raw_value();
    }

    async_file(async_file&&) = default;
    async_file& operator=(async_file&&) = default;

};