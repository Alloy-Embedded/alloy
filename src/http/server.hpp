/**
 * @file server.hpp
 * @brief HTTP Server abstraction
 *
 * Provides a clean C++ interface for ESP32 HTTP server functionality.
 * Wraps esp_http_server with RAII pattern and modern C++ API.
 *
 * Example:
 * @code
 * #include "http/server.hpp"
 *
 * using alloy::http::Server;
 *
 * Server server(80);
 * auto result = server.start();
 *
 * server.on(Method::GET, "/hello", [](Request& req, Response& res) {
 *     res.send("Hello, World!");
 *     return Status::OK;
 * });
 * @endcode
 */

#pragma once

#include "core/error.hpp"
#include "core/esp_error.hpp"

#include "types.hpp"

#ifdef ESP_PLATFORM
    #include "esp_http_server.h"
#endif

namespace alloy::http {

using core::ErrorCode;
using core::Result;

// Forward declarations
class Request;
class Response;

/**
 * @brief HTTP request handler function type
 *
 * Handler receives Request and Response objects and returns Status code.
 * The handler should process the request and send a response.
 */
using Handler = Status (*)(Request& req, Response& res);

/**
 * @brief HTTP Server configuration
 */
struct ServerConfig {
    uint16_t port;              ///< Server port (default: 80)
    uint16_t max_uri_handlers;  ///< Maximum URI handlers (default: 8)
    uint16_t max_resp_headers;  ///< Maximum response headers (default: 8)
    size_t stack_size;          ///< Task stack size (default: 4096)
    uint8_t task_priority;      ///< Task priority (default: 5)
    bool lru_purge_enable;      ///< Enable LRU purge (default: false)

    ServerConfig()
        : port(80),
          max_uri_handlers(8),
          max_resp_headers(8),
          stack_size(4096),
          task_priority(5),
          lru_purge_enable(false) {}
};

/**
 * @brief HTTP Server
 *
 * Manages HTTP server lifecycle and route registration.
 * Uses RAII pattern: starts on construction, stops on destruction.
 *
 * Note: Only one server instance should be active at a time.
 */
class Server {
   public:
    /**
     * @brief Constructor with default configuration
     */
    Server();

    /**
     * @brief Constructor with custom port
     * @param port Server port
     */
    explicit Server(uint16_t port);

    /**
     * @brief Constructor with full configuration
     * @param config Server configuration
     */
    explicit Server(const ServerConfig& config);

    /**
     * @brief Destructor - stops server if running
     */
    ~Server();

    // Prevent copying
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief Start the HTTP server
     *
     * @return Result<void> - Ok if successful
     */
    Result<void> start();

    /**
     * @brief Stop the HTTP server
     *
     * @return Result<void> - Ok if successful
     */
    Result<void> stop();

    /**
     * @brief Check if server is running
     *
     * @return true if server is active
     */
    bool is_running() const;

    /**
     * @brief Register a route handler
     *
     * @param method HTTP method (GET, POST, etc.)
     * @param uri URI path (e.g., "/api/status")
     * @param handler Handler function
     * @return Result<void> - Ok if successful
     */
    Result<void> on(Method method, const char* uri, Handler handler);

    /**
     * @brief Register a GET route
     *
     * @param uri URI path
     * @param handler Handler function
     * @return Result<void> - Ok if successful
     */
    Result<void> get(const char* uri, Handler handler);

    /**
     * @brief Register a POST route
     *
     * @param uri URI path
     * @param handler Handler function
     * @return Result<void> - Ok if successful
     */
    Result<void> post(const char* uri, Handler handler);

    /**
     * @brief Register a PUT route
     *
     * @param uri URI path
     * @param handler Handler function
     * @return Result<void> - Ok if successful
     */
    Result<void> put(const char* uri, Handler handler);

    /**
     * @brief Register a DELETE route
     *
     * @param uri URI path
     * @param handler Handler function
     * @return Result<void> - Ok if successful
     */
    Result<void> delete_(const char* uri, Handler handler);

    /**
     * @brief Get server port
     *
     * @return Server port number
     */
    uint16_t port() const;

#ifdef ESP_PLATFORM
    /**
     * @brief Get native ESP-IDF server handle
     *
     * @return httpd_handle_t or nullptr if not running
     */
    httpd_handle_t native_handle() const;
#endif

   private:
    ServerConfig config_;
    bool running_;

#ifdef ESP_PLATFORM
    httpd_handle_t server_handle_;

    static esp_err_t handler_wrapper(httpd_req_t* req);
#endif
};

/**
 * @brief HTTP Request wrapper
 *
 * Wraps ESP-IDF httpd_req_t with convenient C++ API
 */
class Request {
   public:
#ifdef ESP_PLATFORM
    explicit Request(httpd_req_t* req);
#else
    Request();
#endif

    /**
     * @brief Get HTTP method
     *
     * @return HTTP method (GET, POST, etc.)
     */
    Method method() const;

    /**
     * @brief Get request URI
     *
     * @return URI string
     */
    const char* uri() const;

    /**
     * @brief Get query parameter value
     *
     * @param key Parameter name
     * @param buffer Buffer to store value
     * @param buffer_size Buffer size
     * @return Result<size_t> - Number of bytes written, or error
     */
    Result<size_t> query(const char* key, char* buffer, size_t buffer_size) const;

    /**
     * @brief Get header value
     *
     * @param key Header name
     * @param buffer Buffer to store value
     * @param buffer_size Buffer size
     * @return Result<size_t> - Number of bytes written, or error
     */
    Result<size_t> header(const char* key, char* buffer, size_t buffer_size) const;

    /**
     * @brief Get request body
     *
     * @param buffer Buffer to store body
     * @param buffer_size Buffer size
     * @return Result<size_t> - Number of bytes read, or error
     */
    Result<size_t> body(char* buffer, size_t buffer_size) const;

    /**
     * @brief Get content length
     *
     * @return Content length in bytes
     */
    size_t content_length() const;

#ifdef ESP_PLATFORM
    /**
     * @brief Get native ESP-IDF request handle
     *
     * @return httpd_req_t pointer
     */
    httpd_req_t* native_handle() const;
#endif

   private:
#ifdef ESP_PLATFORM
    httpd_req_t* req_;
#endif
};

/**
 * @brief HTTP Response wrapper
 *
 * Provides convenient API for sending HTTP responses
 */
class Response {
   public:
#ifdef ESP_PLATFORM
    explicit Response(httpd_req_t* req);
#else
    Response();
#endif

    /**
     * @brief Set response status code
     *
     * @param status HTTP status code
     * @return Reference to self for chaining
     */
    Response& status(Status status);

    /**
     * @brief Set response header
     *
     * @param key Header name
     * @param value Header value
     * @return Reference to self for chaining
     */
    Response& header(const char* key, const char* value);

    /**
     * @brief Set Content-Type header
     *
     * @param content_type Content type (e.g., "text/html", "application/json")
     * @return Reference to self for chaining
     */
    Response& type(const char* content_type);

    /**
     * @brief Send response body
     *
     * Sends the response with the accumulated status and headers.
     *
     * @param body Response body content
     * @return Result<void> - Ok if successful
     */
    Result<void> send(const char* body);

    /**
     * @brief Send response body with length
     *
     * @param body Response body content
     * @param length Body length in bytes
     * @return Result<void> - Ok if successful
     */
    Result<void> send(const char* body, size_t length);

    /**
     * @brief Send JSON response
     *
     * Sets Content-Type to application/json and sends body.
     *
     * @param json JSON string
     * @return Result<void> - Ok if successful
     */
    Result<void> json(const char* json);

    /**
     * @brief Send HTML response
     *
     * Sets Content-Type to text/html and sends body.
     *
     * @param html HTML string
     * @return Result<void> - Ok if successful
     */
    Result<void> html(const char* html);

#ifdef ESP_PLATFORM
    /**
     * @brief Get native ESP-IDF request handle
     *
     * @return httpd_req_t pointer
     */
    httpd_req_t* native_handle() const;
#endif

   private:
#ifdef ESP_PLATFORM
    httpd_req_t* req_;
#endif
    Status status_;
};

}  // namespace alloy::http
