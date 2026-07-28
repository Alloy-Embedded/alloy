// Minimal HTTP/1.1 server over the Socket facade — serve a page with no RTOS and
// no malloc. Builds on tcp_listener + tcp_socket: accept one connection, buffer the
// request headers, hand (method, path) to a user handler, then stream its response
// (status line + headers + body) back and close. One connection at a time,
// Connection: close (no keep-alive); GET-oriented (any request body is ignored).
// The handler's response body must point at memory that outlives the send — a
// static page. Drive http_server::poll(handler) from the stack::poll() loop, e.g.
//   while (true) { net.poll(); http.poll([](auto& rq){ return page(rq); }); }
// Host-tested over lwIP loopback (tests/test_net.cpp).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "alloy/net/socket.hpp"

#include "lwip/sys.h"  // sys_now() — not pulled in transitively under NO_SYS

namespace alloy::net {

// A parsed request line. Views into the server's request buffer — valid only for
// the duration of the handler call.
struct http_request {
    std::string_view method;  // e.g. "GET"
    std::string_view path;    // e.g. "/", "/status"
};

// A response for the server to send. `body` must outlive the send (static memory);
// the status line + headers are formatted by the server.
struct http_response {
    unsigned status = 200;
    std::string_view reason = "OK";
    std::string_view content_type = "text/html";
    std::string_view body;
};

// ReqBuf caps the request headers buffered before a 431 is returned; Backlog is the
// listener's pending-accept depth.
template <std::uint32_t ReqBuf = 1024, std::uint32_t Backlog = 4>
class http_server {
    tcp_listener<Backlog> lst_;
    tcp_socket conn_;
    enum class phase { idle, receiving, sending };
    phase phase_ = phase::idle;

    std::uint8_t req_[ReqBuf];
    std::uint32_t req_len_ = 0;

    char hdr_[256];           // formatted status line + headers
    std::uint32_t hdr_len_ = 0;
    std::string_view body_;   // handler-owned (static) body; valid until fully sent
    std::uint32_t sent_ = 0;  // bytes of (hdr_ ++ body_) already written
    std::uint32_t accepted_at_ = 0;  // sys_now() when the current connection was accepted

    // Index just past the "\r\n\r\n" ending the headers, or -1 if not present yet.
    static int headers_end(const std::uint8_t* b, std::uint32_t n) {
        for (std::uint32_t i = 3; i < n; ++i) {
            if (b[i - 3] == '\r' && b[i - 2] == '\n' && b[i - 1] == '\r' && b[i] == '\n') {
                return static_cast<int>(i + 1);
            }
        }
        return -1;
    }

    // Parse "METHOD SP PATH SP VERSION" from the first line (bounded to n).
    static http_request parse_request(const std::uint8_t* b, std::uint32_t n) {
        const auto* c = reinterpret_cast<const char*>(b);
        std::uint32_t eol = 0;
        while (eol < n && c[eol] != '\r' && c[eol] != '\n') ++eol;  // end of first line
        std::uint32_t m = 0;
        while (m < eol && c[m] != ' ') ++m;                        // end of method
        std::uint32_t ps = (m < eol) ? m + 1 : eol;               // path start
        std::uint32_t pe = ps;
        while (pe < eol && c[pe] != ' ') ++pe;                     // end of path
        return {std::string_view{c, m}, std::string_view{c + ps, pe - ps}};
    }

    // Bounded appends into hdr_ at hdr_len_ (never past sizeof(hdr_)).
    void put(std::string_view s) {
        const std::uint32_t room = static_cast<std::uint32_t>(sizeof(hdr_)) - hdr_len_;
        const std::uint32_t n = s.size() < room ? static_cast<std::uint32_t>(s.size()) : room;
        std::memcpy(hdr_ + hdr_len_, s.data(), n);
        hdr_len_ += n;
    }
    void put_uint(unsigned v) {
        char t[10];
        int k = 0;
        do { t[k++] = static_cast<char>('0' + v % 10); v /= 10; } while (v != 0);
        while (k-- > 0 && hdr_len_ < sizeof(hdr_)) hdr_[hdr_len_++] = t[k];
    }

    void build_header(const http_response& r) {
        hdr_len_ = 0;
        put("HTTP/1.1 "); put_uint(r.status); put(" "); put(r.reason); put("\r\n");
        put("Content-Type: "); put(r.content_type); put("\r\n");
        put("Content-Length: "); put_uint(static_cast<unsigned>(r.body.size())); put("\r\n");
        put("Connection: close\r\n\r\n");
        body_ = r.body;
        sent_ = 0;
        phase_ = phase::sending;
    }

    // Has the current connection blown its request/response deadline? Unsigned
    // subtraction is wraparound-safe over sys_now()'s 32-bit millisecond counter.
    [[nodiscard]] bool timed_out() const {
        return (sys_now() - accepted_at_) >= request_timeout_ms;
    }

public:
    // A stalled peer — a slowloris that never finishes the request, or a reader
    // that stops draining mid-response — is dropped after this long, so it can't
    // hold the single connection hostage. sys_now() is the timebase.
    static constexpr std::uint32_t request_timeout_ms = 10000;

    [[nodiscard]] bool listen(std::uint16_t port) { return lst_.listen(port); }

    // Drive from the poll loop. handler: (const http_request&) -> http_response.
    template <class Handler>
    void poll(Handler&& handler) {
        if (phase_ == phase::idle) {
            if (!lst_.accept(conn_)) return;
            phase_ = phase::receiving;
            req_len_ = 0;
            accepted_at_ = sys_now();
        }
        if (phase_ == phase::receiving) {
            if (req_len_ < ReqBuf) {
                req_len_ += conn_.recv({req_ + req_len_, ReqBuf - req_len_});
            }
            if (headers_end(req_, req_len_) >= 0) {
                build_header(handler(parse_request(req_, req_len_)));  // -> sending
            } else if (req_len_ >= ReqBuf) {
                build_header({431, "Request Header Fields Too Large", "text/plain",
                              "request too large\n"});                  // -> sending
            } else if (conn_.eof() || timed_out()) {  // peer left, or stalled (slowloris)
                conn_.close();
                phase_ = phase::idle;
                return;
            } else {
                return;  // headers still incoming — resume next poll
            }
        }
        if (phase_ == phase::sending) {
            if (!conn_.alive()) {  // peer RST/aborted mid-send — free the server now
                conn_.close();
                phase_ = phase::idle;
                return;
            }
            const std::uint32_t total = hdr_len_ + static_cast<std::uint32_t>(body_.size());
            while (sent_ < total) {
                const std::span<const std::uint8_t> piece =
                    (sent_ < hdr_len_)
                        ? std::span<const std::uint8_t>{
                              reinterpret_cast<const std::uint8_t*>(hdr_) + sent_,
                              hdr_len_ - sent_}
                        : std::span<const std::uint8_t>{
                              reinterpret_cast<const std::uint8_t*>(body_.data()) + (sent_ - hdr_len_),
                              body_.size() - (sent_ - hdr_len_)};
                const std::uint32_t n = conn_.send(piece);
                if (n == 0) break;  // send window full — resume next poll
                sent_ += n;
            }
            if (sent_ >= total || timed_out()) {  // done, or a reader that stopped draining
                conn_.close();
                phase_ = phase::idle;
            }
        }
    }
};

}  // namespace alloy::net
