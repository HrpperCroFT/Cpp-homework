#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>
#include <random>
#include <iostream>

template <class T, class Allocator = std::allocator<T>>
class List {
 private:
  struct BaseNode {
    BaseNode* next;
    BaseNode* previous;
    BaseNode()
        : next(this),
          previous(this) {
    }
    BaseNode(const BaseNode& other)
        : next(other.next),
          previous(other.previous) {
    }
    BaseNode(BaseNode&& other);
    BaseNode& operator=(const BaseNode&);
    BaseNode& operator=(BaseNode&&);
  };
  struct Node : public BaseNode {
    T value;
    Node(const T& val)
        : value(val) {
    }
    Node(T&& val)
        : value(std::move(val)) {
    }
    Node()
        : value() {
    }
    template <class... Args>
    Node(Args&&... args)
        : value(std::forward<Args>(args)...) {
    }
  };
  using NodeAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_allocator_traits = typename std::allocator_traits<NodeAllocator>;
  [[no_unique_address]] NodeAllocator node_allocator_;
  BaseNode end_;
  size_t size_ = 0;
  void make_list(size_t size, const T& value);
  void make_list(size_t size);
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
    base_iterator(const base_iterator& other)
        : node_(other.node_) {
    }
    base_iterator(base_iterator&& other)
        : node_(other.node_) {
    }

    base_iterator& operator=(const base_iterator& other) {
      node_ = other.node_;
      return *this;
    }

    base_iterator& operator=(base_iterator&& other) {
      node_ = other.node_;
      return *this;
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
  List(Allocator&& alloc)
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
  List(List<T, Allocator>&& other);
  List& operator=(const List<T, Allocator>& other);
  List& operator=(List<T, Allocator>&& other);
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
  template <class... Args>
  void emplace(const_iterator, Args&&...);
};

template <class T, class Allocator>
List<T, Allocator>::BaseNode::BaseNode(BaseNode&& other)
    : next(other.next),
      previous(other.previous) {
  next->previous = this;
  previous->next = this;
  other.next = &other;
  other.previous = &other;
}

template <class T, class Allocator>
typename List<T, Allocator>::BaseNode& List<T, Allocator>::BaseNode::operator=(
    const BaseNode& other) {
  next = other.next;
  previous = other.previous;
  return *this;
}

template <class T, class Allocator>
typename List<T, Allocator>::BaseNode& List<T, Allocator>::BaseNode::operator=(
    BaseNode&& other) {
  next = other.next;
  previous = other.previous;
  next->previous = this;
  previous->next = this;
  other.next = &other;
  other.previous = &other;
  return *this;
}

template <class T, class Allocator>
template <bool is_constant>
class List<T, Allocator>::template base_iterator<is_constant>&
List<T, Allocator>::base_iterator<is_constant>::operator++() {
  node_ = node_->next;
  return *this;
}

template <class T, class Allocator>
template <bool is_constant>
class List<T, Allocator>::template base_iterator<is_constant>
List<T, Allocator>::base_iterator<is_constant>::operator++(int) {
  base_iterator<is_constant> result = *this;
  ++(*this);
  return result;
}

template <class T, class Allocator>
template <bool is_constant>
class List<T, Allocator>::template base_iterator<is_constant>&
List<T, Allocator>::base_iterator<is_constant>::operator--() {
  node_ = node_->previous;
  return *this;
}

template <class T, class Allocator>
template <bool is_constant>
class List<T, Allocator>::template base_iterator<is_constant>
List<T, Allocator>::base_iterator<is_constant>::operator--(int) {
  base_iterator<is_constant> result = *this;
  --(*this);
  return result;
}

template <class T, class Allocator>
void List<T, Allocator>::clear() {
  while (size_ > 0) {
    pop_back();
  }
  size_ = 0;
}

template <class T, class Allocator>
void List<T, Allocator>::make_list(size_t size, const T& value) {
  for (size_t i = 0; i < size; ++i) {
    push_back(value);
  }
}

template <class T, class Allocator>
void List<T, Allocator>::make_list(size_t size) {
  if (size == 0) {
    return;
  }
  BaseNode* current = &end_;
  for (size_t i = 0; i < size; ++i) {
    current->next = node_allocator_traits::allocate(node_allocator_, 1);
    try {
      node_allocator_traits::construct(node_allocator_,
                                       static_cast<Node*>(current->next));
    } catch (...) {
      current->next = &end_;
      end_.previous = current;
      throw;
    }
    ++size_;
    current->next->previous = current;
    current = current->next;
  }
  end_.previous = current;
  current->next = &end_;
}

template <class T, class Allocator>
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

template <class T, class Allocator>
List<T, Allocator>::List(List<T, Allocator>&& other)
    : node_allocator_(std::move(other.node_allocator_)),
      end_(std::move(other.end_)),
      size_(other.size_) {
  other.size_ = 0;
}

template <class T, class Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(
    const List<T, Allocator>& other) {
  List<T, Allocator> tmp(other);
  if (node_allocator_traits::propagate_on_container_copy_assignment::value) {
    tmp.node_allocator_ = other.node_allocator_;
  }
  std::swap(*this, std::move(tmp));
  return *this;
}

template <class T, class Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List<T, Allocator>&& other) {
  if (node_allocator_ == other.node_allocator_) {
    end_ = std::move(other.end_);
    size_ = other.size_;
    other.size_ = 0;
  } else {
    if (node_allocator_traits::propagate_on_container_move_assignment::value) {
      node_allocator_ = std::move(other.node_allocator_);
      end_ = std::move(other.end_);
      size_ = other.size_;
      other.size_ = 0;
    } else {
      size_t i = 0;
      try {
        for (auto iter = other.begin(); i < other.size_; ++i, ++iter) {
          emplace(end(), std::move(*iter));
        }
      } catch (...) {
        for (size_t j = 0; j < i; ++j) {
          pop_back();
        }
        throw;
      }
      for (size_t j = 0; j < size_ - i; ++j) {
        pop_front();
      }
    }
  }
  return *this;
}

template <class T, class Allocator>
List<T, Allocator>::List(size_t size, const T& value, const Allocator& alloc)
    : node_allocator_(alloc) {
  try {
    make_list(size, value);
  } catch (...) {
    clear();
    throw;
  }
}

template <class T, class Allocator>
List<T, Allocator>::List(size_t size, const T& value)
    : node_allocator_() {
  try {
    make_list(size, value);
  } catch (...) {
    clear();
    throw;
  }
}

template <class T, class Allocator>
List<T, Allocator>::List(size_t size)
    : node_allocator_() {
  try {
    make_list(size);
  } catch (...) {
    clear();
    throw;
  }
}

template <class T, class Allocator>
List<T, Allocator>::List(size_t size, const Allocator& alloc)
    : node_allocator_(alloc) {
  try {
    make_list(size);
  } catch (...) {
    clear();
    throw;
  }
}

template <class T, class Allocator>
void List<T, Allocator>::push_front(const T& value) {
  emplace(begin(), value);
}

template <class T, class Allocator>
void List<T, Allocator>::push_back(const T& value) {
  emplace(end(), value);
}

template <class T, class Allocator>
void List<T, Allocator>::pop_front() {
  erase(begin());
}

template <class T, class Allocator>
void List<T, Allocator>::pop_back() {
  auto iter = end();
  --iter;
  erase(iter);
}

template <class T, class Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::begin() {
  return iterator(end_.next);
}

template <class T, class Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::begin() const {
  return const_iterator(end_.next);
}

template <class T, class Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cbegin() const {
  return const_iterator(end_.next);
}

template <class T, class Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::end() {
  return iterator(&end_);
}

template <class T, class Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::end() const {
  return const_iterator(&end_);
}

template <class T, class Allocator>
typename List<T, Allocator>::const_iterator List<T, Allocator>::cend() const {
  return const_iterator(&end_);
}

template <class T, class Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rbegin() {
  return reverse_iterator(end());
}

template <class T, class Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rbegin()
    const {
  return const_reverse_iterator(cend());
}

template <class T, class Allocator>
typename List<T, Allocator>::const_reverse_iterator
List<T, Allocator>::crbegin() const {
  return const_reverse_iterator(cend());
}

template <class T, class Allocator>
typename List<T, Allocator>::reverse_iterator List<T, Allocator>::rend() {
  return reverse_iterator(begin());
}

template <class T, class Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::rend()
    const {
  return const_reverse_iterator(cbegin());
}

template <class T, class Allocator>
typename List<T, Allocator>::const_reverse_iterator List<T, Allocator>::crend()
    const {
  return const_reverse_iterator(cbegin());
}

template <class T, class Allocator>
void List<T, Allocator>::insert(const_iterator iter, const T& value) {
  emplace(iter, value);
}

template <class T, class Allocator>
void List<T, Allocator>::erase(const_iterator iter) {
  iter.node_->previous->next = iter.node_->next;
  iter.node_->next->previous = iter.node_->previous;
  node_allocator_traits::destroy(node_allocator_,
                                 static_cast<Node*>(iter.node_));
  node_allocator_traits::deallocate(node_allocator_,
                                    static_cast<Node*>(iter.node_), 1);
  --size_;
}

template <class T, class Allocator>
template <class... Args>
void List<T, Allocator>::emplace(const_iterator iter, Args&&... args) {
  Node* new_node = node_allocator_traits::allocate(node_allocator_, 1);
  try {
    node_allocator_traits::construct(node_allocator_, new_node,
                                     std::forward<Args>(args)...);
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

template <class Key, class Value, class Hash = std::hash<Key>,
          class Equal = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  using NodeType = std::pair<const Key, Value>;

 private:
  size_t buckets_count_;
  [[no_unique_address]] Hash hasher_;
  [[no_unique_address]] Equal equaler_;
  struct HashedNode {
    NodeType node;
    size_t hash;
    HashedNode(const Hash& hasher, const NodeType& noded)
        : node(noded),
          hash(hasher(node.first)) {
    }
    HashedNode(const Hash& hasher, NodeType&& noded)
        : node(const_cast<Key&&>(std::move(noded.first)),
               std::move(noded.second)),
          hash(hasher(node.first)) {
    }
    HashedNode(const Hash& hasher, const Key& key, const Value& value)
        : node(key, value),
          hash(hasher(node.first)) {
    }
    HashedNode(const Hash& hasher, Key&& key, Value&& value)
        : node(std::move(key), std::move(value)),
          hash(hasher(node.first)) {
    }
    HashedNode(const Hash& hasher, const Key& key)
        : node(key),
          hash(hasher(node.first)) {
    }
    HashedNode(const Hash& hasher, Key&& key)
        : node(std::move(key)),
          hash(hasher(node.first)) {
    }
    HashedNode(const HashedNode& other)
        : node(other.node),
          hash(other.hash) {
    }
    HashedNode(HashedNode&& other)
        : node(const_cast<Key&&>(std::move(other.node.first)),
               std::move(other.node.second)),
          hash(other.hash) {
    }
  };
  using node_allocator_traits = typename std::allocator_traits<Alloc>;
  using HashedNodeAllocator =
      typename node_allocator_traits::template rebind_alloc<HashedNode>;
  using hashed_node_allocator_traits =
      typename std::allocator_traits<HashedNodeAllocator>;
  using StorageList = List<HashedNode, HashedNodeAllocator>;
  using vector_allocator =
      typename hashed_node_allocator_traits::template rebind_alloc<
          typename StorageList::iterator>;

  [[no_unique_address]] HashedNodeAllocator hnode_allocator_;
  StorageList storage_;
  std::vector<typename StorageList::iterator, vector_allocator>
      bucket_iterators_;
  float max_load_factor_ = 0.5;

  template <bool is_constant = false>
  struct base_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type =
        std::conditional_t<is_constant, const NodeType, NodeType>;
    using pointer = std::conditional_t<is_constant, const NodeType*, NodeType*>;
    using reference =
        std::conditional_t<is_constant, const NodeType&, NodeType&>;
    using difference_type = int;

   private:
    using inner_iterator =
        std::conditional_t<is_constant, typename StorageList::const_iterator,
                           typename StorageList::iterator>;

    inner_iterator iter_;

    size_t hash() const {
      return iter_->hash;
    }

   public:
    base_iterator() = delete;
    base_iterator(const inner_iterator& iter)
        : iter_(const_cast<inner_iterator&>(iter)) {
    }
    base_iterator(const base_iterator& other)
        : iter_(other.iter_) {
    }
    base_iterator(base_iterator&& other)
        : iter_(other.iter_) {
    }

    base_iterator& operator=(const base_iterator& other) {
      iter_ = other.iter_;
      return *this;
    }

    base_iterator& operator=(base_iterator&& other) {
      iter_ = other.iter_;
      return *this;
    }

    base_iterator& operator++();
    base_iterator operator++(int);

    bool operator==(const base_iterator& other) const {
      return iter_ == other.iter_;
    }
    bool operator!=(const base_iterator& other) const {
      return iter_ != other.iter_;
    }

    operator base_iterator<true>() const {
      return base_iterator<true>(iter_);
    }

    pointer operator->() const {
      return &(iter_->node);
    }

    reference operator*() const {
      return iter_->node;
    }

    friend class UnorderedMap<Key, Value, Hash, Equal, Alloc>;
  };

  void rehash(size_t);

  size_t get_hash(size_t hash) const {
    return hash % buckets_count_;
  }

 public:
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;

  UnorderedMap();
  UnorderedMap(const Alloc& allocator);
  UnorderedMap(Alloc&& allocator);
  UnorderedMap(const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other);
  UnorderedMap(UnorderedMap<Key, Value, Hash, Equal, Alloc>&&) = default;
  ~UnorderedMap() = default;

  UnorderedMap& operator=(const UnorderedMap&);
  UnorderedMap& operator=(UnorderedMap&&);

  Value& operator[](const Key&);
  Value& operator[](Key&&);
  Value& at(const Key&);
  const Value& at(const Key&) const;

  size_t size() const {
    return storage_.size();
  }

  iterator begin();
  const_iterator begin() const;
  const_iterator cbegin() const;
  iterator end();
  const_iterator end() const;
  const_iterator cend() const;

  std::pair<iterator, bool> insert(const NodeType&);
  std::pair<iterator, bool> insert(NodeType&&);
  template <class InputIterator>
  void insert(InputIterator, InputIterator);

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&...);

  void erase(iterator);
  void erase(iterator, iterator);

  iterator find(const Key&);
  const_iterator find(const Key&) const;

  void reserve(size_t new_size) {
    rehash(new_size);
  }
  float load_factor() const {
    return float(size()) / float(buckets_count_);
  };
  float max_load_factor() const {
    return max_load_factor_;
  }
  void max_load_factor(float);

  void swap(UnorderedMap&);

  void clear();
};

template <class Key, class Value, class Hash, class Equal, class Alloc>
template <bool is_constant>
typename UnorderedMap<Key, Value, Hash, Equal,
                      Alloc>::template base_iterator<is_constant>&
    UnorderedMap<Key, Value, Hash, Equal,
                 Alloc>::template base_iterator<is_constant>::operator++() {
  ++iter_;
  return *this;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
template <bool is_constant>
typename UnorderedMap<Key, Value, Hash, Equal,
                      Alloc>::template base_iterator<is_constant>
    UnorderedMap<Key, Value, Hash, Equal,
                 Alloc>::template base_iterator<is_constant>::operator++(int) {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::clear() {
  bucket_iterators_.clear();
  storage_.clear();
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap()
    : buckets_count_(2),
      hnode_allocator_(),
      storage_(hnode_allocator_),
      bucket_iterators_(hnode_allocator_) {
  bucket_iterators_.assign(buckets_count_, storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(const Alloc& alloc)
    : buckets_count_(2),
      hnode_allocator_(alloc),
      storage_(hnode_allocator_),
      bucket_iterators_(hnode_allocator_) {
  bucket_iterators_.assign(buckets_count_, storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(Alloc&& alloc)
    : buckets_count_(2),
      hnode_allocator_(std::move(alloc)),
      storage_(hnode_allocator_),
      bucket_iterators_(hnode_allocator_) {
  bucket_iterators_.assign(buckets_count_, storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::UnorderedMap(
    const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other)
    : UnorderedMap(other.hnode_allocator_) {
  try {
    for (auto& el : other) {
      insert(el);
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
    const UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) {
  UnorderedMap tmp(other);
  if (hashed_node_allocator_traits::propagate_on_container_copy_assignment::
          value) {
    tmp.hnode_allocator_ = other.hnode_allocator_;
  }
  swap(*this, tmp);
  return *this;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
UnorderedMap<Key, Value, Hash, Equal, Alloc>&
UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator=(
    UnorderedMap<Key, Value, Hash, Equal, Alloc>&& other) {
  if (hnode_allocator_ != other.hnode_allocator_ &&
      hashed_node_allocator_traits::propagate_on_container_move_assignment::
          value) {
    hnode_allocator_ = std::move(other.hnode_allocator_);
  }
  bucket_iterators_ = std::move(other.bucket_iterators_);
  for (auto& el : bucket_iterators_) {
    if (el == other.storage_.end()) {
      el = storage_.end();
    }
  }
  storage_ = std::move(other.storage_);
  buckets_count_ = other.buckets_count_;
  max_load_factor_ = other.max_load_factor_;
  other.max_load_factor_ = 0;
  other.buckets_count_ = 0;
  return *this;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](
    const Key& key) {
  return (*(emplace(std::move(key), Value()).first)).second;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::operator[](Key&& key) {
  return (*(emplace(std::move(key), Value()).first)).second;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(const Key& key) {
  auto iter = find(key);
  if (iter == end()) {
    throw std::out_of_range("Blin");
  }
  return (*iter).second;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
const Value& UnorderedMap<Key, Value, Hash, Equal, Alloc>::at(
    const Key& key) const {
  auto iter = find(key);
  if (iter == end()) {
    throw std::out_of_range("BBBBBBBBBBlin");
  }
  return (*iter).second;
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
template <class... Args>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::emplace(Args&&... args) {
  char place[sizeof(HashedNode)];
  HashedNode* node = reinterpret_cast<HashedNode*>(place);
  hashed_node_allocator_traits::construct(hnode_allocator_, node, hasher_,
                                          std::forward<Args>(args)...);
  size_t current_hash = node->hash;
  auto iter = bucket_iterators_[get_hash(current_hash)];
  while (iter != storage_.end() &&
         get_hash(iter->hash) == get_hash(current_hash)) {
    if (equaler_(iter->node.first, node->node.first)) {
      return {iterator(iter), false};
    }
    ++iter;
  }
  storage_.emplace(bucket_iterators_[get_hash(current_hash)],
                   std::forward<HashedNode>(*node));
  --bucket_iterators_[get_hash(current_hash)];
  if (load_factor() > max_load_factor_) {
    rehash(size_t(2 * (storage_.size() / max_load_factor_ + 2)));
  }
  return {iterator(bucket_iterators_[get_hash(current_hash)]), true};
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(const NodeType& node) {
  return emplace(node);
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
std::pair<typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator, bool>
UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(NodeType&& node) {
  return emplace(std::move(node));
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
template <typename InputIterator>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::insert(
    InputIterator beginning, InputIterator ending) {
  for (; beginning != ending; ++beginning) {
    emplace(std::forward<decltype(*beginning)>(*beginning));
  }
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(iterator iter) {
  size_t myhash = get_hash(iter.hash());
  if (iter == bucket_iterators_[myhash]) {
    ++bucket_iterators_[myhash];
    if (bucket_iterators_[myhash] != storage_.end() &&
        get_hash(bucket_iterators_[myhash]->hash) != myhash) {
      bucket_iterators_[myhash] = storage_.end();
    }
  }
  storage_.erase(iter.iter_);
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::erase(iterator beginning,
                                                         iterator ending) {
  while (beginning != ending) {
    auto copy_beginning = beginning;
    ++copy_beginning;
    erase(beginning);
    beginning = copy_beginning;
  }
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() {
  return iterator(storage_.begin());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::begin() const {
  return const_iterator(storage_.begin());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cbegin() const {
  return const_iterator(storage_.begin());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() {
  return iterator(storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::end() const {
  return const_iterator(storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::cend() const {
  return const_iterator(storage_.end());
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::rehash(size_t new_size) {
  if (new_size < size()) {
    return;
  }
  new_size = std::max(new_size, size_t(2));
  std::vector<typename StorageList::iterator, vector_allocator> new_vector(
      new_size, storage_.end());
  for (int i = storage_.size(); i > 0; --i) {
    auto iter = storage_.begin();
    size_t new_hash = iter->hash % new_size;
    storage_.emplace(new_vector[new_hash], std::move(*iter));
    --new_vector[new_hash];
    storage_.erase(iter);
  }
  buckets_count_ = new_size;
  bucket_iterators_ = std::move(new_vector);
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
void UnorderedMap<Key, Value, Hash, Equal, Alloc>::swap(
    UnorderedMap<Key, Value, Hash, Equal, Alloc>& other) {
  std::swap(hnode_allocator_, other.hnode_allocator_);
  std::swap(storage_, other.storage_);
  std::swap(buckets_count_, other.buckets_count_);
  std::swap(bucket_iterators_, other.bucket_iterators_);
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) {
  size_t myhash = hasher_(key);
  auto iter = bucket_iterators_[get_hash(myhash)];
  while (iter != storage_.end() && get_hash(iter->hash) == get_hash(myhash)) {
    if (equaler_(iter->node.first, key)) {
      return iterator(iter);
    }
    ++iter;
  }
  return end();
}

template <class Key, class Value, class Hash, class Equal, class Alloc>
typename UnorderedMap<Key, Value, Hash, Equal, Alloc>::const_iterator
UnorderedMap<Key, Value, Hash, Equal, Alloc>::find(const Key& key) const {
  size_t myhash = hasher_(key);
  auto iter = bucket_iterators_[get_hash(myhash)];
  while (iter != storage_.end() && get_hash(iter->hash) == get_hash(myhash)) {
    if (equaler_(iter->node.first, key)) {
      return const_iterator(iter);
    }
    ++iter;
  }
  return cend();
}