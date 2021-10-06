#pragma once

#include <map>
#include <vector>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap
{
public:
    static_assert(std::is_integral_v<Key>);

    struct Access
    {
        explicit Access(std::map<Key, Value> &map, const Key &key, std::mutex &value_mutex)
            : guard(value_mutex), ref_to_value(map[key])
        {
        }
        std::lock_guard<std::mutex> guard;
        Value &ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : bucket_count_(bucket_count), buckets_(bucket_count)
    {
    }

    Access operator[](const Key &key)
    {
        uint64_t bucket_number = static_cast<uint64_t>(key) % bucket_count_;
        return Access(buckets_[bucket_number].values, key, buckets_[bucket_number].value_mutex);
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (const auto &bucket : buckets_)
        {
            for (const auto &[key, val] : bucket.values)
            {
                uint64_t bucket_number = static_cast<uint64_t>(key) % bucket_count_;
                std::lock_guard<std::mutex> guard(buckets_[bucket_number].value_mutex);
                result[key] = val;
            }
        }
        return result;
    }

private:
    struct Bucket
    {
        std::map<Key, Value> values;
        std::mutex value_mutex;
    };
    size_t bucket_count_;
    std::vector<Bucket> buckets_;
};