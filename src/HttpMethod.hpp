#ifndef CODECRAFTERS_HTTP_SERVER_HTTPMETHOD_HPP_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTPMETHOD_HPP_INCLUDED

#include <string>

enum class HttpMethod {
    GET,
    POST,
};

inline std::string method_to_str(HttpMethod method) {
    switch (method) {
    case HttpMethod::GET:
        return "GET";
    case HttpMethod::POST:
        return "POST";
    }
}

#endif
