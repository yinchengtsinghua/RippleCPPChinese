
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
#include "table/cuckoo_table_builder.h"

#include <assert.h>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "db/dbformat.h"
#include "rocksdb/env.h"
#include "rocksdb/table.h"
#include "table/block_builder.h"
#include "table/cuckoo_table_factory.h"
#include "table/format.h"
#include "table/meta_blocks.h"
#include "util/autovector.h"
#include "util/file_reader_writer.h"
#include "util/random.h"
#include "util/string_util.h"

namespace rocksdb {
const std::string CuckooTablePropertyNames::kEmptyKey =
      "rocksdb.cuckoo.bucket.empty.key";
const std::string CuckooTablePropertyNames::kNumHashFunc =
      "rocksdb.cuckoo.hash.num";
const std::string CuckooTablePropertyNames::kHashTableSize =
      "rocksdb.cuckoo.hash.size";
const std::string CuckooTablePropertyNames::kValueLength =
      "rocksdb.cuckoo.value.length";
const std::string CuckooTablePropertyNames::kIsLastLevel =
      "rocksdb.cuckoo.file.islastlevel";
const std::string CuckooTablePropertyNames::kCuckooBlockSize =
      "rocksdb.cuckoo.hash.cuckooblocksize";
const std::string CuckooTablePropertyNames::kIdentityAsFirstHash =
      "rocksdb.cuckoo.hash.identityfirst";
const std::string CuckooTablePropertyNames::kUseModuleHash =
      "rocksdb.cuckoo.hash.usemodule";
const std::string CuckooTablePropertyNames::kUserKeyLength =
      "rocksdb.cuckoo.hash.userkeylength";

//通过运行echo rocksdb.table.cuckoo_sha1sum获得
extern const uint64_t kCuckooTableMagicNumber = 0x926789d0c5f17873ull;

CuckooTableBuilder::CuckooTableBuilder(
    WritableFileWriter* file, double max_hash_table_ratio,
    uint32_t max_num_hash_table, uint32_t max_search_depth,
    const Comparator* user_comparator, uint32_t cuckoo_block_size,
    bool use_module_hash, bool identity_as_first_hash,
    uint64_t (*get_slice_hash)(const Slice&, uint32_t, uint64_t),
    uint32_t column_family_id, const std::string& column_family_name)
    : num_hash_func_(2),
      file_(file),
      max_hash_table_ratio_(max_hash_table_ratio),
      max_num_hash_func_(max_num_hash_table),
      max_search_depth_(max_search_depth),
      cuckoo_block_size_(std::max(1U, cuckoo_block_size)),
      hash_table_size_(use_module_hash ? 0 : 2),
      is_last_level_file_(false),
      has_seen_first_key_(false),
      has_seen_first_value_(false),
      key_size_(0),
      value_size_(0),
      num_entries_(0),
      num_values_(0),
      ucomp_(user_comparator),
      use_module_hash_(use_module_hash),
      identity_as_first_hash_(identity_as_first_hash),
      get_slice_hash_(get_slice_hash),
      closed_(false) {
//数据在一个巨大的块中。
  properties_.num_data_blocks = 1;
  properties_.index_size = 0;
  properties_.filter_size = 0;
  properties_.column_family_id = column_family_id;
  properties_.column_family_name = column_family_name;
}

void CuckooTableBuilder::Add(const Slice& key, const Slice& value) {
  if (num_entries_ >= kMaxVectorIdx - 1) {
    status_ = Status::NotSupported("Number of keys in a file must be < 2^32-1");
    return;
  }
  ParsedInternalKey ikey;
  if (!ParseInternalKey(key, &ikey)) {
    status_ = Status::Corruption("Unable to parse key into inernal key.");
    return;
  }
  if (ikey.type != kTypeDeletion && ikey.type != kTypeValue) {
    status_ = Status::NotSupported("Unsupported key type " +
                                   ToString(ikey.type));
    return;
  }

//确定是否可以忽略序列号和值类型
//通过查看第一个键的序列号来获得内部键。我们假设
//如果第一个键有一个零序号，那么剩下的所有键
//键将具有零序列。不。
  if (!has_seen_first_key_) {
    is_last_level_file_ = ikey.sequence == 0;
    has_seen_first_key_ = true;
    smallest_user_key_.assign(ikey.user_key.data(), ikey.user_key.size());
    largest_user_key_.assign(ikey.user_key.data(), ikey.user_key.size());
    key_size_ = is_last_level_file_ ? ikey.user_key.size() : key.size();
  }
  if (key_size_ != (is_last_level_file_ ? ikey.user_key.size() : key.size())) {
    status_ = Status::NotSupported("all keys have to be the same size");
    return;
  }

  if (ikey.type == kTypeValue) {
    if (!has_seen_first_value_) {
      has_seen_first_value_ = true;
      value_size_ = value.size();
    }
    if (value_size_ != value.size()) {
      status_ = Status::NotSupported("all values have to be the same size");
      return;
    }

    if (is_last_level_file_) {
      kvs_.append(ikey.user_key.data(), ikey.user_key.size());
    } else {
      kvs_.append(key.data(), key.size());
    }
    kvs_.append(value.data(), value.size());
    ++num_values_;
  } else {
    if (is_last_level_file_) {
      deleted_keys_.append(ikey.user_key.data(), ikey.user_key.size());
    } else {
      deleted_keys_.append(key.data(), key.size());
    }
  }
  ++num_entries_;

//为了填充哈希表中的空桶，我们将
//尚未使用的密钥（未使用的用户密钥）。我们的决定是
//保持迄今为止按顺序插入的最小和最大键
//并使用它们在finish（）操作中查找超出此范围的键。
//注意，这个策略独立于这里使用的用户比较器。
  if (ikey.user_key.compare(smallest_user_key_) < 0) {
    smallest_user_key_.assign(ikey.user_key.data(), ikey.user_key.size());
  } else if (ikey.user_key.compare(largest_user_key_) > 0) {
    largest_user_key_.assign(ikey.user_key.data(), ikey.user_key.size());
  }
  if (!use_module_hash_) {
    if (hash_table_size_ < num_entries_ / max_hash_table_ratio_) {
      hash_table_size_ *= 2;
    }
  }
}

bool CuckooTableBuilder::IsDeletedKey(uint64_t idx) const {
  assert(closed_);
  return idx >= num_values_;
}

Slice CuckooTableBuilder::GetKey(uint64_t idx) const {
  assert(closed_);
  if (IsDeletedKey(idx)) {
    return Slice(&deleted_keys_[(idx - num_values_) * key_size_], key_size_);
  }
  return Slice(&kvs_[idx * (key_size_ + value_size_)], key_size_);
}

Slice CuckooTableBuilder::GetUserKey(uint64_t idx) const {
  assert(closed_);
  return is_last_level_file_ ? GetKey(idx) : ExtractUserKey(GetKey(idx));
}

Slice CuckooTableBuilder::GetValue(uint64_t idx) const {
  assert(closed_);
  if (IsDeletedKey(idx)) {
    static std::string empty_value(value_size_, 'a');
    return Slice(empty_value);
  }
  return Slice(&kvs_[idx * (key_size_ + value_size_) + key_size_], value_size_);
}

Status CuckooTableBuilder::MakeHashTable(std::vector<CuckooBucket>* buckets) {
  buckets->resize(hash_table_size_ + cuckoo_block_size_ - 1);
  uint32_t make_space_for_key_call_id = 0;
  for (uint32_t vector_idx = 0; vector_idx < num_entries_; vector_idx++) {
    uint64_t bucket_id;
    bool bucket_found = false;
    autovector<uint64_t> hash_vals;
    Slice user_key = GetUserKey(vector_idx);
    for (uint32_t hash_cnt = 0; hash_cnt < num_hash_func_ && !bucket_found;
        ++hash_cnt) {
      uint64_t hash_val = CuckooHash(user_key, hash_cnt, use_module_hash_,
          hash_table_size_, identity_as_first_hash_, get_slice_hash_);
//如果发生碰撞，检查下一个布谷鸟块大小位置
//空位置。检查时，如果我们到达哈希表的末尾，
//停止搜索并继续下一个哈希函数。
      for (uint32_t block_idx = 0; block_idx < cuckoo_block_size_;
          ++block_idx, ++hash_val) {
        if ((*buckets)[hash_val].vector_idx == kMaxVectorIdx) {
          bucket_id = hash_val;
          bucket_found = true;
          break;
        } else {
          if (ucomp_->Compare(user_key,
                GetUserKey((*buckets)[hash_val].vector_idx)) == 0) {
            return Status::NotSupported("Same key is being inserted again.");
          }
          hash_vals.push_back(hash_val);
        }
      }
    }
    while (!bucket_found && !MakeSpaceForKey(hash_vals,
          ++make_space_for_key_call_id, buckets, &bucket_id)) {
//通过增加哈希表的数量来重新刷新。
      if (num_hash_func_ >= max_num_hash_func_) {
        return Status::NotSupported("Too many collisions. Unable to hash.");
      }
//我们真的不需要重新粉刷整张桌子，因为旧的哈希表
//仍然有效，我们只增加了哈希函数的数量。
      uint64_t hash_val = CuckooHash(user_key, num_hash_func_, use_module_hash_,
          hash_table_size_, identity_as_first_hash_, get_slice_hash_);
      ++num_hash_func_;
      for (uint32_t block_idx = 0; block_idx < cuckoo_block_size_;
          ++block_idx, ++hash_val) {
        if ((*buckets)[hash_val].vector_idx == kMaxVectorIdx) {
          bucket_found = true;
          bucket_id = hash_val;
          break;
        } else {
          hash_vals.push_back(hash_val);
        }
      }
    }
    (*buckets)[bucket_id].vector_idx = vector_idx;
  }
  return Status::OK();
}

Status CuckooTableBuilder::Finish() {
  assert(!closed_);
  closed_ = true;
  std::vector<CuckooBucket> buckets;
  Status s;
  std::string unused_bucket;
  if (num_entries_ > 0) {
//如果启用了模块哈希，则计算实际哈希大小。
    if (use_module_hash_) {
      hash_table_size_ =
        static_cast<uint64_t>(num_entries_ / max_hash_table_ratio_);
    }
    s = MakeHashTable(&buckets);
    if (!s.ok()) {
      return s;
    }
//确定未使用的_user_键来填充空桶。
    std::string unused_user_key = smallest_user_key_;
    int curr_pos = static_cast<int>(unused_user_key.size()) - 1;
    while (curr_pos >= 0) {
      --unused_user_key[curr_pos];
      if (Slice(unused_user_key).compare(smallest_user_key_) < 0) {
        break;
      }
      --curr_pos;
    }
    if (curr_pos < 0) {
//尝试使用最大的密钥来标识未使用的密钥。
      unused_user_key = largest_user_key_;
      curr_pos = static_cast<int>(unused_user_key.size()) - 1;
      while (curr_pos >= 0) {
        ++unused_user_key[curr_pos];
        if (Slice(unused_user_key).compare(largest_user_key_) > 0) {
          break;
        }
        --curr_pos;
      }
    }
    if (curr_pos < 0) {
      return Status::Corruption("Unable to find unused key");
    }
    if (is_last_level_file_) {
      unused_bucket = unused_user_key;
    } else {
      ParsedInternalKey ikey(unused_user_key, 0, kTypeValue);
      AppendInternalKey(&unused_bucket, ikey);
    }
  }
  properties_.num_entries = num_entries_;
  properties_.fixed_key_len = key_size_;
  properties_.user_collected_properties[
        CuckooTablePropertyNames::kValueLength].assign(
        reinterpret_cast<const char*>(&value_size_), sizeof(value_size_));

  uint64_t bucket_size = key_size_ + value_size_;
  unused_bucket.resize(bucket_size, 'a');
//写下表格。
  uint32_t num_added = 0;
  for (auto& bucket : buckets) {
    if (bucket.vector_idx == kMaxVectorIdx) {
      s = file_->Append(Slice(unused_bucket));
    } else {
      ++num_added;
      s = file_->Append(GetKey(bucket.vector_idx));
      if (s.ok()) {
        if (value_size_ > 0) {
          s = file_->Append(GetValue(bucket.vector_idx));
        }
      }
    }
    if (!s.ok()) {
      return s;
    }
  }
  assert(num_added == NumEntries());
  properties_.raw_key_size = num_added * properties_.fixed_key_len;
  properties_.raw_value_size = num_added * value_size_;

  uint64_t offset = buckets.size() * bucket_size;
  properties_.data_size = offset;
  unused_bucket.resize(properties_.fixed_key_len);
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kEmptyKey] = unused_bucket;
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kNumHashFunc].assign(
        reinterpret_cast<char*>(&num_hash_func_), sizeof(num_hash_func_));

  properties_.user_collected_properties[
    CuckooTablePropertyNames::kHashTableSize].assign(
        reinterpret_cast<const char*>(&hash_table_size_),
        sizeof(hash_table_size_));
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kIsLastLevel].assign(
        reinterpret_cast<const char*>(&is_last_level_file_),
        sizeof(is_last_level_file_));
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kCuckooBlockSize].assign(
        reinterpret_cast<const char*>(&cuckoo_block_size_),
        sizeof(cuckoo_block_size_));
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kIdentityAsFirstHash].assign(
        reinterpret_cast<const char*>(&identity_as_first_hash_),
        sizeof(identity_as_first_hash_));
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kUseModuleHash].assign(
        reinterpret_cast<const char*>(&use_module_hash_),
        sizeof(use_module_hash_));
  uint32_t user_key_len = static_cast<uint32_t>(smallest_user_key_.size());
  properties_.user_collected_properties[
    CuckooTablePropertyNames::kUserKeyLength].assign(
        reinterpret_cast<const char*>(&user_key_len),
        sizeof(user_key_len));

//写元块。
  MetaIndexBuilder meta_index_builder;
  PropertyBlockBuilder property_block_builder;

  property_block_builder.AddTableProperty(properties_);
  property_block_builder.Add(properties_.user_collected_properties);
  Slice property_block = property_block_builder.Finish();
  BlockHandle property_block_handle;
  property_block_handle.set_offset(offset);
  property_block_handle.set_size(property_block.size());
  s = file_->Append(property_block);
  offset += property_block.size();
  if (!s.ok()) {
    return s;
  }

  meta_index_builder.Add(kPropertiesBlock, property_block_handle);
  Slice meta_index_block = meta_index_builder.Finish();

  BlockHandle meta_index_block_handle;
  meta_index_block_handle.set_offset(offset);
  meta_index_block_handle.set_size(meta_index_block.size());
  s = file_->Append(meta_index_block);
  if (!s.ok()) {
    return s;
  }

  Footer footer(kCuckooTableMagicNumber, 1);
  footer.set_metaindex_handle(meta_index_block_handle);
  footer.set_index_handle(BlockHandle::NullBlockHandle());
  std::string footer_encoding;
  footer.EncodeTo(&footer_encoding);
  s = file_->Append(footer_encoding);
  return s;
}

void CuckooTableBuilder::Abandon() {
  assert(!closed_);
  closed_ = true;
}

uint64_t CuckooTableBuilder::NumEntries() const {
  return num_entries_;
}

uint64_t CuckooTableBuilder::FileSize() const {
  if (closed_) {
    return file_->GetFileSize();
  } else if (num_entries_ == 0) {
    return 0;
  }

  if (use_module_hash_) {
    return static_cast<uint64_t>((key_size_ + value_size_) *
        num_entries_ / max_hash_table_ratio_);
  } else {
//说明桶是2的幂。
//添加元素后，文件大小将保持一段时间不变，并且
//它的尺寸加倍。因为压缩算法停止添加元素
//只有当它超过文件限制后，我们才会考虑额外的元素
//在此处添加。
    uint64_t expected_hash_table_size = hash_table_size_;
    if (expected_hash_table_size < (num_entries_ + 1) / max_hash_table_ratio_) {
      expected_hash_table_size *= 2;
    }
    return (key_size_ + value_size_) * expected_hash_table_size - 1;
  }
}

//当没有插入目标键的位置时，将调用此方法。
//它搜索可以移动以适应目标的一组元素
//关键。搜索是一个具有第一级（散列值）的BFS图遍历
//所有的buckets目标键都可以转到。
//然后，从每个节点（curr_节点）中找到curr_节点的所有存储桶。
//可以去。它们构成树中当前节点的子节点。
//我们继续遍历，直到找到一个空桶，在这种情况下，我们
//沿路径将所有元素从第一级移动到此空存储桶，
//为在第一级插入的目标密钥腾出空间（*bucket_id）。
//如果树深度超过最大深度，则返回错误，表示失败。
bool CuckooTableBuilder::MakeSpaceForKey(
    const autovector<uint64_t>& hash_vals,
    const uint32_t make_space_for_key_call_id,
    std::vector<CuckooBucket>* buckets, uint64_t* bucket_id) {
  struct CuckooNode {
    uint64_t bucket_id;
    uint32_t depth;
    uint32_t parent_pos;
    CuckooNode(uint64_t _bucket_id, uint32_t _depth, int _parent_pos)
        : bucket_id(_bucket_id), depth(_depth), parent_pos(_parent_pos) {}
  };
//这是简单存储为向量的BFS搜索树。
//每个节点都将父节点的索引存储在向量中。
  std::vector<CuckooNode> tree;
//我们要在当前方法调用中标识已访问的bucket，因此
//我们不再在树上添加相同的桶进行探索。
//我们通过在
//为\u key \u call \u id腾出\u空间，它作为此调用的唯一ID
//的方法。我们将这个数字存储到我们在其中探索的节点中
//当前方法调用。
//增量操作不太可能溢出，因为
//调用的次数<=max_num_hash_func_+num_entries_uu。
  for (uint32_t hash_cnt = 0; hash_cnt < num_hash_func_; ++hash_cnt) {
    uint64_t bid = hash_vals[hash_cnt];
    (*buckets)[bid].make_space_for_key_call_id = make_space_for_key_call_id;
    tree.push_back(CuckooNode(bid, 0, 0));
  }
  bool null_found = false;
  uint32_t curr_pos = 0;
  while (!null_found && curr_pos < tree.size()) {
    CuckooNode& curr_node = tree[curr_pos];
    uint32_t curr_depth = curr_node.depth;
    if (curr_depth >= max_search_depth_) {
      break;
    }
    CuckooBucket& curr_bucket = (*buckets)[curr_node.bucket_id];
    for (uint32_t hash_cnt = 0;
        hash_cnt < num_hash_func_ && !null_found; ++hash_cnt) {
      uint64_t child_bucket_id = CuckooHash(GetUserKey(curr_bucket.vector_idx),
          hash_cnt, use_module_hash_, hash_table_size_, identity_as_first_hash_,
          get_slice_hash_);
//在布谷鸟块内迭代。
      for (uint32_t block_idx = 0; block_idx < cuckoo_block_size_;
          ++block_idx, ++child_bucket_id) {
        if ((*buckets)[child_bucket_id].make_space_for_key_call_id ==
            make_space_for_key_call_id) {
          continue;
        }
        (*buckets)[child_bucket_id].make_space_for_key_call_id =
          make_space_for_key_call_id;
        tree.push_back(CuckooNode(child_bucket_id, curr_depth + 1,
              curr_pos));
        if ((*buckets)[child_bucket_id].vector_idx == kMaxVectorIdx) {
          null_found = true;
          break;
        }
      }
    }
    ++curr_pos;
  }

  if (null_found) {
//tree.back（）中有一个空节点。现在，从这里穿过这条路
//将节点清空到树的顶部，并在路径中的每个节点处替换
//孩子和父母在一起。到达树中的第一级时停止
//（0<=bucket_to_replace_pos<num_hash_func_x时发生）并返回
//此位置位于要插入的目标密钥的第一级。
    uint32_t bucket_to_replace_pos = static_cast<uint32_t>(tree.size()) - 1;
    while (bucket_to_replace_pos >= num_hash_func_) {
      CuckooNode& curr_node = tree[bucket_to_replace_pos];
      (*buckets)[curr_node.bucket_id] =
        (*buckets)[tree[curr_node.parent_pos].bucket_id];
      bucket_to_replace_pos = curr_node.parent_pos;
    }
    *bucket_id = tree[bucket_to_replace_pos].bucket_id;
  }
  return null_found;
}

}  //命名空间rocksdb
#endif  //摇滚乐
