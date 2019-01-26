
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
 *此文件定义fbsonWriter（模板）和fbsonWriter。
 *
 *fbsonWriter是一个实现fbson序列化程序的模板类。
 *用户调用fbsonWritert对象的各种写入函数来写入值
 *直接到fbson压缩字节。值或键返回的所有写入函数
 *写入fbson的字节数，如果有错误，则为0。写一篇文章
 *对象、数组或字符串，在写入之前必须调用WriteStart[..]
 *值或键，结束后调用writeend[..]。
 *
 *默认情况下，fbsonWritert对象创建输出流缓冲区。
 *或者，也可以将任何输出流对象传递给编写器，如
 *只要流对象实现了std:：ostream的一些基本功能
 （如fbsonoutstream，见fbsonstream.h）。
 *
 *fbsonWriter专门使用fbsonOutstream类型的fbsonWriter（请参见
 *fbsonstream.h）。所以除非你想提供一个不同的输出流
 *类型，使用fbsonParser对象。
 *
 *@author tian xia<tianx@fb.com>
 **/


#ifndef FBSON_FBSONWRITER_H
#define FBSON_FBSONWRITER_H

#include <stack>
#include "FbsonDocument.h"
#include "FbsonStream.h"

namespace fbson {

template <class OS_TYPE>
class FbsonWriterT {
 public:
  FbsonWriterT()
      : alloc_(true), hasHdr_(false), kvState_(WS_Value), str_pos_(0) {
    os_ = new OS_TYPE();
  }

  explicit FbsonWriterT(OS_TYPE& os)
      : os_(&os),
        alloc_(false),
        hasHdr_(false),
        kvState_(WS_Value),
        str_pos_(0) {}

  ~FbsonWriterT() {
    if (alloc_) {
      delete os_;
    }
  }

  void reset() {
    os_->clear();
    os_->seekp(0);
    hasHdr_ = false;
    kvState_ = WS_Value;
    for (; !stack_.empty(); stack_.pop())
      ;
  }

//写入密钥字符串（如果提供了外部dict，则为密钥ID）
  uint32_t writeKey(const char* key,
                    uint8_t len,
                    hDictInsert handler = nullptr) {
    if (len && !stack_.empty() && verifyKeyState()) {
      int key_id = -1;
      if (handler) {
        key_id = handler(key, len);
      }

      uint32_t size = sizeof(uint8_t);
      if (key_id < 0) {
        os_->put(len);
        os_->write(key, len);
        size += len;
      } else if (key_id <= FbsonKeyValue::sMaxKeyId) {
        FbsonKeyValue::keyid_type idx = key_id;
        os_->put(0);
        os_->write((char*)&idx, sizeof(FbsonKeyValue::keyid_type));
        size += sizeof(FbsonKeyValue::keyid_type);
} else { //密钥ID溢出
        assert(0);
        return 0;
      }

      kvState_ = WS_Key;
      return size;
    }

    return 0;
  }

//写密钥ID
  uint32_t writeKey(FbsonKeyValue::keyid_type idx) {
    if (!stack_.empty() && verifyKeyState()) {
      os_->put(0);
      os_->write((char*)&idx, sizeof(FbsonKeyValue::keyid_type));
      kvState_ = WS_Key;
      return sizeof(uint8_t) + sizeof(FbsonKeyValue::keyid_type);
    }

    return 0;
  }

  uint32_t writeNull() {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Null);
      kvState_ = WS_Value;
      return sizeof(FbsonValue);
    }

    return 0;
  }

  uint32_t writeBool(bool b) {
    if (!stack_.empty() && verifyValueState()) {
      if (b) {
        os_->put((FbsonTypeUnder)FbsonType::T_True);
      } else {
        os_->put((FbsonTypeUnder)FbsonType::T_False);
      }

      kvState_ = WS_Value;
      return sizeof(FbsonValue);
    }

    return 0;
  }

  uint32_t writeInt8(int8_t v) {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Int8);
      os_->put(v);
      kvState_ = WS_Value;
      return sizeof(Int8Val);
    }

    return 0;
  }

  uint32_t writeInt16(int16_t v) {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Int16);
      os_->write((char*)&v, sizeof(int16_t));
      kvState_ = WS_Value;
      return sizeof(Int16Val);
    }

    return 0;
  }

  uint32_t writeInt32(int32_t v) {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Int32);
      os_->write((char*)&v, sizeof(int32_t));
      kvState_ = WS_Value;
      return sizeof(Int32Val);
    }

    return 0;
  }

  uint32_t writeInt64(int64_t v) {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Int64);
      os_->write((char*)&v, sizeof(int64_t));
      kvState_ = WS_Value;
      return sizeof(Int64Val);
    }

    return 0;
  }

  uint32_t writeDouble(double v) {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Double);
      os_->write((char*)&v, sizeof(double));
      kvState_ = WS_Value;
      return sizeof(DoubleVal);
    }

    return 0;
  }

//在写入字符串val之前必须调用WriteStartString
  bool writeStartString() {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_String);
      str_pos_ = os_->tellp();

//现在用0填充大小字节
      uint32_t size = 0;
      os_->write((char*)&size, sizeof(uint32_t));

      kvState_ = WS_String;
      return true;
    }

    return false;
  }

//完成字符串val的写入
  bool writeEndString() {
    if (kvState_ == WS_String) {
      std::streampos cur_pos = os_->tellp();
      int32_t size = (int32_t)(cur_pos - str_pos_ - sizeof(uint32_t));
      assert(size >= 0);

      os_->seekp(str_pos_);
      os_->write((char*)&size, sizeof(uint32_t));
      os_->seekp(cur_pos);

      kvState_ = WS_Value;
      return true;
    }

    return false;
  }

  uint32_t writeString(const char* str, uint32_t len) {
    if (kvState_ == WS_String) {
      os_->write(str, len);
      return len;
    }

    return 0;
  }

  uint32_t writeString(char ch) {
    if (kvState_ == WS_String) {
      os_->put(ch);
      return 1;
    }

    return 0;
  }

//在写入二进制值之前必须调用WriteStartBinary
  bool writeStartBinary() {
    if (!stack_.empty() && verifyValueState()) {
      os_->put((FbsonTypeUnder)FbsonType::T_Binary);
      str_pos_ = os_->tellp();

//现在用0填充大小字节
      uint32_t size = 0;
      os_->write((char*)&size, sizeof(uint32_t));

      kvState_ = WS_Binary;
      return true;
    }

    return false;
  }

//完成二进制VAL的写入
  bool writeEndBinary() {
    if (kvState_ == WS_Binary) {
      std::streampos cur_pos = os_->tellp();
      int32_t size = (int32_t)(cur_pos - str_pos_ - sizeof(uint32_t));
      assert(size >= 0);

      os_->seekp(str_pos_);
      os_->write((char*)&size, sizeof(uint32_t));
      os_->seekp(cur_pos);

      kvState_ = WS_Value;
      return true;
    }

    return false;
  }

  uint32_t writeBinary(const char* bin, uint32_t len) {
    if (kvState_ == WS_Binary) {
      os_->write(bin, len);
      return len;
    }

    return 0;
  }

//在写入对象val之前必须调用WriteStartObject
  bool writeStartObject() {
    if (stack_.empty() || verifyValueState()) {
      if (stack_.empty()) {
//如果这是一个新的fbson，则写入头
        if (!hasHdr_) {
          writeHeader();
        } else
          return false;
      }

      os_->put((FbsonTypeUnder)FbsonType::T_Object);
//保存大小位置
      stack_.push(WriteInfo({WS_Object, os_->tellp()}));

//现在用0填充大小字节
      uint32_t size = 0;
      os_->write((char*)&size, sizeof(uint32_t));

      kvState_ = WS_Value;
      return true;
    }

    return false;
  }

//完成对象val的写入
  bool writeEndObject() {
    if (!stack_.empty() && stack_.top().state == WS_Object &&
        kvState_ == WS_Value) {
      WriteInfo& ci = stack_.top();
      std::streampos cur_pos = os_->tellp();
      int32_t size = (int32_t)(cur_pos - ci.sz_pos - sizeof(uint32_t));
      assert(size >= 0);

      os_->seekp(ci.sz_pos);
      os_->write((char*)&size, sizeof(uint32_t));
      os_->seekp(cur_pos);
      stack_.pop();

      return true;
    }

    return false;
  }

//在写入数组val之前必须调用WriteStartArray
  bool writeStartArray() {
    if (stack_.empty() || verifyValueState()) {
      if (stack_.empty()) {
//如果这是一个新的fbson，则写入头
        if (!hasHdr_) {
          writeHeader();
        } else
          return false;
      }

      os_->put((FbsonTypeUnder)FbsonType::T_Array);
//保存大小位置
      stack_.push(WriteInfo({WS_Array, os_->tellp()}));

//现在用0填充大小字节
      uint32_t size = 0;
      os_->write((char*)&size, sizeof(uint32_t));

      kvState_ = WS_Value;
      return true;
    }

    return false;
  }

//完成数组val的写入
  bool writeEndArray() {
    if (!stack_.empty() && stack_.top().state == WS_Array &&
        kvState_ == WS_Value) {
      WriteInfo& ci = stack_.top();
      std::streampos cur_pos = os_->tellp();
      int32_t size = (int32_t)(cur_pos - ci.sz_pos - sizeof(uint32_t));
      assert(size >= 0);

      os_->seekp(ci.sz_pos);
      os_->write((char*)&size, sizeof(uint32_t));
      os_->seekp(cur_pos);
      stack_.pop();

      return true;
    }

    return false;
  }

  OS_TYPE* getOutput() { return os_; }

 private:
//在写入值之前验证我们是否处于正确的状态
  bool verifyValueState() {
    assert(!stack_.empty());
    return (stack_.top().state == WS_Object && kvState_ == WS_Key) ||
           (stack_.top().state == WS_Array && kvState_ == WS_Value);
  }

//在写入密钥之前验证我们的状态是否正确
  bool verifyKeyState() {
    assert(!stack_.empty());
    return stack_.top().state == WS_Object && kvState_ == WS_Value;
  }

  void writeHeader() {
    os_->put(FBSON_VER);
    hasHdr_ = true;
  }

 private:
  enum WriteState {
    WS_NONE,
    WS_Array,
    WS_Object,
    WS_Key,
    WS_Value,
    WS_String,
    WS_Binary,
  };

  struct WriteInfo {
    WriteState state;
    std::streampos sz_pos;
  };

 private:
  OS_TYPE* os_;
  bool alloc_;
  bool hasHdr_;
WriteState kvState_; //键或值状态
  std::streampos str_pos_;
  std::stack<WriteInfo> stack_;
};

typedef FbsonWriterT<FbsonOutStream> FbsonWriter;

} //命名空间FBSON

#endif //FBSON作家
