#ifndef CODECRAFTERS_HTTP_SERVER_HTTPSTATUS_HPP_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTPSTATUS_HPP_INCLUDED

#include <string>

enum class HttpStatus {
    OK,
    NOT_FOUND,
};

inline int status_to_int(HttpStatus status) {
    switch (status) {
    case HttpStatus::OK:
        return 200;
    case HttpStatus::NOT_FOUND:
        return 404;
    }
}

inline std::string status_to_str(HttpStatus status) {
    switch (status) {
    case HttpStatus::OK:
        return "OK";
    case HttpStatus::NOT_FOUND:
        return "Not Found";
    }
}

#endif
