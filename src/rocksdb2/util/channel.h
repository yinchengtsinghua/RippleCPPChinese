
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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

#pragma once

namespace rocksdb {

template <class T>
class channel {
 public:
  explicit channel() : eof_(false) {}

  channel(const channel&) = delete;
  void operator=(const channel&) = delete;

  void sendEof() {
    std::lock_guard<std::mutex> lk(lock_);
    eof_ = true;
    cv_.notify_all();
  }

  bool eof() {
    std::lock_guard<std::mutex> lk(lock_);
    return buffer_.empty() && eof_;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lk(lock_);
    return buffer_.size();
  }

//将Elem写入队列
  void write(T&& elem) {
    std::unique_lock<std::mutex> lk(lock_);
    buffer_.emplace(std::forward<T>(elem));
    cv_.notify_one();
  }

///moves a dequeued elem on elem，blocking until a element
//可用。
//如果eof返回false
  bool read(T& elem) {
    std::unique_lock<std::mutex> lk(lock_);
    cv_.wait(lk, [&] { return eof_ || !buffer_.empty(); });
    if (eof_ && buffer_.empty()) {
      return false;
    }
    elem = std::move(buffer_.front());
    buffer_.pop();
    cv_.notify_one();
    return true;
  }

 private:
  std::condition_variable cv_;
  std::mutex lock_;
  std::queue<T> buffer_;
  bool eof_;
};
}  //命名空间rocksdb
