//===--- Memory.h - ILLVM memory utils ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ILLVM memory utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_MEMORY_H
#define ILLVM_MEMORY_H

#include <cassert>
#include <memory>

namespace illvm {

// Nonnull, const propagation, single assignment.
template <typename T>
class BorrowedPtr {
private:
  T* ptr = nullptr;
  bool released = false;

public:
  BorrowedPtr() = default;

  explicit BorrowedPtr(T *p) {
    assert(p != nullptr);
    ptr = p;
  }

  ~BorrowedPtr() {
    assert((!released && ptr != nullptr) || (released && ptr == nullptr));
  }

  void release() {
    assert(ptr != nullptr);
    ptr = nullptr;
    released = true;
  }

  bool isReleased() const { return released; }

  BorrowedPtr(const BorrowedPtr &other) = delete;
  BorrowedPtr& operator=(const BorrowedPtr &other) = delete;

  BorrowedPtr(BorrowedPtr &&other) noexcept {
    assert(other.ptr != nullptr);
    ptr = other.ptr;
    other.release();
  }

  BorrowedPtr &operator=(BorrowedPtr &&other) noexcept {
    assert(ptr == nullptr && !released && other.ptr != nullptr);
    ptr = other.ptr;
    other.release();
    return *this;
  }

  T *get() { return ptr; }
  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }

  const T* get() const { return ptr; }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr; }

  BorrowedPtr<T> copy() {
    return BorrowedPtr<T>(ptr);
  }

  BorrowedPtr<const T> constCopy() const {
    return BorrowedPtr<const T>(ptr);
  }

  template<typename U>
  BorrowedPtr<U> copyTo() {
    return BorrowedPtr<U>(static_cast<U*>(ptr));
  }

  template<typename U>
  BorrowedPtr<const U> constCopyTo() const {
    return BorrowedPtr<const U>(static_cast<const U*>(ptr));
  }
};

template<typename T, typename... Args>
BorrowedPtr<T> make_borrowed(Args&&... args) {
  return BorrowedPtr<T>(new T(std::forward<Args>(args)...));
}

// Nonnull, single assignment.
template <typename T>
class OwnerPtr {
private:
  T* ptr = nullptr;
  bool released = false;

public:
  OwnerPtr() = default;

  explicit OwnerPtr(T *p) {
    assert(p != nullptr);
    ptr = p;
  }

  ~OwnerPtr() {
    if (!released) {
      assert(ptr != nullptr);
      delete ptr;
    } else {
      assert(ptr == nullptr);
    }
  }

  void release() {
    assert(ptr != nullptr);
    ptr = nullptr;
    released = true;
  }

  bool isReleased() const { return released; }

  OwnerPtr(const OwnerPtr&) = delete;
  OwnerPtr& operator=(const OwnerPtr&) = delete;

  OwnerPtr(OwnerPtr &&other) noexcept {
    assert(other.ptr != nullptr);
    ptr = other.ptr;
    other.release();
  }

  OwnerPtr& operator=(OwnerPtr &&other) noexcept {
    assert(ptr == nullptr && !released && other.ptr != nullptr);
    ptr = other.ptr;
    other.release();
    return *this;
  }

  T *get() { return ptr; }
  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }

  const T* get() const { return ptr; }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr; }

  template<typename U>
  OwnerPtr<U> moveTo() {
    auto res = OwnerPtr<U>(static_cast<U*>(ptr));
    release();
    return res;
  }

  template<typename U>
  OwnerPtr<const U> constMoveTo() const {
    auto res = OwnerPtr<const U>(static_cast<const U*>(ptr));
    release();
    return res;
  }

  BorrowedPtr<T> borrow() {
    return BorrowedPtr<T>(ptr);
  }

  BorrowedPtr<const T> constBorrow() const {
    return BorrowedPtr<const T>(ptr);
  }
};

template<typename T, typename... Args>
OwnerPtr<T> make_owner(Args&&... args) {
  return OwnerPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
using OPtr = OwnerPtr<T>;

template <typename T>
using BPtr = BorrowedPtr<T>;

class MemoryUtils {
public:
  static void read(void *dest, const uint8_t *&src, const size_t byteNum) {
    memcpy(dest, src, byteNum);
    src += byteNum;
  }

  static void write(uint8_t *&dest, const void *src, const size_t byteNum) {
    memcpy(dest, src, byteNum);
    dest += byteNum;
  }

  static std::size_t alignOffset(const std::uint64_t offset,
                                 const std::uint64_t align) {
    if (offset % align == 0) {
      return offset;
    }
    return (offset + align - 1) & ~(align - 1);
  }
};

}

#endif // ILLVM_MEMORY_H
