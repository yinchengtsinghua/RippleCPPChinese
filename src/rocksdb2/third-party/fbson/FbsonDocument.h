
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

/*
 *此标题定义fbsonDocument、fbsonKeyValue和各种值类。
 *从fbsonValue派生，以及容器的前向迭代器
 *值-基本上与fbson二进制数据相关的所有内容
 ＊结构。
 *
 *实施说明：
 *
 *此头文件中的所有类都不能直接实例化（即
 *不能创建fbsonKeyValue或fbsonValue对象-所有构造函数
 *宣布为非公开）。我们使用类作为打包的fbson上的包装器
 *字节（序列化），并将类（类型）强制转换为底层压缩
 *字节数组。
 *
 *出于同样的原因，我们不能将任何fbson值类定义为虚拟的，
 *因为我们从不调用构造函数，也不会实例化vtbl和vptrs。
 *
 *因此，类被定义为打包结构（即没有数据
 *对齐和填充），类的私有成员变量为
 *与FBSON规范的顺序完全相同。这确保了我们
 *正确访问压缩的fbson字节。
 *
 *包装结构高度优化，适合低成本就地操作。
 *开销。读取（和就地写入）直接在压缩包上执行
 *字节。运行时根本没有内存分配。
 *
 *对于将扩展原始fbson大小的值的更新/写入，
 *写入失败，调用方需要处理缓冲区增加。
 *
 ***迭代器**
 *ObjectVal类和ArrayVal类都有迭代器类型，可以使用
 *在容器对象上声明迭代器以遍历键值
 *对或值列表。迭代器同时具有非常量和常量类型。
 *
 *注：迭代器仅为正向。
 *
 ***查询**
 *通过成员函数find（键/值）查询容器
 *对）和get（用于数组元素），并且是流式的。我们不
 *需要读取/扫描整个fbson压缩字节以返回结果。
 *一旦找到键/索引，我们将停止搜索。可以使用文本进行查询
 *对象和数组（对于数组，文本将转换为整数索引）。
 *并使用索引从数组中检索。数组索引基于0。
 *
 ***外部字典**
 *在查询处理过程中，还可以传递回调函数，因此
 *搜索将首先尝试检查字典中是否存在密钥字符串。
 *如果是，搜索将基于ID而不是键字符串。
 *
 *@author tian xia<tianx@fb.com>
 **/


#ifndef FBSON_FBSONDOCUMENT_H
#define FBSON_FBSONDOCUMENT_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>

namespace fbson {

#pragma pack(push, 1)

#define FBSON_VER 1

//远期申报
class FbsonValue;
class ObjectVal;

/*
 *fbsondocument是访问和查询fbson packed的主要对象。
 *字节。注意：fbsondocument只允许对象容器作为顶层
 * FBSON值。但是，可以使用静态方法“createValue”获取
 *来自压缩字节的fbsonValue对象。
 *
 *fbsonDocument对象也取消对对象容器值的引用
 *（objectval）加载fbson后。
 *
 ***加载**
 *将压缩字节（内存位置）加载到
 *对象。我们只需要头和负载的前几个字节
 *验证FBSON的标题。
 *
 *注意：创建fbsondocument（通过createddocument）不分配
 *任何记忆。文档对象是压缩字节上的有效包装器
 *直接访问。
 *
 ***查询**
 *查询是通过取消对ObjectVal的引用。
 **/

class FbsonDocument {
 public:
//从fbson压缩字节创建fbsondocument对象
  static FbsonDocument* createDocument(const char* pb, uint32_t size);

//从fbson压缩字节创建fbson值
  static FbsonValue* createValue(const char* pb, uint32_t size);

  uint8_t version() { return header_.ver_; }

  FbsonValue* getValue() { return ((FbsonValue*)payload_); }

  ObjectVal* operator->() { return ((ObjectVal*)payload_); }

  const ObjectVal* operator->() const { return ((const ObjectVal*)payload_); }

 private:
  /*
   *fbsonHeader类定义fbson Header（fbsonDocument内部）。
   *
   *目前只包含版本信息（1字节）。我们可以扩大
   *头部包含fbson二进制校验和，以提高安全性。
   **/

  struct FbsonHeader {
    uint8_t ver_;
  } header_;

  char payload_[1];

  FbsonDocument();

  FbsonDocument(const FbsonDocument&) = delete;
  FbsonDocument& operator=(const FbsonDocument&) = delete;
};

/*
 *fbsonfwditerator实现fbson的迭代器模板。
 *
 *注：由于fbson格式的设计，它是一个前向迭代器。
 **/

template <class Iter_Type, class Cont_Type>
class FbsonFwdIteratorT {
  typedef Iter_Type iterator;
  typedef typename std::iterator_traits<Iter_Type>::pointer pointer;
  typedef typename std::iterator_traits<Iter_Type>::reference reference;

 public:
  explicit FbsonFwdIteratorT(const iterator& i) : current_(i) {}

//允许非常量到常量迭代器转换（同一容器类型）
  template <class Iter_Ty>
  FbsonFwdIteratorT(const FbsonFwdIteratorT<Iter_Ty, Cont_Type>& rhs)
      : current_(rhs.base()) {}

  bool operator==(const FbsonFwdIteratorT& rhs) const {
    return (current_ == rhs.current_);
  }

  bool operator!=(const FbsonFwdIteratorT& rhs) const {
    return !operator==(rhs);
  }

  bool operator<(const FbsonFwdIteratorT& rhs) const {
    return (current_ < rhs.current_);
  }

  bool operator>(const FbsonFwdIteratorT& rhs) const { return !operator<(rhs); }

  FbsonFwdIteratorT& operator++() {
    current_ = (iterator)(((char*)current_) + current_->numPackedBytes());
    return *this;
  }

  FbsonFwdIteratorT operator++(int) {
    auto tmp = *this;
    current_ = (iterator)(((char*)current_) + current_->numPackedBytes());
    return tmp;
  }

  explicit operator pointer() { return current_; }

  reference operator*() const { return *current_; }

  pointer operator->() const { return current_; }

  iterator base() const { return current_; }

 private:
  iterator current_;
};

typedef int (*hDictInsert)(const char* key, unsigned len);
typedef int (*hDictFind)(const char* key, unsigned len);

/*
 *fbsonType定义了10个基本类型和2个容器类型，如前所述。
 *下面。
 *
 *原始值：=
 *0x00//空值（0字节）
 *0x01//布尔值真（0字节）
 *0x02//布尔值错误（0字节）
 *0x03 int8//char/int8（1字节）
 *0x04 int16//int16（2字节）
 *0x05 int32//int32（4字节）
 *0x06 int64//int64（8字节）
 *0x07 double//浮点（8字节）
 *0x08字符串//可变长度字符串
 *0x09 binary//可变长度binary
 *
 *容器：：=
 *0x0A Int32 key_value_list//对象，Int32是对象的总字节数。
 *0x0B int32 value_list//数组，int32是数组的总字节数
 **/

enum class FbsonType : char {
  T_Null = 0x00,
  T_True = 0x01,
  T_False = 0x02,
  T_Int8 = 0x03,
  T_Int16 = 0x04,
  T_Int32 = 0x05,
  T_Int64 = 0x06,
  T_Double = 0x07,
  T_String = 0x08,
  T_Binary = 0x09,
  T_Object = 0x0A,
  T_Array = 0x0B,
  NUM_TYPES,
};

typedef std::underlying_type<FbsonType>::type FbsonTypeUnder;

/*
 *fbsonkeyValue类定义fbson键类型，如下所述。
 *
 *键：
 *0x00 int8//1字节字典ID
 *Int8（byte*）//Int8（>0）是密钥字符串的大小
 *
 *值：：=原语容器
 *
 *fbsonkeyValue可以是指向外部密钥字符串的ID映射
 *字典，或者是原始密钥字符串。是否读取ID或
 *字符串由第一个字节（大小）决定。
 *
 *注意：键对象后面必须跟一个值对象。因此，一把钥匙
 *对象隐式引用一个键值对，可以得到
 *对象在键对象之后。因此，函数numpackedbytes
 *表示键值对的总大小，这样我们就可以
 *从键到下一对。
 *
 ***字典大小**
 *默认字典大小为255（1字节）。用户可以定义
 *使用“大字典”将字典大小增加到655535（2字节）。
 **/

class FbsonKeyValue {
 public:
#ifdef USE_LARGE_DICT
  static const int sMaxKeyId = 65535;
  typedef uint16_t keyid_type;
#else
  static const int sMaxKeyId = 255;
  typedef uint8_t keyid_type;
#endif //使用大型口述

  static const uint8_t sMaxKeyLen = 64;

//键的大小。0表示存储为ID
  uint8_t klen() const { return size_; }

//获取密钥字符串。注意，字符串不能以空结尾。
  const char* getKeyStr() const { return key_.str_; }

  keyid_type getKeyId() const { return key_.id_; }

  unsigned int keyPackedBytes() const {
    return size_ ? (sizeof(size_) + size_)
                 : (sizeof(size_) + sizeof(keyid_type));
  }

  FbsonValue* value() const {
    return (FbsonValue*)(((char*)this) + keyPackedBytes());
  }

//总压缩字节的大小（键+值）
  unsigned int numPackedBytes() const;

 private:
  uint8_t size_;

  union key_ {
    keyid_type id_;
    char str_[1];
  } key_;

  FbsonKeyValue();
};

/*
 *fbsonValue是所有fbson类型的基类。它只包含一个成员
 *variable-类型信息，成员函数可以检索到的是[type]（）
 *或类型（）。
 **/

class FbsonValue {
 public:
static const uint32_t sMaxValueLen = 1 << 24; //16m

  bool isNull() const { return (type_ == FbsonType::T_Null); }
  bool isTrue() const { return (type_ == FbsonType::T_True); }
  bool isFalse() const { return (type_ == FbsonType::T_False); }
  bool isInt8() const { return (type_ == FbsonType::T_Int8); }
  bool isInt16() const { return (type_ == FbsonType::T_Int16); }
  bool isInt32() const { return (type_ == FbsonType::T_Int32); }
  bool isInt64() const { return (type_ == FbsonType::T_Int64); }
  bool isDouble() const { return (type_ == FbsonType::T_Double); }
  bool isString() const { return (type_ == FbsonType::T_String); }
  bool isBinary() const { return (type_ == FbsonType::T_Binary); }
  bool isObject() const { return (type_ == FbsonType::T_Object); }
  bool isArray() const { return (type_ == FbsonType::T_Array); }

  FbsonType type() const { return type_; }

//总压缩字节的大小
  unsigned int numPackedBytes() const;

//值的大小（字节）
  unsigned int size() const;

//获取值的原始字节数组
  const char* getValuePtr() const;

//通过键路径字符串查找fbson值（以空结尾）
  FbsonValue* findPath(const char* key_path,
                       const char* delim = ".",
                       hDictFind handler = nullptr) {
    return findPath(key_path, (unsigned int)strlen(key_path), delim, handler);
  }

//通过键路径字符串（带长度）查找fbson值
  FbsonValue* findPath(const char* key_path,
                       unsigned int len,
                       const char* delim,
                       hDictFind handler);

 protected:
FbsonType type_; //类型信息

  FbsonValue();
};

/*
 *NumerValt是所有数字的模板类（从fbsonValue派生）。
 *类型（整数和双精度）。
 **/

template <class T>
class NumberValT : public FbsonValue {
 public:
  T val() const { return num_; }

  unsigned int numPackedBytes() const { return sizeof(FbsonValue) + sizeof(T); }

//捕获模板类的所有未知专用化
  bool setVal(T value) { return false; }

 private:
  T num_;

  NumberValT();
};

typedef NumberValT<int8_t> Int8Val;

//重写int8val的setval
template <>
inline bool Int8Val::setVal(int8_t value) {
  if (!isInt8()) {
    return false;
  }

  num_ = value;
  return true;
}

typedef NumberValT<int16_t> Int16Val;

//重写int16val的setval
template <>
inline bool Int16Val::setVal(int16_t value) {
  if (!isInt16()) {
    return false;
  }

  num_ = value;
  return true;
}

typedef NumberValT<int32_t> Int32Val;

//重写int32val的setval
template <>
inline bool Int32Val::setVal(int32_t value) {
  if (!isInt32()) {
    return false;
  }

  num_ = value;
  return true;
}

typedef NumberValT<int64_t> Int64Val;

//重写int64val的setval
template <>
inline bool Int64Val::setVal(int64_t value) {
  if (!isInt64()) {
    return false;
  }

  num_ = value;
  return true;
}

typedef NumberValT<double> DoubleVal;

//覆盖双重评估的setval
template <>
inline bool DoubleVal::setVal(double value) {
  if (!isDouble()) {
    return false;
  }

  num_ = value;
  return true;
}

/*
 *blobval是字符串和二进制的基类（从fbsonValue派生）。
 *类型。大小表示有效负载的总字节数。
 **/

class BlobVal : public FbsonValue {
 public:
//仅Blob负载的大小
  unsigned int getBlobLen() const { return size_; }

//将blob作为字节数组返回
  const char* getBlob() const { return payload_; }

//总压缩字节的大小
  unsigned int numPackedBytes() const {
    return sizeof(FbsonValue) + sizeof(size_) + size_;
  }

 protected:
  uint32_t size_;
  char payload_[1];

//设置新的blob字节
  bool internalSetVal(const char* blob, uint32_t blobSize) {
//如果我们无法适应新的blob，则操作失败
    if (blobSize > size_) {
      return false;
    }

    memcpy(payload_, blob, blobSize);

//将字节重置为0。注意，我们不能更改
//当前有效载荷，因为所有值都已打包。
    memset(payload_ + blobSize, 0, size_ - blobSize);

    return true;
  }

  BlobVal();

 private:
//禁用，因为此类只能动态分配
  BlobVal(const BlobVal&) = delete;
  BlobVal& operator=(const BlobVal&) = delete;
};

/*
 ＊二进制类型
 **/

class BinaryVal : public BlobVal {
 public:
  bool setVal(const char* blob, uint32_t blobSize) {
    if (!isBinary()) {
      return false;
    }

    return internalSetVal(blob, blobSize);
  }

 private:
  BinaryVal();
};

/*
 *字符串类型
 *注：fbson字符串不能是C字符串（以空结尾）
 **/

class StringVal : public BlobVal {
 public:
  bool setVal(const char* str, uint32_t blobSize) {
    if (!isString()) {
      return false;
    }

    return internalSetVal(str, blobSize);
  }

 private:
  StringVal();
};

/*
 *ContainerVal是对象和
 *数组类型。大小表示有效负载的总字节数。
 **/

class ContainerVal : public FbsonValue {
 public:
//仅集装箱有效载荷的大小
  unsigned int getContainerSize() const { return size_; }

//将容器负载作为字节数组返回
  const char* getPayload() const { return payload_; }

//总压缩字节的大小
  unsigned int numPackedBytes() const {
    return sizeof(FbsonValue) + sizeof(size_) + size_;
  }

 protected:
  uint32_t size_;
  char payload_[1];

  ContainerVal();

  ContainerVal(const ContainerVal&) = delete;
  ContainerVal& operator=(const ContainerVal&) = delete;
};

/*
 *对象类型
 **/

class ObjectVal : public ContainerVal {
 public:
//通过键字符串查找fbson值（以空结尾）
  FbsonValue* find(const char* key, hDictFind handler = nullptr) const {
    if (!key)
      return nullptr;

    return find(key, (unsigned int)strlen(key), handler);
  }

//通过键字符串（带长度）查找fbson值
  FbsonValue* find(const char* key,
                   unsigned int klen,
                   hDictFind handler = nullptr) const {
    if (!key || !klen)
      return nullptr;

    int key_id = -1;
    if (handler && (key_id = handler(key, klen)) >= 0) {
      return find(key_id);
    }

    return internalFind(key, klen);
  }

//通过键字典ID查找fbson值
  FbsonValue* find(int key_id) const {
    if (key_id < 0 || key_id > FbsonKeyValue::sMaxKeyId)
      return nullptr;

    const char* pch = payload_;
    const char* fence = payload_ + size_;

    while (pch < fence) {
      FbsonKeyValue* pkey = (FbsonKeyValue*)(pch);
      if (!pkey->klen() && key_id == pkey->getKeyId()) {
        return pkey->value();
      }
      pch += pkey->numPackedBytes();
    }

    assert(pch == fence);

    return nullptr;
  }

  typedef FbsonKeyValue value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef FbsonFwdIteratorT<pointer, ObjectVal> iterator;
  typedef FbsonFwdIteratorT<const_pointer, ObjectVal> const_iterator;

  iterator begin() { return iterator((pointer)payload_); }

  const_iterator begin() const { return const_iterator((pointer)payload_); }

  iterator end() { return iterator((pointer)(payload_ + size_)); }

  const_iterator end() const {
    return const_iterator((pointer)(payload_ + size_));
  }

 private:
  FbsonValue* internalFind(const char* key, unsigned int klen) const {
    const char* pch = payload_;
    const char* fence = payload_ + size_;

    while (pch < fence) {
      FbsonKeyValue* pkey = (FbsonKeyValue*)(pch);
      if (klen == pkey->klen() && strncmp(key, pkey->getKeyStr(), klen) == 0) {
        return pkey->value();
      }
      pch += pkey->numPackedBytes();
    }

    assert(pch == fence);

    return nullptr;
  }

 private:
  ObjectVal();
};

/*
 *数组类型
 **/

class ArrayVal : public ContainerVal {
 public:
//在索引处获取fbson值
  FbsonValue* get(int idx) const {
    if (idx < 0)
      return nullptr;

    const char* pch = payload_;
    const char* fence = payload_ + size_;

    while (pch < fence && idx-- > 0)
      pch += ((FbsonValue*)pch)->numPackedBytes();

    if (idx == -1)
      return (FbsonValue*)pch;
    else {
      assert(pch == fence);
      return nullptr;
    }
  }

//获取数组中的元素数
  unsigned int numElem() const {
    const char* pch = payload_;
    const char* fence = payload_ + size_;

    unsigned int num = 0;
    while (pch < fence) {
      ++num;
      pch += ((FbsonValue*)pch)->numPackedBytes();
    }

    assert(pch == fence);

    return num;
  }

  typedef FbsonValue value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef FbsonFwdIteratorT<pointer, ArrayVal> iterator;
  typedef FbsonFwdIteratorT<const_pointer, ArrayVal> const_iterator;

  iterator begin() { return iterator((pointer)payload_); }

  const_iterator begin() const { return const_iterator((pointer)payload_); }

  iterator end() { return iterator((pointer)(payload_ + size_)); }

  const_iterator end() const {
    return const_iterator((pointer)(payload_ + size_));
  }

 private:
  ArrayVal();
};

inline FbsonDocument* FbsonDocument::createDocument(const char* pb,
                                                    uint32_t size) {
  if (!pb || size < sizeof(FbsonHeader) + sizeof(FbsonValue)) {
    return nullptr;
  }

  FbsonDocument* doc = (FbsonDocument*)pb;
  if (doc->header_.ver_ != FBSON_VER) {
    return nullptr;
  }

  FbsonValue* val = (FbsonValue*)doc->payload_;
  if (!val->isObject() || size != sizeof(FbsonHeader) + val->numPackedBytes()) {
    return nullptr;
  }

  return doc;
}

inline FbsonValue* FbsonDocument::createValue(const char* pb, uint32_t size) {
  if (!pb || size < sizeof(FbsonHeader) + sizeof(FbsonValue)) {
    return nullptr;
  }

  FbsonDocument* doc = (FbsonDocument*)pb;
  if (doc->header_.ver_ != FBSON_VER) {
    return nullptr;
  }

  FbsonValue* val = (FbsonValue*)doc->payload_;
  if (size != sizeof(FbsonHeader) + val->numPackedBytes()) {
    return nullptr;
  }

  return val;
}

inline unsigned int FbsonKeyValue::numPackedBytes() const {
  unsigned int ks = keyPackedBytes();
  FbsonValue* val = (FbsonValue*)(((char*)this) + ks);
  return ks + val->numPackedBytes();
}

//穷人的“虚拟”函数fbsonValue:：numPackedBytes
inline unsigned int FbsonValue::numPackedBytes() const {
  switch (type_) {
  case FbsonType::T_Null:
  case FbsonType::T_True:
  case FbsonType::T_False: {
    return sizeof(type_);
  }

  case FbsonType::T_Int8: {
    return sizeof(type_) + sizeof(int8_t);
  }
  case FbsonType::T_Int16: {
    return sizeof(type_) + sizeof(int16_t);
  }
  case FbsonType::T_Int32: {
    return sizeof(type_) + sizeof(int32_t);
  }
  case FbsonType::T_Int64: {
    return sizeof(type_) + sizeof(int64_t);
  }
  case FbsonType::T_Double: {
    return sizeof(type_) + sizeof(double);
  }
  case FbsonType::T_String:
  case FbsonType::T_Binary: {
    return ((BlobVal*)(this))->numPackedBytes();
  }

  case FbsonType::T_Object:
  case FbsonType::T_Array: {
    return ((ContainerVal*)(this))->numPackedBytes();
  }
  default:
    return 0;
  }
}

inline unsigned int FbsonValue::size() const {
  switch (type_) {
  case FbsonType::T_Int8: {
    return sizeof(int8_t);
  }
  case FbsonType::T_Int16: {
    return sizeof(int16_t);
  }
  case FbsonType::T_Int32: {
    return sizeof(int32_t);
  }
  case FbsonType::T_Int64: {
    return sizeof(int64_t);
  }
  case FbsonType::T_Double: {
    return sizeof(double);
  }
  case FbsonType::T_String:
  case FbsonType::T_Binary: {
    return ((BlobVal*)(this))->getBlobLen();
  }

  case FbsonType::T_Object:
  case FbsonType::T_Array: {
    return ((ContainerVal*)(this))->getContainerSize();
  }
  case FbsonType::T_Null:
  case FbsonType::T_True:
  case FbsonType::T_False:
  default:
    return 0;
  }
}

inline const char* FbsonValue::getValuePtr() const {
  switch (type_) {
  case FbsonType::T_Int8:
  case FbsonType::T_Int16:
  case FbsonType::T_Int32:
  case FbsonType::T_Int64:
  case FbsonType::T_Double:
    return ((char*)this) + sizeof(FbsonType);

  case FbsonType::T_String:
  case FbsonType::T_Binary:
    return ((BlobVal*)(this))->getBlob();

  case FbsonType::T_Object:
  case FbsonType::T_Array:
    return ((ContainerVal*)(this))->getPayload();

  case FbsonType::T_Null:
  case FbsonType::T_True:
  case FbsonType::T_False:
  default:
    return nullptr;
  }
}

inline FbsonValue* FbsonValue::findPath(const char* key_path,
                                        unsigned int kp_len,
                                        const char* delim = ".",
                                        hDictFind handler = nullptr) {
  if (!key_path || !kp_len)
    return nullptr;

  if (!delim)
delim = "."; //默认分隔符

  FbsonValue* pval = this;
  const char* fence = key_path + kp_len;
char idx_buf[21]; //分析数组索引的缓冲区（整数值）

  while (pval && key_path < fence) {
    const char* key = key_path;
    unsigned int klen = 0;
//查找当前密钥
    for (; key_path != fence && *key_path != *delim; ++key_path, ++klen)
      ;

    if (!klen)
      return nullptr;

    switch (pval->type_) {
    case FbsonType::T_Object: {
      pval = ((ObjectVal*)pval)->find(key, klen, handler);
      break;
    }

    case FbsonType::T_Array: {
//将字符串解析为整数（数组索引）
      if (klen >= sizeof(idx_buf))
        return nullptr;

      memcpy(idx_buf, key, klen);
      idx_buf[klen] = 0;

      char* end = nullptr;
      int index = (int)strtol(idx_buf, &end, 10);
      if (end && !*end)
        pval = ((fbson::ArrayVal*)pval)->get(index);
      else
//索引字符串不正确
        return nullptr;
      break;
    }

    default:
      return nullptr;
    }

//跳过分隔符
    if (key_path < fence) {
      ++key_path;
      if (key_path == fence)
//结尾有一个尾随分隔符
        return nullptr;
    }
  }

  return pval;
}

#pragma pack(pop)

} //命名空间FBSON

#endif //FBSON文件
