/**
 * @file types.hpp
 * @brief HTTP common types and structures
 *
 * Common types used across HTTP Server functionality.
 */

#pragma once

#include <stdint.h>

#include "core/types.hpp"

namespace alloy::http {

/**
 * @brief HTTP request methods
 */
enum class Method : uint8_t {
    GET = 0,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
};

/**
 * @brief HTTP status codes
 */
enum class Status : uint16_t {
    // 1xx Informational
    Continue = 100,
    SwitchingProtocols = 101,

    // 2xx Success
    OK = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,

    // 3xx Redirection
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    // 4xx Client Error
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PayloadTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    TooManyRequests = 429,

    // 5xx Server Error
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
};

/**
 * @brief Convert HTTP method to string
 */
inline const char* method_to_string(Method method) {
    switch (method) {
        case Method::GET:
            return "GET";
        case Method::POST:
            return "POST";
        case Method::PUT:
            return "PUT";
        case Method::DELETE:
            return "DELETE";
        case Method::HEAD:
            return "HEAD";
        case Method::OPTIONS:
            return "OPTIONS";
        case Method::PATCH:
            return "PATCH";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Convert string to HTTP method
 */
inline Method string_to_method(const char* str) {
    if (str == nullptr)
        return Method::GET;

    // Simple string comparison (case-sensitive)
    if (str[0] == 'G' && str[1] == 'E' && str[2] == 'T' && str[3] == '\0')
        return Method::GET;
    if (str[0] == 'P' && str[1] == 'O' && str[2] == 'S' && str[3] == 'T' && str[4] == '\0')
        return Method::POST;
    if (str[0] == 'P' && str[1] == 'U' && str[2] == 'T' && str[3] == '\0')
        return Method::PUT;
    if (str[0] == 'D' && str[1] == 'E' && str[2] == 'L')
        return Method::DELETE;
    if (str[0] == 'H' && str[1] == 'E' && str[2] == 'A' && str[3] == 'D' && str[4] == '\0')
        return Method::HEAD;
    if (str[0] == 'O' && str[1] == 'P' && str[2] == 'T')
        return Method::OPTIONS;
    if (str[0] == 'P' && str[1] == 'A' && str[2] == 'T' && str[3] == 'C' && str[4] == 'H' &&
        str[5] == '\0')
        return Method::PATCH;

    return Method::GET;  // Default
}

/**
 * @brief Get status code description
 */
inline const char* status_description(Status status) {
    switch (status) {
        case Status::Continue:
            return "Continue";
        case Status::SwitchingProtocols:
            return "Switching Protocols";
        case Status::OK:
            return "OK";
        case Status::Created:
            return "Created";
        case Status::Accepted:
            return "Accepted";
        case Status::NoContent:
            return "No Content";
        case Status::MovedPermanently:
            return "Moved Permanently";
        case Status::Found:
            return "Found";
        case Status::SeeOther:
            return "See Other";
        case Status::NotModified:
            return "Not Modified";
        case Status::TemporaryRedirect:
            return "Temporary Redirect";
        case Status::PermanentRedirect:
            return "Permanent Redirect";
        case Status::BadRequest:
            return "Bad Request";
        case Status::Unauthorized:
            return "Unauthorized";
        case Status::Forbidden:
            return "Forbidden";
        case Status::NotFound:
            return "Not Found";
        case Status::MethodNotAllowed:
            return "Method Not Allowed";
        case Status::NotAcceptable:
            return "Not Acceptable";
        case Status::RequestTimeout:
            return "Request Timeout";
        case Status::Conflict:
            return "Conflict";
        case Status::Gone:
            return "Gone";
        case Status::LengthRequired:
            return "Length Required";
        case Status::PayloadTooLarge:
            return "Payload Too Large";
        case Status::URITooLong:
            return "URI Too Long";
        case Status::UnsupportedMediaType:
            return "Unsupported Media Type";
        case Status::TooManyRequests:
            return "Too Many Requests";
        case Status::InternalServerError:
            return "Internal Server Error";
        case Status::NotImplemented:
            return "Not Implemented";
        case Status::BadGateway:
            return "Bad Gateway";
        case Status::ServiceUnavailable:
            return "Service Unavailable";
        case Status::GatewayTimeout:
            return "Gateway Timeout";
        case Status::HTTPVersionNotSupported:
            return "HTTP Version Not Supported";
        default:
            return "Unknown";
    }
}

}  // namespace alloy::http
