
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

#include <rocksdb/compaction_filter.h>
#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/options.h>

class MyMerge : public rocksdb::MergeOperator {
 public:
  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
    merge_out->new_value.clear();
    if (merge_in.existing_value != nullptr) {
      merge_out->new_value.assign(merge_in.existing_value->data(),
                                  merge_in.existing_value->size());
    }
    for (const rocksdb::Slice& m : merge_in.operand_list) {
      fprintf(stderr, "Merge(%s)\n", m.ToString().c_str());
//压缩过滤器过滤掉坏值
      assert(m.ToString() != "bad");
      merge_out->new_value.assign(m.data(), m.size());
    }
    return true;
  }

  const char* Name() const override { return "MyMerge"; }
};

class MyFilter : public rocksdb::CompactionFilter {
 public:
  bool Filter(int level, const rocksdb::Slice& key,
              const rocksdb::Slice& existing_value, std::string* new_value,
              bool* value_changed) const override {
    fprintf(stderr, "Filter(%s)\n", key.ToString().c_str());
    ++count_;
    assert(*value_changed == false);
    return false;
  }

  bool FilterMergeOperand(int level, const rocksdb::Slice& key,
                          const rocksdb::Slice& existing_value) const override {
    fprintf(stderr, "FilterMerge(%s)\n", key.ToString().c_str());
    ++merge_count_;
    return existing_value == "bad";
  }

  const char* Name() const override { return "MyFilter"; }

  mutable int count_ = 0;
  mutable int merge_count_ = 0;
};

int main() {
  rocksdb::DB* raw_db;
  rocksdb::Status status;

  MyFilter filter;

  int ret = system("rm -rf /tmp/rocksmergetest");
  if (ret != 0) {
    fprintf(stderr, "Error deleting /tmp/rocksmergetest, code: %d\n", ret);
    return ret;
  }
  rocksdb::Options options;
  options.create_if_missing = true;
  options.merge_operator.reset(new MyMerge);
  options.compaction_filter = &filter;
  status = rocksdb::DB::Open(options, "/tmp/rocksmergetest", &raw_db);
  assert(status.ok());
  std::unique_ptr<rocksdb::DB> db(raw_db);

  rocksdb::WriteOptions wopts;
db->Merge(wopts, "0", "bad");  //这个被过滤掉了
  db->Merge(wopts, "1", "data1");
  db->Merge(wopts, "1", "bad");
  db->Merge(wopts, "1", "data2");
  db->Merge(wopts, "1", "bad");
  db->Merge(wopts, "3", "data3");
  db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
  fprintf(stderr, "filter.count_ = %d\n", filter.count_);
  assert(filter.count_ == 0);
  fprintf(stderr, "filter.merge_count_ = %d\n", filter.merge_count_);
  assert(filter.merge_count_ == 6);
}
