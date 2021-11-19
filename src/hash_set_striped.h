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
    mutexs_[GetMutexIndex(elem)].lock();
    std::vector<T>& bucket = table_.at(GetBucketIndex(elem));

    auto iter = bucket.begin();
    while (iter != bucket.end()) {
      if (*iter == elem) {
        mutexs_[GetMutexIndex(elem)].unlock();
        return false;
      }
      ++iter;
    }

    size_++;
    bucket.push_back(elem);

    mutexs_[GetMutexIndex(elem)].unlock();

    size_t size = Size();
    if (Policy(size)) {
      Resize();
    }

    return true;
  }

  bool Remove(T elem) final {
    std::scoped_lock<std::mutex> scoped_lock(mutexs_[GetMutexIndex(elem)]);

    if (size_.load() == 0) return false;

    std::vector<T>& bucket = table_.at(GetBucketIndex(elem));

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
    std::scoped_lock<std::mutex> scoped_lock(mutexs_[GetMutexIndex(elem)]);
    std::vector<T> bucket = table_.at(GetBucketIndex(elem));

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
        std::vector<T>& curr_bucket = table_.at(GetBucketIndex(elem));
        curr_bucket.push_back(elem);
      }
    }

    for (size_t i = 0; i < initial_capacity_; i++) {
      mutexs_[i].unlock();
    }
  }

  size_t GetBucketIndex(T elem) { return hash_(elem) % current_capacity_; }
  size_t GetMutexIndex(T elem) { return hash_(elem) % initial_capacity_; }

  std::hash<T> hash_;
  size_t const initial_capacity_;
  std::atomic<size_t> current_capacity_;
  std::vector<std::vector<T>> table_;
  std::mutex* mutexs_;
  std::atomic<size_t> size_;
};

#endif  // HASH_SET_STRIPED_H
