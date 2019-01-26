
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

#include "cache/lru_cache.h"

#include <string>
#include <vector>
#include "util/testharness.h"

namespace rocksdb {

class LRUCacheTest : public testing::Test {
 public:
  LRUCacheTest() {}
  ~LRUCacheTest() {}

  void NewCache(size_t capacity, double high_pri_pool_ratio = 0.0) {
    cache_.reset(
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4316) //我们已经验证了与新操作员的一致性
#endif
      new LRUCacheShard()
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    );
    cache_->SetCapacity(capacity);
    cache_->SetStrictCapacityLimit(false);
    cache_->SetHighPriorityPoolRatio(high_pri_pool_ratio);
  }

  void Insert(const std::string& key,
              Cache::Priority priority = Cache::Priority::LOW) {
    /*他->insert（key，0/*hash*/，nullptr/*value*/，1/*charge*/，
                   nullptr/*删除*/, nullptr /*handle*/, priority);

  }

  void Insert(char key, Cache::Priority priority = Cache::Priority::LOW) {
    Insert(std::string(1, key), priority);
  }

  bool Lookup(const std::string& key) {
    /*o handle=cache_->lookup（key，0/*hash*/）；
    如果（句柄）{
      缓存->释放（句柄）；
      回归真实；
    }
    返回错误；
  }

  bool lookup（char key）返回查找（std:：string（1，key））；

  void erase（const std:：string&key）cache_u->erase（key，0/*具有*/); }


  void ValidateLRUList(std::vector<std::string> keys,
                       size_t num_high_pri_pool_keys = 0) {
    LRUHandle* lru;
    LRUHandle* lru_low_pri;
    cache_->TEST_GetLRUList(&lru, &lru_low_pri);
    LRUHandle* iter = lru;
    bool in_high_pri_pool = false;
    size_t high_pri_pool_keys = 0;
    if (iter == lru_low_pri) {
      in_high_pri_pool = true;
    }
    for (const auto& key : keys) {
      iter = iter->next;
      ASSERT_NE(lru, iter);
      ASSERT_EQ(key, iter->key().ToString());
      ASSERT_EQ(in_high_pri_pool, iter->InHighPriPool());
      if (in_high_pri_pool) {
        high_pri_pool_keys++;
      }
      if (iter == lru_low_pri) {
        ASSERT_FALSE(in_high_pri_pool);
        in_high_pri_pool = true;
      }
    }
    ASSERT_EQ(lru, iter->next);
    ASSERT_TRUE(in_high_pri_pool);
    ASSERT_EQ(num_high_pri_pool_keys, high_pri_pool_keys);
  }

 private:
  std::unique_ptr<LRUCacheShard> cache_;
};

TEST_F(LRUCacheTest, BasicLRU) {
  NewCache(5);
  for (char ch = 'a'; ch <= 'e'; ch++) {
    Insert(ch);
  }
  ValidateLRUList({"a", "b", "c", "d", "e"});
  for (char ch = 'x'; ch <= 'z'; ch++) {
    Insert(ch);
  }
  ValidateLRUList({"d", "e", "x", "y", "z"});
  ASSERT_FALSE(Lookup("b"));
  ValidateLRUList({"d", "e", "x", "y", "z"});
  ASSERT_TRUE(Lookup("e"));
  ValidateLRUList({"d", "x", "y", "z", "e"});
  ASSERT_TRUE(Lookup("z"));
  ValidateLRUList({"d", "x", "y", "e", "z"});
  Erase("x");
  ValidateLRUList({"d", "y", "e", "z"});
  ASSERT_TRUE(Lookup("d"));
  ValidateLRUList({"y", "e", "z", "d"});
  Insert("u");
  ValidateLRUList({"y", "e", "z", "d", "u"});
  Insert("v");
  ValidateLRUList({"e", "z", "d", "u", "v"});
}

TEST_F(LRUCacheTest, MidPointInsertion) {
//为高优先级池分配2个缓存项。
  NewCache(5, 0.45);

  Insert("a", Cache::Priority::LOW);
  Insert("b", Cache::Priority::LOW);
  Insert("c", Cache::Priority::LOW);
  ValidateLRUList({"a", "b", "c"}, 0);

//如果可用，低优先级条目可能需要高优先级池容量
  Insert("u", Cache::Priority::LOW);
  Insert("v", Cache::Priority::LOW);
  ValidateLRUList({"a", "b", "c", "u", "v"}, 0);

  Insert("X", Cache::Priority::HIGH);
  Insert("Y", Cache::Priority::HIGH);
  ValidateLRUList({"c", "u", "v", "X", "Y"}, 2);

//高优先级条目可能溢出到低优先级池。
  Insert("Z", Cache::Priority::HIGH);
  ValidateLRUList({"u", "v", "X", "Y", "Z"}, 2);

//低优先级条目将插入低优先级池的头部。
  Insert("a", Cache::Priority::LOW);
  ValidateLRUList({"v", "X", "a", "Y", "Z"}, 2);

//低优先级条目将在查找后插入到低优先级池的头中。
  ASSERT_TRUE(Lookup("v"));
  ValidateLRUList({"X", "a", "v", "Y", "Z"}, 2);

//查找后，高优先级条目将插入到列表头。
  ASSERT_TRUE(Lookup("X"));
  ValidateLRUList({"a", "v", "Y", "Z", "X"}, 2);
  ASSERT_TRUE(Lookup("Z"));
  ValidateLRUList({"a", "v", "Y", "X", "Z"}, 2);

  Erase("Y");
  ValidateLRUList({"a", "v", "X", "Z"}, 2);
  Erase("X");
  ValidateLRUList({"a", "v", "Z"}, 1);
  Insert("d", Cache::Priority::LOW);
  Insert("e", Cache::Priority::LOW);
  ValidateLRUList({"a", "v", "d", "e", "Z"}, 1);
  Insert("f", Cache::Priority::LOW);
  Insert("g", Cache::Priority::LOW);
  ValidateLRUList({"d", "e", "f", "g", "Z"}, 1);
  ASSERT_TRUE(Lookup("d"));
  ValidateLRUList({"e", "f", "g", "d", "Z"}, 1);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
