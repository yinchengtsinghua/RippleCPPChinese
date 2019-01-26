
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2013，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//

#if !defined(OS_WIN) && !defined(ROCKSDB_LITE)

#ifndef GFLAGS
#include <cstdio>
int main() { fprintf(stderr, "Please install gflags to run tools\n"); }
#else
#include <gflags/gflags.h>

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

#include "port/port_posix.h"
#include "rocksdb/env.h"
#include "util/mutexlock.h"
#include "utilities/persistent_cache/hash_table.h"

using std::string;

DEFINE_int32(nsec, 10, "nsec");
DEFINE_int32(nthread_write, 1, "insert %");
DEFINE_int32(nthread_read, 1, "lookup %");
DEFINE_int32(nthread_erase, 1, "erase %");

namespace rocksdb {

//
//HashTableImpl接口
//
//哈希表实现的抽象
template <class Key, class Value>
class HashTableImpl {
 public:
  virtual ~HashTableImpl() {}

  virtual bool Insert(const Key& key, const Value& val) = 0;
  virtual bool Erase(const Key& key) = 0;
  virtual bool Lookup(const Key& key, Value* val) = 0;
};

//哈希表基准
//
//用于测试给定哈希表实现的抽象。主要测试
//专注于插入、查找和擦除。测试可以在测试模式下运行，并且
//基准模式。
class HashTableBenchmark {
 public:
  explicit HashTableBenchmark(HashTableImpl<size_t, std::string>* impl,
                              const size_t sec = 10,
                              const size_t nthread_write = 1,
                              const size_t nthread_read = 1,
                              const size_t nthread_erase = 1)
      : impl_(impl),
        sec_(sec),
        ninserts_(0),
        nreads_(0),
        nerases_(0),
        nerases_failed_(0),
        quit_(false) {
    Prepop();

    StartThreads(nthread_write, WriteMain);
    StartThreads(nthread_read, ReadMain);
    StartThreads(nthread_erase, EraseMain);

    uint64_t start = NowInMillSec();
    while (!quit_) {
      quit_ = NowInMillSec() - start > sec_ * 1000;
      /*睡眠覆盖*/睡眠（1）；
    }

    env*env=env:：default（）；
    env->waitForJoin（）；

    如果（SECARI）{
      printf（“结果\n”）；
      printf（“=”，\n”）；
      printf（“insert/sec=%f\n”，ninserts_uu static_cast<double>（sec_u））；
      printf（“read/sec=%f\n”，nreads_u/static_cast<double>（sec_u））；
      printf（“erases/sec=%f\n”，n erases_u/static_cast<double>（sec_u））；
      const uint64_t ops=ninserts_u+nreads_u+nerases_u；
      printf（“ops/sec=%f\n”，ops/static_cast<double>（sec_u））；
      printf（“erase fail=%d（%f%%）\n”，static_cast<int>（nerases_failed_ux），静态_cast<int>，
             static_cast<float>（nerases_failed_ux/nerases_x 100））；
      printf（“=”，\n”）；
    }
  }

  void runwrite（）
    而（！）{ }）
      尺寸_t k=插入_键
      std：：字符串tmp（1000，k%255）；
      bool status=impl_u->insert（k，tmp）；
      断言（状态）；
      nSnsitS++；
    }
  }

  void runread（）
    random64 rgen（时间（nullptr））；
    而（！）{ }）
      STD：：String S；
      size_t k=rgen.next（）%max_preop_key；
      bool status=impl_u->lookup（k，&s）；
      断言（状态）；
      断言（s==std:：string（1000，k%255））；
      nRead s++；
    }
  }

  void runerase（）
    而（！）{ }）
      尺寸_t k=擦除_键
      bool status=impl_u->erase（k）；
      Nerases_failed_+=！现状；
      纳拉塞斯+；
    }
  }

 私人：
  //为给定函数启动线程
  void startthreads（常量大小，void（*fn）（void*）
    env*env=env:：default（）；
    对于（尺寸_t i=0；i<n；+i）
      env->startthread（fn，this）；
    }
  }

  //用1M键预映射哈希表
  空预（）
    对于（尺寸_t i=0；i<max_preop_key；++i）
      bool status=impl_u->insert（i，std:：string（1000，i%255））；
      断言（状态）；
    }

    擦除_key_uu=插入_key_uu=max_preop_key；

    对于（尺寸_t i=0；i<10*max_preop_key；++i）
      bool status=impl_->insert（insert_key_++，std:：string（1000，'x'））；
      断言（状态）；
    }
  }

  静态uint64_t nowinmillsec（）
    时间电视；
    GetTimeOfDay（&TV，/*TZ）*/nullptr);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
  }

//
//线程项的包装函数
//
  static void WriteMain(void* args) {
    reinterpret_cast<HashTableBenchmark*>(args)->RunWrite();
  }

  static void ReadMain(void* args) {
    reinterpret_cast<HashTableBenchmark*>(args)->RunRead();
  }

  static void EraseMain(void* args) {
    reinterpret_cast<HashTableBenchmark*>(args)->RunErase();
  }

HashTableImpl<size_t, std::string>* impl_;         //要测试的实现
const size_t sec_;                                 //测试时间
const size_t max_prepop_key = 1ULL * 1024 * 1024;  //马克斯准备钥匙
std::atomic<size_t> insert_key_;                   //上次插入的密钥
std::atomic<size_t> erase_key_;                    //擦除键
std::atomic<size_t> ninserts_;                     //插入件数量
std::atomic<size_t> nreads_;                       //读数
std::atomic<size_t> nerases_;                      //擦除次数
std::atomic<size_t> nerases_failed_;               //失败的擦除次数
bool quit_;  //线程应该退出吗？
};

//
//简单IMPL
//锁安全无序映射实现
class SimpleImpl : public HashTableImpl<size_t, string> {
 public:
  bool Insert(const size_t& key, const string& val) override {
    WriteLock _(&rwlock_);
    map_.insert(make_pair(key, val));
    return true;
  }

  bool Erase(const size_t& key) override {
    WriteLock _(&rwlock_);
    auto it = map_.find(key);
    if (it == map_.end()) {
      return false;
    }
    map_.erase(it);
    return true;
  }

  bool Lookup(const size_t& key, string* val) override {
    ReadLock _(&rwlock_);
    auto it = map_.find(key);
    if (it != map_.end()) {
      *val = it->second;
    }
    return it != map_.end();
  }

 private:
  port::RWMutex rwlock_;
  std::unordered_map<size_t, string> map_;
};

//
//颗粒锁模
//线程安全自定义rocksdb用粒度实现哈希表
//锁定
class GranularLockImpl : public HashTableImpl<size_t, string> {
 public:
  bool Insert(const size_t& key, const string& val) override {
    Node n(key, val);
    return impl_.Insert(n);
  }

  bool Erase(const size_t& key) override {
    Node n(key, string());
    return impl_.Erase(n, nullptr);
  }

  bool Lookup(const size_t& key, string* val) override {
    Node n(key, string());
    port::RWMutex* rlock;
    bool status = impl_.Find(n, &n, &rlock);
    if (status) {
      ReadUnlock _(rlock);
      *val = n.val_;
    }
    return status;
  }

 private:
  struct Node {
    explicit Node(const size_t key, const string& val) : key_(key), val_(val) {}

    size_t key_ = 0;
    string val_;
  };

  struct Hash {
    uint64_t operator()(const Node& node) {
      return std::hash<uint64_t>()(node.key_);
    }
  };

  struct Equal {
    bool operator()(const Node& lhs, const Node& rhs) {
      return lhs.key_ == rhs.key_;
    }
  };

  HashTable<Node, Hash, Equal> impl_;
};

}  //命名空间rocksdb

//
//主要的
//
int main(int argc, char** argv) {
  GFLAGS::SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
                          " [OPTIONS]...");
  GFLAGS::ParseCommandLineFlags(&argc, &argv, false);

//
//微基准无序图
//
  printf("Micro benchmarking std::unordered_map \n");
  {
    rocksdb::SimpleImpl impl;
    rocksdb::HashTableBenchmark _(&impl, FLAGS_nsec, FLAGS_nthread_write,
                                  FLAGS_nthread_read, FLAGS_nthread_erase);
  }
//
//微基准可扩展哈希表
//
  printf("Micro benchmarking scalable hash map \n");
  {
    rocksdb::GranularLockImpl impl;
    rocksdb::HashTableBenchmark _(&impl, FLAGS_nsec, FLAGS_nthread_write,
                                  FLAGS_nthread_read, FLAGS_nthread_erase);
  }

  return 0;
}
#endif  //γIFNDEF GFLAGS
#else
/*主（int/*argc*/，char*/*argv*/）返回0；
第二节
