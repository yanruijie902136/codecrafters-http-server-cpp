#include <iostream>
#include <cstdlib>
#include <set>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpStatus.hpp"

int main(int argc, char **argv) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cout << "Logs from your program will appear here!\n";

    // Uncomment this block to pass the first stage

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(4221);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    fd_set readfds, readfds_copy;
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    std::set<int> client_fds;

    std::cout << "Waiting for a client to connect...\n";

    for ( ; ; ) {
        std::memcpy(&readfds_copy, &readfds, sizeof(readfds));
        select(FD_SETSIZE, &readfds_copy, nullptr, nullptr, nullptr);

        if (FD_ISSET(server_fd, &readfds_copy)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            std::cout << "Client connected\n";
            FD_SET(client_fd, &readfds);
            client_fds.insert(client_fd);
        }

        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            int client_fd = *it;
            if (!FD_ISSET(client_fd, &readfds_copy)) {
                it++;
                continue;
            }

            HttpRequest request = HttpRequest::read_from_socket(client_fd);
            // std::cout << request << "\n";

            HttpResponse response;
            if (request.target == "/user-agent") {
                response.status = HttpStatus::OK;
                response.body = request.headers["User-Agent"];
                response.headers["Content-Type"] = "text/plain";
                response.headers["Content-Length"] = std::to_string(response.body.length());
            }
            else if (request.target.starts_with("/echo/")) {
                response.status = HttpStatus::OK;
                response.body = request.target.substr(6);
                response.headers["Content-Type"] = "text/plain";
                response.headers["Content-Length"] = std::to_string(response.body.length());
            }
            else {
                response.status = request.target == "/" ? HttpStatus::OK : HttpStatus::NOT_FOUND;
            }

            std::string response_str = response.str();
            send(client_fd, response_str.c_str(), response_str.length(), 0);
        }
    }
}
