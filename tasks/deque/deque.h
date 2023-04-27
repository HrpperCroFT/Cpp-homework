#pragma once
#include <vector>
#include <iterator>
#include <type_traits>

template <typename T>
class Deque {
 private:
  struct Bucket {
    static const size_t size = 32;
    T* elements_;
    Bucket()
        : elements_(reinterpret_cast<T*>(new char[size * sizeof(T)])) {
    }
    T& operator[](size_t position) {
      return elements_[position];
    }
  };

  size_t bucket_index(size_t position) const;
  size_t in_bucket_index(size_t position) const;

  size_t size_, begin_bucket_, begin_index_, bucket_quantity_;
  Bucket* buckets_;

  template<typename... Args>
  void make_deque(size_t sz, const Args&... value);

  template <bool is_constant = false>
  class base_iterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::conditional_t<is_constant, const T, T>;
    using pointer = std::conditional_t<is_constant, const T*, T*>;
    using reference = std::conditional_t<is_constant, const T&, T&>;
    using difference_type = ptrdiff_t;
    using bucket_type = std::conditional_t<is_constant, const Bucket, Bucket>;

   private:
    bucket_type* bucket_;
    size_t position_;
    pointer pointer_;

   public:
    base_iterator() = delete;
    base_iterator(bucket_type* bucket, size_t position)
        : bucket_(bucket),
          position_(position),
          pointer_(bucket->elements_ + position) {
    }

    base_iterator& operator++();
    base_iterator operator++(int);

    base_iterator& operator--();
    base_iterator operator--(int);

    bool operator==(const base_iterator&) const;
    bool operator!=(const base_iterator&) const;
    bool operator<(const base_iterator&) const;
    bool operator>(const base_iterator&) const;
    bool operator<=(const base_iterator&) const;
    bool operator>=(const base_iterator&) const;

    operator base_iterator<true>() const;

    base_iterator& operator+=(int);
    base_iterator& operator-=(int);

    base_iterator operator+(int) const;
    base_iterator operator-(int) const;

    difference_type operator-(const base_iterator&) const;

    pointer operator->() const {
      return pointer_;
    }

    reference operator*() const {
      return *pointer_;
    }

    friend class Deque<T>;
  };
  void delete_all(size_t last);
  std::pair<size_t, size_t> get_rbegin() const;
  std::pair<size_t, size_t> get_end() const;
  template <bool is_constant>
  std::pair<size_t, size_t> get_position(
      const base_iterator<is_constant>&) const;

 public:
  using iterator = base_iterator<false>;
  using const_iterator = base_iterator<true>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  Deque()
      : size_(0),
        begin_bucket_(1),
        begin_index_(0),
        bucket_quantity_(2),
        buckets_(new Bucket[bucket_quantity_]) {
  }
  Deque(size_t);
  Deque(size_t, const T&);
  Deque(const Deque&);
  ~Deque();

  Deque& operator=(const Deque&);

  void push_back(const T&);
  void push_front(const T&);
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

  void insert(iterator, const T&);
  void erase(iterator);

  T& operator[](size_t);
  const T& operator[](size_t) const;

  T& at(size_t);
  const T& at(size_t) const;

  size_t size() const {
    return size_;
  }
};

template <typename T>
size_t Deque<T>::bucket_index(size_t position) const {
  return position / Bucket::size;
}

template <typename T>
size_t Deque<T>::in_bucket_index(size_t position) const {
  return position % Bucket::size;
}

template <typename T>
void Deque<T>::delete_all(size_t last) {
  size_t i = begin_bucket_, j = begin_index_;
  for (size_t current = 0; current < last; ++current) {
    (buckets_[i].elements_ + j)->~T();
    ++j;
    if (j == Bucket::size) {
      j = 0, ++i;
    }
  }
  delete[] buckets_;
  size_ = 0;
  bucket_quantity_ = 2;
  begin_index_ = 0;
  begin_bucket_ = 1;
}

template <typename T>
std::pair<size_t, size_t> Deque<T>::get_rbegin() const {
  if (size_ == 0) {
    return {begin_bucket_ - 1, Bucket::size - 1};
  }
  size_t index1 = begin_index_ + size_ - 1;
  size_t index2 = begin_bucket_ + index1 / Bucket::size;
  index1 %= Bucket::size;
  return {index2, index1};
}

template <typename T>
std::pair<size_t, size_t> Deque<T>::get_end() const {
  size_t index1 = begin_index_ + size_;
  size_t index2 = begin_bucket_ + index1 / Bucket::size;
  index1 %= Bucket::size;
  return {index2, index1};
}

template <typename T>
template <bool is_constant>
std::pair<size_t, size_t> Deque<T>::get_position(
    const base_iterator<is_constant>& it) const {
  return {it.bucket_ - buckets_, it.position_};
}

template<typename T>
template<typename... Args>
void Deque<T>::make_deque(size_t sz, const Args&... value) {
buckets_ = new Bucket[bucket_quantity_];
  size_t i = begin_bucket_, j = begin_index_, current = 0;
  try {
    for (; current < size_; ++current) {
      new (buckets_[i].elements_ + j) T(value...);
      ++j;
      if (j == Bucket::size) {
        j = 0, ++i;
      }
    }
  } catch (...) {
    delete_all(current);
    throw;
  }
}

template <typename T>
Deque<T>::Deque(size_t sz)
    : size_(sz),
      begin_bucket_(1),
      begin_index_(0),
      bucket_quantity_((sz + Bucket::size - 1) / Bucket::size + 2) {
  make_deque<true>(sz);
}

template <typename T>
Deque<T>::Deque(size_t sz, const T& value)
    : size_(sz),
      begin_bucket_(1),
      begin_index_(0),
      bucket_quantity_((sz + Bucket::size - 1) / Bucket::size + 2) {
  make_deque<false>(sz, value);
}

template <typename T>
Deque<T>::Deque(const Deque& other)
    : size_(other.size_),
      begin_bucket_(other.begin_bucket_),
      begin_index_(other.begin_index_),
      bucket_quantity_(other.bucket_quantity_) {
  buckets_ = new Bucket[bucket_quantity_];
  size_t i = begin_bucket_, j = begin_index_, current = 0;
  try {
    for (; current < size_; ++current) {
      buckets_[i][j] = other.buckets_[i][j];
      ++j;
      if (j == Bucket::size) {
        j = 0, ++i;
      }
    }
  } catch (...) {
    delete_all(current);
    throw;
  }
}

template <typename T>
Deque<T>::~Deque() {
  delete_all(size_);
}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque& other) {
  try {
    Deque new_deque(other);
    delete_all(size_);
    size_ = new_deque.size_;
    begin_index_ = new_deque.begin_index_;
    begin_bucket_ = new_deque.begin_bucket_;
    bucket_quantity_ = new_deque.bucket_quantity_;
    buckets_ = new_deque.buckets_;
    new_deque.buckets_ = nullptr;
    new_deque.bucket_quantity_ = 0;
  } catch (...) {
    throw;
  }
  return *this;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>&
Deque<T>::base_iterator<is_constant>::operator++() {
  ++position_;
  if (position_ == Bucket::size) {
    ++bucket_;
    position_ = 0;
    pointer_ = bucket_->elements_;
  } else {
    ++pointer_;
  }
  return *this;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>
Deque<T>::base_iterator<is_constant>::operator++(int) {
  base_iterator<is_constant> answer(*this);
  ++(*this);
  return answer;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>&
Deque<T>::base_iterator<is_constant>::operator--() {
  if (position_ == 0) {
    --bucket_;
    position_ = Bucket::size - 1;
    pointer_ = bucket_->elements_ + Bucket::size - 1;
  } else {
    --position_;
    --pointer_;
  }
  return *this;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>
Deque<T>::base_iterator<is_constant>::operator--(int) {
  base_iterator<is_constant> answer(*this);
  --(*this);
  return answer;
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator==(
    const base_iterator& other) const {
  return bucket_ == other.bucket_ && position_ == other.position_;
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator!=(
    const base_iterator& other) const {
  return pointer_ != other.pointer_;
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator<(
    const base_iterator& other) const {
  return (bucket_ == other.bucket_ && position_ < other.position_) ||
         (bucket_ < other.bucket_);
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator>(
    const base_iterator& other) const {
  return other < *this;
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator<=(
    const base_iterator& other) const {
  return !(*this > other);
}

template <typename T>
template <bool is_constant>
bool Deque<T>::base_iterator<is_constant>::operator>=(
    const base_iterator& other) const {
  return !(*this < other);
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>&
Deque<T>::base_iterator<is_constant>::operator+=(int shift) {
  difference_type new_position = position_;
  new_position += shift;
  bucket_ += new_position / (difference_type)Bucket::size;
  new_position %= (difference_type)Bucket::size;
  if (new_position < 0) {
    --bucket_;
    new_position += Bucket::size;
  }
  position_ = new_position;
  pointer_ = bucket_->elements_ + position_;
  return *this;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>&
Deque<T>::base_iterator<is_constant>::operator-=(int shift) {
  (*this) += -shift;
  return *this;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>
Deque<T>::base_iterator<is_constant>::operator+(int shift) const {
  base_iterator<is_constant> answer(*this);
  answer += shift;
  return answer;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>
Deque<T>::base_iterator<is_constant>::operator-(int shift) const {
  base_iterator<is_constant> answer(*this);
  answer -= shift;
  return answer;
}

template <typename T>
template <bool is_constant>
typename Deque<T>::template base_iterator<is_constant>::difference_type
Deque<T>::base_iterator<is_constant>::operator-(
    const base_iterator<is_constant>& other) const {
  difference_type answer = (bucket_ - other.bucket_) * Bucket::size +
                           (difference_type)(position_) -
                           (difference_type)(other.position_);
  return answer;
}

template <typename T>
template <bool is_constant>
Deque<T>::base_iterator<is_constant>::operator base_iterator<true>() const {
  return base_iterator<true>(bucket_, position_);
}

template <typename T>
void Deque<T>::push_back(const T& value) {
  auto pos = get_rbegin();
  ++pos.second;
  if (pos.second == Bucket::size) {
    pos.second -= Bucket::size;
    pos.first += 1;
  }
  if (pos.first == bucket_quantity_ - 1 && pos.second == Bucket::size - 1) {
    Bucket* new_buckets = new Bucket[bucket_quantity_ << 1];
    try {
      size_t last_bucket = get_rbegin().first;
      for (size_t i = begin_bucket_; i <= last_bucket; ++i) {
        new_buckets[i] = buckets_[i];
      }
      new (new_buckets[pos.first].elements_ + pos.second) T(value);
    } catch (...) {
      delete[] new_buckets;
      throw;
    }
    delete[] buckets_;
    buckets_ = new_buckets;
    bucket_quantity_ <<= 1;
  } else {
    new (buckets_[pos.first].elements_ + pos.second) T(value);
  }
  ++size_;
}

template <typename T>
void Deque<T>::push_front(const T& value) {
  if (begin_bucket_ == 1 && begin_index_ == 0) {
    Bucket* new_buckets = new Bucket[bucket_quantity_ << 1];
    try {
      size_t last_bucket = get_rbegin().first;
      for (size_t i = begin_bucket_; i <= last_bucket; ++i) {
        new_buckets[i + bucket_quantity_] = buckets_[i];
      }
      new (new_buckets[bucket_quantity_].elements_ + Bucket::size - 1) T(value);
    } catch (...) {
      delete[] new_buckets;
      throw;
    }
    delete[] buckets_;
    buckets_ = new_buckets;
    begin_bucket_ = bucket_quantity_;
    begin_index_ = Bucket::size - 1;
    bucket_quantity_ <<= 1;
  } else {
    if (begin_index_ > 0) {
      --begin_index_;
    } else {
      --begin_bucket_;
      begin_index_ = Bucket::size - 1;
    }
    new (buckets_[begin_bucket_].elements_ + begin_index_) T(value);
  }
  ++size_;
}

template <typename T>
void Deque<T>::pop_back() {
  auto pos = get_rbegin();
  (buckets_[pos.first].elements_ + pos.second)->~T();
  --size_;
  if (size_ == 0) {
    begin_index_ = 0;
    begin_bucket_ = bucket_quantity_ >> 1;
  }
}

template <typename T>
void Deque<T>::pop_front() {
  (buckets_[begin_bucket_].elements_ + begin_index_)->~T();
  ++begin_index_;
  if (begin_index_ == Bucket::size) {
    begin_index_ = 0;
    ++begin_bucket_;
  }
  --size_;
  if (size_ == 0) {
    begin_index_ = 0;
    begin_bucket_ = bucket_quantity_ >> 1;
  }
}

template <typename T>
typename Deque<T>::iterator Deque<T>::begin() {
  return iterator(buckets_ + begin_bucket_, begin_index_);
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const {
  return const_iterator(buckets_ + begin_bucket_, begin_index_);
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
  return const_iterator(buckets_ + begin_bucket_, begin_index_);
}

template <typename T>
typename Deque<T>::iterator Deque<T>::end() {
  auto pos = get_end();
  return iterator(buckets_ + pos.first, pos.second);
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::end() const {
  auto pos = get_end();
  return const_iterator(buckets_ + pos.first, pos.second);
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
  auto pos = get_end();
  return const_iterator(buckets_ + pos.first, pos.second);
}

template <typename T>
typename Deque<T>::reverse_iterator Deque<T>::rbegin() {
  auto pos = get_end();
  return reverse_iterator(iterator(buckets_ + pos.first, pos.second));
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rbegin() const {
  auto pos = get_end();
  return const_reverse_iterator(
      const_iterator(buckets_ + pos.first, pos.second));
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crbegin() const {
  auto pos = get_end();
  return const_reverse_iterator(
      const_iterator(buckets_ + pos.first, pos.second));
}

template <typename T>
typename Deque<T>::reverse_iterator Deque<T>::rend() {
  auto it = reverse_iterator(iterator(buckets_ + begin_bucket_, begin_index_));
  return it;
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rend() const {
  auto it = reverse_iterator(iterator(buckets_ + begin_bucket_, begin_index_));
  return const_reverse_iterator(it);
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crend() const {
  auto it = reverse_iterator(buckets_ + begin_bucket_, begin_index_);
  return const_reverse_iterator(it);
}

template <typename T>
T& Deque<T>::operator[](size_t position) {
  position += begin_bucket_ * Bucket::size + begin_index_;
  return buckets_[bucket_index(position)][in_bucket_index(position)];
}

template <typename T>
const T& Deque<T>::operator[](size_t position) const {
  position += begin_bucket_ * Bucket::size + begin_index_;
  return buckets_[bucket_index(position)][in_bucket_index(position)];
}

template <typename T>
T& Deque<T>::at(size_t position) {
  if (position >= size_) {
    throw std::out_of_range("Too big number");
  }
  return (*this)[position];
}

template <typename T>
const T& Deque<T>::at(size_t position) const {
  if (position >= size_) {
    throw std::out_of_range("Too big number");
  }
  return (*this)[position];
}

template <typename T>
void Deque<T>::insert(iterator it, const T& value) {
  auto pos = get_position(it);
  pos.first -= begin_bucket_;
  push_back(value);
  auto stop_iter = iterator(buckets_ + begin_bucket_ + pos.first, pos.second);
  auto iter = end() - 1;
  for (auto iter2 = iter - 1; iter2 >= stop_iter; --iter2, --iter) {
    (*iter) = (*iter2);
  }
  buckets_[begin_bucket_ + pos.first][pos.second] = value;
}

template <typename T>
void Deque<T>::erase(iterator it) {
  auto iter2 = it + 1;
  for (; iter2 < end(); ++it, ++iter2) {
    (*it) = (*iter2);
  }
  pop_back();
}