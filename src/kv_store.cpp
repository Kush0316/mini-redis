#include "kv_store.hpp"

KVStore::KVStore(size_t max_capacity) : max_capacity_(max_capacity) {}

void KVStore::set(const std::string& key, const std::string& value,
                  std::optional<long long> ttl_ms) {
    if (max_capacity_ == 0) return;

    // 1. Calculate expiration deadline if the user gave us a TTL
    std::optional<TimePoint> expires_at = std::nullopt;
    if (ttl_ms.has_value()) {
        expires_at = Clock::now() + std::chrono::milliseconds(ttl_ms.value());
    }

    // 2. If key already exists, update it in place and move to front of LRU
    auto it = store_.find(key);
    if (it != store_.end()) {
        it->second.value = value;
        it->second.expires_at = expires_at;
        // splice moves the existing node to the front with ZERO memory allocation
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second.lru_it);
        return;
    }

    // 3. New key: if we are at full capacity, evict the least recently used key
    evict_if_needed();

    // 4. Put the new key at the front of the LRU list and store its iterator in the map
    lru_list_.push_front(key);
    store_[key] = CacheEntry{value, lru_list_.begin(), expires_at};
}

std::optional<std::string> KVStore::get(const std::string& key) {
    auto it = store_.find(key);
    if (it == store_.end()) {
        return std::nullopt; // Key doesn't exist
    }

    // Lazy Expiration: check if the key died before returning it
    if (is_expired(it->second)) {
        lru_list_.erase(it->second.lru_it);
        store_.erase(it);
        return std::nullopt;
    }

    // Key is alive! Move it to the front of the LRU list since it was just accessed
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second.lru_it);
    return it->second.value;
}

bool KVStore::del(const std::string& key) {
    auto it = store_.find(key);
    if (it == store_.end()) {
        return false;
    }

    lru_list_.erase(it->second.lru_it);
    store_.erase(it);
    return true;
}

size_t KVStore::cleanup_expired(size_t max_checks) {
    size_t removed = 0;
    size_t checked = 0;

    for (auto it = store_.begin(); it != store_.end() && checked < max_checks; ) {
        checked++;
        if (is_expired(it->second)) {
            lru_list_.erase(it->second.lru_it);
            // store_.erase(it) returns the next valid iterator safely
            it = store_.erase(it);
            removed++;
        } else {
            ++it;
        }
    }
    return removed;
}

size_t KVStore::size() const {
    return store_.size();
}

bool KVStore::is_expired(const CacheEntry& entry) const {
    if (!entry.expires_at.has_value()) {
        return false; // No TTL set, lives forever
    }
    return Clock::now() >= entry.expires_at.value();
}

void KVStore::evict_if_needed() {
    // First try purging expired keys so we don't kick out a valid key unnecessarily
    cleanup_expired(5);

    while (store_.size() >= max_capacity_ && !lru_list_.empty()) {
        // The key at the very back of the list is the Least Recently Used
        const std::string& oldest_key = lru_list_.back();
        store_.erase(oldest_key);
        lru_list_.pop_back();
    }
}