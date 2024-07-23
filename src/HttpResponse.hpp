#ifndef CODECRAFTERS_HTTP_SERVER_HTTPRESPONSE_HPP_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTPRESPONSE_HPP_INCLUDED

#include <map>
#include <string>

#include "HttpStatus.hpp"

class HttpResponse {
public:
    std::string str() {
        std::string response_str;
        response_str += "HTTP/1.1 " + std::to_string(status_to_int(status)) + " " + status_to_str(status) + "\r\n";
        for (const auto &it : headers) {
            response_str += it.first + ": " + it.second + "\r\n";
        }
        response_str += "\r\n";
        response_str += body;
        return response_str;
    }

public:
    HttpStatus status;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif
