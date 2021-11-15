#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <stdio.h>

#include <algorithm>
#include <cassert>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetSequential : public HashSetBase<T> {
 public:
  explicit HashSetSequential(size_t initial_capacity)
      : initial_capacity_(initial_capacity), current_size_(0) {
    table_.reserve(initial_capacity);
    for (size_t i = 0; i < initial_capacity; i++) {
      table_.push_back(std::vector<T>());
    }
  }

  bool Add(T elem) final {
    size_t hash = hash_(elem);

    size_t index = hash % initial_capacity_;
    std::vector<T>& current_vector = table_.at(index);

    auto iter = current_vector.begin();
    while (iter != current_vector.end()) {
      if (*iter == elem) {
        return false;
      }
      ++iter;
    }

    current_size_++;
    current_vector.push_back(elem);

    if (Policy()) {
      Resize();
    }

    return true;
  }

  bool Remove(T elem) final {
    size_t hash = hash_(elem);
    size_t index = hash % initial_capacity_;
    std::vector<T>& current_vector = table_.at(index);

    auto iter = current_vector.begin();

    while (iter != current_vector.end()) {
      if (*iter == elem) {
        current_vector.erase(iter);
        current_size_--;
        return true;
      }
      ++iter;
    }

    return false;
  }

  bool Contains(T elem) final {
    size_t hash = hash_(elem);
    size_t index = hash % initial_capacity_;
    std::vector<T> current_vector = table_.at(index);

    return std::find(current_vector.begin(), current_vector.end(), elem) !=
           current_vector.end();
  }

  [[nodiscard]] size_t Size() const final { return current_size_; }

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
};

#endif  // HASH_SET_SEQUENTIAL_H
