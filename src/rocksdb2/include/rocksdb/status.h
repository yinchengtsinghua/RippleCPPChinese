
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//状态封装操作的结果。这可能意味着成功，
//或者，它可能指示相关错误消息的错误。
//
//多个线程可以在没有
//外部同步，但如果任何线程可以调用
//非常量方法，访问相同状态的所有线程都必须使用
//外部同步。

#ifndef STORAGE_ROCKSDB_INCLUDE_STATUS_H_
#define STORAGE_ROCKSDB_INCLUDE_STATUS_H_

#include <string>
#include "rocksdb/slice.h"

namespace rocksdb {

class Status {
 public:
//创建成功状态。
  Status() : code_(kOk), subcode_(kNone), state_(nullptr) {}
  ~Status() { delete[] state_; }

//复制指定的状态。
  Status(const Status& s);
  Status& operator=(const Status& s);
  Status(Status&& s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
      noexcept
#endif
      ;
  Status& operator=(Status&& s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
      noexcept
#endif
      ;
  bool operator==(const Status& rhs) const;
  bool operator!=(const Status& rhs) const;

  enum Code {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5,
    kMergeInProgress = 6,
    kIncomplete = 7,
    kShutdownInProgress = 8,
    kTimedOut = 9,
    kAborted = 10,
    kBusy = 11,
    kExpired = 12,
    kTryAgain = 13
  };

  Code code() const { return code_; }

  enum SubCode {
    kNone = 0,
    kMutexTimeout = 1,
    kLockTimeout = 2,
    kLockLimit = 3,
    kNoSpace = 4,
    kDeadlock = 5,
    kStaleFile = 6,
    kMemoryLimit = 7,
    kMaxSubCode
  };

  SubCode subcode() const { return subcode_; }

//返回指示状态消息的C样式字符串
  const char* getState() const { return state_; }

//返回成功状态。
  static Status OK() { return Status(); }

//返回适当类型的错误状态。
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotFound, msg, msg2);
  }
//没有malloc，找不到的快速路径；
  static Status NotFound(SubCode msg = kNone) { return Status(kNotFound, msg); }

  static Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kCorruption, msg, msg2);
  }
  static Status Corruption(SubCode msg = kNone) {
    return Status(kCorruption, msg);
  }

  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotSupported, msg, msg2);
  }
  static Status NotSupported(SubCode msg = kNone) {
    return Status(kNotSupported, msg);
  }

  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kInvalidArgument, msg, msg2);
  }
  static Status InvalidArgument(SubCode msg = kNone) {
    return Status(kInvalidArgument, msg);
  }

  static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kIOError, msg, msg2);
  }
  static Status IOError(SubCode msg = kNone) { return Status(kIOError, msg); }

  static Status MergeInProgress(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kMergeInProgress, msg, msg2);
  }
  static Status MergeInProgress(SubCode msg = kNone) {
    return Status(kMergeInProgress, msg);
  }

  static Status Incomplete(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kIncomplete, msg, msg2);
  }
  static Status Incomplete(SubCode msg = kNone) {
    return Status(kIncomplete, msg);
  }

  static Status ShutdownInProgress(SubCode msg = kNone) {
    return Status(kShutdownInProgress, msg);
  }
  static Status ShutdownInProgress(const Slice& msg,
                                   const Slice& msg2 = Slice()) {
    return Status(kShutdownInProgress, msg, msg2);
  }
  static Status Aborted(SubCode msg = kNone) { return Status(kAborted, msg); }
  static Status Aborted(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kAborted, msg, msg2);
  }

  static Status Busy(SubCode msg = kNone) { return Status(kBusy, msg); }
  static Status Busy(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kBusy, msg, msg2);
  }

  static Status TimedOut(SubCode msg = kNone) { return Status(kTimedOut, msg); }
  static Status TimedOut(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kTimedOut, msg, msg2);
  }

  static Status Expired(SubCode msg = kNone) { return Status(kExpired, msg); }
  static Status Expired(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kExpired, msg, msg2);
  }

  static Status TryAgain(SubCode msg = kNone) { return Status(kTryAgain, msg); }
  static Status TryAgain(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kTryAgain, msg, msg2);
  }

  static Status NoSpace() { return Status(kIOError, kNoSpace); }
  static Status NoSpace(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kIOError, kNoSpace, msg, msg2);
  }

  static Status MemoryLimit() { return Status(kAborted, kMemoryLimit); }
  static Status MemoryLimit(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kAborted, kMemoryLimit, msg, msg2);
  }

//如果状态指示成功，则返回true。
  bool ok() const { return code() == kOk; }

//返回true如果状态指示NotFound错误。
  bool IsNotFound() const { return code() == kNotFound; }

//返回true如果状态指示损坏错误。
  bool IsCorruption() const { return code() == kCorruption; }

//返回true如果状态指示NotSupported错误。
  bool IsNotSupported() const { return code() == kNotSupported; }

//返回true如果状态指示无效参数错误。
  bool IsInvalidArgument() const { return code() == kInvalidArgument; }

//返回true如果状态指示IOError。
  bool IsIOError() const { return code() == kIOError; }

//返回true如果状态指示正在进行合并。
  bool IsMergeInProgress() const { return code() == kMergeInProgress; }

//返回true如果状态指示不完整
  bool IsIncomplete() const { return code() == kIncomplete; }

//返回true如果状态指示正在关闭
  bool IsShutdownInProgress() const { return code() == kShutdownInProgress; }

  bool IsTimedOut() const { return code() == kTimedOut; }

  bool IsAborted() const { return code() == kAborted; }

  bool IsLockLimit() const {
    return code() == kAborted && subcode() == kLockLimit;
  }

//返回true iff状态指示资源正忙且
//暂时无法获取。
  bool IsBusy() const { return code() == kBusy; }

  bool IsDeadlock() const { return code() == kBusy && subcode() == kDeadlock; }

//如果状态指示操作已过期，则返回true。
  bool IsExpired() const { return code() == kExpired; }

//如果状态指示TryGain错误，则返回true。
//这通常意味着操作失败，但如果
//重新尝试。
  bool IsTryAgain() const { return code() == kTryAgain; }

//返回true如果状态指示nospace错误
//这是由返回特定“空间不足”的I/O错误引起的。
//错误条件。严格的传感器，nospace错误是I/O错误
//使用特定的子代码，允许用户采取适当的操作
//如果需要
  bool IsNoSpace() const {
    return (code() == kIOError) && (subcode() == kNoSpace);
  }

//返回true如果状态指示内存限制错误。可能有
//限制某些操作中使用的内存（如大小）的情况
//以避免内存不足的异常。
  bool IsMemoryLimit() const {
    return (code() == kAborted) && (subcode() == kMemoryLimit);
  }

//返回适合打印的此状态的字符串表示形式。
//返回字符串“OK”以获得成功。
  std::string ToString() const;

 private:
//nullptr状态_uu（通常是OK的情况）表示消息
//是空的。
//格式如下：
//state_[0..3]=消息长度
//state_[4..]==消息
  Code code_;
  SubCode subcode_;
  const char* state_;

  static const char* msgs[static_cast<int>(kMaxSubCode)];

  explicit Status(Code _code, SubCode _subcode = kNone)
      : code_(_code), subcode_(_subcode), state_(nullptr) {}

  Status(Code _code, SubCode _subcode, const Slice& msg, const Slice& msg2);
  Status(Code _code, const Slice& msg, const Slice& msg2)
      : Status(_code, kNone, msg, msg2) {}

  static const char* CopyState(const char* s);
};

inline Status::Status(const Status& s) : code_(s.code_), subcode_(s.subcode_) {
  state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_);
}
inline Status& Status::operator=(const Status& s) {
//以下条件捕获两个别名（当此值为=&s时），
//通常情况下，S和*都可以。
  if (this != &s) {
    code_ = s.code_;
    subcode_ = s.subcode_;
    delete[] state_;
    state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_);
  }
  return *this;
}

inline Status::Status(Status&& s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
    noexcept
#endif
    : Status() {
  *this = std::move(s);
}

inline Status& Status::operator=(Status&& s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
    noexcept
#endif
{
  if (this != &s) {
    code_ = std::move(s.code_);
    s.code_ = kOk;
    subcode_ = std::move(s.subcode_);
    s.subcode_ = kNone;
    delete[] state_;
    state_ = nullptr;
    std::swap(state_, s.state_);
  }
  return *this;
}

inline bool Status::operator==(const Status& rhs) const {
  return (code_ == rhs.code_);
}

inline bool Status::operator!=(const Status& rhs) const {
  return !(*this == rhs);
}

}  //命名空间rocksdb

#endif  //储物架包括状态
