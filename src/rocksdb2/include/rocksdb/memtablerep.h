
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
//此文件包含必须由任何集合实现的接口
//用作memtable的备份存储。这样的收藏必须
//满足以下属性：
//（1）不存储重复项。
//（2）它使用memTableRep:：keyComparator比较迭代和
//平等。
//（3）可由多个读卡器同时访问，支持
//阅读期间。但是，它不需要支持多个并发写入。
//（4）从不删除项目。
//鼓励自由使用断言来执行（1）。
//
//当新的memtablerep
//请求。
//
//用户可以实现自己的memtable表示。我们包括三个
//内置类型：
//-skiplitrep:这是默认值，由跳过列表支持。
//-hashskiplitrep：最适合用于
//结构类似于“前缀：后缀”，其中前缀内的迭代是
//不同前缀之间的通用和迭代很少。它是由
//一个哈希图，其中每个bucket都是一个跳过列表。
//-vectorep：这由无序的std：：vector支持。在迭代中，
//向量已排序。排序是智能的；一旦markreadonly（）。
//如果调用了，则只对向量排序一次。它是为
//随机写入繁重的工作负载。
//
//最后四个实现是为以下情况而设计的
//整个集合的迭代很少，因为这样做需要
//要复制到已排序数据结构中的键。

#pragma once

#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>

namespace rocksdb {

class Arena;
class Allocator;
class LookupKey;
class Slice;
class SliceTransform;
class Logger;

typedef void* KeyHandle;

class MemTableRep {
 public:
//KeyComparator提供了一种比较键的方法，这些键是内部键。
//与值连接。
  class KeyComparator {
   public:
//比较a和b。如果a小于b，则返回负值；如果a小于b，则返回0
//等于，如果a大于b，则为正值
    virtual int operator()(const char* prefix_len_key1,
                           const char* prefix_len_key2) const = 0;

    virtual int operator()(const char* prefix_len_key,
                           const Slice& key) const = 0;

    virtual ~KeyComparator() { }
  };

  explicit MemTableRep(Allocator* allocator) : allocator_(allocator) {}

//为存储密钥分配一个长度为的buf。我的想法是
//特定的memtable表示知道其基础数据结构
//更好。通过允许它分配内存，它可以
//在连续的内存区域中相关的东西，使处理器
//预取更有效。
  virtual KeyHandle Allocate(const size_t len, char** buf);

//在集合中插入键。（调用方将密钥和值打包到
//单个缓冲区并将其作为要插入的参数传入）。
//要求：当前没有与键比较等于的内容位于
//集合，并且没有对正在进行的表进行并发修改
  virtual void Insert(KeyHandle handle) = 0;

//与insert（）相同，但在另外一个函数中，将提示传递给
//关键。如果提示指向nullptr，则将填充新提示。
//否则，提示将被更新以反映上一次插入的位置。
//
//目前只有跳过基于列表的memtable实现接口。其他
//默认情况下，实现将回退到insert（）。
  virtual void InsertWithHint(KeyHandle handle, void** hint) {
//默认情况下忽略提示。
    Insert(handle);
  }

//与insert（handle）类似，但可以与其他调用同时调用
//同时插入其他手柄
  virtual void InsertConcurrently(KeyHandle handle) {
#ifndef ROCKSDB_LITE
    throw std::runtime_error("concurrent insert not supported");
#else
    abort();
#endif
  }

//返回true iff集合中比较等于key的条目。
  virtual bool Contains(const char* key) const = 0;

//通知此表代表它将不再添加到。默认情况下，
//什么也不做。调用markreadonly（）后，此表rep将
//不写入（即不再调用allocate（）、insert（），
//或者直接写入通过迭代器访问的条目。）
  virtual void MarkReadOnly() { }

//从MEM表中查找键，因为MEM表中的第一个键
//用户_键与给定的k匹配，调用函数callback_func（），使用
//回调参数直接作为第一个参数转发，以及mem表
//键作为第二个参数。如果返回值为假，则终止。
//否则，请按下一个键。
//
//get（）在完成所有潜在的
//是否为k.user_key（）的密钥。
//
//违约：
//get（）函数，默认值为动态构造迭代器，
//查找并调用回调函数。
  virtual void Get(const LookupKey& k, void* callback_args,
                   bool (*callback_func)(void* arg, const char* entry));

  virtual uint64_t ApproximateNumEntries(const Slice& start_ikey,
                                         const Slice& end_key) {
    return 0;
  }

//报告除内存外使用了多少内存的近似值
//它是通过分配器分配的。从任何线程调用都是安全的。
  virtual size_t ApproximateMemoryUsage() = 0;

  virtual ~MemTableRep() { }

//跳过集合内容的迭代
  class Iterator {
   public:
//在指定集合上初始化迭代器。
//返回的迭代器无效。
//显式迭代器（const memtablerep*collection）；
    virtual ~Iterator() {}

//如果迭代器位于有效节点，则返回true。
    virtual bool Valid() const = 0;

//返回当前位置的键。
//要求：有效（）
    virtual const char* key() const = 0;

//前进到下一个位置。
//要求：有效（）
    virtual void Next() = 0;

//前进到上一个位置。
//要求：有效（）
    virtual void Prev() = 0;

//使用大于等于target的键前进到第一个条目
    virtual void Seek(const Slice& internal_key, const char* memtable_key) = 0;

//使用键<=target返回到第一个条目
    virtual void SeekForPrev(const Slice& internal_key,
                             const char* memtable_key) = 0;

//在集合中的第一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToFirst() = 0;

//在集合中的最后一个条目处定位。
//迭代器的最终状态为valid（）iff集合不为空。
    virtual void SeekToLast() = 0;
  };

//返回此表示形式中键的迭代器。
//竞技场：如果不为空，则需要使用竞技场来分配迭代器。
//销毁迭代器时，调用方不会调用“delete”
//但是迭代器：：~直接迭代器（）。毁灭者需要毁灭
//所有的州，除了那些分配在竞技场的州。
  virtual Iterator* GetIterator(Arena* arena = nullptr) = 0;

//返回具有特殊查找语义的迭代器。结果
//查找只能包含与目标键具有相同前缀的键。
//竞技场：如果不为空，则竞技场用于分配迭代器。
//销毁迭代器时，调用方不会调用“delete”
//但是迭代器：：~直接迭代器（）。毁灭者需要毁灭
//所有的州，除了那些分配在竞技场的州。
  virtual Iterator* GetDynamicPrefixIterator(Arena* arena = nullptr) {
    return GetIterator(arena);
  }

//如果当前memtablerep支持合并运算符，则返回true。
//默认值：真
  virtual bool IsMergeOperatorSupported() const { return true; }

//如果当前memtablerep支持快照，则返回true
//默认值：真
  virtual bool IsSnapshotSupported() const { return true; }

 protected:
//当*键是与值连接的内部键时，返回
//用户密钥。
  virtual Slice UserKey(const char* key) const;

  Allocator* allocator_;
};

//这是rocksdb用于创建的所有工厂的基类
//新建memtablerep对象
class MemTableRepFactory {
 public:
  virtual ~MemTableRepFactory() {}

  virtual MemTableRep* CreateMemTableRep(const MemTableRep::KeyComparator&,
                                         Allocator*, const SliceTransform*,
                                         Logger* logger) = 0;
  virtual MemTableRep* CreateMemTableRep(
      const MemTableRep::KeyComparator& key_cmp, Allocator* allocator,
      const SliceTransform* slice_transform, Logger* logger,
      /*t32_t/*列_family_id*/）
    返回createMemTableRep（key_cmp，allocator，slice_transform，logger）；
  }

  虚拟常量char*name（）const=0；

  //如果当前memtablerep支持并发插入，则返回true
  //默认值：假
  虚拟bool isinsertConcurrentlysupported（）const返回false；
}；

//这将使用跳过列表来存储键。这是默认值。
/ /
/ /参数：
//lookahead:如果非零，则每个迭代器的seek操作将启动
//从以前访问过的记录中搜索（最多执行“lookahead”
/ /步骤）。这是对访问模式的优化，包括
//使用连续键查找。
类SkipListFactory:公共memTableRepFactory
 公众：
  显式SkipListFactory（大小_t lookahead=0）：lookahead_u（lookahead）

  使用memTableRepFactory:：CreateMemTableRep；
  virtual memtablerep*createMemtablerep（const memtablerep:：keyComparator&，
                                         分配器*，const slicetransform*，
                                         记录器*记录器）超控；
  virtual const char*name（）const override返回“skipListFactory”；

  bool isinsertConcurrentlysupported（）const override返回true；

 私人：
  常量大小\u t lookahead \；
}；

ifndef岩石
//这将创建由std：：vector支持的memtablerep。在迭代中，
//向量已排序。这对于迭代非常重要的工作负载非常有用。
//稀有和写入通常不会在读取开始后发出。
/ /
/ /参数：
//count:传递给每个的基础std:：vector的构造函数
//vectorep.初始化时，基础数组将至少为Count
//为使用保留的字节。
VectorRepFactory类：公共memTableRepFactory
  常量大小\计数\；

 公众：
  显式VectorRepFactory（大小_t Count=0）：计数_u（Count）

  使用memTableRepFactory:：CreateMemTableRep；
  virtual memtablerep*createMemtablerep（const memtablerep:：keyComparator&，
                                         分配器*，const slicetransform*，
                                         记录器*记录器）超控；

  virtual const char*name（）const override_
    返回“vectorepfactory”；
  }
}；

//此类包含固定的存储桶数组，每个存储桶
//指向Skiplist（如果bucket为空，则为空）。
//bucket_count:固定数组bucket数
//skiplist_height：skiplist的最大高度
//Skiplist_分支因子：相邻的概率大小比
//skiplist中的链接列表
extern memtablerepfactory*newhashskiplitrepfactory（
    尺寸\u t桶\计数=1000000，int32 \u t skiplist \高度=4，
    Int32_t Skiplist_分支系数=4
；

//工厂将基于哈希表创建memtables：
//它包含一个固定的存储桶数组，每个存储桶都指向一个链接列表
//如果bucket中的条目数超过
//阈值\使用\u skiplist。
//@bucket_count:固定数组bucket数
//@hugh_page_tlb_size:如果<=0，则从malloc分配哈希表字节。
//否则从大页面TLB。用户需要预留
//要分配的大页面，如：
//sysctl-w vm.nr_hugepages=20
//参见linux文档/vm/hugetlbpage.txt
//@bucket_entries_logging_threshold:如果一个bucket中的条目数
//超过此数字，请记录。
//@if_log_bucket_dist_when_flash:if true，log distribution of number of
//刷新时的条目。
//@threshold_use_skip list:a bucket switches to skip list if number of
//项超过此参数。
extern memtablerepfactory*新哈希链接列表repfactory（
    大小\u t桶\u count=50000，大小\u t大\u页\u tlb \u size=0，
    int bucket_entries_logging_threshold=4096，
    bool如果在flash=true时记录桶距离，
    uint32_t阈值_use_skiplist=256）；

//此工厂创建基于布谷鸟哈希的MEM表表示。
//布谷散列是一种封闭的散列策略，其中所有的键/值对
//存储在某些数据结构中的bucket数组本身中
//在bucket数组外部。此外，布谷鸟散列中的每个键
//在bucket数组中有恒定数量的可能bucket。这些
//两个属性结合在一起可以提高布谷鸟哈希的内存效率和
//恒定的最坏情况读取时间。布谷杂烩最适合
//点查找工作负荷。
/ /
//插入键/值时，首先检查是否有可能
//存储桶为空。如果是这样，则键/值将插入到该空白处
/桶。否则，最初存储在其中一个密钥中的一个密钥
//可能的存储桶将被“踢出”并移动到可能的存储桶之一
//水桶（可能会把另一个受害者踢出去）。
//实现，这样的“踢出”路径是有界的。如果找不到
//“退出”路径对于特定密钥，此密钥将存储在备份中
//结构，当前memtable被强制为不可变。
/ /
//请注意，当前此MEM表表示不支持
//快照（即只查询最新状态）和迭代器。此外，
//由于缺少
//快照支持。
/ /
/ /参数：
//写入缓冲区大小：以字节为单位的写入缓冲区大小。
//平均\u数据\大小：键+值的平均大小（以字节为单位）。这个值
//与写入缓冲区一起使用大小来计算数字
//桶数。
//哈希\函数\计数：将由
//布谷鸟杂烩。这个数字也等于
//每个键都有buckets。
extern memtablerepfactory*newhashcuckoorepfactory（
    大小写缓冲区大小，大小平均数据大小=64，
    unsigned int hash_function_count=4）；
endif//rocksdb_lite
//命名空间rocksdb
