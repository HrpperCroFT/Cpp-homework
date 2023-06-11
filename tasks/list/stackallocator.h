#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>

template <size_t N>
class StackStorage {
 private:
  char data_[N];
  size_t shift_ = 0;

 public:
  StackStorage() = default;
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;
  template <typename T>
  T* allocate(size_t n);
};

template <size_t N>
template <typename T>
T* StackStorage<N>::allocate(size_t n) {
  void* result = data_ + shift_;
  size_t left = N - shift_;
  result = std::align(alignof(T), n * sizeof(T), result, left);
  if (result == nullptr) {
    throw std::bad_alloc();
  }
  shift_ = reinterpret_cast<char*>(result) - data_ + n * sizeof(T);
  return reinterpret_cast<T*>(result);
}

template <typename T, size_t N>
class StackAllocator {
 public:
  StackStorage<N>* storage_;
  using value_type = T;

  StackAllocator() = delete;
  StackAllocator(StackStorage<N>& storage)
      : storage_(&storage) {
  }
  ~StackAllocator() {
  }

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other);

  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& other);

  size_t max_size() const {
    return N / sizeof(T);
  }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  T* allocate(size_t n) {
    return storage_->template allocate<T>(n);
  }

  void deallocate(T*, size_t) {
  }

  template <typename U, size_t M>
  bool operator==(const StackAllocator<U, M>& other) const {
    return storage_ == other.storage_;
  }

  template <typename U, size_t M>
  bool operator!=(const StackAllocator<U, M>& other) const {
    return !(*this == other);
  }
};

template <typename T, size_t N>
template <typename U>
StackAllocator<T, N>::StackAllocator(const StackAllocator<U, N>& other) {
  storage_ = other.storage_;
}

template <typename T, size_t N>
template <typename U>
StackAllocator<T, N>& StackAllocator<T, N>::operator=(
    const StackAllocator<U, N>& other) {
  storage_ = other.storage_;
}

template <typename T, typename Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* next;
    BaseNode* previous;
    BaseNode()
        : next(this),
          previous(this) {
    }
  };
  struct Node : public BaseNode {
    T value;
    Node(const T& val)
        : value(val) {
    }
    Node()
        : value() {
    }
  };
  using NodeAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_allocator_traits = typename std::allocator_traits<NodeAllocator>;
  [[no_unique_address]] NodeAllocator node_allocator_;
  BaseNode end_;
  size_t size_ = 0;

  template <typename... Args>
  void make_list(size_t size, Args&&...);

  void repair_pointers();

  template <bool is_constant>
  class base_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::conditional_t<is_constant, const T, T>;
    using pointer = std::conditional_t<is_constant, const T*, T*>;
    using reference = std::conditional_t<is_constant, const T&, T&>;
    using difference_type = int;

   private:
    BaseNode* node_;

   public:
    base_iterator() = delete;
    base_iterator(const BaseNode* node)
        : node_(const_cast<BaseNode*>(node)) {
    }

    base_iterator& operator++();
    base_iterator operator++(int);

    base_iterator& operator--();
    base_iterator operator--(int);

    bool operator==(const base_iterator& other) const {
      return node_ == other.node_;
    }
    bool operator!=(const base_iterator& other) const {
      return node_ != other.node_;
    }

    operator base_iterator<true>() const {
      return base_iterator<true>(node_);
    }

    pointer operator->() const {
      return &(static_cast<Node*>(node_)->value);
    }

    reference operator*() const {
      return static_cast<Node*>(node_)->value;
    }

    friend class List<T, Allocator>;
  };

 public:
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List(const Allocator& alloc)
      : node_allocator_(alloc) {
  }
  List(size_t size, const T& value, const Allocator& alloc);
  List(size_t size, const Allocator& alloc);
  List()
      : node_allocator_() {
  }
  List(size_t size, const T& value);
  List(size_t size);
  List(const List<T, Allocator>& other);
  List& operator=(const List<T, Allocator>& other);
  ~List() {
    clear();
  }

  void clear();

  size_t size() const {
    return size_;
  }

  Allocator get_allocator() const {
    return Allocator(node_allocator_);
  }

  void push_back(const T& value);
  void push_front(const T& value);
  void pop_back();
  void pop_front();

  iterator begin();
  const_iterator begin() const;
  const_iterator cbegin() const;
  iterator end();
  const_iterator end() const;
  const_iterator cend() const;

  reverse_iterator rbegin();
  const_reverse_iterator rbegin() const;
  const_reverse_iterator crbegin() const;
  reverse_iterator rend();
  const_reverse_iterator rend() const;
  const_reverse_iterator crend() const;

  void insert(const_iterator, const T&);
  void erase(const_iterator);

  template <typename... Args>
  void emplace(const_iterator, Args&&...);
};

template <typename T, typename Allocator>
template <bool is_constant>
typename List<T, Allocator>::template base_iterator<is_constant>&
List<T, Allocator>::base_iterator<is_constant>::operator++() {
  node_ = node_->next;
  return *this;
}

template <typename T, typename Allocator>
template <bool is_constant>
typename List<T, Allocator>::template base_iterator<is_constant>
List<T, Allocator>::base_iterator<is_constant>::operator++(int) {
  base_iterator<is_constant> result = *this;
  ++(*this);
  return result;
}

template <typename T, typename Allocator>
template <bool is_constant>
typename List<T, Allocator>::template base_iterator<is_constant>&
List<T, Allocator>::base_iterator<is_constant>::operator--() {
  node_ = node_->previous;
  return *this;
}

template <typename T, typename Allocator>
template <bool is_constant>
typename List<T, Allocator>::template base_iterator<is_constant>
List<T, Allocator>::base_iterator<is_constant>::operator--(int) {
  base_iterator<is_constant> result = *this;
  --(*this);
  return result;
}

template <typename T, typename Allocator>
void List<T, Allocator>::clear() {
  while (size_ > 0) {
    pop_back();
  }
}

template <typename T, typename Allocator>
template <typename... Args>
void List<T, Allocator>::make_list(size_t size, Args&&... args) {
  try {
    for (size_t i = 0; i < size; ++i) {
      emplace(end(), args...);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
void List<T, Allocator>::repair_pointers() {
  end_.next->previous = &end_;
  end_.previous->next = &end_;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List<T, Allocator>& other)
    : node_allocator_(
          node_allocator_traits::select_on_container_copy_construction(
              other.node_allocator_)) {
  try {
    for (auto& x : other) {
      push_back(x);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(
    const List<T, Allocator>& other) {
  List<T, Allocator> tmp(other);
  if (node_allocator_traits::propagate_on_container_copy_assignment::value) {
    tmp.node_allocator_ = other.node_allocator_;
  }
  std::swap(node_allocator_, tmp.node_allocator_);
  std::swap(size_, tmp.size_);
  std::swap(end_.previous, tmp.end_.previous);
  std::swap(end_.next, tmp.end_.next);
  repair_pointers();
  tmp.repair_pointers();
  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value, const Allocator& alloc)
    : node_allocator_(alloc) {
  make_list(size, value);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const T& value)
    : List(size, value, Allocator()) {
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t size)
    : List(size, Allocator()) {
}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_t size, const Allocator& alloc)
    : node_allocator_(alloc) {
  make_list(size);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_front(const T& value) {
  insert(begin(), value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::push_back(const T& value) {
  insert(end(), value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_front() {
  erase(begin());
}

template <typename T, typename Allocator>
void List<T, Allocator>::pop_back() {
  const_iterator rbeg = end();
  --rbeg;
  erase(rbeg);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
  return iterator(end_.next);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::begin() const {
  return const_iterator(end_.next);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
  return const_iterator(end_.next);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() {
  return iterator(&end_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::end() const {
  return const_iterator(&end_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const {
  return const_iterator(&end_);
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rbegin() {
  return reverse_iterator(end());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rbegin()
    const {
  return const_reverse_iterator(cend());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator
List<T, Allocator>::crbegin() const {
  return const_reverse_iterator(cend());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rend() {
  return reverse_iterator(begin());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rend()
    const {
  return const_reverse_iterator(cbegin());
}

template <typename T, typename Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crend()
    const {
  return const_reverse_iterator(cbegin());
}

template <typename T, typename Allocator>
void List<T, Allocator>::insert(const_iterator iter, const T& value) {
  emplace(iter, value);
}

template <typename T, typename Allocator>
void List<T, Allocator>::erase(const_iterator iter) {
  iter.node_->previous->next = iter.node_->next;
  iter.node_->next->previous = iter.node_->previous;
  node_allocator_traits::destroy(node_allocator_,
                                 static_cast<Node*>(iter.node_));
  node_allocator_traits::deallocate(node_allocator_,
                                    static_cast<Node*>(iter.node_), 1);
  --size_;
}

template <typename T, typename Allocator>
template <typename... Args>
void List<T, Allocator>::emplace(const_iterator iter, Args&&... args) {
  Node* new_node = node_allocator_traits::allocate(node_allocator_, 1);
  try {
    node_allocator_traits::construct(node_allocator_, new_node, args...);
  } catch (...) {
    node_allocator_traits::deallocate(node_allocator_, new_node, 1);
    throw;
  }
  new_node->previous = iter.node_->previous;
  new_node->previous->next = new_node;
  new_node->next = iter.node_;
  iter.node_->previous = new_node;
  ++size_;
}