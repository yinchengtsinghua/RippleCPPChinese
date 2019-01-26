
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
#pragma once

#ifdef FAILED
#undef FAILED
#endif

namespace rocksdb {

class LDBCommandExecuteResult {
public:
  enum State {
    EXEC_NOT_STARTED = 0, EXEC_SUCCEED = 1, EXEC_FAILED = 2,
  };

  LDBCommandExecuteResult() : state_(EXEC_NOT_STARTED), message_("") {}

  LDBCommandExecuteResult(State state, std::string& msg) :
    state_(state), message_(msg) {}

  std::string ToString() {
    std::string ret;
    switch (state_) {
    case EXEC_SUCCEED:
      break;
    case EXEC_FAILED:
      ret.append("Failed: ");
      break;
    case EXEC_NOT_STARTED:
      ret.append("Not started: ");
    }
    if (!message_.empty()) {
      ret.append(message_);
    }
    return ret;
  }

  void Reset() {
    state_ = EXEC_NOT_STARTED;
    message_ = "";
  }

  bool IsSucceed() {
    return state_ == EXEC_SUCCEED;
  }

  bool IsNotStarted() {
    return state_ == EXEC_NOT_STARTED;
  }

  bool IsFailed() {
    return state_ == EXEC_FAILED;
  }

  static LDBCommandExecuteResult Succeed(std::string msg) {
    return LDBCommandExecuteResult(EXEC_SUCCEED, msg);
  }

  static LDBCommandExecuteResult Failed(std::string msg) {
    return LDBCommandExecuteResult(EXEC_FAILED, msg);
  }

private:
  State state_;
  std::string message_;

  bool operator==(const LDBCommandExecuteResult&);
  bool operator!=(const LDBCommandExecuteResult&);
};

}
