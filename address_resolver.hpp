#pragma once

#include "expected.hpp"

struct address_resolver {
    struct address_ref {
        struct sockaddr* m_addr;
        size_t m_addrlen;
    };

    struct address {
        union {
            struct sockaddr m_addr;
            struct sockaddr_storage m_addr_storage;
        };
        size_t m_addrlen = sizeof(struct sockaddr_storage);

        operator address_ref() {
            return { &m_addr, m_addrlen };
        }
    };

    struct address_info {
        struct addrinfo* m_curr = nullptr;

        address_ref get_address() const {
            return { m_curr->ai_addr, m_curr->ai_addrlen };
        }

        SOCKET create_socket() const {
            return convert_error(socket(m_curr->ai_family, m_curr->ai_socktype, m_curr->ai_protocol)).expect("socket");
        }

        SOCKET create_socket_and_bind() const {
            auto sockfd = create_socket();
            address_ref serve_addr = get_address();
            //int on = 1;
            //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            //setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
            u_long mode = 1;
            ioctlsocket(sockfd, FIONBIO, &mode);

            convert_error(bind(sockfd, serve_addr.m_addr, (int)serve_addr.m_addrlen)).expect("bind");
            convert_error(listen(sockfd, SOMAXCONN)).expect("listen");

            int n_zero = 0;
            convert_error(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&n_zero, sizeof(n_zero))).expect("setsockopt");

            return sockfd;
        }

        [[nodiscard]] bool next_entry() {
            m_curr = m_curr->ai_next;
            if (m_curr == nullptr) {
                return false;
            }
            return true;
        }

        address_info() = default;
        address_info(struct addrinfo* addr) : m_curr(addr) {};

        address_info(address_info&& that) noexcept : m_curr(that.m_curr)  {
            that.m_curr = nullptr;
        }

        ~address_info() {
            if (m_curr) {
                freeaddrinfo(m_curr);
            }
        }
    };

    address_info resolve(std::string const& name, std::string const& service) {
        struct addrinfo* head = nullptr;
        convert_error(getaddrinfo(name.c_str(), service.c_str(), NULL, &head)).expect("getaddrinfo");
        return { head };
    }

    address_resolver() = default;

};