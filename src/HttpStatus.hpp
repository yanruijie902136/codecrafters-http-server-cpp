#ifndef CODECRAFTERS_HTTP_SERVER_HTTPSTATUS_HPP_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTPSTATUS_HPP_INCLUDED

#include <string>

enum class HttpStatus {
    OK,
};

inline int status_to_int(HttpStatus status) {
    switch (status) {
    case HttpStatus::OK:
        return 200;
    }
}

inline std::string status_to_str(HttpStatus status) {
    switch (status) {
    case HttpStatus::OK:
        return "OK";
    }
}

#endif
