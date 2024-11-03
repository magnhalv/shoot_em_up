#pragma once

#include <cassert>

#include "memory_arena.h"

template <typename T> struct LinkedListEntry {
  T* data = nullptr;
  LinkedListEntry<T>* next = nullptr;
};

template <typename T> struct LinkedList {
  LinkedList() : _size(0), _first(nullptr), _last(nullptr) {
  }

  T& operator[](size_t index) {
    assert(index < _size);
    LinkedListEntry<T>* result = _first;
    size_t i = 0;
    while (i++ < index) {
      result = result->next;
    }
    return *result->data;
  }

  const T& operator[](size_t index) const {
    assert(index < _size);
    LinkedListEntry<T>* result = _first;
    size_t i = 0;
    while (i++ < index) {
      result = result->next;
    }
    return *result->data;
  }

  [[nodiscard]] auto inline size() const -> size_t {
    return _size;
  }

  auto inline add_link(T* data, MemoryArena& arena) {
    _size++;
    auto* entry = allocate<LinkedListEntry<T>>(arena);
    entry->data = data;
    if (_last != nullptr) {
      _last->next = entry;
    }
    if (_first == nullptr) {
      _first = entry;
    }
    _last = entry;
  }

  auto inline insert(T& data, MemoryArena& arena) -> T* {
    auto* stored_data = allocate<T>(arena);
    memcpy_s(stored_data, sizeof(T), &data, sizeof(T));
    add_link(stored_data, arena);
    return stored_data;
  }

  auto inline insert(T&& data, MemoryArena& arena) -> T* {
    auto* stored_data = allocate<T>(arena);
    memcpy_s(stored_data, sizeof(T), &data, sizeof(T));
    add_link(stored_data, arena);
    return stored_data;
  }

  auto inline alloc(MemoryArena& arena) -> T* {
    _size++;
    auto* entry = allocate<LinkedListEntry<T>>(arena);
    // This might be really dumb, perhaps we don't need the linked list?
    auto* data = allocate<T>(arena);
    entry->data = data;
    if (_last != nullptr) {
      _last->next = entry;
    }
    if (_first == nullptr) {
      _first = entry;
    }
    _last = entry;
    return entry->data;
  }

  class LinkedListIterator {
    private:
    LinkedListEntry<T>* ptr;

    public:
    explicit LinkedListIterator(LinkedListEntry<T>* ptr) : ptr(ptr) {
    }

    LinkedListIterator& operator++() {
      ptr = ptr->next;
      return *this;
    }

    bool operator!=(const LinkedListIterator& other) const {
      return ptr != other.ptr;
    }

    T& operator*() const {
      return *(ptr->data);
    }
  };

  [[nodiscard]] LinkedListIterator begin() const {
    return LinkedListIterator(_first);
  }

  [[nodiscard]] LinkedListIterator end() const {
    return LinkedListIterator(_last);
  }

  private:
  size_t _size;
  LinkedListEntry<T>* _first;
  LinkedListEntry<T>* _last;
};
