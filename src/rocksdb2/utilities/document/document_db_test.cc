
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

#ifndef ROCKSDB_LITE

#include <algorithm>

#include "rocksdb/utilities/json_document.h"
#include "rocksdb/utilities/document_db.h"

#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

class DocumentDBTest : public testing::Test {
 public:
  DocumentDBTest() {
    dbname_ = test::TmpDir() + "/document_db_test";
    DestroyDB(dbname_, Options());
  }
  ~DocumentDBTest() {
    delete db_;
    DestroyDB(dbname_, Options());
  }

  void AssertCursorIDs(Cursor* cursor, std::vector<int64_t> expected) {
    std::vector<int64_t> got;
    while (cursor->Valid()) {
      ASSERT_TRUE(cursor->Valid());
      ASSERT_TRUE(cursor->document().Contains("_id"));
      got.push_back(cursor->document()["_id"].GetInt64());
      cursor->Next();
    }
    std::sort(expected.begin(), expected.end());
    std::sort(got.begin(), got.end());
    ASSERT_TRUE(got == expected);
  }

//把“to”转换成“to”，这样我们就不必“到处跑”。
  std::string ConvertQuotes(const std::string& input) {
    std::string output;
    for (auto x : input) {
      if (x == '\'') {
        output.push_back('\"');
      } else {
        output.push_back(x);
      }
    }
    return output;
  }

  void CreateIndexes(std::vector<DocumentDB::IndexDescriptor> indexes) {
    for (auto i : indexes) {
      ASSERT_OK(db_->CreateIndex(WriteOptions(), i));
    }
  }

  JSONDocument* Parse(const std::string& doc) {
    return JSONDocument::ParseJSON(ConvertQuotes(doc).c_str());
  }

  std::string dbname_;
  DocumentDB* db_;
};

TEST_F(DocumentDBTest, SimpleQueryTest) {
  DocumentDBOptions options;
  DocumentDB::IndexDescriptor index;
  index.description = Parse("{\"name\": 1}");
  index.name = "name_index";

  ASSERT_OK(DocumentDB::Open(options, dbname_, {}, &db_));
  CreateIndexes({index});
  delete db_;
//现在有了索引
  ASSERT_OK(DocumentDB::Open(options, dbname_, {index}, &db_));
  delete index.description;

  std::vector<std::string> json_objects = {
      "{\"_id\': 1, \"name\": \"One\"}",   "{\"_id\": 2, \"name\": \"Two\"}",
      "{\"_id\": 3, \"name\": \"Three\"}", "{\"_id\": 4, \"name\": \"Four\"}"};

  for (auto& json : json_objects) {
    std::unique_ptr<JSONDocument> document(Parse(json));
    ASSERT_TRUE(document.get() != nullptr);
    ASSERT_OK(db_->Insert(WriteOptions(), *document));
  }

//插入具有现有主键的文档将返回失败
  {
    std::unique_ptr<JSONDocument> document(Parse(json_objects[0]));
    ASSERT_TRUE(document.get() != nullptr);
    Status s = db_->Insert(WriteOptions(), *document);
    ASSERT_TRUE(s.IsInvalidArgument());
  }

//等于“二”
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'name': 'Two', '$index': 'name_index'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {2});
  }

//找到少于“三”的
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'name': {'$lt': 'Three'}, '$index': "
        "'name_index'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));

    AssertCursorIDs(cursor.get(), {1, 4});
  }

//查找不到索引的小于“三”
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'name': {'$lt': 'Three'} }}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {1, 4});
  }

//删除小于或等于“三”
  {
    std::unique_ptr<JSONDocument> query(
        Parse("{'name': {'$lte': 'Three'}, '$index': 'name_index'}"));
    ASSERT_OK(db_->Remove(ReadOptions(), WriteOptions(), *query));
  }

//全部查找--只剩下“两个”，其他所有内容都应删除
  {
    std::unique_ptr<JSONDocument> query(Parse("[]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {2});
  }
}

TEST_F(DocumentDBTest, ComplexQueryTest) {
  DocumentDBOptions options;
  DocumentDB::IndexDescriptor priority_index;
  priority_index.description = Parse("{'priority': 1}");
  priority_index.name = "priority";
  DocumentDB::IndexDescriptor job_name_index;
  job_name_index.description = Parse("{'job_name': 1}");
  job_name_index.name = "job_name";
  DocumentDB::IndexDescriptor progress_index;
  progress_index.description = Parse("{'progress': 1}");
  progress_index.name = "progress";

  ASSERT_OK(DocumentDB::Open(options, dbname_, {}, &db_));
  CreateIndexes({priority_index, progress_index});
  delete priority_index.description;
  delete progress_index.description;

  std::vector<std::string> json_objects = {
      "{'_id': 1, 'job_name': 'play', 'priority': 10, 'progress': 14.2}",
      "{'_id': 2, 'job_name': 'white', 'priority': 2, 'progress': 45.1}",
      "{'_id': 3, 'job_name': 'straw', 'priority': 5, 'progress': 83.2}",
      "{'_id': 4, 'job_name': 'temporary', 'priority': 3, 'progress': 14.9}",
      "{'_id': 5, 'job_name': 'white', 'priority': 4, 'progress': 44.2}",
      "{'_id': 6, 'job_name': 'tea', 'priority': 1, 'progress': 12.4}",
      "{'_id': 7, 'job_name': 'delete', 'priority': 2, 'progress': 77.54}",
      "{'_id': 8, 'job_name': 'rock', 'priority': 3, 'progress': 93.24}",
      "{'_id': 9, 'job_name': 'steady', 'priority': 3, 'progress': 9.1}",
      "{'_id': 10, 'job_name': 'white', 'priority': 1, 'progress': 61.4}",
      "{'_id': 11, 'job_name': 'who', 'priority': 4, 'progress': 39.41}",
      "{'_id': 12, 'job_name': 'who', 'priority': -1, 'progress': 39.42}",
      "{'_id': 13, 'job_name': 'who', 'priority': -2, 'progress': 39.42}", };

//快速添加索引！
  CreateIndexes({job_name_index});
  delete job_name_index.description;

  for (auto& json : json_objects) {
    std::unique_ptr<JSONDocument> document(Parse(json));
    ASSERT_TRUE(document != nullptr);
    ASSERT_OK(db_->Insert(WriteOptions(), *document));
  }

//2<priority<4，progress>10.0，index priority
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'priority': {'$lt': 4, '$gt': 2}, 'progress': {'$gt': "
        "10.0}, '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {4, 8});
  }

//-1<=优先级<=1，索引优先级
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'priority': {'$lte': 1, '$gte': -1},"
        " '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {6, 10, 12});
  }

//2<priority<4，progress>10.0，index progress
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'priority': {'$lt': 4, '$gt': 2}, 'progress': {'$gt': "
        "10.0}, '$index': 'progress'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {4, 8});
  }

//job_name='white'和priority>=2，索引job_name
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'job_name': 'white', 'priority': {'$gte': "
        "2}, '$index': 'job_name'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {2, 5});
  }

//35.0<=进度<65.5，指标进度
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'progress': {'$gt': 5.0, '$gte': 35.0, '$lt': 65.5}, "
        "'$index': 'progress'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {2, 5, 10, 11, 12, 13});
  }

//2<优先级<=4，索引优先级
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'priority': {'$gt': 2, '$lt': 8, '$lte': 4}, "
        "'$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {4, 5, 8, 9, 11});
  }

//删除所有进度大于50%的项目
  {
    std::unique_ptr<JSONDocument> query(
        Parse("{'progress': {'$gt': 50.0}, '$index': 'progress'}"));
    ASSERT_OK(db_->Remove(ReadOptions(), WriteOptions(), *query));
  }

//2<优先级<6，索引优先级
  {
    std::unique_ptr<JSONDocument> query(Parse(
        "[{'$filter': {'priority': {'$gt': 2, '$lt': 6}, "
        "'$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    AssertCursorIDs(cursor.get(), {4, 5, 9, 11});
  }

//更新将优先级设置为10，其中作业名称为“白色”
  {
    std::unique_ptr<JSONDocument> query(Parse("{'job_name': 'white'}"));
    std::unique_ptr<JSONDocument> update(Parse("{'$set': {'priority': 10}}"));
    ASSERT_OK(db_->Update(ReadOptions(), WriteOptions(), *query, *update));
  }

//更新两次：将优先级设置为15，其中作业名称为“白色”
  {
    std::unique_ptr<JSONDocument> query(Parse("{'job_name': 'white'}"));
    std::unique_ptr<JSONDocument> update(Parse("{'$set': {'priority': 10},"
                                               "'$set': {'priority': 15}}"));
    ASSERT_OK(db_->Update(ReadOptions(), WriteOptions(), *query, *update));
  }

//更新两次：将优先级设置为15和
//工作名称为“白色”时，进展到40
  {
    std::unique_ptr<JSONDocument> query(Parse("{'job_name': 'white'}"));
    std::unique_ptr<JSONDocument> update(
        Parse("{'$set': {'priority': 10, 'progress': 35},"
              "'$set': {'priority': 15, 'progress': 40}}"));
    ASSERT_OK(db_->Update(ReadOptions(), WriteOptions(), *query, *update));
  }

//优先级＜0
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'priority': {'$lt': 0}, '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    ASSERT_OK(cursor->status());
    AssertCursorIDs(cursor.get(), {12, 13});
  }

//-2<优先级<0
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'priority': {'$gt': -2, '$lt': 0},"
        " '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    ASSERT_OK(cursor->status());
    AssertCursorIDs(cursor.get(), {12});
  }

//-2<=优先级<0
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'priority': {'$gte': -2, '$lt': 0},"
        " '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    ASSERT_OK(cursor->status());
    AssertCursorIDs(cursor.get(), {12, 13});
  }

//4优先
  {
    std::unique_ptr<JSONDocument> query(
        Parse("[{'$filter': {'priority': {'$gt': 4}, '$index': 'priority'}}]"));
    std::unique_ptr<Cursor> cursor(db_->Query(ReadOptions(), *query));
    ASSERT_OK(cursor->status());
    AssertCursorIDs(cursor.get(), {1, 2, 5});
  }

  Status s = db_->DropIndex("doesnt-exist");
  ASSERT_TRUE(!s.ok());
  ASSERT_OK(db_->DropIndex("priority"));
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as DocumentDB is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
