#pragma once

#include <map>
#include <mutex>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Buket {
        std::map<Key, Value> map;
        std::mutex mutex;
    };

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : bucket_count_(bucket_count),
        vector_maps_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        size_t num_of_bukets = static_cast<uint64_t>(key) % bucket_count_;
        return { std::lock_guard(vector_maps_[num_of_bukets].mutex),
            vector_maps_[num_of_bukets].map[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& buket : vector_maps_) {
            std::lock_guard guard(buket.mutex);
            result.insert(buket.map.begin(), buket.map.end());
        }
        return result;
    }

private:
    size_t bucket_count_;
    std::vector<Buket> vector_maps_;
};