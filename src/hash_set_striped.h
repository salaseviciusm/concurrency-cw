#ifndef HASH_SET_STRIPED_H
#define HASH_SET_STRIPED_H

#include <stdio.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <mutex>
#include <thread>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetStriped : public HashSetBase<T> {
 public:
  explicit HashSetStriped(size_t initial_capacity)
      : initial_capacity_(initial_capacity),
        current_capacity_(initial_capacity) {
    table_.reserve(initial_capacity);
    size_.store(0);
    mutexs_ = new std::mutex[initial_capacity];
    for (size_t i = 0; i < initial_capacity; i++) {
      table_.push_back(std::vector<T>());
    }
  }

  ~HashSetStriped() override { delete[] mutexs_; }

  bool Add(T elem) final {
    size_t hash = hash_(elem);
    mutexs_[hash % initial_capacity_].lock();

    size_t index = hash % current_capacity_;
    std::vector<T>& bucket = table_.at(index);

    auto iter = bucket.begin();
    while (iter != bucket.end()) {
      if (*iter == elem) {
        mutexs_[hash % initial_capacity_].unlock();
        return false;
      }
      ++iter;
    }

    size_++;
    bucket.push_back(elem);

    mutexs_[hash % initial_capacity_].unlock();

    size_t size = Size();
    if (Policy(size)) {
      Resize();
    }

    return true;
  }

  bool Remove(T elem) final {
    size_t hash = hash_(elem);
    std::scoped_lock<std::mutex> scoped_lock(mutexs_[hash % initial_capacity_]);

    size_t index = hash % current_capacity_;

    if (size_.load() == 0) return false;

    std::vector<T>& bucket = table_.at(index);

    auto iter = bucket.begin();

    while (iter != bucket.end()) {
      if (*iter == elem) {
        bucket.erase(iter);
        size_--;
        return true;
      }
      ++iter;
    }

    return false;
  }

  bool Contains(T elem) final {
    size_t hash = hash_(elem);
    std::scoped_lock<std::mutex> scoped_lock(mutexs_[hash % initial_capacity_]);

    size_t index = hash % current_capacity_;
    std::vector<T> bucket = table_.at(index);

    return std::find(bucket.begin(), bucket.end(), elem) != bucket.end();
  }

  [[nodiscard]] size_t Size() const final { return size_.load(); }

 private:
  bool Policy(size_t size) { return size / current_capacity_ > 4; }

  void Resize() {
    for (size_t i = 0; i < initial_capacity_; i++) {
      mutexs_[i].lock();
    }

    current_capacity_ = current_capacity_ * 2;
    auto old_table = table_;
    table_ = std::vector<std::vector<T>>(current_capacity_.load());
    for (std::vector<T> bucket : old_table) {
      for (T elem : bucket) {
        size_t index = hash_(elem) % current_capacity_.load();
        std::vector<T>& curr_bucket = table_.at(index);
        curr_bucket.push_back(elem);
      }
    }

    for (size_t i = 0; i < initial_capacity_; i++) {
      mutexs_[i].unlock();
    }
  }

  std::hash<T> hash_;
  size_t const initial_capacity_;
  std::atomic<size_t> current_capacity_;
  std::vector<std::vector<T>> table_;
  std::mutex* mutexs_;
  std::atomic<size_t> size_;
};

#endif  // HASH_SET_STRIPED_H
