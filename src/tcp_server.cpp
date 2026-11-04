#include "tcp_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

TcpServer::TcpServer(uint16_t port, KVStore& store)
    : port_(port), server_fd_(-1), kv_store_(store) {}

TcpServer::~TcpServer() {
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

// --- COPY-PASTE THIS OS SOCKET SETUP BOILERPLATE ---
void TcpServer::start() {
    // 1. socket(): Ask macOS kernel for an IPv4 (AF_INET) TCP (SOCK_STREAM) endpoint
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    // Enable SO_REUSEADDR so we can restart the server immediately after Ctrl+C
    // without waiting 60s for the TCP TIME_WAIT state to clear
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. bind(): Attach our socket to localhost:6379
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any local network interface
    address.sin_port = htons(port_);      // Convert port number to network byte order

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Failed to bind to port " << port_ << " (Is another server running?)\n";
        return;
    }

    // 3. listen(): Tell the OS to queue up to 128 incoming client handshakes
    if (listen(server_fd_, 128) < 0) {
        std::cerr << "Failed to listen on socket\n";
        return;
    }

    std::cout << "[Mini-Redis] Server listening on port " << port_ << "...\n";

    // 4. accept(): Wait for clients to connect
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // accept() sleeps at 0% CPU until a client connects, then returns a new fd for that client
        int client_fd = accept(server_fd_, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
        if (client_fd < 0) {
            continue;
        }

        std::cout << "[Mini-Redis] Client connected (fd=" << client_fd << ")\n";
        handle_client(client_fd);
        close(client_fd);
        std::cout << "[Mini-Redis] Client disconnected (fd=" << client_fd << ")\n";
    }
}

// MAIN LOGICAL FUNCTIONS ---
void TcpServer::handle_client(int client_fd) {
    char buffer[4096];

    while (true) {
        std::memset(buffer, 0, sizeof(buffer));

        // Read raw bytes sent by the client over the TCP connection
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            break; // 0 means the client closed the terminal/connection
        }

        std::string input(buffer, bytes_read);
        std::string response = process_command(input);

        // Send the response string back across the network to the client
        write(client_fd, response.c_str(), response.size());
    }
}

std::string TcpServer::process_command(const std::string& command_line) {
    std::stringstream ss(command_line);
    std::vector<std::string> tokens;
    std::string word;

    // Split the incoming line by whitespace into individual words
    while (ss >> word) {
        tokens.push_back(word);
    }

    if (tokens.empty()) {
        return "";
    }

    // Convert command verb (first word) to uppercase so both "get" and "GET" work
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (cmd == "PING") {
        return "+PONG\n";
    }

    if (cmd == "SET") {
        if (tokens.size() < 3) {
            return "-ERR wrong number of arguments for 'SET'\n";
        }
        std::optional<long long> ttl_ms = std::nullopt;

        // Check if the user passed an optional expiration: SET key value EX <ms>
        if (tokens.size() >= 5) {
            std::string opt = tokens[3];
            std::transform(opt.begin(), opt.end(), opt.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            if (opt == "EX" || opt == "PX") {
                try {
                    long long val = std::stoll(tokens[4]);
                    // EX = seconds, PX = milliseconds (standard Redis convention)
                    ttl_ms = (opt == "EX") ? val * 1000 : val;
                } catch (...) {
                    return "-ERR invalid expire time in 'SET'\n";
                }
            }
        }

        kv_store_.set(tokens[1], tokens[2], ttl_ms);
        return "+OK\n";
    }

    if (cmd == "GET") {
        if (tokens.size() < 2) {
            return "-ERR wrong number of arguments for 'GET'\n";
        }
        auto val = kv_store_.get(tokens[1]);
        if (!val.has_value()) {
            return "(nil)\n";
        }
        return "\"" + val.value() + "\"\n";
    }

    if (cmd == "DEL") {
        if (tokens.size() < 2) {
            return "-ERR wrong number of arguments for 'DEL'\n";
        }
        bool deleted = kv_store_.del(tokens[1]);
        return deleted ? ":1\n" : ":0\n";
    }

    return "-ERR unknown command '" + tokens[0] + "'\n";
}