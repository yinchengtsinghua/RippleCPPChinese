
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#ifndef ROCKSDB_LITE

#include "utilities/persistent_cache/persistent_cache_test.h"

#include <functional>
#include <memory>
#include <thread>

#include "utilities/persistent_cache/block_cache_tier.h"

namespace rocksdb {

static const double kStressFactor = .125;

#ifdef OS_LINUX
static void OnOpenForRead(void* arg) {
  int* val = static_cast<int*>(arg);
  *val &= ~O_DIRECT;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "NewRandomAccessFile:O_DIRECT",
      std::bind(OnOpenForRead, std::placeholders::_1));
}

static void OnOpenForWrite(void* arg) {
  int* val = static_cast<int*>(arg);
  *val &= ~O_DIRECT;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "NewWritableFile:O_DIRECT",
      std::bind(OnOpenForWrite, std::placeholders::_1));
}
#endif

static void RemoveDirectory(const std::string& folder) {
  std::vector<std::string> files;
  Status status = Env::Default()->GetChildren(folder, &files);
  if (!status.ok()) {
//我们假设目录不存在
    return;
  }

//使用patter:digi:.rc清理文件
  for (auto file : files) {
    if (file == "." || file == "..") {
      continue;
    }
    status = Env::Default()->DeleteFile(folder + "/" + file);
    assert(status.ok());
  }

  status = Env::Default()->DeleteDir(folder);
  assert(status.ok());
}

static void OnDeleteDir(void* arg) {
  char* dir = static_cast<char*>(arg);
  RemoveDirectory(std::string(dir));
}

//
//在stdout上打印消息的简单记录器
//
class ConsoleLogger : public Logger {
 public:
  using Logger::Logv;
  ConsoleLogger() : Logger(InfoLogLevel::ERROR_LEVEL) {}

  void Logv(const char* format, va_list ap) override {
    MutexLock _(&lock_);
    vprintf(format, ap);
    printf("\n");
  }

  port::Mutex lock_;
};

//构建分层RAM+块缓存
std::unique_ptr<PersistentTieredCache> NewTieredCache(
    const size_t mem_size, const PersistentCacheConfig& opt) {
  std::unique_ptr<PersistentTieredCache> tcache(new PersistentTieredCache());
//创建主要层
  assert(mem_size);
  auto pcache = std::shared_ptr<PersistentCacheTier>(new VolatileCacheTier(
      /*s撘compressed*/true，mem_size））；
  tcache->addtier（pcache）；
  //创建二级
  auto scache=std:：shared_ptr<persistentcachetier>（new blockcachetier（opt））；
  tcache->addtier（scache）；

  状态s=tcache->open（）；
  断言（S.O.（））；
  返回TACHACE；
}

//创建块缓存
std:：unique_ptr<persistentcachetier>newblockcache（
    env*env，const std：：字符串和路径，
    const uint64_t max_size=std：：numeric_limits<uint64_t>：max（），
    const bool enable_direct_writes=false）
  const uint32_t max_file_size=static_cast<uint32_t>（12*1024*1024*kstressfactor）；
  auto log=std:：make_shared<consolelogger>（）；
  persistentcacheconfig opt（env，path，max_size，log）；
  opt.cache文件大小=最大文件大小；
  opt.max_write_pipeline_backlog_size=std:：numeric_limits<uint64_t>：：max（）；
  opt.enable_direct_writes=启用_direct_writes；
  std:：unique_ptr<persistentcachetier>scache（new blockcachetier（opt））；
  状态s=scache->open（）；
  断言（S.O.（））；
  返回SCACHE；
}

//创建新的缓存层
std:：unique_ptr<persistenttieredcache>newtieredcache（
    env*env，const std:：string&path，const uint64_t max_volatile_cache_size，
    const uint64_t max_block_cache_size=
        std:：numeric_limits<uint64_t>：：max（））
  const uint32_t max_file_size=static_cast<uint32_t>（12*1024*1024*kstressfactor）；
  auto log=std:：make_shared<consolelogger>（）；
  auto opt=persistentcacheconfig（env，path，max_block_cache_size，log）；
  opt.cache文件大小=最大文件大小；
  opt.max_write_pipeline_backlog_size=std:：numeric_limits<uint64_t>：：max（）；
  //在两个缓存中创建层
  自动缓存=newtieredcache（最大易失性缓存大小，opt）；
  返回高速缓存；
}

PersistentCachetiertest:：PersistentCachetiertest（）。
    ：path_u（test:：tmpdir（env:：default（））+“/cache_test”）
iFIFF OSLinux
  rocksdb:：syncpoint:：getInstance（）->启用处理（）；
  rocksdb:：syncpoint:：getinstance（）->setcallback（“newrandomaccessfile:o_direct”，
                                                 OnOpenFRADE）；
  rocksdb:：syncpoint:：getinstance（）->setcallback（“newwritablefile:o_direct”，
                                                 OnOpenForWrite）；
第二节
}

//块缓存测试
测试_f（persistentcachetiertest，disabled_blockcacheinsertwithfilecreateerror）
  cache_u=newblockcache（env:：default（），path_u，
                         /*尺寸*/std::numeric_limits<uint64_t>::max(),

                         /*irect_writes=*/false）；
  rocksdb:：syncpoint:：getInstance（）->setCallback（
    “blockcachetier:：newcachefile:deletedir”，ondeletedir）；

  runnegativeinserttest（/*nthreads*/ 1,

                        /*AXYKEY*/
                          static_cast<size_t>(10 * 1024 * kStressFactor));

  rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();
}

#ifdef TRAVIS
//Travis无法处理运行中的测试的正常版本
//FDS，空间不足和超时。这是一个更简单的测试版本
//专为特拉维斯写的
TEST_F(PersistentCacheTierTest, BasicTest) {
  cache_ = std::make_shared<VolatileCacheTier>();
  /*inserttest（/*nthreads=*/1，/*max_keys=*/1024）；

  cache_u=newblockcache（env:：default（），path_u，
                         /*尺寸*/std::numeric_limits<uint64_t>::max(),

                         /*直接写入=*/true）；
  runinserttest（/*nthreads*/1, /*max_keys=*/1024);


  cache_ = NewTieredCache(Env::Default(), path_,
                          /*emory_size=*/static_cast<size_t>（1*1024*1024））；
  runinserttest（/*nthreads*/1, /*max_keys=*/1024);

}
#else
//易失性缓存测试
TEST_F(PersistentCacheTierTest, VolatileCacheInsert) {
  for (auto nthreads : {1, 5}) {
    for (auto max_keys :
         {10 * 1024 * kStressFactor, 1 * 1024 * 1024 * kStressFactor}) {
      cache_ = std::make_shared<VolatileCacheTier>();
      RunInsertTest(nthreads, static_cast<size_t>(max_keys));
    }
  }
}

TEST_F(PersistentCacheTierTest, VolatileCacheInsertWithEviction) {
  for (auto nthreads : {1, 5}) {
    for (auto max_keys : {1 * 1024 * 1024 * kStressFactor}) {
      cache_ = std::make_shared<VolatileCacheTier>(
          /*compressed=*/true，/*size=*/static_cast<size_t>（1*1024*1024*kstressfactor））；
      runinserttestweeviction（nthreads，static_cast<size_t>（max_keys））；
    }
  }
}

//块缓存测试
测试_F（PersistentCachetiertest，BlockCacheInsert）
  for（自动直接写入：真，假）
    for（自动n读数：1，5）
      对于（自动最大键：
           10*1024*kstressfactor，1*1024*1024*kstressfactor）
        cache_u=newblockcache（env:：default（），path_u，
                               /*尺寸*/std::numeric_limits<uint64_t>::max(),

                               direct_writes);
        RunInsertTest(nthreads, static_cast<size_t>(max_keys));
      }
    }
  }
}

TEST_F(PersistentCacheTierTest, BlockCacheInsertWithEviction) {
  for (auto nthreads : {1, 5}) {
    for (auto max_keys : {1 * 1024 * 1024 * kStressFactor}) {
      cache_ = NewBlockCache(Env::Default(), path_,
                             /*ax_size=*/static_cast<size_t>（200*1024*1024*kstressfactor））；
      runinserttestweeviction（nthreads，static_cast<size_t>（max_keys））；
    }
  }
}

//分层缓存测试
测试_F（PersistentCachetiertest，tieredCacheInsert）
  for（自动n读数：1，5）
    对于（自动最大键：
         10*1024*kstressfactor，1*1024*1024*kstressfactor）
      cache_u=newtieredcache（env:：default（），path_u，
                              /*内存大小*/static_cast<size_t>(1 * 1024 * 1024 * kStressFactor));

      RunInsertTest(nthreads, static_cast<size_t>(max_keys));
    }
  }
}

//测试会导致大量文件删除，Travis限制了测试
//环境无法处理
TEST_F(PersistentCacheTierTest, TieredCacheInsertWithEviction) {
  for (auto nthreads : {1, 5}) {
    for (auto max_keys : {1 * 1024 * 1024 * kStressFactor}) {
      cache_ = NewTieredCache(
          Env::Default(), path_,
          /*emory_size=*/static_cast<size_t>（1*1024*1024*kstressfactor），
          /*块缓存大小*/ static_cast<size_t>(200 * 1024 * 1024 * kStressFactor));

      RunInsertTestWithEviction(nthreads, static_cast<size_t>(max_keys));
    }
  }
}
#endif

std::shared_ptr<PersistentCacheTier> MakeVolatileCache(
    /*st std：：字符串&/*dbname*/）
  返回std:：make_shared<volatilecachetier>（）；
}

std:：shared_ptr<persistentcachetier>makeblockcache（const std:：string&dbname）
  返回newblockcache（env:：default（），dbname）；
}

std:：shared_ptr<persistentcachetier>maketieredcache（
    const std:：string&dbname）
  const auto memory_size=1*1024*1024*kstressfactor；
  返回newtieredcache（env:：default（），dbname，static_cast<size_t>（memory_size））；
}

iFIFF OSLinux
静态void uniqueidcallback（void*arg）
  int*result=reinterpret_cast<int*>（arg）；
  如果（*result=-1）
    ＊结果＝0；
  }

  rocksdb:：syncpoint:：getInstance（）->cleartrace（）；
  rocksdb:：syncpoint:：getInstance（）->setCallback（
      “getuniqueidfromfile:fs_ioc_getversion”，uniqueidcallback）；
}
第二节

测试（PersistentCachetiertest，FactoryTest）
  for（auto-nvm_opt:真，假）
    断言“假”（缓存）；
    auto log=std:：make_shared<consolelogger>（）；
    std:：shared&ptr<persistentcache>cache；
    断言_OK（newPersistentCache（env:：default（），path_u，
                                 /*尺寸*/1 * 1024 * 1024 * 1024, log, nvm_opt,

                                 &cache));
    ASSERT_TRUE(cache);
    ASSERT_EQ(cache->Stats().size(), 1);
    ASSERT_TRUE(cache->Stats()[0].size());
    cache.reset();
  }
}

PersistentCacheDBTest::PersistentCacheDBTest() : DBTestBase("/cache_test") {
#ifdef OS_LINUX
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "GetUniqueIdFromFile:FS_IOC_GETVERSION", UniqueIdCallback);
  rocksdb::SyncPoint::GetInstance()->SetCallBack("NewRandomAccessFile:O_DIRECT",
                                                 OnOpenForRead);
#endif
}

//测试模板
void PersistentCacheDBTest::RunTest(
    const std::function<std::shared_ptr<PersistentCacheTier>(bool)>& new_pcache,
    const size_t max_keys = 100 * 1024, const size_t max_usecase = 5) {
  if (!Snappy_Supported()) {
    return;
  }

//插入交互次数
  int num_iter = static_cast<int>(max_keys * kStressFactor);

  for (size_t iter = 0; iter < max_usecase; iter++) {
    Options options;
    options.write_buffer_size =
static_cast<size_t>(64 * 1024 * kStressFactor);  //小写缓冲区
    options.statistics = rocksdb::CreateDBStatistics();
    options = CurrentOptions(options);

//设置页缓存
    std::shared_ptr<PersistentCacheTier> pcache;
    BlockBasedTableOptions table_options;
    table_options.cache_index_and_filter_blocks = true;

    const size_t size_max = std::numeric_limits<size_t>::max();

    switch (iter) {
      case 0:
//页面缓存、块缓存、无压缩缓存
        /*che=新的缓存（/*是压缩的=*/true）；
        table_options.persistent_cache=pcache；
        table_options.block_cache=newlrucache（大小_max）；
        table_options.block_cache_compressed=nullptr；
        options.table_factory.reset（newblockbasedTableFactory（table_options））；
        断裂；
      案例1：
        //页缓存、块缓存、压缩缓存
        pcache=新的\u pcache（/*已压缩*/true);

        table_options.persistent_cache = pcache;
        table_options.block_cache = NewLRUCache(size_max);
        table_options.block_cache_compressed = NewLRUCache(size_max);
        options.table_factory.reset(NewBlockBasedTableFactory(table_options));
        break;
      case 2:
//页面缓存、块缓存、压缩缓存+knocompression
//块缓存和压缩缓存，但数据库未压缩
//另外，使块缓存大小变大，以触发块缓存命中
        /*che=新的缓存（/*是压缩的=*/true）；
        table_options.persistent_cache=pcache；
        table_options.block_cache=newlrucache（大小_max）；
        table_options.block_cache_compressed=newlrucache（大小_max）；
        options.table_factory.reset（newblockbasedTableFactory（table_options））；
        options.compression=knocompression；
        断裂；
      案例3：
        //页缓存，无块缓存，无压缩缓存
        pcache=新的\u pcache（/*已压缩*/false);

        table_options.persistent_cache = pcache;
        table_options.block_cache = nullptr;
        table_options.block_cache_compressed = nullptr;
        options.table_factory.reset(NewBlockBasedTableFactory(table_options));
        break;
      case 4:
//页面缓存，无块缓存，无压缩缓存
//页面缓存缓存压缩块
        /*che=新的缓存（/*是压缩的=*/true）；
        table_options.persistent_cache=pcache；
        table_options.block_cache=nullptr；
        table_options.block_cache_compressed=nullptr；
        options.table_factory.reset（newblockbasedTableFactory（table_options））；
        断裂；
      违约：
        失效（）；
    }

    std:：vector<std:：string>值；
    /插入数据
    插入（选项、表选项、数字和值）；
    //将缓存中的所有数据刷新到设备
    pcache->test_flush（）；
    /验证数据
    验证（num-iter，值）；

    auto block_miss=testGetTickerCount（选项，block_cache_miss）；
    自动压缩块命中=
        testgettickercount（选项，block_cache_compressed_hit）；
    自动压缩块丢失=
        testGetTickerCount（选项，块缓存未压缩）；
    auto page_hit=testGetTickerCount（选项，持久的_cache_hit）；
    auto page_miss=testGetTickerCount（选项，持久的_cache_miss）；

    //检查是否触发了缓存中适当的代码路径
    开关（ITER）{
      案例0：
        //页缓存，块缓存，无压缩缓存
        断言（缺页，0）；
        断言gt（页面命中，0）；
        断言“gt”（块未命中，0）；
        断言_Eq（压缩_Block_Miss，0）；
        断言“eq”（压缩的“block”命中，0）；
        断裂；
      案例1：
        //页缓存、块缓存、压缩缓存
        断言（缺页，0）；
        断言“gt”（块未命中，0）；
        断言gt（压缩块丢失，0）；
        断裂；
      案例2：
        //页缓存、块缓存、压缩缓存+knocompression
        断言（缺页，0）；
        断言gt（页面命中，0）；
        断言“gt”（块未命中，0）；
        断言gt（压缩块丢失，0）；
        //记住knocompression
        断言“eq”（压缩的“block”命中，0）；
        断裂；
      案例3：
      案例4：
        //页缓存，无块缓存，无压缩缓存
        断言（缺页，0）；
        断言gt（页面命中，0）；
        断言“eq”（压缩的“block”命中，0）；
        断言_Eq（压缩_Block_Miss，0）；
        断裂；
      违约：
        失效（）；
    }

    options.create_if_missing=true；
    拆毁和打开（选项）；

    pcache->close（）；
  }
}

特拉维斯
//Travis无法处理运行中的测试的正常版本
//FDS，空间不足和超时。这是一个更简单的测试版本
//专为Travis编写
测试_f（PersistentCacheDBTest，BasicTest）
  运行测试（std:：bind（&makeblockcache，dbname_u），/*max_键*/1024,

          /*ax_用例=*/1）；
}
否则
//带有块页缓存的测试表
测试_f（PersistentCacheDBTest，BlockCacheTest）
  运行测试（std:：bind（&makeblockcache，dbname_uu））；
}

//具有可变页缓存的测试表
测试F（PersistentCacheDBTest，VolatileChetest）
  运行测试（std:：bind（&makevolatilecache，dbname_u））；
}

//具有分层页缓存的测试表
测试F（PersistentCacheDBTest，TieredCacheTest）
  运行测试（std:：bind（&maketieredcache，dbname_u））；
}
第二节

//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}
否则
int main（）返回0；
第二节
