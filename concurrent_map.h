#pragma once

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <cstddef>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(std::size_t bucket_count)
        : vector_maps_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        const std::size_t num_of_bukets = static_cast<uint64_t>(key) % vector_maps_.size();
        return { std::lock_guard(vector_maps_[num_of_bukets].mutex),
            vector_maps_[num_of_bukets].map[key] };
    }

    void erase(const Key& key) {
        const std::size_t num_of_bukets = static_cast<uint64_t>(key) % vector_maps_.size();
        std::lock_guard(vector_maps_[num_of_bukets].mutex);
        vector_maps_[num_of_bukets].map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [map, mutex] : vector_maps_) {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    struct Buket {
        std::map<Key, Value> map;
        std::mutex mutex;
    };

    std::vector<Buket> vector_maps_;
};