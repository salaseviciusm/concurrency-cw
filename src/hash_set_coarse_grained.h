#ifndef HASH_SET_COARSE_GRAINED_H
#define HASH_SET_COARSE_GRAINED_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <mutex>
#include <thread>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetCoarseGrained : public HashSetBase<T> {
 public:
  explicit HashSetCoarseGrained(size_t initial_capacity)
      : initial_capacity_(initial_capacity), current_size_(0) {
    table_.reserve(initial_capacity);
    for (size_t i = 0; i < initial_capacity; i++) {
      table_.push_back(std::vector<T>());
    }
  }

  bool Add(T elem) final {
    std::scoped_lock<std::mutex> scoped_lock(mutex_);

    std::vector<T>& bucket = table_.at(GetIndex(elem));

    auto iter = bucket.begin();
    while (iter != bucket.end()) {
      if (*iter == elem) {
        return false;
      }
      ++iter;
    }

    current_size_++;
    bucket.push_back(elem);

    if (Policy()) {
      Resize();
    }

    return true;
  }

  bool Remove(T elem) final {
    std::scoped_lock<std::mutex> scoped_lock(mutex_);

    if (current_size_ == 0) return false;

    std::vector<T>& bucket = table_.at(GetIndex(elem));

    auto iter = bucket.begin();

    while (iter != bucket.end()) {
      if (*iter == elem) {
        bucket.erase(iter);
        current_size_--;
        return true;
      }
      ++iter;
    }

    return false;
  }

  bool Contains(T elem) final {
    std::scoped_lock<std::mutex> scoped_lock(mutex_);

    std::vector<T> bucket = table_.at(GetIndex(elem));

    return std::find(bucket.begin(), bucket.end(), elem) != bucket.end();
  }

  [[nodiscard]] size_t Size() const final { return current_size_.load(); }

 private:
  bool Policy() { return current_size_ / initial_capacity_ > 4; }

  void Resize() {
    initial_capacity_ *= 2;
    auto old_table = table_;
    table_ = std::vector<std::vector<T>>(initial_capacity_);
    for (std::vector<T> bucket : old_table) {
      for (T elem : bucket) {
        std::vector<T>& curr_bucket = table_.at(GetIndex(elem));
        curr_bucket.push_back(elem);
      }
    }
  }

  size_t GetIndex(T elem) { return hash_(elem) % initial_capacity_; }

  std::hash<T> hash_;
  size_t initial_capacity_;
  std::atomic<size_t> current_size_;
  std::vector<std::vector<T>> table_;
  std::mutex mutex_;
};

#endif  // HASH_SET_COARSE_GRAINED_H
