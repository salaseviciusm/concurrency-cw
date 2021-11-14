#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include <stdio.h>

#include <cassert>
#include <vector>

#include "src/hash_set_base.h"

template <typename T>
class HashSetSequential : public HashSetBase<T> {
 public:
  explicit HashSetSequential(size_t initial_capacity) {
    _current_size = 0;
    _initial_capacity = initial_capacity;
    for (size_t i = 0; i < initial_capacity; i++) {
      _table.push_back(std::vector<T>());
    }
  }

  bool Add(T elem) final {
    size_t hash = _hash(elem);

    size_t index = hash % _initial_capacity;
    std::vector<T> current_vector = _table.at(index);

    auto iter = current_vector.begin();
    while (iter != current_vector.end()) {
      if (*iter == elem) {
        return false;
      }
      ++iter;
    }

    _current_size++;

    current_vector.push_back(elem);
    _table.at(index) = current_vector;

    return true;
  }

  bool Remove(T elem) final {
    size_t hash = _hash(elem);
    size_t index = hash % _initial_capacity;
    std::vector<T> current_vector = _table.at(index);

    auto iter = current_vector.begin();

    while (iter != current_vector.end()) {
      if (*iter == elem) {
        current_vector.erase(iter);
        _current_size--;
        return true;
      }
      ++iter;
    }

    return false;
  }

  bool Contains(T elem) final {
    size_t hash = _hash(elem);

    size_t index = hash % _initial_capacity;
    std::vector<T> current_vector = _table.at(index);
    auto iter = current_vector.begin();
    while (iter != current_vector.end()) {
      if (*iter == elem) {
        return true;
      }
      ++iter;
    }

    return false;
  }

  [[nodiscard]] size_t Size() const final { return _current_size; }

 private:
  std::hash<T> _hash;
  size_t _initial_capacity;
  size_t _current_size;
  std::vector<std::vector<T>> _table;
};

#endif  // HASH_SET_SEQUENTIAL_H
