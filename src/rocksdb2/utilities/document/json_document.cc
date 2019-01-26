
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
#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/json_document.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>


#include "third-party/fbson/FbsonDocument.h"
#include "third-party/fbson/FbsonJsonParser.h"
#include "third-party/fbson/FbsonUtil.h"
#include "util/coding.h"

using std::placeholders::_1;

namespace {

size_t ObjectNumElem(const fbson::ObjectVal& objectVal) {
  size_t size = 0;
  for (auto keyValuePair : objectVal) {
    (void)keyValuePair;
    ++size;
  }
  return size;
}

template <typename Func>
void InitJSONDocument(std::unique_ptr<char[]>* data,
                      fbson::FbsonValue** value,
                      Func f) {
//TODO（stash）：是否可以向fbsondocument添加函数以避免创建数组？
  fbson::FbsonWriter writer;
  bool res __attribute__((unused)) = writer.writeStartArray();
  assert(res);
  uint32_t bytesWritten __attribute__((unused)) = f(writer);
  assert(bytesWritten != 0);
  res = writer.writeEndArray();
  assert(res);
  char* buf = new char[writer.getOutput()->getSize()];
  memcpy(buf, writer.getOutput()->getBuffer(), writer.getOutput()->getSize());

  *value = ((fbson::FbsonDocument *)buf)->getValue();
  assert((*value)->isArray());
  assert(((fbson::ArrayVal*)*value)->numElem() == 1);
  *value = ((fbson::ArrayVal*)*value)->get(0);
  data->reset(buf);
}

void InitString(std::unique_ptr<char[]>* data,
                fbson::FbsonValue** value,
                const std::string& s) {
  InitJSONDocument(data, value, std::bind(
      [](fbson::FbsonWriter& writer, const std::string& str) -> uint32_t {
        bool res __attribute__((unused)) = writer.writeStartString();
        assert(res);
        auto bytesWritten = writer.writeString(str.c_str(),
                            static_cast<uint32_t>(str.length()));
        res = writer.writeEndString();
        assert(res);
//如果字符串为空，则byteswritten==0，并在
//initjsondocument将失败。
        return bytesWritten + static_cast<uint32_t>(str.empty());
      },
  _1, s));
}

bool IsNumeric(fbson::FbsonValue* value) {
  return value->isInt8() || value->isInt16() ||
         value->isInt32() ||  value->isInt64();
}

int64_t GetInt64ValFromFbsonNumericType(fbson::FbsonValue* value) {
  switch (value->type()) {
    case fbson::FbsonType::T_Int8:
      return reinterpret_cast<fbson::Int8Val*>(value)->val();
    case fbson::FbsonType::T_Int16:
      return reinterpret_cast<fbson::Int16Val*>(value)->val();
    case fbson::FbsonType::T_Int32:
      return reinterpret_cast<fbson::Int32Val*>(value)->val();
    case fbson::FbsonType::T_Int64:
      return reinterpret_cast<fbson::Int64Val*>(value)->val();
    default:
      assert(false);
  }
  return 0;
}

bool IsComparable(fbson::FbsonValue* left, fbson::FbsonValue* right) {
  if (left->type() == right->type()) {
    return true;
  }
  if (IsNumeric(left) && IsNumeric(right)) {
    return true;
  }
  return false;
}

void CreateArray(std::unique_ptr<char[]>* data, fbson::FbsonValue** value) {
  fbson::FbsonWriter writer;
  bool res __attribute__((unused)) = writer.writeStartArray();
  assert(res);
  res = writer.writeEndArray();
  assert(res);
  data->reset(new char[writer.getOutput()->getSize()]);
  memcpy(data->get(),
         writer.getOutput()->getBuffer(),
         writer.getOutput()->getSize());
  *value = reinterpret_cast<fbson::FbsonDocument*>(data->get())->getValue();
}

void CreateObject(std::unique_ptr<char[]>* data, fbson::FbsonValue** value) {
  fbson::FbsonWriter writer;
  bool res __attribute__((unused)) = writer.writeStartObject();
  assert(res);
  res = writer.writeEndObject();
  assert(res);
  data->reset(new char[writer.getOutput()->getSize()]);
  memcpy(data->get(),
         writer.getOutput()->getBuffer(),
         writer.getOutput()->getSize());
  *value = reinterpret_cast<fbson::FbsonDocument*>(data->get())->getValue();
}

}  //命名空间

namespace rocksdb {


//托多：更容易找到
JSONDocument::JSONDocument() {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeNull, _1));
}

JSONDocument::JSONDocument(bool b) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeBool, _1, b));
}

JSONDocument::JSONDocument(double d) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeDouble, _1, d));
}

JSONDocument::JSONDocument(int8_t i) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeInt8, _1, i));
}

JSONDocument::JSONDocument(int16_t i) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeInt16, _1, i));
}

JSONDocument::JSONDocument(int32_t i) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeInt32, _1, i));
}

JSONDocument::JSONDocument(int64_t i) {
  InitJSONDocument(&data_,
                   &value_,
                   std::bind(&fbson::FbsonWriter::writeInt64, _1, i));
}

JSONDocument::JSONDocument(const std::string& s) {
  InitString(&data_, &value_, s);
}

JSONDocument::JSONDocument(const char* s) : JSONDocument(std::string(s)) {
}

void JSONDocument::InitFromValue(const fbson::FbsonValue* val) {
  data_.reset(new char[val->numPackedBytes()]);
  memcpy(data_.get(), val, val->numPackedBytes());
  value_ = reinterpret_cast<fbson::FbsonValue*>(data_.get());
}

//私有构造函数
JSONDocument::JSONDocument(fbson::FbsonValue* val, bool makeCopy) {
  if (makeCopy) {
    InitFromValue(val);
  } else {
    value_ = val;
  }
}

JSONDocument::JSONDocument(Type _type) {
//todo（icanadi）通过使用模板使所有这些都变得更好
  switch (_type) {
    case kNull:
      InitJSONDocument(&data_, &value_,
                       std::bind(&fbson::FbsonWriter::writeNull, _1));
      break;
    case kObject:
      CreateObject(&data_, &value_);
      break;
    case kBool:
      InitJSONDocument(&data_, &value_,
                       std::bind(&fbson::FbsonWriter::writeBool, _1, false));
      break;
    case kDouble:
      InitJSONDocument(&data_, &value_,
                       std::bind(&fbson::FbsonWriter::writeDouble, _1, 0.));
      break;
    case kArray:
      CreateArray(&data_, &value_);
      break;
    case kInt64:
      InitJSONDocument(&data_, &value_,
                       std::bind(&fbson::FbsonWriter::writeInt64, _1, 0));
      break;
    case kString:
      InitString(&data_, &value_, "");
      break;
    default:
      assert(false);
  }
}

JSONDocument::JSONDocument(const JSONDocument& jsonDocument) {
  if (jsonDocument.IsOwner()) {
    InitFromValue(jsonDocument.value_);
  } else {
    value_ = jsonDocument.value_;
  }
}

JSONDocument::JSONDocument(JSONDocument&& jsonDocument) {
  value_ = jsonDocument.value_;
  data_.swap(jsonDocument.data_);
}

JSONDocument& JSONDocument::operator=(JSONDocument jsonDocument) {
  value_ = jsonDocument.value_;
  data_.swap(jsonDocument.data_);
  return *this;
}

JSONDocument::Type JSONDocument::type() const {
  switch (value_->type()) {
    case fbson::FbsonType::T_Null:
      return JSONDocument::kNull;

    case fbson::FbsonType::T_True:
    case fbson::FbsonType::T_False:
      return JSONDocument::kBool;

    case fbson::FbsonType::T_Int8:
    case fbson::FbsonType::T_Int16:
    case fbson::FbsonType::T_Int32:
    case fbson::FbsonType::T_Int64:
      return JSONDocument::kInt64;

    case fbson::FbsonType::T_Double:
      return JSONDocument::kDouble;

    case fbson::FbsonType::T_String:
      return JSONDocument::kString;

    case fbson::FbsonType::T_Object:
      return JSONDocument::kObject;

    case fbson::FbsonType::T_Array:
      return JSONDocument::kArray;

    case fbson::FbsonType::T_Binary:
    default:
      assert(false);
  }
  return JSONDocument::kNull;
}

bool JSONDocument::Contains(const std::string& key) const {
  assert(IsObject());
  auto objectVal = reinterpret_cast<fbson::ObjectVal*>(value_);
  return objectVal->find(key.c_str()) != nullptr;
}

JSONDocument JSONDocument::operator[](const std::string& key) const {
  assert(IsObject());
  auto objectVal = reinterpret_cast<fbson::ObjectVal*>(value_);
  auto foundValue = objectVal->find(key.c_str());
  assert(foundValue != nullptr);
//不需要保存常量对象中的路径
  JSONDocument ans(foundValue, false);
  return ans;
}

size_t JSONDocument::Count() const {
  assert(IsObject() || IsArray());
  if (IsObject()) {
//托多（斯塔什）：添加到fbson？
    const fbson::ObjectVal& objectVal =
          *reinterpret_cast<fbson::ObjectVal*>(value_);
    return ObjectNumElem(objectVal);
  } else if (IsArray()) {
    auto arrayVal = reinterpret_cast<fbson::ArrayVal*>(value_);
    return arrayVal->numElem();
  }
  assert(false);
  return 0;
}

JSONDocument JSONDocument::operator[](size_t i) const {
  assert(IsArray());
  auto arrayVal = reinterpret_cast<fbson::ArrayVal*>(value_);
  auto foundValue = arrayVal->get(static_cast<int>(i));
  JSONDocument ans(foundValue, false);
  return ans;
}

bool JSONDocument::IsNull() const {
  return value_->isNull();
}

bool JSONDocument::IsArray() const {
  return value_->isArray();
}

bool JSONDocument::IsBool() const {
  return value_->isTrue() || value_->isFalse();
}

bool JSONDocument::IsDouble() const {
  return value_->isDouble();
}

bool JSONDocument::IsInt64() const {
  return value_->isInt8() || value_->isInt16() ||
         value_->isInt32() || value_->isInt64();
}

bool JSONDocument::IsObject() const {
  return value_->isObject();
}

bool JSONDocument::IsString() const {
  return value_->isString();
}

bool JSONDocument::GetBool() const {
  assert(IsBool());
  return value_->isTrue();
}

double JSONDocument::GetDouble() const {
  assert(IsDouble());
  return ((fbson::DoubleVal*)value_)->val();
}

int64_t JSONDocument::GetInt64() const {
  assert(IsInt64());
  return GetInt64ValFromFbsonNumericType(value_);
}

std::string JSONDocument::GetString() const {
  assert(IsString());
  fbson::StringVal* stringVal = (fbson::StringVal*)value_;
  return std::string(stringVal->getBlob(), stringVal->getBlobLen());
}

namespace {

//fbsonValue可以是int8、int16、int32、int64
bool CompareNumeric(fbson::FbsonValue* left, fbson::FbsonValue* right) {
  assert(IsNumeric(left) && IsNumeric(right));
  return GetInt64ValFromFbsonNumericType(left) ==
         GetInt64ValFromFbsonNumericType(right);
}

bool CompareSimpleTypes(fbson::FbsonValue* left, fbson::FbsonValue* right) {
  if (IsNumeric(left)) {
    return CompareNumeric(left, right);
  }
  if (left->numPackedBytes() != right->numPackedBytes()) {
    return false;
  }
  return memcmp(left, right, left->numPackedBytes()) == 0;
}

bool CompareFbsonValue(fbson::FbsonValue* left, fbson::FbsonValue* right) {
  if (!IsComparable(left, right)) {
    return false;
  }

  switch (left->type()) {
    case fbson::FbsonType::T_True:
    case fbson::FbsonType::T_False:
    case fbson::FbsonType::T_Null:
      return true;
    case fbson::FbsonType::T_Int8:
    case fbson::FbsonType::T_Int16:
    case fbson::FbsonType::T_Int32:
    case fbson::FbsonType::T_Int64:
      return CompareNumeric(left, right);
    case fbson::FbsonType::T_String:
    case fbson::FbsonType::T_Double:
      return CompareSimpleTypes(left, right);
    case fbson::FbsonType::T_Object:
    {
      auto leftObject = reinterpret_cast<fbson::ObjectVal*>(left);
      auto rightObject = reinterpret_cast<fbson::ObjectVal*>(right);
      if (ObjectNumElem(*leftObject) != ObjectNumElem(*rightObject)) {
        return false;
      }
      for (auto && keyValue : *leftObject) {
        std::string str(keyValue.getKeyStr(), keyValue.klen());
        if (rightObject->find(str.c_str()) == nullptr) {
          return false;
        }
        if (!CompareFbsonValue(keyValue.value(),
                               rightObject->find(str.c_str()))) {
          return false;
        }
      }
      return true;
    }
    case fbson::FbsonType::T_Array:
    {
      auto leftArr = reinterpret_cast<fbson::ArrayVal*>(left);
      auto rightArr = reinterpret_cast<fbson::ArrayVal*>(right);
      if (leftArr->numElem() != rightArr->numElem()) {
        return false;
      }
      for (int i = 0; i < static_cast<int>(leftArr->numElem()); ++i) {
        if (!CompareFbsonValue(leftArr->get(i), rightArr->get(i))) {
          return false;
        }
      }
      return true;
    }
    default:
      assert(false);
  }
  return false;
}

}  //命名空间

bool JSONDocument::operator==(const JSONDocument& rhs) const {
  return CompareFbsonValue(value_, rhs.value_);
}

bool JSONDocument::operator!=(const JSONDocument& rhs) const {
  return !(*this == rhs);
}

JSONDocument JSONDocument::Copy() const {
  return JSONDocument(value_, true);
}

bool JSONDocument::IsOwner() const {
  return data_.get() != nullptr;
}

std::string JSONDocument::DebugString() const {
  fbson::FbsonToJson fbsonToJson;
  return fbsonToJson.json(value_);
}

JSONDocument::ItemsIteratorGenerator JSONDocument::Items() const {
  assert(IsObject());
  return ItemsIteratorGenerator(*(reinterpret_cast<fbson::ObjectVal*>(value_)));
}

//todo（icanadi）（perf）使用竞技场分配对象
JSONDocument* JSONDocument::ParseJSON(const char* json) {
  fbson::FbsonJsonParser parser;
  if (!parser.parse(json)) {
    return nullptr;
  }

  auto fbsonVal = fbson::FbsonDocument::createValue(
                    parser.getWriter().getOutput()->getBuffer(),
              static_cast<uint32_t>(parser.getWriter().getOutput()->getSize()));

  if (fbsonVal == nullptr) {
    return nullptr;
  }

  return new JSONDocument(fbsonVal, true);
}

void JSONDocument::Serialize(std::string* dst) const {
//头保留第一个字节
//目前，header只是版本号。这将帮助我们提供
//向后兼容。如果
//必要的
  dst->push_back(kSerializationFormatVersion);
  dst->push_back(FBSON_VER);
  dst->append(reinterpret_cast<char*>(value_), value_->numPackedBytes());
}

const char JSONDocument::kSerializationFormatVersion = 2;

JSONDocument* JSONDocument::Deserialize(const Slice& src) {
  Slice input(src);
  if (src.size() == 0) {
    return nullptr;
  }
  char header = input[0];
  if (header == 1) {
    assert(false);
  }
  input.remove_prefix(1);
  auto value = fbson::FbsonDocument::createValue(input.data(),
                static_cast<uint32_t>(input.size()));
  if (value == nullptr) {
    return nullptr;
  }

  return new JSONDocument(value, true);
}

class JSONDocument::const_item_iterator::Impl {
 public:
  typedef fbson::ObjectVal::const_iterator It;

  explicit Impl(It it) : it_(it) {}

  const char* getKeyStr() const {
    return it_->getKeyStr();
  }

  uint8_t klen() const {
    return it_->klen();
  }

  It& operator++() {
    return ++it_;
  }

  bool operator!=(const Impl& other) {
    return it_ != other.it_;
  }

  fbson::FbsonValue* value() const {
    return it_->value();
  }

 private:
  It it_;
};

JSONDocument::const_item_iterator::const_item_iterator(Impl* impl)
: it_(impl) {}

JSONDocument::const_item_iterator::const_item_iterator(const_item_iterator&& a)
: it_(std::move(a.it_)) {}

JSONDocument::const_item_iterator&
  JSONDocument::const_item_iterator::operator++() {
  ++(*it_);
  return *this;
}

bool JSONDocument::const_item_iterator::operator!=(
                                  const const_item_iterator& other) {
  return *it_ != *(other.it_);
}

JSONDocument::const_item_iterator::~const_item_iterator() {
}

JSONDocument::const_item_iterator::value_type
  JSONDocument::const_item_iterator::operator*() {
  return JSONDocument::const_item_iterator::value_type(std::string(it_->getKeyStr(), it_->klen()),
    JSONDocument(it_->value(), false));
}

JSONDocument::ItemsIteratorGenerator::ItemsIteratorGenerator(
                                      const fbson::ObjectVal& object)
  : object_(object) {}

JSONDocument::const_item_iterator
      JSONDocument::ItemsIteratorGenerator::begin() const {
  return const_item_iterator(new const_item_iterator::Impl(object_.begin()));
}

JSONDocument::const_item_iterator
      JSONDocument::ItemsIteratorGenerator::end() const {
  return const_item_iterator(new const_item_iterator::Impl(object_.end()));
}

}  //命名空间rocksdb
#endif  //摇滚乐
