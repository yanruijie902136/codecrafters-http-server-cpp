#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <set>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <cassert>
#include <vector>
#include <algorithm>
#include <zconf.h>
#include <zlib.h>

#include "HttpMethod.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpStatus.hpp"

static std::vector<std::string> delimit_string(const std::string &str, const std::string &delimiter) {
    std::vector<std::string> res;
    std::size_t pos = 0, nextpos;
    while ((nextpos = str.find(delimiter, pos)) != std::string::npos) {
        res.push_back(str.substr(pos, nextpos - pos));
        pos = nextpos + delimiter.length();
    }
    res.push_back(str.substr(pos));
    return res;
}

std::pair<char *, uLong> gzip_compress(const char *source, uLong source_len) {
    static char buffer[8192];

    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = (uInt)source_len;
    zs.next_in = (Bytef *)source;
    zs.avail_out = (uInt)sizeof(buffer);
    zs.next_out = (Bytef *)buffer;

    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);
    return {buffer, zs.total_out};
}

int main(int argc, char **argv) {
    std::filesystem::path root_dir;
    if (argc > 2) {
        root_dir = argv[2];
    }

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

            auto [request, connected] = HttpRequest::read_from_socket(client_fd);
            if (!connected) {
                close(client_fd);
                FD_CLR(client_fd, &readfds);
                client_fds.erase(it++);
                continue;
            }

            HttpResponse response;
            if (request.method == HttpMethod::POST) {
                assert(request.target.starts_with("/files/"));
                std::filesystem::path file_path = request.target.substr(7);
                std::ofstream fp(root_dir / file_path);
                fp << request.body;
                response.status = HttpStatus::CREATED;
            }
            else if (request.target == "/user-agent") {
                response.status = HttpStatus::OK;
                response.body = request.headers["User-Agent"];
                response.headers["Content-Type"] = "text/plain";
                response.headers["Content-Length"] = std::to_string(response.body.length());
            }
            else if (request.target.starts_with("/echo/")) {
                response.status = HttpStatus::OK;
                response.body = request.target.substr(6);
                if (request.headers.find("Accept-Encoding") != request.headers.end()) {
                    auto compression_schemes = delimit_string(request.headers["Accept-Encoding"], ", ");
                    if (std::find(compression_schemes.begin(), compression_schemes.end(), "gzip") != compression_schemes.end()) {
                        response.headers["Content-Encoding"] = "gzip";
                        auto [buffer, size] = gzip_compress(response.body.c_str(), response.body.length());
                        response.body = std::string(buffer, size);
                    }
                }
                response.headers["Content-Type"] = "text/plain";
                response.headers["Content-Length"] = std::to_string(response.body.length());
            }
            else if (request.target.starts_with("/files/")) {
                std::filesystem::path file_path = request.target.substr(7);
                std::ifstream fp(root_dir / file_path);
                if (fp.fail()) {
                    response.status = HttpStatus::NOT_FOUND;
                }
                else {
                    response.status = HttpStatus::OK;
                    std::stringstream ss;
                    ss << fp.rdbuf();
                    response.body = ss.str();
                    response.headers["Content-Type"] = "application/octet-stream";
                    response.headers["Content-Length"] = std::to_string(response.body.length());
                }
            }
            else {
                response.status = request.target == "/" ? HttpStatus::OK : HttpStatus::NOT_FOUND;
            }

            std::string response_str = response.str();
            send(client_fd, response_str.c_str(), response_str.length(), 0);

            it++;
        }
    }
}
