#include "kv_store.hpp"
#include "tcp_server.hpp"

int main() {
    // Initialize in-memory store with a capacity of 1000 keys
    KVStore store(1000);

    // Start TCP server on default Redis port 6379
    TcpServer server(6379, store);
    server.start();

    return 0;
}