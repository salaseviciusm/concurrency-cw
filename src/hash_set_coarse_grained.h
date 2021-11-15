#ifndef HASH_SET_COARSE_GRAINED_H
#define HASH_SET_COARSE_GRAINED_H

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetCoarseGrained : public HashSetBase<T> {
 public:
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

    size_t hash = hash_(elem);
    size_t index = hash % initial_capacity_;
    std::vector<T>& bucket = table_.at(index);

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

    size_t hash = hash_(elem);
    size_t index = hash % initial_capacity_;
    std::vector<T>& bucket = table_.at(index);

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

    size_t hash = hash_(elem);
    size_t index = hash % initial_capacity_;
    std::vector<T> bucket = table_.at(index);

    return std::find(bucket.begin(), bucket.end(), elem) != bucket.end();
  }

  [[nodiscard]] size_t Size() const final {
    std::scoped_lock<std::mutex> scoped_lock(mutex_);
    return current_size_;
  }

 private:
  bool Policy() { return current_size_ / initial_capacity_ > 4; }

  void Resize() {
    initial_capacity_ *= 2;
    auto old_table = table_;
    table_ = std::vector<std::vector<T>>(initial_capacity_);
    for (std::vector<T> bucket : old_table) {
      for (T elem : bucket) {
        size_t index = hash_(elem) % initial_capacity_;
        std::vector<T>& curr_bucket = table_.at(index);
        curr_bucket.push_back(elem);
      }
    }
  }

  std::hash<T> hash_;
  size_t initial_capacity_;
  size_t current_size_;
  std::vector<std::vector<T>> table_;
  mutable std::mutex mutex_;
};

#endif  // HASH_SET_COARSE_GRAINED_H
