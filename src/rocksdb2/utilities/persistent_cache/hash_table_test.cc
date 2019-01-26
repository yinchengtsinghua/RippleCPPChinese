
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
#include <stdlib.h>
#include <iostream>
#include <set>
#include <string>

#include "db/db_test_util.h"
#include "util/arena.h"
#include "util/random.h"
#include "util/testharness.h"
#include "utilities/persistent_cache/hash_table.h"
#include "utilities/persistent_cache/hash_table_evictable.h"

#ifndef ROCKSDB_LITE

namespace rocksdb {

struct HashTableTest : public testing::Test {
  ~HashTableTest() { map_.Clear(&HashTableTest::ClearNode); }

  struct Node {
    Node() {}
    explicit Node(const uint64_t key, const std::string& val = std::string())
        : key_(key), val_(val) {}

    uint64_t key_ = 0;
    std::string val_;
  };

  struct Equal {
    bool operator()(const Node& lhs, const Node& rhs) {
      return lhs.key_ == rhs.key_;
    }
  };

  struct Hash {
    uint64_t operator()(const Node& node) {
      return std::hash<uint64_t>()(node.key_);
    }
  };

  static void ClearNode(Node node) {}

  HashTable<Node, Hash, Equal> map_;
};

struct EvictableHashTableTest : public testing::Test {
  ~EvictableHashTableTest() { map_.Clear(&EvictableHashTableTest::ClearNode); }

  struct Node : LRUElement<Node> {
    Node() {}
    explicit Node(const uint64_t key, const std::string& val = std::string())
        : key_(key), val_(val) {}

    uint64_t key_ = 0;
    std::string val_;
    std::atomic<uint32_t> refs_{0};
  };

  struct Equal {
    bool operator()(const Node* lhs, const Node* rhs) {
      return lhs->key_ == rhs->key_;
    }
  };

  struct Hash {
    uint64_t operator()(const Node* node) {
      return std::hash<uint64_t>()(node->key_);
    }
  };

  static void ClearNode(Node* node) {}

  EvictableHashTable<Node, Hash, Equal> map_;
};

TEST_F(HashTableTest, TestInsert) {
  const uint64_t max_keys = 1024 * 1024;

//插入
  for (uint64_t k = 0; k < max_keys; ++k) {
    map_.Insert(Node(k, std::string(1000, k % 255)));
  }

//验证
  for (uint64_t k = 0; k < max_keys; ++k) {
    Node val;
    port::RWMutex* rlock = nullptr;
    assert(map_.Find(Node(k), &val, &rlock));
    rlock->ReadUnlock();
    assert(val.val_ == std::string(1000, k % 255));
  }
}

TEST_F(HashTableTest, TestErase) {
  const uint64_t max_keys = 1024 * 1024;
//插入
  for (uint64_t k = 0; k < max_keys; ++k) {
    map_.Insert(Node(k, std::string(1000, k % 255)));
  }

  auto rand = Random64(time(nullptr));
//随机删除几个键
  std::set<uint64_t> erased;
  for (int i = 0; i < 1024; ++i) {
    uint64_t k = rand.Next() % max_keys;
    if (erased.find(k) != erased.end()) {
      continue;
    }
    /*ert（映射擦除（节点（k），/*ret=*/nullptr））；
    删除。插入（k）；
  }

  /验证
  对于（uint64_t k=0；k<max_keys；+k）
    节点瓦尔；
    端口：：rwmutex*rlock=nullptr；
    bool status=映射查找（node（k），&val，&rlock）；
    if（eraged.find（k）==eraged.end（））
      断言（状态）；
      rlock->readunlock（）；
      断言（val.val==std:：string（1000，k%255））；
    }否则{
      断言（！）状态）；
    }
  }
}

测试_F（可收回性分析表测试，测试收回）
  const uint64_t max_keys=1024*1024；

  //插入
  对于（uint64_t k=0；k<max_keys；+k）
    映射插入（新节点（k，std:：string（1000，k%255））；
  }

  /验证
  对于（uint64_t k=0；k<max_keys；+k）
    node*val=map_.evict（）；
    //不幸的是，我们无法预测逐出值，因为它来自
    //锁条
    断言（VAL）；
    断言（val->val_==std:：string（1000，val->key_255））；
    删除VALL；
  }
}

//命名空间rocksdb
第二节

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}
