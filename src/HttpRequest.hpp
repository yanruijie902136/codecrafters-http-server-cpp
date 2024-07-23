#ifndef CODECRAFTERS_HTTP_SERVER_HTTPREQUEST_HPP_INCLUDED
#define CODECRAFTERS_HTTP_SERVER_HTTPREQUEST_HPP_INCLUDED

#include <cassert>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "HttpMethod.hpp"

class HttpRequest {
public:
    static std::pair<HttpRequest, bool> read_from_socket(int socket_fd) {
        HttpRequest request;

        static char input_buffer[1024];
        ssize_t r = recv(socket_fd, input_buffer, sizeof(input_buffer), 0);
        if (r == 0) {
            return {request, false};
        }
        input_buffer[r] = 0;

        std::stringstream ss(input_buffer);
        std::string line;

        // Parse the first line.
        std::getline(ss, line);
        assert(line.starts_with("GET "));
        request.method = HttpMethod::GET;
        std::size_t start_pos = line.find(' ') + 1, end_pos = line.rfind(' ');
        request.target = line.substr(start_pos, end_pos - start_pos);

        // Parse the headers.
        while (std::getline(ss, line)) {
            assert(line.back() == '\r');
            line.pop_back();
            if (line.empty()) {
                break;
            }

            std::size_t pos = line.find(':');
            std::string field = line.substr(0, pos);
            while (line[++pos] == ' ') {
            }
            std::string value = line.substr(pos);
            while (value.back() == ' ') {
                value.pop_back();
            }

            request.headers[field] = value;
        }

        // Extract the body.
        std::getline(ss, request.body);

        return {request, true};
    }

    // For debugging purposes.
    friend std::ostream &operator<<(std::ostream &os, const HttpRequest &request) {
        os << "method: " << method_to_str(request.method) << "\n";
        os << "target: " << request.target << "\n";
        for (const auto &it : request.headers) {
            os << it.first << ": " << it.second << "\n";
        }
        os << "body: " << request.body << "\n";
        return os;
    }

public:
    HttpMethod method;
    std::string target;
    std::map<std::string, std::string> headers;
    std::string body;
};

#endif
