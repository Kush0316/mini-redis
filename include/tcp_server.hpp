#pragma once

#include "kv_store.hpp"
#include <cstdint>
#include <string>

class TcpServer {
public:
    TcpServer(uint16_t port, KVStore& store);
    ~TcpServer();

    // Opens port 6379 and enters the accept() loop
    void start();

private:
    uint16_t port_;
    int server_fd_;
    KVStore& kv_store_; // Reference to our Day 1 in-memory engine

    // Reads commands from a connected client socket and sends back replies
    void handle_client(int client_fd);

    // Parses a raw text command ("SET key val EX 5000") and calls kv_store_
    std::string process_command(const std::string& command_line);
};