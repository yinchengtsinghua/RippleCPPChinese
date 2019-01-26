
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

#include <assert.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//仅从db_stress.cc设置，仅用于测试。
//如果非零，则在源代码中的各个点上以概率1/this终止
extern int rocksdb_kill_odds;
//如果杀戮点在此列表中有前缀，将跳过杀戮。
extern std::vector<std::string> rocksdb_kill_prefix_blacklist;

#ifdef NDEBUG
//发布版本中为空
#define TEST_KILL_RANDOM(kill_point, rocksdb_kill_odds)
#else

namespace rocksdb {
//以1/1的概率终止测试过程。
extern void TestKillRandom(std::string kill_point, int odds,
                           const std::string& srcfile, int srcline);

//为了避免在某些经常执行的代码路径（在
//杀死随机测试），使用这个因素来降低几率
#define REDUCE_ODDS 2
#define REDUCE_ODDS2 4

#define TEST_KILL_RANDOM(kill_point, rocksdb_kill_odds)                  \
  {                                                                      \
    if (rocksdb_kill_odds > 0) {                                         \
      TestKillRandom(kill_point, rocksdb_kill_odds, __FILE__, __LINE__); \
    }                                                                    \
  }
}  //命名空间rocksdb
#endif

#ifdef NDEBUG
#define TEST_SYNC_POINT(x)
#define TEST_SYNC_POINT_CALLBACK(x, y)
#else

namespace rocksdb {

//此类提供了确定地重现竞争条件的功能
//在单元测试中。
//开发者可以通过测试同步点在代码库中指定同步点。
//每个同步点表示线程执行流中的一个位置。
//在单元测试中，同步点之间的“发生在”关系可以是
//通过syncpoint:：loadDependency设置，以复制所需的交错
//线程执行。
//有关示例用例，请参阅（dbtest、transactionlogitorrace）。

class SyncPoint {
 public:
  static SyncPoint* GetInstance();

  struct SyncPointPair {
    std::string predecessor;
    std::string successor;
  };

//在测试开始时调用一次以设置
//同步点
  void LoadDependency(const std::vector<SyncPointPair>& dependencies);

//在测试开始时调用一次以设置
//同步点和设置标记指示仅启用后续项
//当它与前一个线程在同一线程上处理时。
//添加标记时，它隐式地为标记对添加依赖项。
  void LoadDependencyAndMarkers(const std::vector<SyncPointPair>& dependencies,
                                const std::vector<SyncPointPair>& markers);

//在同步点中设置回调函数。
  void SetCallBack(const std::string point,
                   std::function<void(void*)> callback);

//逐点清除回调函数
  void ClearCallBack(const std::string point);

//清除所有回调函数。
  void ClearAllCallBacks();

//启用同步点处理（启动时禁用）
  void EnableProcessing();

//禁用同步点处理
  void DisableProcessing();

//删除所有同步点的执行跟踪
  void ClearTrace();

//由测试同步点触发，阻止执行直到所有前置任务
//被执行。
//和/或调用已注册的回调函数n，参数为'cb_arg'
  void Process(const std::string& point, void* cb_arg = nullptr);

//TODO:提供一个在
//同步点被清除。

 private:
  bool PredecessorsAllCleared(const std::string& point);
  bool DisabledByMarker(const std::string& point, std::thread::id thread_id);

//从LoadDependency加载的后续任务/前置任务映射
  std::unordered_map<std::string, std::vector<std::string>> successors_;
  std::unordered_map<std::string, std::vector<std::string>> predecessors_;
  std::unordered_map<std::string, std::function<void(void*)> > callbacks_;
  std::unordered_map<std::string, std::vector<std::string> > markers_;
  std::unordered_map<std::string, std::thread::id> marked_thread_id_;

  std::mutex mutex_;
  std::condition_variable cv_;
//已通过的同步点
  std::unordered_set<std::string> cleared_points_;
  bool enabled_ = false;
  int num_callbacks_running_ = 0;
};

}  //命名空间rocksdb

//使用测试同步点指定代码库内的同步点。
//同步点可能发生在其他同步点上的depedence之后，
//在运行时通过SyncPoint:：LoadDependency配置。这可能是
//用于在线程之间重新生成竞争条件。
//有关示例用例，请参见db_test.cc中的TransactionLogitorrace。
//测试同步点在版本构建中不起作用。
#define TEST_SYNC_POINT(x) rocksdb::SyncPoint::GetInstance()->Process(x)
#define TEST_SYNC_POINT_CALLBACK(x, y) \
  rocksdb::SyncPoint::GetInstance()->Process(x, y)
#endif  //调试程序
