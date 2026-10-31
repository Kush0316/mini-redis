#pragma once

#include <chrono>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

class KVStore {
public:
    // Monotonic stopwatch that never jumps backward if system time changes
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Max number of keys before LRU eviction kicks in
    explicit KVStore(size_t max_capacity = 1000);

    // Store a key-value pair, with an optional TTL in milliseconds
    void set(const std::string& key, const std::string& value,
             std::optional<long long> ttl_ms = std::nullopt);

    // Fetch a value by key; returns std::nullopt if missing or expired
    std::optional<std::string> get(const std::string& key);

    // Delete a key manually; returns true if it existed
    bool del(const std::string& key);

    // Active expiration: scan up to max_checks keys and purge expired ones
    size_t cleanup_expired(size_t max_checks = 20);

    // Current number of keys in the store
    size_t size() const;

private:
    struct CacheEntry {
        std::string value;
        std::list<std::string>::iterator lru_it; // Pointer directly into lru_list_
        std::optional<TimePoint> expires_at;     // Empty if key never expires
    };

    size_t max_capacity_;
    std::list<std::string> lru_list_; // Front = Most Recent, Back = Least Recent
    std::unordered_map<std::string, CacheEntry> store_;

    // Helper to check if a specific entry has passed its TTL deadline
    bool is_expired(const CacheEntry& entry) const;

    // Helper to kick out the oldest key when we hit max_capacity_
    void evict_if_needed();
};