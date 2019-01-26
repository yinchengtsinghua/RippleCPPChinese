
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
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "rocksdb/status.h"
#include <stdio.h>
#include <cstring>
#include "port/port.h"

namespace rocksdb {

const char* Status::CopyState(const char* state) {
  char* const result =
new char[std::strlen(state) + 1];  //+1表示空终止符
  std::strcpy(result, state);
  return result;
}

Status::Status(Code _code, SubCode _subcode, const Slice& msg, const Slice& msg2)
    : code_(_code), subcode_(_subcode) {
  assert(code_ != kOk);
  assert(subcode_ != kMaxSubCode);
  const size_t len1 = msg.size();
  const size_t len2 = msg2.size();
  const size_t size = len1 + (len2 ? (2 + len2) : 0);
char* const result = new char[size + 1];  //+1表示空终止符
  memcpy(result, msg.data(), len1);
  if (len2) {
    result[len1] = ':';
    result[len1 + 1] = ' ';
    memcpy(result + len1 + 2, msg2.data(), len2);
  }
result[size] = '\0';  //C样式字符串的空终止符
  state_ = result;
}

std::string Status::ToString() const {
  char tmp[30];
  const char* type;
  switch (code_) {
    case kOk:
      return "OK";
    case kNotFound:
      type = "NotFound: ";
      break;
    case kCorruption:
      type = "Corruption: ";
      break;
    case kNotSupported:
      type = "Not implemented: ";
      break;
    case kInvalidArgument:
      type = "Invalid argument: ";
      break;
    case kIOError:
      type = "IO error: ";
      break;
    case kMergeInProgress:
      type = "Merge in progress: ";
      break;
    case kIncomplete:
      type = "Result incomplete: ";
      break;
    case kShutdownInProgress:
      type = "Shutdown in progress: ";
      break;
    case kTimedOut:
      type = "Operation timed out: ";
      break;
    case kAborted:
      type = "Operation aborted: ";
      break;
    case kBusy:
      type = "Resource busy: ";
      break;
    case kExpired:
      type = "Operation expired: ";
      break;
    case kTryAgain:
      type = "Operation failed. Try again.: ";
      break;
    default:
      snprintf(tmp, sizeof(tmp), "Unknown code(%d): ",
               static_cast<int>(code()));
      type = tmp;
      break;
  }
  std::string result(type);
  if (subcode_ != kNone) {
    uint32_t index = static_cast<int32_t>(subcode_);
    assert(sizeof(msgs) > index);
    result.append(msgs[index]);
  }

  if (state_ != nullptr) {
    result.append(state_);
  }
  return result;
}

}  //命名空间rocksdb
