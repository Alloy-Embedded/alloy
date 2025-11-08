/**
 * @file server.cpp
 * @brief HTTP Server implementation
 */

#include "server.hpp"

#ifdef ESP_PLATFORM
    #include <cstring>

    #include "esp_log.h"

namespace alloy::http {

static const char* TAG = "http_server";

// Structure to hold handler and context
struct HandlerContext {
    Handler handler;
};

// ============================================================================
// Server Implementation
// ============================================================================

Server::Server()
    : config_(),
      running_(false)
    #ifdef ESP_PLATFORM
      ,
      server_handle_(nullptr)
    #endif
{
}

Server::Server(uint16_t port)
    : config_(),
      running_(false)
    #ifdef ESP_PLATFORM
      ,
      server_handle_(nullptr)
    #endif
{
    config_.port = port;
}

Server::Server(const ServerConfig& config)
    : config_(config),
      running_(false)
    #ifdef ESP_PLATFORM
      ,
      server_handle_(nullptr)
    #endif
{
}

Server::~Server() {
    if (running_) {
        stop();
    }
}

Result<void> Server::start() {
    if (running_) {
        return Result<void>::ok();
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = config_.port;
    config.max_uri_handlers = config_.max_uri_handlers;
    config.max_resp_headers = config_.max_resp_headers;
    config.stack_size = config_.stack_size;
    config.task_priority = config_.task_priority;
    config.lru_purge_enable = config_.lru_purge_enable;

    ESP_TRY(httpd_start(&server_handle_, &config));

    running_ = true;
    ESP_LOGI(TAG, "HTTP server started on port %d", config_.port);

    return Result<void>::ok();
}

Result<void> Server::stop() {
    if (!running_) {
        return Result<void>::ok();
    }

    if (server_handle_) {
        ESP_TRY(httpd_stop(server_handle_));
        server_handle_ = nullptr;
    }

    running_ = false;
    ESP_LOGI(TAG, "HTTP server stopped");

    return Result<void>::ok();
}

bool Server::is_running() const {
    return running_;
}

Result<void> Server::on(Method method, const char* uri, Handler handler) {
    if (!running_) {
        return Result<void>::error(ErrorCode::NotInitialized);
    }

    if (uri == nullptr || handler == nullptr) {
        return Result<void>::error(ErrorCode::InvalidParameter);
    }

    // Create context (will be freed by ESP-IDF when unregistering)
    HandlerContext* ctx = new HandlerContext{handler};

    // Convert method
    httpd_method_t httpd_method;
    switch (method) {
        case Method::GET:
            httpd_method = HTTP_GET;
            break;
        case Method::POST:
            httpd_method = HTTP_POST;
            break;
        case Method::PUT:
            httpd_method = HTTP_PUT;
            break;
        case Method::DELETE:
            httpd_method = HTTP_DELETE;
            break;
        case Method::HEAD:
            httpd_method = HTTP_HEAD;
            break;
        case Method::OPTIONS:
            httpd_method = HTTP_OPTIONS;
            break;
        case Method::PATCH:
            httpd_method = HTTP_PATCH;
            break;
        default:
            httpd_method = HTTP_GET;
            break;
    }

    // Register handler
    httpd_uri_t uri_config = {
        .uri = uri, .method = httpd_method, .handler = &Server::handler_wrapper, .user_ctx = ctx};

    esp_err_t err = httpd_register_uri_handler(server_handle_, &uri_config);
    if (err != ESP_OK) {
        delete ctx;
        return core::esp_result_error_void(err);
    }

    ESP_LOGI(TAG, "Registered %s %s", method_to_string(method), uri);
    return Result<void>::ok();
}

Result<void> Server::get(const char* uri, Handler handler) {
    return on(Method::GET, uri, handler);
}

Result<void> Server::post(const char* uri, Handler handler) {
    return on(Method::POST, uri, handler);
}

Result<void> Server::put(const char* uri, Handler handler) {
    return on(Method::PUT, uri, handler);
}

Result<void> Server::delete_(const char* uri, Handler handler) {
    return on(Method::DELETE, uri, handler);
}

uint16_t Server::port() const {
    return config_.port;
}

httpd_handle_t Server::native_handle() const {
    return server_handle_;
}

esp_err_t Server::handler_wrapper(httpd_req_t* req) {
    // Get handler from context
    HandlerContext* ctx = static_cast<HandlerContext*>(req->user_ctx);
    if (ctx == nullptr || ctx->handler == nullptr) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Handler not found");
        return ESP_FAIL;
    }

    // Create Request and Response wrappers
    Request request(req);
    Response response(req);

    // Call user handler
    (void)ctx->handler(request, response);

    // Status was set by response.send(), so we just return
    return ESP_OK;
}

// ============================================================================
// Request Implementation
// ============================================================================

Request::Request(httpd_req_t* req) : req_(req) {}

Method Request::method() const {
    switch (req_->method) {
        case HTTP_GET:
            return Method::GET;
        case HTTP_POST:
            return Method::POST;
        case HTTP_PUT:
            return Method::PUT;
        case HTTP_DELETE:
            return Method::DELETE;
        case HTTP_HEAD:
            return Method::HEAD;
        case HTTP_OPTIONS:
            return Method::OPTIONS;
        case HTTP_PATCH:
            return Method::PATCH;
        default:
            return Method::GET;
    }
}

const char* Request::uri() const {
    return req_->uri;
}

Result<size_t> Request::query(const char* key, char* buffer, size_t buffer_size) const {
    if (key == nullptr || buffer == nullptr || buffer_size == 0) {
        return Result<size_t>::error(ErrorCode::InvalidParameter);
    }

    // Get query string
    size_t query_len = httpd_req_get_url_query_len(req_);
    if (query_len == 0) {
        buffer[0] = '\0';
        return Result<size_t>::ok(0);
    }

    // Allocate buffer for query string
    char* query_str = new char[query_len + 1];
    if (httpd_req_get_url_query_str(req_, query_str, query_len + 1) != ESP_OK) {
        delete[] query_str;
        return Result<size_t>::error(ErrorCode::HardwareError);
    }

    // Get parameter value
    esp_err_t err = httpd_query_key_value(query_str, key, buffer, buffer_size);
    delete[] query_str;

    if (err == ESP_ERR_NOT_FOUND) {
        buffer[0] = '\0';
        return Result<size_t>::ok(0);
    } else if (err != ESP_OK) {
        return core::esp_result_error<size_t>(err);
    }

    return Result<size_t>::ok(strlen(buffer));
}

Result<size_t> Request::header(const char* key, char* buffer, size_t buffer_size) const {
    if (key == nullptr || buffer == nullptr || buffer_size == 0) {
        return Result<size_t>::error(ErrorCode::InvalidParameter);
    }

    size_t header_len = httpd_req_get_hdr_value_len(req_, key);
    if (header_len == 0) {
        buffer[0] = '\0';
        return Result<size_t>::ok(0);
    }

    if (header_len >= buffer_size) {
        return Result<size_t>::error(ErrorCode::BufferFull);
    }

    ESP_TRY_T(size_t, httpd_req_get_hdr_value_str(req_, key, buffer, buffer_size));

    return Result<size_t>::ok(strlen(buffer));
}

Result<size_t> Request::body(char* buffer, size_t buffer_size) const {
    if (buffer == nullptr || buffer_size == 0) {
        return Result<size_t>::error(ErrorCode::InvalidParameter);
    }

    size_t content_len = req_->content_len;
    if (content_len == 0) {
        buffer[0] = '\0';
        return Result<size_t>::ok(0);
    }

    if (content_len >= buffer_size) {
        return Result<size_t>::error(ErrorCode::BufferFull);
    }

    int ret = httpd_req_recv(req_, buffer, buffer_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            return Result<size_t>::error(ErrorCode::Timeout);
        }
        return Result<size_t>::error(ErrorCode::CommunicationError);
    }

    buffer[ret] = '\0';  // Null-terminate
    return Result<size_t>::ok(ret);
}

size_t Request::content_length() const {
    return req_->content_len;
}

httpd_req_t* Request::native_handle() const {
    return req_;
}

// ============================================================================
// Response Implementation
// ============================================================================

Response::Response(httpd_req_t* req) : req_(req), status_(Status::OK) {}

Response& Response::status(Status status) {
    status_ = status;
    return *this;
}

Response& Response::header(const char* key, const char* value) {
    if (key && value) {
        httpd_resp_set_hdr(req_, key, value);
    }
    return *this;
}

Response& Response::type(const char* content_type) {
    if (content_type) {
        httpd_resp_set_type(req_, content_type);
    }
    return *this;
}

Result<void> Response::send(const char* body) {
    if (body == nullptr) {
        return send("", 0);
    }
    return send(body, strlen(body));
}

Result<void> Response::send(const char* body, size_t length) {
    // Set status
    const char* status_str = status_description(status_);
    httpd_resp_set_status(req_, status_str);

    // Send response
    ESP_TRY(httpd_resp_send(req_, body, length));

    return Result<void>::ok();
}

Result<void> Response::json(const char* json) {
    type("application/json");
    return send(json);
}

Result<void> Response::html(const char* html) {
    type("text/html");
    return send(html);
}

httpd_req_t* Response::native_handle() const {
    return req_;
}

}  // namespace alloy::http

#else  // !ESP_PLATFORM

namespace alloy::http {

// Stub implementations for non-ESP platforms

Server::Server() : config_(), running_(false) {}
Server::Server(uint16_t port) : config_(), running_(false) {
    config_.port = port;
}
Server::Server(const ServerConfig& config) : config_(config), running_(false) {}
Server::~Server() {}

Result<void> Server::start() {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Server::stop() {
    return Result<void>::error(ErrorCode::NotSupported);
}

bool Server::is_running() const {
    return false;
}

Result<void> Server::on(Method, const char*, Handler) {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Server::get(const char*, Handler) {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Server::post(const char*, Handler) {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Server::put(const char*, Handler) {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Server::delete_(const char*, Handler) {
    return Result<void>::error(ErrorCode::NotSupported);
}

uint16_t Server::port() const {
    return config_.port;
}

// Request stubs
Request::Request() {}
Method Request::method() const {
    return Method::GET;
}
const char* Request::uri() const {
    return "";
}
Result<size_t> Request::query(const char*, char*, size_t) const {
    return Result<size_t>::error(ErrorCode::NotSupported);
}
Result<size_t> Request::header(const char*, char*, size_t) const {
    return Result<size_t>::error(ErrorCode::NotSupported);
}
Result<size_t> Request::body(char*, size_t) const {
    return Result<size_t>::error(ErrorCode::NotSupported);
}
size_t Request::content_length() const {
    return 0;
}

// Response stubs
Response::Response() : status_(Status::OK) {}
Response& Response::status(Status status) {
    status_ = status;
    return *this;
}
Response& Response::header(const char*, const char*) {
    return *this;
}
Response& Response::type(const char*) {
    return *this;
}
Result<void> Response::send(const char*) {
    return Result<void>::error(ErrorCode::NotSupported);
}
Result<void> Response::send(const char*, size_t) {
    return Result<void>::error(ErrorCode::NotSupported);
}
Result<void> Response::json(const char*) {
    return Result<void>::error(ErrorCode::NotSupported);
}
Result<void> Response::html(const char*) {
    return Result<void>::error(ErrorCode::NotSupported);
}

}  // namespace alloy::http

#endif  // ESP_PLATFORM
