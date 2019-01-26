
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
#pragma once

#ifndef ROCKSDB_LITE

#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "monitoring/histogram.h"
#include "rocksdb/env.h"
#include "rocksdb/persistent_cache.h"
#include "rocksdb/status.h"

//持久缓存
//
//持久缓存是可以使用持久介质的分层键值缓存。它
//是一种通用设计，可以利用任何存储介质——磁盘/ssd/nvm/ram。
//代码保持通用性，但具有重要的基准/设计/开发。
//已花费时间确保缓存在
//各自的存储介质。
//文件定义
//PersistentCacheTier:处理单个缓存层的实现
//PersistentTieRescache：将所有层作为逻辑层处理的实现
//单元
//
//PersistentTieredCache体系结构：
//+——处理多个层的PersistentCacheTier
//+————————+
//RAM处理RAM的PersistentCachetier（volatileCacheImpl）
//+————————+
//下一步
//_V_
//+————————+
//NVM处理NVM的PersistentCachetier实现
//+------------+（blockcacheimpl）
//下一步
//_V_
//+————————+
//le-ssd处理le-ssd的PersistentCacheTier实现
//+------------+（blockcacheimpl）
//|   |                      |
//_V_
//空
//+--------------------------+
//γ
//V
//无效的
namespace rocksdb {

//持久缓存配置
//
//此结构捕获用于配置持久性的所有选项
//隐藏物。用于命名选项的一些术语是
//
//调度大小：
//这是将IO发送到设备的大小
//
//写入缓冲区大小：
//这是单个写缓冲区大小。写缓冲区是
//分组以形成缓冲文件。
//
//缓存大小：
//这是缓存大小的逻辑最大值
//
//Q深度：
//这是可以并行发送到设备的最大IOS数。
//
//佩皮林：
//编写器代码路径遵循流水线结构，这意味着
//操作从一个阶段移交给另一个阶段
//
//管道积压量：
//使用流水线结构，在
//管道队列。这是除去操作后的最大积压量
//从队列
struct PersistentCacheConfig {
  explicit PersistentCacheConfig(
      Env* const _env, const std::string& _path, const uint64_t _cache_size,
      const std::shared_ptr<Logger>& _log,
      /*st uint32_t_write_buffer_size=1*1024*1024/*1MB*/）
    Env＝γEnv；
    路径＝Y路径；
    log＝γlog；
    cache_-size=_-cache_-size；
    writer_dispatch_size=write_buffer_size=_write_buffer_size；
  }

  / /
  //验证设置。我们的目的是抓住前面的错误设置
  //而不是违反不变量或导致死锁。
  / /
  状态验证设置（）常量
    //（1）检查变量的前提条件
    如果（！）env path.empty（））_
      返回状态：：invalidArgument（“空或空参数”）；
    }

    //（2）断言与大小相关的不变量
    //-缓存大小不能小于缓存文件大小
    //-单个写入缓冲区大小不能大于缓存文件大小
    //-总写入缓冲区大小不能小于2x缓存文件大小
    if（cache_size<cache_file_size write_buffer_size>=cache_file size
        write_buffer_size*write_buffer_count（）<2*cache_file_size）
      返回状态：：invalidArgument（“缓存大小无效”）；
    }

    //（2）检查写入程序设置
    //-队列深度不能为0
    //-写入程序\调度\大小不能大于写入程序\缓冲区\大小
    //-调度大小和缓冲区大小需要对齐
    如果（！）writer_qdepth writer_dispatch_size>write_buffer_size
        write_buffer_size%writer_dispatch_size）
      返回状态：：InvalidArgument（“编写器设置无效”）；
    }

    返回状态：：OK（）；
  }

  / /
  //用于系统级操作的env抽象
  / /
  Env＊Env；

  / /
  //持久化块的块缓存路径
  / /
  std：：字符串路径；

  / /
  //日志消息的日志句柄
  / /
  std:：shared&ptr<logger>log；

  / /
  //启用直接IO读取
  / /
  bool enable_direct_reads=true；

  / /
  //启用直接IO写入
  / /
  bool enable_direct_writes=false；

  / /
  //逻辑缓存大小
  / /
  uint64_t cache_size=std:：numeric_limits<uint64_t>：：max（）；

  //缓存文件大小
  / /
  //缓存由多个小文件组成。此参数定义
  //单个缓存文件的大小
  / /
  /默认值：1M
  uint32缓存文件大小=100ull*1024*1024；

  //编写器qdepth
  / /
  //编写器可以并行向设备发出IO。这个参数
  //控制可以与块并行发出的IOS的最大数目
  /设备
  / /
  /默认值：1
  uint32_t writer_qdepth=1；

  //管道写入
  / /
  //写操作可以遵循流水线结构。这有助于
  //避免在主层的逐出代码路径中回归。这个
  //参数定义是否启用或禁用管道
  / /
  //默认值：真
  bool pipeline_writes=true；

  //最大写入管道积压量
  / /
  //最大管道缓冲区大小。这是我们可以累积的最大积压量
  //等待写入时。超过限制后，新操作将被取消。
  / /
  //默认值：1gib
  uint64_t max_write_pipeline_backlog_size=1所有*1024*1024*1024*1024；

  //写入缓冲区大小
  / /
  //这是分配缓冲区板的大小。
  / /
  /默认值：1M
  uint32_t写入缓冲区_size=1所有*1024*1024；

  //写入缓冲区计数
  / /
  //这是缓冲区板的总数。这是作为
  //文件大小以避免死锁。
  大小写缓冲区计数（）常量
    断言（写入缓冲区大小）；
    返回静态缓存文件大小
                               写入缓冲区大小）；
  }

  //写入程序调度大小
  / /
  //编写器线程将按指定的IO大小调度IO
  / /
  /默认值：1M
  uint64_t writer_dispatch_size=1所有*1024*1024；

  //被压缩
  / /
  //此选项确定缓存是否以压缩模式运行，或者
  //未压缩模式
  bool is_compressed=true；

  persistentcacheconfig生成persistentcacheconfig（
      const std：：字符串和路径，const uint64_t大小，
      const std：：shared&ptr<logger>&log）；

  std:：string tostring（）常量；
}；

//持久缓存层
/ /
//这是定义持久缓存层的逻辑抽象。层级
//可以相互堆叠。PersistentCahe提供了基本定义
//用于访问/存储在缓存中。PersistentCacheTier扩展接口
//启用层的管理和堆叠。
类PersistentCacheTier:公共PersistentCache_
 公众：
  typedef std:：shared_ptr<persistentcachetier>tier；

  虚拟~PersistentCacheTier（）

  //打开持久缓存层
  虚拟状态open（）；

  //关闭持久缓存层
  虚拟状态close（）；

  //最多保留“大小”字节的空间
  虚拟bool reserve（const size_t size）；

  //从缓存中删除密钥
  虚拟布尔擦除（const slice&key）；

  //以递归方式将状态打印到字符串
  虚拟std:：string printstats（）；

  虚拟持久缓存：：stattype stats（）；

  //插入到页面缓存
  虚拟状态插入（const slice&page_key，const char*data，
                        常量大小=0；

  //按页标识符查找页缓存
  虚拟状态查找（const slice&page_key，std:：unique_ptr<char[]>*data，
                        尺寸_t*尺寸=0；

  //是否存储压缩数据？
  虚拟bool iscompressed（）=0；

  虚拟std:：string getprintableoptions（）const=0；

  //返回对下一层的引用
  虚拟层&next_tier（）返回下一个_tier_

  //设置下一层的值
  虚拟空集下一层（const-tier&tier）
    断言（！）NEXTH）
    下一层
  }

  虚拟空隙测试_flush（）
    如果（下一层）
      下一层测试flush（）；
    }
  }

 私人：
  下一层//下一层
}；

//persistenttieredcache
/ /
//有助于将持久缓存层构造为
//统一缓存。缓存的层将作为管理的单一层
//简化并支持persistentcache方法访问数据。
类persistenttieredcache:public persistentcachetier_
 公众：
  虚拟~persistenttieredcache（）；

  状态open（）覆盖；
  status close（）覆盖；
  bool erase（const slice&key）override；
  std:：string printstats（）重写；
  persistentcache:：stattype stats（）重写；
  状态插入（const slice&page_key，const char*data，
                常量大小）覆盖；
  状态查找（const slice&page_key，std:：unique_ptr<char[]>*data，
                大小）覆盖；
  bool iscompressed（）重写；

  std:：string getprintableoptions（）const override_
    返回“persistenttieredcache”；
  }

  void addtier（const tier&tier）；

  下一层和下一层（）覆盖
    auto it=tiers_.end（）；
    返回（*it）->下一层（）；
  }

  void set_next_tier（const tier&tier）override_
    auto it=tiers_.end（）；
    （*它）->设置下一层（层）；
  }

  void test_flush（）override_
    断言（！）层_.empty（））；
    层_.front（）->测试_flush（）；
    persistentcachetier:：test_flush（）；
  }

 受保护的：
  std:：list<tier>tiers_；//从上到下的层列表
}；

//命名空间rocksdb

第二节
