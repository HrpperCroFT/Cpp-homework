#pragma once
#include <iostream>
#include <type_traits>
#include <memory>

template <typename T>
class WeakPtr;

struct BaseControlBlock {
  size_t count_shared = 0;
  size_t count_weak = 0;

  virtual void* get_ptr() = 0;

  virtual void delete_object() = 0;

  virtual ~BaseControlBlock() = default;
};

template <typename T>
class SharedPtr {
 private:
  template <typename U, typename Deleter, typename Alloc>
  struct ControlBlock : public BaseControlBlock {
    U* ptr;
    Deleter deleter;
    Alloc alloc;

    using AllocTraitsObject = typename std::allocator_traits<Alloc>;
    using AllocNormal = typename AllocTraitsObject::template rebind_alloc<
        ControlBlock<U, Deleter, Alloc>>;
    using AllocTraits = typename std::allocator_traits<AllocNormal>;

    void* get_ptr() {
      return static_cast<void*>(ptr);
    }

    ControlBlock(U* ptr)
        : ptr(ptr),
          deleter(),
          alloc() {
    }
    ControlBlock(U* ptr, const Deleter& deleter)
        : ptr(ptr),
          deleter(deleter),
          alloc() {
    }
    ControlBlock(U* ptr, const Deleter& deleter, const Alloc& alloc)
        : ptr(ptr),
          deleter(deleter),
          alloc(alloc) {
    }
    ~ControlBlock();

    void delete_object();
  };

  template <typename Alloc>
  struct SpecificControlBlock : public BaseControlBlock {
    Alloc alloc;
    char object[sizeof(T)];

    using AllocTraitsObject = typename std::allocator_traits<Alloc>;
    using AllocNormal = typename AllocTraitsObject::template rebind_alloc<
        SpecificControlBlock<Alloc>>;
    using AllocTraits = typename std::allocator_traits<AllocNormal>;

    void* get_ptr() {
      return static_cast<void*>(object);
    }

    template <typename... Args>
    SpecificControlBlock(Args&&... args)
        : alloc() {
      new (reinterpret_cast<T*>(object)) T(std::forward<Args>(args)...);
    }
    template <typename... Args>
    SpecificControlBlock(const Alloc& alloc, Args&&... args)
        : alloc(alloc) {
      new (reinterpret_cast<T*>(object)) T(std::forward<Args>(args)...);
    }
    ~SpecificControlBlock();

    void delete_object();
  };

  BaseControlBlock* block_ = nullptr;

  SharedPtr(BaseControlBlock*);

  void delete_block();

 public:
  SharedPtr() = default;
  template <typename U>
  SharedPtr(U*);
  template <typename U>
  SharedPtr(const SharedPtr<U>&);
  SharedPtr(const SharedPtr& other);
  template <typename U>
  SharedPtr(SharedPtr<U>&&);
  SharedPtr(SharedPtr&&);

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>&);
  SharedPtr& operator=(const SharedPtr&);
  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&&);
  SharedPtr& operator=(SharedPtr&&);

  size_t use_count() const {
    return block_->count_shared;
  };

  template <typename U>
  void reset(U*);

  void reset() {
    delete_block();
  }

  template <typename U, typename Deleter = std::default_delete<U>>
  SharedPtr(U*, const Deleter&);

  template <typename U, typename Deleter = std::default_delete<U>,
            typename Alloc = std::allocator<U>>
  SharedPtr(U*, const Deleter&, const Alloc&);

  ~SharedPtr() {
    delete_block();
  }

  T* get() const;

  T* operator->() const {
    return get();
  }

  T& operator*() const {
    return *get();
  }

  void swap(SharedPtr& other) {
    std::swap(block_, other.block_);
  }

  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&...);

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc&, Args&&...);
};

template <typename T>
T* SharedPtr<T>::get() const {
  if (block_ == nullptr) {
    return nullptr;
  }
  return static_cast<T*>(block_->get_ptr());
}

template <typename T>
SharedPtr<T>::SharedPtr(BaseControlBlock* block)
    : block_(block) {
  if (block != nullptr) {
    ++block_->count_shared;
  }
}

template <typename T>
template <typename U, typename Deleter, typename Alloc>
SharedPtr<T>::ControlBlock<U, Deleter, Alloc>::~ControlBlock() {
  AllocNormal(alloc).deallocate(this, 1);
}

template <typename T>
template <typename U, typename Deleter, typename Alloc>
void SharedPtr<T>::ControlBlock<U, Deleter, Alloc>::delete_object() {
  deleter(ptr);
}

template <typename T>
template <typename Alloc>
SharedPtr<T>::SpecificControlBlock<Alloc>::~SpecificControlBlock() {
  AllocNormal(alloc).deallocate(this, 1);
}

template <typename T>
template <typename Alloc>
void SharedPtr<T>::SpecificControlBlock<Alloc>::delete_object() {
  AllocTraitsObject::destroy(alloc, reinterpret_cast<T*>(object));
}

template <typename T>
void SharedPtr<T>::delete_block() {
  if (block_ == nullptr) {
    return;
  }
  --block_->count_shared;
  if (block_->count_shared == 0) {
    block_->delete_object();
    if (block_->count_weak == 0) {
      block_->~BaseControlBlock();
    }
  }
  block_ = nullptr;
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(U* ptr) {
  block_ = new ControlBlock<U, std::default_delete<U>, std::allocator<U>>(ptr);
  ++block_->count_shared;
}

template <typename T>
template <typename U, typename Deleter>
SharedPtr<T>::SharedPtr(U* ptr, const Deleter& deleter) {
  block_ = new ControlBlock<U, Deleter, std::allocator<U>>(ptr, deleter);
  ++block_->count_shared;
}

template <typename T>
template <typename U, typename Deleter, typename Alloc>
SharedPtr<T>::SharedPtr(U* ptr, const Deleter& deleter, const Alloc& alloc) {
  auto rebinded = typename std::allocator_traits<Alloc>::template rebind_alloc<
      typename SharedPtr<T>::template ControlBlock<U, Deleter, Alloc>>(alloc);
  block_ = rebinded.allocate(1);
  new (static_cast<ControlBlock<U, Deleter, Alloc>*>(block_))
      ControlBlock<U, Deleter, Alloc>(ptr, deleter, alloc);
  ++block_->count_shared;
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(const SharedPtr<U>& other) {
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_shared;
  }
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& other) {
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_shared;
  }
}

template <typename T>
template <typename U>
SharedPtr<T>::SharedPtr(SharedPtr<U>&& other) {
  block_ = other.block_;
  other.block_ = nullptr;
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& other) {
  block_ = other.block_;
  other.block_ = nullptr;
}

template <typename T>
template <typename U>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<U>& other) {
  delete_block();
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_shared;
  }
  return *this;
}

template <typename T>
template <typename U>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<U>&& other) {
  delete_block();
  block_ = other.block_;
  other.block_ = nullptr;
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& other) {
  delete_block();
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_shared;
  }
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& other) {
  delete_block();
  block_ = other.block_;
  other.block_ = nullptr;
  return *this;
}

template <typename T>
template <typename U>
void SharedPtr<T>::reset(U* ptr) {
  delete_block();
  block_ = new ControlBlock<U, std::default_delete<U>, std::allocator<U>>(ptr);
  ++block_->count_shared;
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  SharedPtr<T> result;
  result.block_ = new
      typename SharedPtr<T>::template SpecificControlBlock<std::allocator<T>>(
          std::forward<Args>(args)...);
  ++result.block_->count_shared;
  return result;
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  SharedPtr<T> result;
  auto rebinded = typename std::allocator_traits<Alloc>::template rebind_alloc<
      typename SharedPtr<T>::template SpecificControlBlock<Alloc>>(alloc);
  result.block_ = rebinded.allocate(1);
  rebinded.construct(
      static_cast<typename SharedPtr<T>::template SpecificControlBlock<Alloc>*>(
          result.block_),
      std::forward<Args>(args)...);
  if (result.block_ != nullptr) {
    ++result.block_->count_shared;
  }
  return result;
}

template <typename T>
class WeakPtr {
 private:
  BaseControlBlock* block_ = nullptr;

  void delete_block();

 public:
  WeakPtr() = default;

  template <typename U>
  WeakPtr(const SharedPtr<U>&);
  template <typename U>
  WeakPtr(const WeakPtr<U>&);
  WeakPtr(const WeakPtr&);
  template <typename U>
  WeakPtr(WeakPtr<U>&&);
  WeakPtr(WeakPtr&&);

  ~WeakPtr() {
    delete_block();
  }

  template <typename U>
  WeakPtr<T>& operator=(const WeakPtr<U>&);
  WeakPtr<T>& operator=(const WeakPtr&);

  template <typename U>
  WeakPtr<T>& operator=(WeakPtr<U>&&);
  WeakPtr<T>& operator=(WeakPtr&&);

  bool expired() const {
    return block_ == nullptr || block_->count_shared == 0;
  }

  SharedPtr<T> lock() const {
    return SharedPtr<T>(block_);
  }

  size_t use_count() const {
    return block_->count_shared;
  }

  template <typename U>
  friend class WeakPtr;
};

template <typename T>
void WeakPtr<T>::delete_block() {
  if (block_ == nullptr) {
    return;
  }
  --block_->count_weak;
  if (block_->count_shared == 0 && block_->count_weak == 0) {
    block_->~BaseControlBlock();
  }
  block_ = nullptr;
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const SharedPtr<U>& ptr) {
  block_ = ptr.block_;
  if (block_ != nullptr) {
    ++block_->count_weak;
  }
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(const WeakPtr<U>& ptr) {
  block_ = ptr.block_;
  if (block_ != nullptr) {
    ++block_->count_weak;
  }
}

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr& ptr) {
  block_ = ptr.block_;
  if (block_ != nullptr) {
    ++block_->count_weak;
  }
}

template <typename T>
template <typename U>
WeakPtr<T>::WeakPtr(WeakPtr<U>&& ptr) {
  delete_block();
  block_ = ptr.block_;
  ptr.block_ = nullptr;
}

template <typename T>
WeakPtr<T>::WeakPtr(WeakPtr&& ptr) {
  delete_block();
  block_ = ptr.block_;
  ptr.block_ = nullptr;
}

template <typename T>
template <typename U>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<U>& other) {
  delete_block();
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_weak;
  }
  return *this;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr& other) {
  delete_block();
  block_ = other.block_;
  if (block_ != nullptr) {
    ++block_->count_weak;
  }
  return *this;
}

template <typename T>
template <typename U>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<U>&& other) {
  delete_block();
  block_ = other.block_;
  other.block_ = nullptr;
  return *this;
}

template <typename T>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr&& other) {
  delete_block();
  block_ = other.block_;
  other.block_ = nullptr;
  return *this;
}