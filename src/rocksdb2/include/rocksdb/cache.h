
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
//
//缓存是一个将键映射到值的接口。它有内部的
//同步，可以从
//多线程。它可以自动收回条目以腾出空间
//对于新条目。值对缓存具有指定的费用
//容量。例如，值为变量的缓存
//长度字符串，可以使用字符串的长度作为
//字符串。
//
//具有最近使用最少的逐出的内置缓存实现
//提供了策略。如果
//他们想要更复杂的东西（比如扫描电阻，
//自定义逐出策略、可变缓存大小等）

#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include "rocksdb/slice.h"
#include "rocksdb/statistics.h"
#include "rocksdb/status.h"

namespace rocksdb {

class Cache;

//创建具有固定大小容量的新缓存。缓存被切分了
//到2^num-shard-bits-shards，通过键的散列。总容量
//被分割并均匀分配给每个碎片。如果严格的容量限制
//如果设置了，则当缓存已满时，插入到缓存将失败。用户也可以
//通过设置高优先级条目的缓存保留百分比
//高优先级池百分比。
//num_shard_bits=-1表示自动确定：每个碎片
//将至少为512KB，并且碎片位的数量将不超过6。
extern std::shared_ptr<Cache> NewLRUCache(size_t capacity,
                                          int num_shard_bits = -1,
                                          bool strict_capacity_limit = false,
                                          double high_pri_pool_ratio = 0.0);

//类似于newlrucache，但使用
//在某些情况下，更好的并发性能。有关
//更详细。
//
//如果不支持，则返回nullptr。
extern std::shared_ptr<Cache> NewClockCache(size_t capacity,
                                            int num_shard_bits = -1,
                                            bool strict_capacity_limit = false);

class Cache {
 public:
//根据实现的不同，具有高优先级的缓存条目可能会更少
//可能会比低优先级条目被逐出。
  enum class Priority { HIGH, LOW };

  Cache() {}

//通过调用“deleter”销毁所有现有条目
//通过insert（）函数传递的函数。
//
//参见插入
  virtual ~Cache() {}

//存储在缓存中的条目的不透明句柄。
  struct Handle {};

//缓存的类型
  virtual const char* Name() const = 0;

//将key->value的映射插入缓存并分配给它
//对总缓存容量的指定费用。
//如果严格的“容量限制”为真，并且缓存达到其最大容量，
//返回状态：：未完成。
//
//如果handle不是nullptr，则返回与
//映射。调用方必须在返回
//不再需要映射。如果出现错误，呼叫方应负责
//清除该值（即调用“deleter”）。
//
//如果handle为nullptr，就好像在
//插入。如果出现错误，值将被清除。
//
//当不再需要插入的条目时，键和
//值将传递给“deleter”。
  virtual Status Insert(const Slice& key, void* value, size_t charge,
                        void (*deleter)(const Slice& key, void* value),
                        Handle** handle = nullptr,
                        Priority priority = Priority::LOW) = 0;

//如果缓存没有“key”的映射，则返回nullptr。
//
//否则返回对应于映射的句柄。呼叫者
//当返回的映射为no时，必须调用此->release（handle）
//需要更长的时间。
//如果stats不是nullptr，则可以在
//功能。
  virtual Handle* Lookup(const Slice& key, Statistics* stats = nullptr) = 0;

//如果句柄引用
//高速缓存。如果refcount递增，则返回true；否则返回
//错误的。
//要求：句柄必须已由*this上的方法返回。
  virtual bool Ref(Handle* handle) = 0;

  /*
   *释放先前查找（）返回的映射。发布的条目可能
   *仍保留在缓存中，以防稍后被其他人查找。如果
   *设置了强制擦除，如果没有，也会将其从缓存中删除。
   *其他参考。删除它应该调用
   *是在
   *已插入条目。
   *
   *如果条目也被删除，则返回“真”。
   **/

//要求：句柄必须尚未释放。
//要求：句柄必须已由*this上的方法返回。
  virtual bool Release(Handle* handle, bool force_erase = false) = 0;

//返回由返回的句柄中封装的值
//查找成功（）。
//要求：句柄必须尚未释放。
//要求：句柄必须已由*this上的方法返回。
  virtual void* Value(Handle* handle) = 0;

//如果缓存包含键的条目，请将其清除。请注意
//基础条目将一直保留到所有现有句柄
//它已经被释放了。
  virtual void Erase(const Slice& key) = 0;
//返回一个新的数字ID。可以由多个
//共享同一缓存以划分密钥空间。通常情况下
//客户端将在启动时分配一个新的ID，并将该ID预先发送到
//它的缓存密钥。
  virtual uint64_t NewId() = 0;

//设置缓存的最大配置容量。当新
//容量小于旧容量，现有使用量为
//比新能力更大的是，实施将尽其最大努力
//清除缓存中释放的条目，以降低使用率
  virtual void SetCapacity(size_t capacity) = 0;

//设置缓存满时是否返回插入错误
//容量。
  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) = 0;

//获取当缓存到达其
//满负荷。
  virtual bool HasStrictCapacityLimit() const = 0;

//返回缓存的最大配置容量
  virtual size_t GetCapacity() const = 0;

//返回驻留在缓存中的条目的内存大小。
  virtual size_t GetUsage() const = 0;

//返回缓存中特定项的内存大小。
  virtual size_t GetUsage(Handle* handle) const = 0;

//返回系统正在使用的项的内存大小
  virtual size_t GetPinnedUsage() const = 0;

//如果你想加快速度，可以在关机时调用它。缓存将被拒绝
//任何基础数据，删除时不会释放。这个电话会泄露的
//内存-只有在关闭进程时才调用此函数。
//此调用之后，任何使用缓存的尝试都将失败。
//调用此方法之前，始终删除db对象！
  virtual void DisownData(){
//默认实现为noop
  };

//将回调应用于缓存中的所有条目
//如果线程安全为真，它也将锁定访问。否则，它会
//访问缓存而不保留锁
  virtual void ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                      bool thread_safe) = 0;

//删除所有条目。
//前提条件：没有引用任何条目。
  virtual void EraseUnRefEntries() = 0;

  virtual std::string GetPrintableOptions() const { return ""; }

//将最后插入的对象标记为原始数据块。这将被使用
//在测试中。默认实现什么也不做。
  virtual void TEST_mark_as_data_block(const Slice& key, size_t charge) {}

 private:
//不允许复制
  Cache(const Cache&);
  Cache& operator=(const Cache&);
};

}  //命名空间rocksdb
