
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#include <stdio.h>

#if !defined(ROCKSDB_LITE)

#if defined(LUA)

#include <string>

#include "db/db_test_util.h"
#include "port/stack_trace.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/lua/rocks_lua_compaction_filter.h"
#include "util/testharness.h"

namespace rocksdb {

class StopOnErrorLogger : public Logger {
 public:
  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    FAIL();
  }
};


class RocksLuaTest : public testing::Test {
 public:
  RocksLuaTest() : rnd_(301) {
    temp_dir_ = test::TmpDir(Env::Default());
    db_ = nullptr;
  }

  std::string RandomString(int len) {
    std::string res;
    for (int i = 0; i < len; ++i) {
      res += rnd_.Uniform(26) + 'a';
    }
    return res;
  }

  void CreateDBWithLuaCompactionFilter(
      const lua::RocksLuaCompactionFilterOptions& lua_opt,
      const std::string& db_path,
      std::unordered_map<std::string, std::string>* kvs,
      const int kNumFlushes = 5,
      std::shared_ptr<rocksdb::lua::RocksLuaCompactionFilterFactory>*
          output_factory = nullptr) {
    const int kKeySize = 10;
    const int kValueSize = 50;
    const int kKeysPerFlush = 2;
    auto factory =
        std::make_shared<rocksdb::lua::RocksLuaCompactionFilterFactory>(
            lua_opt);
    if (output_factory != nullptr) {
      *output_factory = factory;
    }

    options_ = Options();
    options_.create_if_missing = true;
    options_.compaction_filter_factory = factory;
    options_.disable_auto_compactions = true;
    options_.max_bytes_for_level_base =
        (kKeySize + kValueSize) * kKeysPerFlush * 2;
    options_.max_bytes_for_level_multiplier = 2;
    options_.target_file_size_base = (kKeySize + kValueSize) * kKeysPerFlush;
    options_.level0_file_num_compaction_trigger = 2;
    DestroyDB(db_path, options_);
    ASSERT_OK(DB::Open(options_, db_path, &db_));

    for (int f = 0; f < kNumFlushes; ++f) {
      for (int i = 0; i < kKeysPerFlush; ++i) {
        std::string key = RandomString(kKeySize);
        std::string value = RandomString(kValueSize);
        kvs->insert({key, value});
        ASSERT_OK(db_->Put(WriteOptions(), key, value));
      }
      db_->Flush(FlushOptions());
    }
  }

  ~RocksLuaTest() {
    if (db_) {
      delete db_;
    }
  }
  std::string temp_dir_;
  DB* db_;
  Random rnd_;
  Options options_;
};

TEST_F(RocksLuaTest, Default) {
//如果luacompletionfilterOptions中未设置任何内容，则
//rocksdb将保留所有键/值对，但它也将
//打印故障日志。
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);

  for (auto const& entry : kvs) {
    std::string value;
    ASSERT_OK(db_->Get(ReadOptions(), entry.first, &value));
    ASSERT_EQ(value, entry.second);
  }
}

TEST_F(RocksLuaTest, KeepsAll) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//保留所有键值对
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  return false, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"KeepsAll\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);

  for (auto const& entry : kvs) {
    std::string value;
    ASSERT_OK(db_->Get(ReadOptions(), entry.first, &value));
    ASSERT_EQ(value, entry.second);
  }
}

TEST_F(RocksLuaTest, GetName) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
  const std::string kScriptName = "SimpleLuaCompactionFilter";
  lua_opt.lua_script =
      std::string(
          "function Filter(level, key, existing_value)\n"
          "  return false, false, \"\"\n"
          "end\n"
          "\n"
          "function FilterMergeOperand(level, key, operand)\n"
          "  return false\n"
          "end\n"
          "function Name()\n"
          "  return \"") + kScriptName + "\"\n"
      "end\n"
      "\n";

  std::shared_ptr<CompactionFilterFactory> factory =
      std::make_shared<lua::RocksLuaCompactionFilterFactory>(lua_opt);
  std::string factory_name(factory->Name());
  ASSERT_NE(factory_name.find(kScriptName), std::string::npos);
}

TEST_F(RocksLuaTest, RemovesAll) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//删除所有键值对
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  return true, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"RemovesAll\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }
}

TEST_F(RocksLuaTest, FilterByKey) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//删除初始值小于“r”的所有键
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  if key:sub(1,1) < 'r' then\n"
      "    return true, false, \"\"\n"
      "  end\n"
      "  return false, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"KeepsAll\"\n"
      "end\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    if (entry.first[0] < 'r') {
      ASSERT_TRUE(s.IsNotFound());
    } else {
      ASSERT_TRUE(s.ok());
      ASSERT_TRUE(value == entry.second);
    }
  }
}

TEST_F(RocksLuaTest, FilterByValue) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//删除初始值小于“r”的所有值
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  if existing_value:sub(1,1) < 'r' then\n"
      "    return true, false, \"\"\n"
      "  end\n"
      "  return false, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"FilterByValue\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    if (entry.second[0] < 'r') {
      ASSERT_TRUE(s.IsNotFound());
    } else {
      ASSERT_TRUE(s.ok());
      ASSERT_EQ(value, entry.second);
    }
  }
}

TEST_F(RocksLuaTest, ChangeValue) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//用反转键替换所有值
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  return false, true, key:reverse()\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"ChangeValue\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    ASSERT_OK(db_->Get(ReadOptions(), entry.first, &value));
    std::string new_value = entry.first;
    std::reverse(new_value.begin(), new_value.end());
    ASSERT_EQ(value, new_value);
  }
}

TEST_F(RocksLuaTest, ConditionallyChangeAndFilterValue) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//执行以下逻辑：
//if键[0]<“h”->用reverse键替换值
//if key[0]>='R'->保留原始键值
//否则，过滤键值
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  if key:sub(1,1) < 'h' then\n"
      "    return false, true, key:reverse()\n"
      "  elseif key:sub(1,1) < 'r' then\n"
      "    return true, false, \"\"\n"
      "  end\n"
      "  return false, false, \"\"\n"
      "end\n"
      "function Name()\n"
      "  return \"ConditionallyChangeAndFilterValue\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs);
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    if (entry.first[0] < 'h') {
      ASSERT_TRUE(s.ok());
      std::string new_value = entry.first;
      std::reverse(new_value.begin(), new_value.end());
      ASSERT_EQ(value, new_value);
    } else if (entry.first[0] < 'r') {
      ASSERT_TRUE(s.IsNotFound());
    } else {
      ASSERT_TRUE(s.ok());
      ASSERT_EQ(value, entry.second);
    }
  }
}

TEST_F(RocksLuaTest, DynamicChangeScript) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
  lua_opt.error_log = std::make_shared<StopOnErrorLogger>();
//保留所有键值对
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  return false, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"KeepsAll\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
  std::shared_ptr<rocksdb::lua::RocksLuaCompactionFilterFactory> factory;
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs, 30, &factory);
  uint64_t count = 0;
  ASSERT_TRUE(db_->GetIntProperty(
      rocksdb::DB::Properties::kNumEntriesActiveMemTable, &count));
  ASSERT_EQ(count, 0);
  ASSERT_TRUE(db_->GetIntProperty(
      rocksdb::DB::Properties::kNumEntriesImmMemTables, &count));
  ASSERT_EQ(count, 0);

  CompactRangeOptions cr_opt;
  cr_opt.bottommost_level_compaction =
      rocksdb::BottommostLevelCompaction::kForce;

//发布完全压缩，并期望所有内容都在数据库中。
  ASSERT_OK(db_->CompactRange(cr_opt, nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    ASSERT_OK(db_->Get(ReadOptions(), entry.first, &value));
    ASSERT_EQ(value, entry.second);
  }

//更改lua脚本以删除所有键值对
  factory->SetScript(
      "function Filter(level, key, existing_value)\n"
      "  return true, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"RemovesAll\"\n"
      "end\n"
      "\n");
  {
    std::string key = "another-key";
    std::string value = "another-value";
    kvs.insert({key, value});
    ASSERT_OK(db_->Put(WriteOptions(), key, value));
    db_->Flush(FlushOptions());
  }

  cr_opt.change_level = true;
  cr_opt.target_level = 5;
//发布完全压缩，数据库中不存在任何内容。
  ASSERT_OK(db_->CompactRange(cr_opt, nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    ASSERT_TRUE(s.IsNotFound());
  }
}

TEST_F(RocksLuaTest, LuaConditionalTypeError) {
  std::string db_path = temp_dir_ + "/rocks_lua_test";

  lua::RocksLuaCompactionFilterOptions lua_opt;
//filter（）输入键的初始值大于等于“r”时出错
  lua_opt.lua_script =
      "function Filter(level, key, existing_value)\n"
      "  if existing_value:sub(1,1) >= 'r' then\n"
      "    return true, 2, \"\" -- incorrect type of 2nd return value\n"
      "  end\n"
      "  return true, false, \"\"\n"
      "end\n"
      "\n"
      "function FilterMergeOperand(level, key, operand)\n"
      "  return false\n"
      "end\n"
      "function Name()\n"
      "  return \"BuggyCode\"\n"
      "end\n"
      "\n";

  std::unordered_map<std::string, std::string> kvs;
//创建包含10个文件的数据库
  CreateDBWithLuaCompactionFilter(lua_opt, db_path, &kvs, 10);

//发出完全压缩，并期望初始值<“r”的所有键
//将被删除，因为我们在出错时保留键值。
  ASSERT_OK(db_->CompactRange(CompactRangeOptions(), nullptr, nullptr));

  for (auto const& entry : kvs) {
    std::string value;
    auto s = db_->Get(ReadOptions(), entry.first, &value);
    if (entry.second[0] < 'r') {
      ASSERT_TRUE(s.IsNotFound());
    } else {
      ASSERT_TRUE(s.ok());
      ASSERT_EQ(value, entry.second);
    }
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  rocksdb::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else

int main(int argc, char** argv) {
  printf("LUA_PATH is not set.  Ignoring the test.\n");
}

#endif  //定义（LUA）

#else

int main(int argc, char** argv) {
  printf("Lua is not supported in RocksDBLite.  Ignoring the test.\n");
}

#endif  //！已定义（RocksDB-Lite）
