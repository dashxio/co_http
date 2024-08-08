#pragma once
#include <boost/locale.hpp>
#include <WinSock2.h>
#include <fmt/format.h>

using boost::locale::conv::utf_to_utf;
using boost::locale::conv::to_utf;

//std::error_category const& gai_category() {
//    static struct final : std::error_category {
//        const char* name() const noexcept override {
//            return "getaddrinfo";
//        }
//
//        std::string message(int err) const override {
//            return utf_to_utf<char>(gai_strerrorW(err));
//        }
//    } instance;
//    return instance;
//}

inline std::error_category const& utf8_system_category() {
    static struct final : std::_System_error_category {
        std::string message(int err) const override {
            return to_utf<char>(_System_error_category::message(err), "GBK");
        }
    } instance;
    return instance;
}

template <class T = int, class R = int>
struct [[nodiscard]] expected {

    std::make_signed_t<T> m_res;
    R m_real_value;

    expected() = default;
    //expected(std::make_signed_t<T> res) noexcept : m_res(res) {}

    bool error() const noexcept {
        if (m_res < 0) {
            return true;
        }
        return false;
    }

    bool is_error(int err) const noexcept {
        return m_res == -err;
    }

    std::error_code error_code() const noexcept {
        if (m_res < 0) {
            return std::error_code((int)-m_res, utf8_system_category());
        }
        return std::error_code();
    }

    R expect(const char* what) const {
        if (m_res < 0) {
            auto ec = error_code();
            fmt::println(stderr, "{}: {}", what, ec.message());
            throw std::system_error(ec, what);
        }
        return m_real_value;
    }

    R value() const {
        if (m_res < 0) {
            auto ec = error_code();
            fmt::println(stderr, "{}", ec.message());
            throw std::system_error(ec);
        }
        // assert(m_res >= 0);
        return m_real_value;
    }

    R raw_value() const {
        return m_real_value;
    }
};

//template <class U, class T>
//expected<U, T> convert_error(T res) {
//    if (res == -1) {
//        return  { -WSAGetLastError(), res };
//    }
//    return { 0, res };
//}
//
//template <int = 0, class T>
//expected<size_t, T> convert_error(T res) {
//    if (res == -1) {
//        return { -WSAGetLastError(), res };
//    }
//    return { 0, res };
//}


//struct WindowsBool {
//    BOOL m_value = FALSE; //0被认为是false
//    WindowsBool(BOOL value) : m_value(value) {}
//    operator BOOL() noexcept {
//        return m_value;
//    }
//};
//
//struct WindowsSocket {
//    SOCKET m_value = INVALID_SOCKET; //-1被认为是INVALID_SOCKET
//    WindowsSocket(SOCKET value) : m_value(value) {}
//    operator SOCKET() noexcept {
//        return m_value;
//    }
//};
//
//struct WindowsHandle {
//    HANDLE m_value = NULL; //0被认为是NULL
//    WindowsHandle(HANDLE value) : m_value(value) {}
//    operator HANDLE() noexcept {
//        return m_value;
//    }
//};

#define ForGetWSALastError 0
#define ForGetLastError 1

template <int err_value = -1, int err_flag = ForGetWSALastError, typename U = size_t, typename T>
inline expected<U, T> convert_error(T res) {
    int right_value;
    if constexpr (err_value == -1) {
        right_value = 0;
    }
    else {
        right_value = 1;
    }

    if (res == err_value) {
        if constexpr (err_flag == ForGetWSALastError) {
            return { -(std::make_signed_t<U>)WSAGetLastError(), res };
        }
        else {
            return { -(std::make_signed_t<U>)GetLastError(), res };
        }

    }
    return { right_value, res };
}

//expected<size_t, int> convert_error(int res) {
//    if (res == -1) {
//        return { -GetLastError(), res };
//    }
//    return { 0, res };
//}
//
//expected<size_t, BOOL> convert_error(BOOL res) {
//    if (res == FALSE) {
//        return { -GetLastError(), FALSE };
//    }
//    return { 0, res };
//}
//
//expected<size_t, SOCKET> convert_error(SOCKET res) {
//    if (res == INVALID_SOCKET) {
//        return { -WSAGetLastError(), INVALID_SOCKET };
//    }
//    return { 0, res };
//}
//

inline expected<DWORD, HANDLE> convert_error(HANDLE res) {
    if (res == NULL) {
        return  { -std::make_signed_t<DWORD>(GetLastError()), NULL };
    }
    return { 0, res };
}
