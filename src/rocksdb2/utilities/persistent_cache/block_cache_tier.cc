
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
#ifndef ROCKSDB_LITE

#include "utilities/persistent_cache/block_cache_tier.h"

#include <regex>
#include <utility>
#include <vector>

#include "port/port.h"
#include "util/logging.h"
#include "util/stop_watch.h"
#include "util/sync_point.h"
#include "utilities/persistent_cache/block_cache_tier_file.h"

namespace rocksdb {

//
//块状聚酰亚胺
//
Status BlockCacheTier::Open() {
  Status status;

  WriteLock _(&lock_);

  assert(!size_);

//检查选项的有效性
  status = opt_.ValidateSettings();
  assert(status.ok());
  if (!status.ok()) {
    Error(opt_.log, "Invalid block cache options");
    return status;
  }

//创建基目录或清除现有目录
  status = opt_.env->CreateDirIfMissing(opt_.path);
  if (!status.ok()) {
    Error(opt_.log, "Error creating directory %s. %s", opt_.path.c_str(),
          status.ToString().c_str());
    return status;
  }

//创建base/<cache dir>目录
  status = opt_.env->CreateDir(GetCachePath());
  if (!status.ok()) {
//目录已经存在，请清除它
    status = CleanupCacheFolder(GetCachePath());
    assert(status.ok());
    if (!status.ok()) {
      Error(opt_.log, "Error creating directory %s. %s", opt_.path.c_str(),
            status.ToString().c_str());
      return status;
    }
  }

//创建新文件
  assert(!cache_file_);
  status = NewCacheFile();
  if (!status.ok()) {
    Error(opt_.log, "Error creating new file %s. %s", opt_.path.c_str(),
          status.ToString().c_str());
    return status;
  }

  assert(cache_file_);

  if (opt_.pipeline_writes) {
    assert(!insert_th_.joinable());
    insert_th_ = port::Thread(&BlockCacheTier::InsertMain, this);
  }

  return Status::OK();
}

bool IsCacheFile(const std::string& file) {
//检查文件是否有.rc后缀
//不幸的是，跨编译器的regex支持并不均匀，因此我们使用简单的
//字符串解析
  size_t pos = file.find(".");
  if (pos == std::string::npos) {
    return false;
  }

  std::string suffix = file.substr(pos);
  return suffix == ".rc";
}

Status BlockCacheTier::CleanupCacheFolder(const std::string& folder) {
  std::vector<std::string> files;
  Status status = opt_.env->GetChildren(folder, &files);
  if (!status.ok()) {
    Error(opt_.log, "Error getting files for %s. %s", folder.c_str(),
          status.ToString().c_str());
    return status;
  }

//使用patter:digi:.rc清理文件
  for (auto file : files) {
    if (IsCacheFile(file)) {
//缓存文件
      Info(opt_.log, "Removing file %s.", file.c_str());
      status = opt_.env->DeleteFile(folder + "/" + file);
      if (!status.ok()) {
        Error(opt_.log, "Error deleting file %s. %s", file.c_str(),
              status.ToString().c_str());
        return status;
      }
    } else {
      ROCKS_LOG_DEBUG(opt_.log, "Skipping file %s", file.c_str());
    }
  }
  return Status::OK();
}

Status BlockCacheTier::Close() {
//停止插入螺纹
  if (opt_.pipeline_writes && insert_th_.joinable()) {
    /*ertop op（/*quit=*/true）；
    插入Ops Push（std:：move（op））；
    插入“th”join（）；
  }

  //在此之前停止编写器
  写入器停止（）；

  //清除所有元数据
  写锁（&lock）；
  元数据清除（）；
  返回状态：：OK（）；
}

模板<class t>
void添加（std:：map<std:：string，double>>*stats，const std:：string&key，
         cont t&t）{
  stats->insert（key，static_cast<double>（t））；
}

persistentcache:：stattype blockcachetier:：stats（）
  std:：map<std:：string，double>stats；
  添加（&stats，“persistentcache.blockcachetier.bytes”piplined“，
      stats_u.bytes_pipelined_u.average（））；
  添加（&stats，“persistentcache.blockcachetier.bytes写入”，
      stats_u.bytes_written_u.average（））；
  添加（&stats，“persistentcache.blockcachetier.bytes读取”，
      stats_u.bytes_read_u.average（））；
  添加（&stats，“persistentcache.blockcachetier.insert_dropped”，
      统计插入掉了
  添加（&stats，“persistentcache.blockcachetier.cache_hits”，
      统计缓存命中率；
  添加（&stats，“persistentcache.blockcachetier.cache_misses”，
      统计缓存未命中；
  添加（&stats，“persistentcache.blockcachetier.cache_错误”，
      统计缓存错误；
  添加（&stats，“persistentcache.blockcachetier.cache_hits_pct”，
      stats_.cachehitpct（））；
  添加（&stats，“persistentcache.blockcachetier.cache_misses_pct”，
      stats_.cachemisspct（））；
  添加（&stats，“persistentcache.blockcachetier.read_hit_latency”，
      stats_u.read_hit_latency_u.average（））；
  添加（&stats，“persistentcache.blockcachetier.read_miss_latency”，
      stats_u.read_miss_latency_u.average（））；
  添加（&stats，“persistenetcache.blockcachetier.write_latency”，
      stats_u.write_latency_u.average（））；

  auto out=persistentcachetier:：stats（）；
  向外。向后推（统计）；
  退回；
}

状态块缓存层：：插入（const slice&key，const char*data，
                              常量大小
  /更新统计数据
  stats_u.bytes_pipelined_u.add（大小）；

  如果（opt_uu.pipeline_writes）
    //将写操作卸载到写线程
    插入“操作”按钮（
        insertop（key.toString（），std:：move（std:：string（data，size）））；
    返回状态：：OK（）；
  }

  断言（！）opt？pipeline？写入）；
  返回insertimpl（key，slice（data，size））；
}

void块缓存层：：insertmain（）
  当（真）{
    insert op op（insert_ops_u.pop（））；

    如果（操作信号
      //这是退出的秘密信号
      断裂；
    }

    大小重试=0；
    状态S；
    while（（s=insertimpl（slice（op.key_u），slice（op.data_u））.istrigain（））
      如果（重试>kmaxretry）
        断裂；
      }

      //当缓冲区已满时，可能会发生这种情况，我们等待一些缓冲区
      免费。我们为什么不在代码里等呢？这是因为我们想要
      //同时支持管道和非管道模式
      缓冲区分配程序waituntilusable（）；
      重试++；
    }

    如果（！）S.O.（））{
      统计插入丢失
    }
  }
}

状态块缓存层：：insertimpl（const slice&key，const slice&data）
  //前提条件
  断言（key.size（））；
  断言（data.size（））；
  断言（缓存文件）；

  StopWatchnano计时器（可选环境，/*自动启动*/ true);


  WriteLock _(&lock_);

  LBA lba;
  if (metadata_.Lookup(key, &lba)) {
//该键已存在，这是重复的插入
    return Status::OK();
  }

  while (!cache_file_->Append(key, data, &lba)) {
    if (!cache_file_->Eof()) {
      ROCKS_LOG_DEBUG(opt_.log, "Error inserting to cache file %d",
                      cache_file_->cacheid());
      stats_.write_latency_.Add(timer.ElapsedNanos() / 1000);
      return Status::TryAgain();
    }

    assert(cache_file_->Eof());
    Status status = NewCacheFile();
    if (!status.ok()) {
      return status;
    }
  }

//插入到查找索引中
  BlockInfo* info = metadata_.Insert(key, lba);
  assert(info);
  if (!info) {
    return Status::IOError("Unexpected error inserting to index");
  }

//插入到缓存文件反向映射
  cache_file_->Add(info);

//更新统计数据
  stats_.bytes_written_.Add(data.size());
  stats_.write_latency_.Add(timer.ElapsedNanos() / 1000);
  return Status::OK();
}

Status BlockCacheTier::Lookup(const Slice& key, unique_ptr<char[]>* val,
                              size_t* size) {
  /*pWatchnano计时器（opt_u.env，/*自动启动=*/true）；

  LBA LBA；
  布尔状态；
  状态=元数据查找（键，&lba）；
  如果（！）状态）{
    统计缓存未命中
    stats_u.read_miss_latency_u.add（timer.elapsednanos（）/1000）；
    返回状态：：NotFound（“blockcache:key not found”）；
  }

  blockcachefile*const file=元数据查找（lba.cache\u id）；
  如果（！）文件）{
    //这可能是因为块索引和缓存文件索引
    //不同，在两次查找之间可能会删除缓存文件
    统计缓存未命中
    stats_u.read_miss_latency_u.add（timer.elapsednanos（）/1000）；
    返回状态：：NotFound（“BlockCache:未找到缓存文件”）；
  }

  断言（文件->引用）；

  unique_ptr<char[]>scratch（new char[lba.size_u]）；
  切片BLKYKY；
  切片BLKYVAL；

  status=file->read（lba，&blk_key，&blk_val，scratch.get（））；
  文件->
  如果（！）状态）{
    统计缓存未命中
    统计缓存错误
    stats_u.read_miss_latency_u.add（timer.elapsednanos（）/1000）；
    返回状态：：NotFound（“BlockCache:读取数据时出错”）；
  }

  断言（blk_key==key）；

  val->reset（新字符[blk_val.size（）]）；
  memcpy（val->get（），blk_val.data（），blk_val.size（））；
  *size=blk_val.size（）；

  stats_u.bytes_u read_u.add（*大小）；
  统计缓存命中率
  状态读取命中延迟添加（timer.elapsednanos（）/1000）；

  返回状态：：OK（）；
}

bool blockcachetier:：erase（const slice&key）
  写锁（&lock）；
  blockinfo*info=元数据删除（key）；
  断言（信息）；
  删除信息；
  回归真实；
}

状态块缓存层：：newcachefile（）
  lock.assertHolded（）；

  测试同步点回调（“blockcachetier:：newcachefile:deletedir”，
                           （void*）（getcachepath（）.c_str（））；

  std:：unique_ptr<writeablecachefile>f（
    新的可写缓存文件（opt_u.env，&buffer_allocator_u，&writer_u，
                           getcachepath（），写入程序缓存
                           opt_u.cache_file_size，opt_u.log））；

  bool status=f->create（opt_u.enable_direct_writes，opt_u.enable_direct_reads）；
  如果（！）状态）{
    返回状态：：ioerror（“创建文件时出错”）；
  }

  信息（opt_u.log，“已创建缓存文件%d”，writer_cache_id_u）；

  写入程序缓存
  缓存文件释放（）；

  //插入到缓存文件树
  状态=元数据插入（缓存文件）；
  断言（状态）；
  如果（！）状态）{
    错误（opt.log，“错误插入元数据”）；
    返回状态：：ioerror（“插入元数据时出错”）；
  }

  返回状态：：OK（）；
}

bool blockcachetier:：reserve（const size_t size）
  写锁（&lock）；
  断言（大小<=opt_uu.cache_size）；

  if（大小+大小<=opt_u.cache_大小）
    //有足够的空间写入
    尺寸=大小；
    回归真实；
  }

  断言（大小+大小=可选缓存大小）；
  //空间不足，无法容纳请求的数据
  //我们可以通过提取冷数据来清除一些空间

  const double retain_fac=（100-kevictpct）/static_cast<double>（100）；
  同时（大小+大小u>opt uu.cache u size*保留）
    unique_ptr<blockcachefile>f（metadata_.evict（））；
    如果（！）f）{
      //没有可收回的内容
      返回错误；
    }
    断言（！）F-＞Refs*）；
    uint64_t文件大小；
    如果（！）f->delete（&file_size）.ok（））
      //无法删除文件
      返回错误；
    }

    断言（文件大小<=大小）；
    size_-=文件大小；
  }

  尺寸=大小；
  断言（大小<=opt_u.cache_size*0.9）；
  回归真实；
}

状态newPersistentCache（env*const env，const std:：string&path，
                          总尺寸
                          const std:：shared&ptr<logger>日志，
                          const bool优化了_Nvm，
                          std:：shared_ptr<persistentcache>>*cache）
  如果（！）缓存）{
    返回状态：：ioerror（“参数缓存无效”）；
  }

  auto opt=persistentcacheconfig（env，path，size，log）；
  if（优化后的_Nvm）
    //默认设置针对SSD进行了优化
    //nvm设备可以通过4K直接IO更好地访问，并使用
    /并行性
    opt.enable_direct_writes=true；
    opt.writer_qdepth=4；
    opt.writer_dispatch_size=4*1024；
  }

  auto-pcache=std:：make_shared<blockcachetier>（opt）；
  状态s=pcache->open（）；

  如果（！）S.O.（））{
    返回S；
  }

  *缓存=pcache；
  返回S；
}

//命名空间rocksdb

endif//ifndef rocksdb_lite
