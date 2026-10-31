#include "kv_store.hpp"
#include <iostream>
#include <thread>

int main() {
    std::cout << "=== Testing Day 1: In-Memory KVStore (LRU + TTL) ===\n\n";

    // Create a store that holds a maximum of 3 keys to easily test LRU eviction
    KVStore db(3);

    // 1. Insert 3 keys
    db.set("user:1", "Kushagra");
    db.set("user:2", "Rahul");
    db.set("user:3", "Aman");

    // 2. Access "user:1" so it becomes the MOST recently used!
    // That makes "user:2" the LEAST recently used.
    std::cout << "[GET user:1] -> " << db.get("user:1").value_or("NIL") << "\n";

    // 3. Insert a 4th key. Since capacity is 3, "user:2" should be evicted!
    db.set("user:4", "Priya");

    std::cout << "[After inserting user:4 at capacity 3]\n";
    std::cout << "  user:1 (was touched): " << db.get("user:1").value_or("EVICTED!") << "\n";
    std::cout << "  user:2 (oldest):      " << db.get("user:2").value_or("EVICTED!") << "\n";
    std::cout << "  user:3:               " << db.get("user:3").value_or("EVICTED!") << "\n";
    std::cout << "  user:4 (newest):      " << db.get("user:4").value_or("EVICTED!") << "\n\n";

    // 4. Test TTL Expiration: set an OTP code that expires in 1500 milliseconds (1.5s)
    std::cout << "[Setting 'otp' = '9942' with a 1500ms TTL...]\n";
    db.set("otp", "9942", 1500);
    std::cout << "  Immediate GET otp: " << db.get("otp").value_or("EXPIRED!") << "\n";

    std::cout << "  Sleeping for 1.6 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1600));

    std::cout << "  GET otp after 1.6s: " << db.get("otp").value_or("EXPIRED!") << "\n";

    return 0;
}