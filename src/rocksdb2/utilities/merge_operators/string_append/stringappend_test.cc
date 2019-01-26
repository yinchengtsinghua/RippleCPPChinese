
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *持久映射：key->（字符串列表），使用rocksdb merge。
 *此文件是StringAppendOperator的测试工具/用例。
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 *版权所有2013 Facebook，Inc.
**/


#include <iostream>
#include <map>

#include "rocksdb/db.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/utilities/db_ttl.h"
#include "utilities/merge_operators.h"
#include "utilities/merge_operators/string_append/stringappend.h"
#include "utilities/merge_operators/string_append/stringappend2.h"
#include "util/testharness.h"
#include "util/random.h"

using namespace rocksdb;

namespace rocksdb {

//文件系统上数据库的路径
const std::string kDbName = test::TmpDir() + "/stringappend_test";

namespace {
//OpenDB使用StringAppendOperator打开（可能是新的）RockSDB数据库
std::shared_ptr<DB> OpenNormalDb(char delim_char) {
  DB* db;
  Options options;
  options.create_if_missing = true;
  options.merge_operator.reset(new StringAppendOperator(delim_char));
  EXPECT_OK(DB::Open(options, kDbName, &db));
  return std::shared_ptr<DB>(db);
}

#ifndef ROCKSDB_LITE  //Lite中不支持ttldb
//用非关联的StringAppendTestOperator打开ttldb
std::shared_ptr<DB> OpenTtlDb(char delim_char) {
  DBWithTTL* db;
  Options options;
  options.create_if_missing = true;
  options.merge_operator.reset(new StringAppendTESTOperator(delim_char));
  EXPECT_OK(DBWithTTL::Open(options, kDbName, &db, 123456));
  return std::shared_ptr<DB>(db);
}
#endif  //！摇滚乐
}  //命名空间

///string lists表示一组字符串列表，每个列表都有一个键索引。
///支持append（list，string）和get（list）
class StringLists {
 public:

//构造函数：指定rocksdb db
  /*隐性的*/
  StringLists(std::shared_ptr<DB> db)
      : db_(db),
        merge_option_(),
        get_option_() {
    assert(db);
  }

//将字符串val追加到键定义的列表中；成功时返回true
  bool Append(const std::string& key, const std::string& val){
    Slice valSlice(val.data(), val.size());
    auto s = db_->Merge(merge_option_, key, valSlice);

    if (s.ok()) {
      return true;
    } else {
      std::cerr << "ERROR " << s.ToString() << std::endl;
      return false;
    }
  }

//返回与键关联的字符串列表（如果不存在则返回“”）
  bool Get(const std::string& key, std::string* const result){
assert(result != nullptr); //我们应该有地方存放结果
    auto s = db_->Get(get_option_, key, result);

    if (s.ok()) {
      return true;
    }

//键不存在，或者存在错误。
*result = "";       //始终返回空字符串（仅用于约定）

//NotFound可以；只返回空值（类似于std:：map）
//但网络或数据库错误等，应通过测试（或至少叫喊）
    if (!s.IsNotFound()) {
      std::cerr << "ERROR " << s.ToString() << std::endl;
    }

//如果s.ok（）不是真的，则始终返回false
    return false;
  }


 private:
  std::shared_ptr<DB> db_;
  WriteOptions merge_option_;
  ReadOptions get_option_;

};


//单元测试类
class StringAppendOperatorTest : public testing::Test {
 public:
  StringAppendOperatorTest() {
DestroyDB(kDbName, Options());    //用新的数据库开始每个测试
  }

  typedef std::shared_ptr<DB> (* OpenFuncPtr)(char);

//允许用户打开具有不同配置的数据库。
//例如：可以打开数据库或TTLDB等。
  static void SetOpenDbFunction(OpenFuncPtr func) {
    OpenDb = func;
  }

 protected:
  static OpenFuncPtr OpenDb;
};
StringAppendOperatorTest::OpenFuncPtr StringAppendOperatorTest::OpenDb = nullptr;

//测试用例从这里开始

TEST_F(StringAppendOperatorTest, IteratorTest) {
  auto db_ = OpenDb(',');
  StringLists slists(db_);

  slists.Append("k1", "v1");
  slists.Append("k1", "v2");
  slists.Append("k1", "v3");

  slists.Append("k2", "a1");
  slists.Append("k2", "a2");
  slists.Append("k2", "a3");

  std::string res;
  std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(ReadOptions()));
  std::string k1("k1");
  std::string k2("k2");
  bool first = true;
  for (it->Seek(k1); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
      ASSERT_EQ(res, "v1,v2,v3");
      first = false;
    } else {
      ASSERT_EQ(res, "a1,a2,a3");
    }
  }
  slists.Append("k2", "a4");
  slists.Append("k1", "v4");

//快照应该仍然相同。应忽略A4和V4。
  first = true;
  for (it->Seek(k1); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
      ASSERT_EQ(res, "v1,v2,v3");
      first = false;
    } else {
      ASSERT_EQ(res, "a1,a2,a3");
    }
  }


//现在应该发布快照并了解新内容
  it.reset(db_->NewIterator(ReadOptions()));
  first = true;
  for (it->Seek(k1); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
      ASSERT_EQ(res, "v1,v2,v3,v4");
      first = false;
    } else {
      ASSERT_EQ(res, "a1,a2,a3,a4");
    }
  }

//这次从k2开始。
  for (it->Seek(k2); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
      ASSERT_EQ(res, "v1,v2,v3,v4");
      first = false;
    } else {
      ASSERT_EQ(res, "a1,a2,a3,a4");
    }
  }

  slists.Append("k3", "g1");

  it.reset(db_->NewIterator(ReadOptions()));
  first = true;
  std::string k3("k3");
  for(it->Seek(k2); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
      ASSERT_EQ(res, "a1,a2,a3,a4");
      first = false;
    } else {
      ASSERT_EQ(res, "g1");
    }
  }
  for(it->Seek(k3); it->Valid(); it->Next()) {
    res = it->value().ToString();
    if (first) {
//不应该被击中
      ASSERT_EQ(res, "a1,a2,a3,a4");
      first = false;
    } else {
      ASSERT_EQ(res, "g1");
    }
  }

}

TEST_F(StringAppendOperatorTest, SimpleTest) {
  auto db = OpenDb(',');
  StringLists slists(db);

  slists.Append("k1", "v1");
  slists.Append("k1", "v2");
  slists.Append("k1", "v3");

  std::string res;
  bool status = slists.Get("k1", &res);

  ASSERT_TRUE(status);
  ASSERT_EQ(res, "v1,v2,v3");
}

TEST_F(StringAppendOperatorTest, SimpleDelimiterTest) {
  auto db = OpenDb('|');
  StringLists slists(db);

  slists.Append("k1", "v1");
  slists.Append("k1", "v2");
  slists.Append("k1", "v3");

  std::string res;
  slists.Get("k1", &res);
  ASSERT_EQ(res, "v1|v2|v3");
}

TEST_F(StringAppendOperatorTest, OneValueNoDelimiterTest) {
  auto db = OpenDb('!');
  StringLists slists(db);

  slists.Append("random_key", "single_val");

  std::string res;
  slists.Get("random_key", &res);
  ASSERT_EQ(res, "single_val");
}

TEST_F(StringAppendOperatorTest, VariousKeys) {
  auto db = OpenDb('\n');
  StringLists slists(db);

  slists.Append("c", "asdasd");
  slists.Append("a", "x");
  slists.Append("b", "y");
  slists.Append("a", "t");
  slists.Append("a", "r");
  slists.Append("b", "2");
  slists.Append("c", "asdasd");

  std::string a, b, c;
  bool sa, sb, sc;
  sa = slists.Get("a", &a);
  sb = slists.Get("b", &b);
  sc = slists.Get("c", &c);

ASSERT_TRUE(sa && sb && sc); //这三把钥匙应该都找到了

  ASSERT_EQ(a, "x\nt\nr");
  ASSERT_EQ(b, "y\n2");
  ASSERT_EQ(c, "asdasd\nasdasd");
}

//从小的分布中生成半随机键/词。
TEST_F(StringAppendOperatorTest, RandomMixGetAppend) {
  auto db = OpenDb(' ');
  StringLists slists(db);

//生成随机键和值的列表
  const int kWordCount = 15;
  std::string words[] = {"sdasd", "triejf", "fnjsdfn", "dfjisdfsf", "342839",
                         "dsuha", "mabuais", "sadajsid", "jf9834hf", "2d9j89",
                         "dj9823jd", "a", "dk02ed2dh", "$(jd4h984$(*", "mabz"};
  const int kKeyCount = 6;
  std::string keys[] = {"dhaiusdhu", "denidw", "daisda", "keykey", "muki",
                        "shzassdianmd"};

//将存储所有数据的本地副本以验证正确性
  std::map<std::string, std::string> parallel_copy;

//生成一堆随机查询（追加和获取）！
  enum query_t  { APPEND_OP, GET_OP, NUM_OPS };
Random randomGen(1337);       //确定性种子；总是得到相同的结果！

  const int kNumQueries = 30;
  for (int q=0; q<kNumQueries; ++q) {
//生成随机查询（append或get）和随机参数
    query_t query = (query_t)randomGen.Uniform((int)NUM_OPS);
    std::string key = keys[randomGen.Uniform((int)kKeyCount)];
    std::string word = words[randomGen.Uniform((int)kWordCount)];

//应用查询和任何检查。
    if (query == APPEND_OP) {

//应用上面定义的RockSDB测试线束附加
slists.Append(key, word);  //应用rocksdb append

//将类似的“append”应用于并行副本
      if (parallel_copy[key].size() > 0) {
        parallel_copy[key] += " " + word;
      } else {
        parallel_copy[key] = word;
      }

    } else if (query == GET_OP) {
//假设不存在的键只返回<empty>
      std::string res;
      slists.Get(key, &res);
      ASSERT_EQ(res, parallel_copy[key]);
    }

  }

}

TEST_F(StringAppendOperatorTest, BIGRandomMixGetAppend) {
  auto db = OpenDb(' ');
  StringLists slists(db);

//生成随机键和值的列表
  const int kWordCount = 15;
  std::string words[] = {"sdasd", "triejf", "fnjsdfn", "dfjisdfsf", "342839",
                         "dsuha", "mabuais", "sadajsid", "jf9834hf", "2d9j89",
                         "dj9823jd", "a", "dk02ed2dh", "$(jd4h984$(*", "mabz"};
  const int kKeyCount = 6;
  std::string keys[] = {"dhaiusdhu", "denidw", "daisda", "keykey", "muki",
                        "shzassdianmd"};

//将存储所有数据的本地副本以验证正确性
  std::map<std::string, std::string> parallel_copy;

//生成一堆随机查询（追加和获取）！
  enum query_t  { APPEND_OP, GET_OP, NUM_OPS };
Random randomGen(9138204);       //确定性种子

  const int kNumQueries = 1000;
  for (int q=0; q<kNumQueries; ++q) {
//生成随机查询（append或get）和随机参数
    query_t query = (query_t)randomGen.Uniform((int)NUM_OPS);
    std::string key = keys[randomGen.Uniform((int)kKeyCount)];
    std::string word = words[randomGen.Uniform((int)kWordCount)];

//应用查询和任何检查。
    if (query == APPEND_OP) {

//应用上面定义的RockSDB测试线束附加
slists.Append(key, word);  //应用rocksdb append

//将类似的“append”应用于并行副本
      if (parallel_copy[key].size() > 0) {
        parallel_copy[key] += " " + word;
      } else {
        parallel_copy[key] = word;
      }

    } else if (query == GET_OP) {
//假设不存在的键只返回<empty>
      std::string res;
      slists.Get(key, &res);
      ASSERT_EQ(res, parallel_copy[key]);
    }

  }

}

TEST_F(StringAppendOperatorTest, PersistentVariousKeys) {
//在有限范围内执行以下操作
  {
    auto db = OpenDb('\n');
    StringLists slists(db);

    slists.Append("c", "asdasd");
    slists.Append("a", "x");
    slists.Append("b", "y");
    slists.Append("a", "t");
    slists.Append("a", "r");
    slists.Append("b", "2");
    slists.Append("c", "asdasd");

    std::string a, b, c;
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);

    ASSERT_EQ(a, "x\nt\nr");
    ASSERT_EQ(b, "y\n2");
    ASSERT_EQ(c, "asdasd\nasdasd");
  }

//重新打开数据库（以前的更改应保留/记住）
  {
    auto db = OpenDb('\n');
    StringLists slists(db);

    slists.Append("c", "bbnagnagsx");
    slists.Append("a", "sa");
    slists.Append("b", "df");
    slists.Append("a", "gh");
    slists.Append("a", "jk");
    slists.Append("b", "l;");
    slists.Append("c", "rogosh");

//以前的更改应该在磁盘上（l0）
//最近的更改应该在内存中（memtable）
//因此，这将测试两个get（）路径。
    std::string a, b, c;
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);

    ASSERT_EQ(a, "x\nt\nr\nsa\ngh\njk");
    ASSERT_EQ(b, "y\n2\ndf\nl;");
    ASSERT_EQ(c, "asdasd\nasdasd\nbbnagnagsx\nrogosh");
  }

//重新打开数据库（以前的更改应保留/记住）
  {
    auto db = OpenDb('\n');
    StringLists slists(db);

//所有更改都应在磁盘上。这将测试versionset get（）。
    std::string a, b, c;
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);

    ASSERT_EQ(a, "x\nt\nr\nsa\ngh\njk");
    ASSERT_EQ(b, "y\n2\ndf\nl;");
    ASSERT_EQ(c, "asdasd\nasdasd\nbbnagnagsx\nrogosh");
  }
}

TEST_F(StringAppendOperatorTest, PersistentFlushAndCompaction) {
//在有限范围内执行以下操作
  {
    auto db = OpenDb('\n');
    StringLists slists(db);
    std::string a, b, c;
    bool success;

//追加、刷新、获取
    slists.Append("c", "asdasd");
    db->Flush(rocksdb::FlushOptions());
    success = slists.Get("c", &c);
    ASSERT_TRUE(success);
    ASSERT_EQ(c, "asdasd");

//附加，刷新，附加，获取
    slists.Append("a", "x");
    slists.Append("b", "y");
    db->Flush(rocksdb::FlushOptions());
    slists.Append("a", "t");
    slists.Append("a", "r");
    slists.Append("b", "2");

    success = slists.Get("a", &a);
    assert(success == true);
    ASSERT_EQ(a, "x\nt\nr");

    success = slists.Get("b", &b);
    assert(success == true);
    ASSERT_EQ(b, "y\n2");

//附加，获取
    success = slists.Append("c", "asdasd");
    assert(success);
    success = slists.Append("b", "monkey");
    assert(success);

//这里我省略了“断言（成功）”检查。
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);

    ASSERT_EQ(a, "x\nt\nr");
    ASSERT_EQ(b, "y\n2\nmonkey");
    ASSERT_EQ(c, "asdasd\nasdasd");
  }

//重新打开数据库（以前的更改应保留/记住）
  {
    auto db = OpenDb('\n');
    StringLists slists(db);
    std::string a, b, c;

//GET（快速检查以前数据库的持久性）
    slists.Get("a", &a);
    ASSERT_EQ(a, "x\nt\nr");

//附加、压缩、获取
    slists.Append("c", "bbnagnagsx");
    slists.Append("a", "sa");
    slists.Append("b", "df");
    db->CompactRange(CompactRangeOptions(), nullptr, nullptr);
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);
    ASSERT_EQ(a, "x\nt\nr\nsa");
    ASSERT_EQ(b, "y\n2\nmonkey\ndf");
    ASSERT_EQ(c, "asdasd\nasdasd\nbbnagnagsx");

//附加，获取
    slists.Append("a", "gh");
    slists.Append("a", "jk");
    slists.Append("b", "l;");
    slists.Append("c", "rogosh");
    slists.Get("a", &a);
    slists.Get("b", &b);
    slists.Get("c", &c);
    ASSERT_EQ(a, "x\nt\nr\nsa\ngh\njk");
    ASSERT_EQ(b, "y\n2\nmonkey\ndf\nl;");
    ASSERT_EQ(c, "asdasd\nasdasd\nbbnagnagsx\nrogosh");

//紧凑，获得
    db->CompactRange(CompactRangeOptions(), nullptr, nullptr);
    ASSERT_EQ(a, "x\nt\nr\nsa\ngh\njk");
    ASSERT_EQ(b, "y\n2\nmonkey\ndf\nl;");
    ASSERT_EQ(c, "asdasd\nasdasd\nbbnagnagsx\nrogosh");

//追加、刷新、压缩、获取
    slists.Append("b", "afcg");
    db->Flush(rocksdb::FlushOptions());
    db->CompactRange(CompactRangeOptions(), nullptr, nullptr);
    slists.Get("b", &b);
    ASSERT_EQ(b, "y\n2\nmonkey\ndf\nl;\nafcg");
  }
}

TEST_F(StringAppendOperatorTest, SimpleTestNullDelimiter) {
  auto db = OpenDb('\0');
  StringLists slists(db);

  slists.Append("k1", "v1");
  slists.Append("k1", "v2");
  slists.Append("k1", "v3");

  std::string res;
  bool status = slists.Get("k1", &res);
  ASSERT_TRUE(status);

//构造所需的字符串。默认构造函数不喜欢'\0'字符。
std::string checker("v1,v2,v3");    //验证字符串的大小是否正确。
checker[2] = '\0';                  //使用空分隔符而不是逗号。
  checker[5] = '\0';
assert(checker.size() == 8);        //确认尺寸是否正确

//检查rocksdb结果字符串是否与所需字符串匹配
  assert(res.size() == checker.size());
  ASSERT_EQ(res, checker);
}

} //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
//使用常规数据库运行
  int result;
  {
    fprintf(stderr, "Running tests with regular db and operator.\n");
    StringAppendOperatorTest::SetOpenDbFunction(&OpenNormalDb);
    result = RUN_ALL_TESTS();
  }

#ifndef ROCKSDB_LITE  //Lite中不支持ttldb
//用TTL运行
  {
    fprintf(stderr, "Running tests with ttl db and generic operator.\n");
    StringAppendOperatorTest::SetOpenDbFunction(&OpenTtlDb);
    result |= RUN_ALL_TESTS();
  }
#endif  //！摇滚乐

  return result;
}
