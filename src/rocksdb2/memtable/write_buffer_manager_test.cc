
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

#include "rocksdb/write_buffer_manager.h"
#include "util/testharness.h"

namespace rocksdb {

class WriteBufferManagerTest : public testing::Test {};

#ifndef ROCKSDB_LITE
TEST_F(WriteBufferManagerTest, ShouldFlush) {
//大小为10MB的写缓冲区管理器
  std::unique_ptr<WriteBufferManager> wbf(
      new WriteBufferManager(10 * 1024 * 1024));

  wbf->ReserveMem(8 * 1024 * 1024);
  ASSERT_FALSE(wbf->ShouldFlush());
//硬极限的90%会碰到这种情况。
  wbf->ReserveMem(1 * 1024 * 1024);
  ASSERT_TRUE(wbf->ShouldFlush());
//安排释放将释放条件
  wbf->ScheduleFreeMem(1 * 1024 * 1024);
  ASSERT_FALSE(wbf->ShouldFlush());

  wbf->ReserveMem(2 * 1024 * 1024);
  ASSERT_TRUE(wbf->ShouldFlush());

  wbf->ScheduleFreeMem(4 * 1024 * 1024);
//11MB总容量，6MB可变容量。仍然达到硬极限
  ASSERT_TRUE(wbf->ShouldFlush());

  wbf->ScheduleFreeMem(2 * 1024 * 1024);
//11MB总容量，4MB可变容量。硬限制静止，但不会冲洗，因为更多
//超过一半的数据已被刷新。
  ASSERT_FALSE(wbf->ShouldFlush());

  wbf->ReserveMem(4 * 1024 * 1024);
//总共15 MB，8 MB可变。
  ASSERT_TRUE(wbf->ShouldFlush());

  wbf->FreeMem(7 * 1024 * 1024);
//总共9MB，8MB可变。
  ASSERT_FALSE(wbf->ShouldFlush());
}

TEST_F(WriteBufferManagerTest, CacheCost) {
//1GB高速缓存
  std::shared_ptr<Cache> cache = NewLRUCache(1024 * 1024 * 1024, 4);
//大小为50MB的写缓冲区管理器
  std::unique_ptr<WriteBufferManager> wbf(
      new WriteBufferManager(50 * 1024 * 1024, cache));

//分配1.5MB将分配2MB
  wbf->ReserveMem(1536 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 2 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 2 * 1024 * 1024 + 10000);

//再分配2MB
  wbf->ReserveMem(2 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 4 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 4 * 1024 * 1024 + 10000);

//再分配20MB
  wbf->ReserveMem(20 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 24 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 24 * 1024 * 1024 + 10000);

//免费的2MB不会导致缓存成本的任何变化
  wbf->FreeMem(2 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 24 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 24 * 1024 * 1024 + 10000);

  ASSERT_FALSE(wbf->ShouldFlush());

//再分配30MB
  wbf->ReserveMem(30 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 52 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 52 * 1024 * 1024 + 10000);
  ASSERT_TRUE(wbf->ShouldFlush());

  ASSERT_TRUE(wbf->ShouldFlush());

  wbf->ScheduleFreeMem(20 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 52 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 52 * 1024 * 1024 + 10000);

//仍然需要刷新，因为硬限制命中
  ASSERT_TRUE(wbf->ShouldFlush());

//空闲20MB将从缓存释放1MB
  wbf->FreeMem(20 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 51 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 51 * 1024 * 1024 + 10000);

  ASSERT_FALSE(wbf->ShouldFlush());

//如果仍然没有命中3/4，每个自由人将释放1兆字节。
  wbf->FreeMem(16 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 50 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 50 * 1024 * 1024 + 10000);

  wbf->FreeMem(16 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 49 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 49 * 1024 * 1024 + 10000);

//免费的2MB不会导致缓存成本的任何变化
  wbf->ReserveMem(2 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 49 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 49 * 1024 * 1024 + 10000);

  wbf->FreeMem(16 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 48 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 48 * 1024 * 1024 + 10000);

//Destroy写入缓冲区管理器应释放所有内容
  wbf.reset();
  ASSERT_LT(cache->GetPinnedUsage(), 1024 * 1024);
}

TEST_F(WriteBufferManagerTest, NoCapCacheCost) {
//1GB高速缓存
  std::shared_ptr<Cache> cache = NewLRUCache(1024 * 1024 * 1024, 4);
//256MB大小的写缓冲区管理器
  std::unique_ptr<WriteBufferManager> wbf(new WriteBufferManager(0, cache));
//分配1.5MB将分配2MB
  wbf->ReserveMem(10 * 1024 * 1024);
  ASSERT_GE(cache->GetPinnedUsage(), 10 * 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 10 * 1024 * 1024 + 10000);
  ASSERT_FALSE(wbf->ShouldFlush());

  wbf->FreeMem(9 * 1024 * 1024);
  for (int i = 0; i < 10; i++) {
    wbf->FreeMem(16 * 1024);
  }
  ASSERT_GE(cache->GetPinnedUsage(), 1024 * 1024);
  ASSERT_LT(cache->GetPinnedUsage(), 1024 * 1024 + 10000);
}
#endif  //摇滚乐
}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
