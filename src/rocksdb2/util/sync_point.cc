
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

#include "util/sync_point.h"
#include <functional>
#include <thread>
#include "port/port.h"
#include "util/random.h"

int rocksdb_kill_odds = 0;
std::vector<std::string> rocksdb_kill_prefix_blacklist;

#ifndef NDEBUG
namespace rocksdb {

void TestKillRandom(std::string kill_point, int odds,
                    const std::string& srcfile, int srcline) {
  for (auto& p : rocksdb_kill_prefix_blacklist) {
    if (kill_point.substr(0, p.length()) == p) {
      return;
    }
  }

  assert(odds > 0);
  if (odds % 7 == 0) {
//类随机使用乘数16807，即7^5。如果赔率是
//乘数为7时，生成的值可能有限。
    odds++;
  }
  auto* r = Random::GetTLSInstance();
  bool crash = r->OneIn(odds);
  if (crash) {
    port::Crash(srcfile, srcline);
  }
}

SyncPoint* SyncPoint::GetInstance() {
  static SyncPoint sync_point;
  return &sync_point;
}

void SyncPoint::LoadDependency(const std::vector<SyncPointPair>& dependencies) {
  std::unique_lock<std::mutex> lock(mutex_);
  successors_.clear();
  predecessors_.clear();
  cleared_points_.clear();
  for (const auto& dependency : dependencies) {
    successors_[dependency.predecessor].push_back(dependency.successor);
    predecessors_[dependency.successor].push_back(dependency.predecessor);
  }
  cv_.notify_all();
}

void SyncPoint::LoadDependencyAndMarkers(
    const std::vector<SyncPointPair>& dependencies,
    const std::vector<SyncPointPair>& markers) {
  std::unique_lock<std::mutex> lock(mutex_);
  successors_.clear();
  predecessors_.clear();
  cleared_points_.clear();
  markers_.clear();
  marked_thread_id_.clear();
  for (const auto& dependency : dependencies) {
    successors_[dependency.predecessor].push_back(dependency.successor);
    predecessors_[dependency.successor].push_back(dependency.predecessor);
  }
  for (const auto& marker : markers) {
    successors_[marker.predecessor].push_back(marker.successor);
    predecessors_[marker.successor].push_back(marker.predecessor);
    markers_[marker.predecessor].push_back(marker.successor);
  }
  cv_.notify_all();
}

bool SyncPoint::PredecessorsAllCleared(const std::string& point) {
  for (const auto& pred : predecessors_[point]) {
    if (cleared_points_.count(pred) == 0) {
      return false;
    }
  }
  return true;
}

void SyncPoint::SetCallBack(const std::string point,
                            std::function<void(void*)> callback) {
  std::unique_lock<std::mutex> lock(mutex_);
  callbacks_[point] = callback;
}

void SyncPoint::ClearCallBack(const std::string point) {
  std::unique_lock<std::mutex> lock(mutex_);
  while (num_callbacks_running_ > 0) {
    cv_.wait(lock);
  }
  callbacks_.erase(point);
}

void SyncPoint::ClearAllCallBacks() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (num_callbacks_running_ > 0) {
    cv_.wait(lock);
  }
  callbacks_.clear();
}

void SyncPoint::EnableProcessing() {
  std::unique_lock<std::mutex> lock(mutex_);
  enabled_ = true;
}

void SyncPoint::DisableProcessing() {
  std::unique_lock<std::mutex> lock(mutex_);
  enabled_ = false;
}

void SyncPoint::ClearTrace() {
  std::unique_lock<std::mutex> lock(mutex_);
  cleared_points_.clear();
}

bool SyncPoint::DisabledByMarker(const std::string& point,
                                 std::thread::id thread_id) {
  auto marked_point_iter = marked_thread_id_.find(point);
  return marked_point_iter != marked_thread_id_.end() &&
         thread_id != marked_point_iter->second;
}

void SyncPoint::Process(const std::string& point, void* cb_arg) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (!enabled_) {
    return;
  }

  auto thread_id = std::this_thread::get_id();

  auto marker_iter = markers_.find(point);
  if (marker_iter != markers_.end()) {
    for (auto marked_point : marker_iter->second) {
      marked_thread_id_.insert(std::make_pair(marked_point, thread_id));
    }
  }

  if (DisabledByMarker(point, thread_id)) {
    return;
  }

  while (!PredecessorsAllCleared(point)) {
    cv_.wait(lock);
    if (DisabledByMarker(point, thread_id)) {
      return;
    }
  }

  auto callback_pair = callbacks_.find(point);
  if (callback_pair != callbacks_.end()) {
    num_callbacks_running_++;
    mutex_.unlock();
    callback_pair->second(cb_arg);
    mutex_.lock();
    num_callbacks_running_--;
    cv_.notify_all();
  }

  cleared_points_.insert(point);
  cv_.notify_all();
}
}  //命名空间rocksdb
#endif  //调试程序
