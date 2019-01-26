
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
#pragma once

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace rocksdb {

#ifdef ROCKSDB_LITE
template <class T, size_t kSize = 8>
class autovector : public std::vector<T> {
  using std::vector<T>::vector;
};
#else
//利用预先分配的基于堆栈的数组实现更好的
//具有少量项的数组的性能。
//
//界面与矢量界面相似，但由于我们的目标是
//解决我们手头的问题，而不是执行
//成熟的通用容器。
//
//目前我们不支持：
//*reserve（）/缩小到适合
//如果使用正确，在大多数情况下，人们不应触摸
//基本向量。
//*random insert（）/erase（），请只使用push_back（）/pop_back（）。
//*无移动/交换操作。每个autovector实例都有一个
//堆栈分配的数组，如果我们想要支持移动/交换操作，我们
//需要复制数组，而不仅仅是交换指针。在这
//既然他们可能
//引导用户通过认为自己很便宜而做出错误的假设
//操作。
//
//公共方法的命名风格几乎遵循STL的命名风格。
template <class T, size_t kSize = 8>
class autovector {
 public:
//常规STL样式容器成员类型。
  typedef T value_type;
  typedef typename std::vector<T>::difference_type difference_type;
  typedef typename std::vector<T>::size_type size_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

//此类是正则/常量迭代器的基础
  template <class TAutoVector, class TValueType>
  class iterator_impl {
   public:
//--迭代器特性
    typedef iterator_impl<TAutoVector, TValueType> self_type;
    typedef TValueType value_type;
    typedef TValueType& reference;
    typedef TValueType* pointer;
    typedef typename TAutoVector::difference_type difference_type;
    typedef std::random_access_iterator_tag iterator_category;

    iterator_impl(TAutoVector* vect, size_t index)
        : vect_(vect), index_(index) {};
    iterator_impl(const iterator_impl&) = default;
    ~iterator_impl() {}
    iterator_impl& operator=(const iterator_impl&) = default;

//--进步
//++迭代器
    self_type& operator++() {
      ++index_;
      return *this;
    }

//迭代器+
    self_type operator++(int) {
      auto old = *this;
      ++index_;
      return old;
    }

//迭代器
    self_type& operator--() {
      --index_;
      return *this;
    }

//迭代器——
    self_type operator--(int) {
      auto old = *this;
      --index_;
      return old;
    }

    self_type operator-(difference_type len) const {
      return self_type(vect_, index_ - len);
    }

    difference_type operator-(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ - other.index_;
    }

    self_type operator+(difference_type len) const {
      return self_type(vect_, index_ + len);
    }

    self_type& operator+=(difference_type len) {
      index_ += len;
      return *this;
    }

    self_type& operator-=(difference_type len) {
      index_ -= len;
      return *this;
    }

//--参考文献
    reference operator*() {
      assert(vect_->size() >= index_);
      return (*vect_)[index_];
    }

    const_reference operator*() const {
      assert(vect_->size() >= index_);
      return (*vect_)[index_];
    }

    pointer operator->() {
      assert(vect_->size() >= index_);
      return &(*vect_)[index_];
    }

    const_pointer operator->() const {
      assert(vect_->size() >= index_);
      return &(*vect_)[index_];
    }


//--逻辑运算符
    bool operator==(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ == other.index_;
    }

    bool operator!=(const self_type& other) const { return !(*this == other); }

    bool operator>(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ > other.index_;
    }

    bool operator<(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ < other.index_;
    }

    bool operator>=(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ >= other.index_;
    }

    bool operator<=(const self_type& other) const {
      assert(vect_ == other.vect_);
      return index_ <= other.index_;
    }

   private:
    TAutoVector* vect_ = nullptr;
    size_t index_ = 0;
  };

  typedef iterator_impl<autovector, value_type> iterator;
  typedef iterator_impl<const autovector, const value_type> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  autovector() = default;

  autovector(std::initializer_list<T> init_list) {
    for (const T& item : init_list) {
      push_back(item);
    }
  }

  ~autovector() = default;

//——不变运算
//指示所有数据是否驻留在堆栈数据结构中。
  bool only_in_stack() const {
//如果根本没有插入任何元素，那么向量的容量将为“0”。
    return vect_.capacity() == 0;
  }

  size_type size() const { return num_stack_items_ + vect_.size(); }

//调整大小并不能保证
//可用元素
  void resize(size_type n) {
    if (n > kSize) {
      vect_.resize(n - kSize);
      num_stack_items_ = kSize;
    } else {
      vect_.clear();
      num_stack_items_ = n;
    }
  }

  bool empty() const { return size() == 0; }

  const_reference operator[](size_type n) const {
    assert(n < size());
    return n < kSize ? values_[n] : vect_[n - kSize];
  }

  reference operator[](size_type n) {
    assert(n < size());
    return n < kSize ? values_[n] : vect_[n - kSize];
  }

  const_reference at(size_type n) const {
    assert(n < size());
    return (*this)[n];
  }

  reference at(size_type n) {
    assert(n < size());
    return (*this)[n];
  }

  reference front() {
    assert(!empty());
    return *begin();
  }

  const_reference front() const {
    assert(!empty());
    return *begin();
  }

  reference back() {
    assert(!empty());
    return *(end() - 1);
  }

  const_reference back() const {
    assert(!empty());
    return *(end() - 1);
  }

//——可变操作
  void push_back(T&& item) {
    if (num_stack_items_ < kSize) {
      values_[num_stack_items_++] = std::move(item);
    } else {
      vect_.push_back(item);
    }
  }

  void push_back(const T& item) {
    if (num_stack_items_ < kSize) {
      values_[num_stack_items_++] = item;
    } else {
      vect_.push_back(item);
    }
  }

  template <class... Args>
  void emplace_back(Args&&... args) {
    push_back(value_type(args...));
  }

  void pop_back() {
    assert(!empty());
    if (!vect_.empty()) {
      vect_.pop_back();
    } else {
      --num_stack_items_;
    }
  }

  void clear() {
    num_stack_items_ = 0;
    vect_.clear();
  }

//--复制和转让
  autovector& assign(const autovector& other);

  autovector(const autovector& other) { assign(other); }

  autovector& operator=(const autovector& other) { return assign(other); }

//--迭代器操作
  iterator begin() { return iterator(this, 0); }

  const_iterator begin() const { return const_iterator(this, 0); }

  iterator end() { return iterator(this, this->size()); }

  const_iterator end() const { return const_iterator(this, this->size()); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

 private:
size_type num_stack_items_ = 0;  //当前项目数
value_type values_[kSize];       //第一个“ksize”项
//仅在有多个“ksize”项时使用。
  std::vector<T> vect_;
};

template <class T, size_t kSize>
autovector<T, kSize>& autovector<T, kSize>::assign(const autovector& other) {
//复制内部向量
  vect_.assign(other.vect_.begin(), other.vect_.end());

//拷贝数组
  num_stack_items_ = other.num_stack_items_;
  std::copy(other.values_, other.values_ + num_stack_items_, values_);

  return *this;
}
#endif  //摇滚乐
}  //命名空间rocksdb
