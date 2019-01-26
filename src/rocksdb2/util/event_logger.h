
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

#include <memory>
#include <sstream>
#include <string>
#include <chrono>

#include "rocksdb/env.h"
#include "util/log_buffer.h"

namespace rocksdb {

class JSONWriter {
 public:
  JSONWriter() : state_(kExpectKey), first_element_(true), in_array_(false) {
    stream_ << "{";
  }

  void AddKey(const std::string& key) {
    assert(state_ == kExpectKey);
    if (!first_element_) {
      stream_ << ", ";
    }
    stream_ << "\"" << key << "\": ";
    state_ = kExpectValue;
    first_element_ = false;
  }

  void AddValue(const char* value) {
    assert(state_ == kExpectValue || state_ == kInArray);
    if (state_ == kInArray && !first_element_) {
      stream_ << ", ";
    }
    stream_ << "\"" << value << "\"";
    if (state_ != kInArray) {
      state_ = kExpectKey;
    }
    first_element_ = false;
  }

  template <typename T>
  void AddValue(const T& value) {
    assert(state_ == kExpectValue || state_ == kInArray);
    if (state_ == kInArray && !first_element_) {
      stream_ << ", ";
    }
    stream_ << value;
    if (state_ != kInArray) {
      state_ = kExpectKey;
    }
    first_element_ = false;
  }

  void StartArray() {
    assert(state_ == kExpectValue);
    state_ = kInArray;
    in_array_ = true;
    stream_ << "[";
    first_element_ = true;
  }

  void EndArray() {
    assert(state_ == kInArray);
    state_ = kExpectKey;
    in_array_ = false;
    stream_ << "]";
    first_element_ = false;
  }

  void StartObject() {
    assert(state_ == kExpectValue);
    state_ = kExpectKey;
    stream_ << "{";
    first_element_ = true;
  }

  void EndObject() {
    assert(state_ == kExpectKey);
    stream_ << "}";
    first_element_ = false;
  }

  void StartArrayedObject() {
    assert(state_ == kInArray && in_array_);
    state_ = kExpectValue;
    if (!first_element_) {
      stream_ << ", ";
    }
    StartObject();
  }

  void EndArrayedObject() {
    assert(in_array_);
    EndObject();
    state_ = kInArray;
  }

  std::string Get() const { return stream_.str(); }

  JSONWriter& operator<<(const char* val) {
    if (state_ == kExpectKey) {
      AddKey(val);
    } else {
      AddValue(val);
    }
    return *this;
  }

  JSONWriter& operator<<(const std::string& val) {
    return *this << val.c_str();
  }

  template <typename T>
  JSONWriter& operator<<(const T& val) {
    assert(state_ != kExpectKey);
    AddValue(val);
    return *this;
  }

 private:
  enum JSONWriterState {
    kExpectKey,
    kExpectValue,
    kInArray,
    kInArrayedObject,
  };
  JSONWriterState state_;
  bool first_element_;
  bool in_array_;
  std::ostringstream stream_;
};

class EventLoggerStream {
 public:
  template <typename T>
  EventLoggerStream& operator<<(const T& val) {
    MakeStream();
    *json_writer_ << val;
    return *this;
  }

  void StartArray() { json_writer_->StartArray(); }
  void EndArray() { json_writer_->EndArray(); }
  void StartObject() { json_writer_->StartObject(); }
  void EndObject() { json_writer_->EndObject(); }

  ~EventLoggerStream();

 private:
  void MakeStream() {
    if (!json_writer_) {
      json_writer_ = new JSONWriter();
      *this << "time_micros"
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch()).count();
    }
  }
  friend class EventLogger;
  explicit EventLoggerStream(Logger* logger);
  explicit EventLoggerStream(LogBuffer* log_buffer);
//只有一个是非空指针
  Logger* const logger_;
  LogBuffer* const log_buffer_;
//所有权
  JSONWriter* json_writer_;
};

//下面是将显示在日志中的输出示例：
//2015/01/15-14:13:25.788019 1105EF000事件日志“时间微秒”：
//142360005788015，“event”：“table_file_creation”，“file_number”：12，
//“文件大小”：1909699
class EventLogger {
 public:
  static const char* Prefix() {
    return "EVENT_LOG_v1";
  }

  explicit EventLogger(Logger* logger) : logger_(logger) {}
  EventLoggerStream Log() { return EventLoggerStream(logger_); }
  EventLoggerStream LogToBuffer(LogBuffer* log_buffer) {
    return EventLoggerStream(log_buffer);
  }
  void Log(const JSONWriter& jwriter);
  static void Log(Logger* logger, const JSONWriter& jwriter);
  static void LogToBuffer(LogBuffer* log_buffer, const JSONWriter& jwriter);

 private:
  Logger* logger_;
};

}  //命名空间rocksdb
