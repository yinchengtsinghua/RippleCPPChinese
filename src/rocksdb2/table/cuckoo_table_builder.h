
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

#pragma once
#ifndef ROCKSDB_LITE
#include <stdint.h>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include "port/port.h"
#include "rocksdb/status.h"
#include "table/table_builder.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "util/autovector.h"

namespace rocksdb {

class CuckooTableBuilder: public TableBuilder {
 public:
  CuckooTableBuilder(WritableFileWriter* file, double max_hash_table_ratio,
                     uint32_t max_num_hash_func, uint32_t max_search_depth,
                     const Comparator* user_comparator,
                     uint32_t cuckoo_block_size, bool use_module_hash,
                     bool identity_as_first_hash,
                     uint64_t (*get_slice_hash)(const Slice&, uint32_t,
                                                uint64_t),
                     uint32_t column_family_id,
                     const std::string& column_family_name);

//要求：已调用finish（）或放弃（）。
  ~CuckooTableBuilder() {}

//向正在构造的表添加键和值。
//要求：根据比较器，键在任何先前添加的键之后。
//要求：finish（），放弃（）尚未调用
  void Add(const Slice& key, const Slice& value) override;

//如果检测到一些错误，返回非OK。
  Status status() const override { return status_; }

//把桌子盖好。停止使用传递给
//此函数后的构造函数返回。
//要求：finish（），放弃（）尚未调用
  Status Finish() override;

//指示应放弃此生成器的内容。停止
//使用此函数返回后传递给构造函数的文件。
//如果调用者不想调用finish（），它必须调用放弃（）。
//在摧毁这个建造者之前。
//要求：finish（），放弃（）尚未调用
  void Abandon() override;

//到目前为止，要添加的调用数（）。
  uint64_t NumEntries() const override;

//到目前为止生成的文件的大小。如果在成功后调用
//finish（）调用，返回最终生成文件的大小。
  uint64_t FileSize() const override;

  TableProperties GetTableProperties() const override { return properties_; }

 private:
  struct CuckooBucket {
    CuckooBucket()
      : vector_idx(kMaxVectorIdx), make_space_for_key_call_id(0) {}
    uint32_t vector_idx;
//这个数字不会超过kvs_.size（）+max_num_hash_func_uu。
//我们假设项目数量<=2^32。
    uint32_t make_space_for_key_call_id;
  };
  static const uint32_t kMaxVectorIdx = port::kMaxInt32;

  bool MakeSpaceForKey(const autovector<uint64_t>& hash_vals,
                       const uint32_t call_id,
                       std::vector<CuckooBucket>* buckets, uint64_t* bucket_id);
  Status MakeHashTable(std::vector<CuckooBucket>* buckets);

  inline bool IsDeletedKey(uint64_t idx) const;
  inline Slice GetKey(uint64_t idx) const;
  inline Slice GetUserKey(uint64_t idx) const;
  inline Slice GetValue(uint64_t idx) const;

  uint32_t num_hash_func_;
  WritableFileWriter* file_;
  const double max_hash_table_ratio_;
  const uint32_t max_num_hash_func_;
  const uint32_t max_search_depth_;
  const uint32_t cuckoo_block_size_;
  uint64_t hash_table_size_;
  bool is_last_level_file_;
  bool has_seen_first_key_;
  bool has_seen_first_value_;
  uint64_t key_size_;
  uint64_t value_size_;
//连接成字符串的固定大小键值对的列表。
//使用getkey（）、getuserkey（）和getvalue（）检索特定的
//给定索引的键/值
  std::string kvs_;
  std::string deleted_keys_;
//存储在kvs中的键值对数\已删除的键数
  uint64_t num_entries_;
//包含值的键数（非删除操作）
  uint64_t num_values_;
  Status status_;
  TableProperties properties_;
  const Comparator* ucomp_;
  bool use_module_hash_;
  bool identity_as_first_hash_;
  uint64_t (*get_slice_hash_)(const Slice& s, uint32_t index,
    uint64_t max_num_buckets);
  std::string largest_user_key_ = "";
  std::string smallest_user_key_ = "";

bool closed_;  //已调用finish（）或放弃（）。

//不允许复制
  CuckooTableBuilder(const CuckooTableBuilder&) = delete;
  void operator=(const CuckooTableBuilder&) = delete;
};

}  //命名空间rocksdb

#endif  //摇滚乐
