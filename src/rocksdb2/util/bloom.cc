
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "rocksdb/filter_policy.h"

#include "rocksdb/slice.h"
#include "table/block_based_filter_block.h"
#include "table/full_filter_bits_builder.h"
#include "table/full_filter_block.h"
#include "util/coding.h"
#include "util/hash.h"

namespace rocksdb {

class BlockBasedFilterBlockBuilder;
class FullFilterBlockBuilder;

FullFilterBitsBuilder::FullFilterBitsBuilder(const size_t bits_per_key,
                                             const size_t num_probes)
    : bits_per_key_(bits_per_key), num_probes_(num_probes) {
  assert(bits_per_key_);
  }

  FullFilterBitsBuilder::~FullFilterBitsBuilder() {}

  void FullFilterBitsBuilder::AddKey(const Slice& key) {
    uint32_t hash = BloomHash(key);
    if (hash_entries_.size() == 0 || hash != hash_entries_.back()) {
      hash_entries_.push_back(hash);
    }
  }

  Slice FullFilterBitsBuilder::Finish(std::unique_ptr<const char[]>* buf) {
    uint32_t total_bits, num_lines;
    char* data = ReserveSpace(static_cast<int>(hash_entries_.size()),
                              &total_bits, &num_lines);
    assert(data);

    if (total_bits != 0 && num_lines != 0) {
      for (auto h : hash_entries_) {
        AddHash(h, data, num_lines, total_bits);
      }
    }
    data[total_bits/8] = static_cast<char>(num_probes_);
    EncodeFixed32(data + total_bits/8 + 1, static_cast<uint32_t>(num_lines));

    const char* const_data = data;
    buf->reset(const_data);
    hash_entries_.clear();

    return Slice(data, total_bits / 8 + 5);
  }

uint32_t FullFilterBitsBuilder::GetTotalBitsForLocality(uint32_t total_bits) {
  uint32_t num_lines =
      (total_bits + CACHE_LINE_SIZE * 8 - 1) / (CACHE_LINE_SIZE * 8);

//使num_行为奇数，以确保包含更多位。
//确定哪个块时。
  if (num_lines % 2 == 0) {
    num_lines++;
  }
  return num_lines * (CACHE_LINE_SIZE * 8);
}

uint32_t FullFilterBitsBuilder::CalculateSpace(const int num_entry,
                                               uint32_t* total_bits,
                                               uint32_t* num_lines) {
  assert(bits_per_key_);
  if (num_entry != 0) {
    uint32_t total_bits_tmp = num_entry * static_cast<uint32_t>(bits_per_key_);

    *total_bits = GetTotalBitsForLocality(total_bits_tmp);
    *num_lines = *total_bits / (CACHE_LINE_SIZE * 8);
    assert(*total_bits > 0 && *total_bits % 8 == 0);
  } else {
//筛选器为空，只需为元数据留出空间
    *total_bits = 0;
    *num_lines = 0;
  }

//为筛选器保留空间
  uint32_t sz = *total_bits / 8;
sz += 5;  //num_行4个字节，num_探测1个字节
  return sz;
}

char* FullFilterBitsBuilder::ReserveSpace(const int num_entry,
                                          uint32_t* total_bits,
                                          uint32_t* num_lines) {
  uint32_t sz = CalculateSpace(num_entry, total_bits, num_lines);
  char* data = new char[sz];
  memset(data, 0, sz);
  return data;
}

int FullFilterBitsBuilder::CalculateNumEntry(const uint32_t space) {
  assert(bits_per_key_);
  assert(space > 0);
  uint32_t dont_care1, dont_care2;
  int high = (int) (space * 8 / bits_per_key_ + 1);
  int low = 1;
  int n = high;
  for (; n >= low; n--) {
    uint32_t sz = CalculateSpace(n, &dont_care1, &dont_care2);
    if (sz <= space) {
      break;
    }
  }
assert(n < high);  //高应该是高估
  return n;
}

inline void FullFilterBitsBuilder::AddHash(uint32_t h, char* data,
    uint32_t num_lines, uint32_t total_bits) {
  assert(num_lines > 0 && total_bits > 0);

const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
  uint32_t b = (h % num_lines) * (CACHE_LINE_SIZE * 8);

  for (uint32_t i = 0; i < num_probes_; ++i) {
//由于缓存线大小被定义为2^n，因此该行将被优化
//编译器的简单操作。
    const uint32_t bitpos = b + (h % (CACHE_LINE_SIZE * 8));
    data[bitpos / 8] |= (1 << (bitpos % 8));

    h += delta;
  }
}

namespace {
class FullFilterBitsReader : public FilterBitsReader {
 public:
  explicit FullFilterBitsReader(const Slice& contents)
      : data_(const_cast<char*>(contents.data())),
        data_len_(static_cast<uint32_t>(contents.size())),
        num_probes_(0),
        num_lines_(0) {
    assert(data_);
    GetFilterMeta(contents, &num_probes_, &num_lines_);
//清除损坏的参数
    if (num_lines_ != 0 && (data_len_-5) % num_lines_ != 0) {
      num_lines_ = 0;
      num_probes_ = 0;
    }
  }

  ~FullFilterBitsReader() {}

  virtual bool MayMatch(const Slice& entry) override {
if (data_len_ <= 5) {   //与原始过滤器保持相同
      return false;
    }
//其他错误参数，包括损坏的过滤器，视为匹配
    if (num_probes_ == 0 || num_lines_ == 0) return true;
    uint32_t hash = BloomHash(entry);
    return HashMayMatch(hash, Slice(data_, data_len_),
                        num_probes_, num_lines_);
  }

 private:
//筛选元数据
  char* data_;
  uint32_t data_len_;
  size_t num_probes_;
  uint32_t num_lines_;

//从过滤器获取num_探针和num_行
//如果筛选器格式被破坏，请将两者都设置为0。
  void GetFilterMeta(const Slice& filter, size_t* num_probes,
                             uint32_t* num_lines);

//“filter”包含前面调用附加到
//此类上的CreateFilterFromHash（）。如果
//密钥在传递给CreateFilter（）的密钥列表中。
//如果密钥不在
//列表，但它的目标应该是以很高的概率返回false。
//
//哈希：要检查的目标
//过滤器：整个过滤器，包括元数据字节
//num_probes：探针数量，手前读取
//num_行：过滤元数据，手工读取
//在调用此函数之前，需要确保输入元数据
//是有效的。
  bool HashMayMatch(const uint32_t& hash, const Slice& filter,
      const size_t& num_probes, const uint32_t& num_lines);

//不允许复制
  FullFilterBitsReader(const FullFilterBitsReader&);
  void operator=(const FullFilterBitsReader&);
};

void FullFilterBitsReader::GetFilterMeta(const Slice& filter,
    size_t* num_probes, uint32_t* num_lines) {
  uint32_t len = static_cast<uint32_t>(filter.size());
  if (len <= 5) {
//过滤器为空或损坏
    *num_probes = 0;
    *num_lines = 0;
    return;
  }

  *num_probes = filter.data()[len - 5];
  *num_lines = DecodeFixed32(filter.data() + len - 4);
}

bool FullFilterBitsReader::HashMayMatch(const uint32_t& hash,
    const Slice& filter, const size_t& num_probes,
    const uint32_t& num_lines) {
  uint32_t len = static_cast<uint32_t>(filter.size());
if (len <= 5) return false;  //与原始过滤器保持相同

//在调用参数之前确保参数有效
  assert(num_probes != 0);
  assert(num_lines != 0 && (len - 5) % num_lines == 0);
  uint32_t cache_line_size = (len - 5) / num_lines;
  const char* data = filter.data();

  uint32_t h = hash;
const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
  uint32_t b = (h % num_lines) * (cache_line_size * 8);

  for (uint32_t i = 0; i < num_probes; ++i) {
//由于缓存线大小被定义为2^n，因此该行将被优化
//一个简单的编译器操作。
    const uint32_t bitpos = b + (h % (cache_line_size * 8));
    if (((data[bitpos / 8]) & (1 << (bitpos % 8))) == 0) {
      return false;
    }

    h += delta;
  }

  return true;
}

//过滤策略的实现
class BloomFilterPolicy : public FilterPolicy {
 public:
  explicit BloomFilterPolicy(int bits_per_key, bool use_block_based_builder)
      : bits_per_key_(bits_per_key), hash_func_(BloomHash),
        use_block_based_builder_(use_block_based_builder) {
    initialize();
  }

  ~BloomFilterPolicy() {
  }

  virtual const char* Name() const override {
    return "rocksdb.BuiltinBloomFilter";
  }

  virtual void CreateFilter(const Slice* keys, int n,
                            std::string* dst) const override {
//计算Bloom筛选器大小（位和字节）
    size_t bits = n * bits_per_key_;

//对于小N，我们可以看到非常高的假阳性率。修复它
//通过实施最小布卢姆过滤器长度。
    if (bits < 64) bits = 64;

    size_t bytes = (bits + 7) / 8;
    bits = bytes * 8;

    const size_t init_size = dst->size();
    dst->resize(init_size + bytes, 0);
dst->push_back(static_cast<char>(num_probes_));  //记住探针
    char* array = &(*dst)[init_size];
    for (size_t i = 0; i < (size_t)n; i++) {
//使用双哈希生成哈希值序列。
//参见[Kirsch，Mitzenmacher 2006]中的分析。
      uint32_t h = hash_func_(keys[i]);
const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
      for (size_t j = 0; j < num_probes_; j++) {
        const uint32_t bitpos = h % bits;
        array[bitpos/8] |= (1 << (bitpos % 8));
        h += delta;
      }
    }
  }

  virtual bool KeyMayMatch(const Slice& key,
                           const Slice& bloom_filter) const override {
    const size_t len = bloom_filter.size();
    if (len < 2) return false;

    const char* array = bloom_filter.data();
    const size_t bits = (len - 1) * 8;

//使用编码的k以便我们可以读取由
//使用不同参数创建的Bloom过滤器。
    const size_t k = array[len-1];
    if (k > 30) {
//为短布卢姆过滤器的潜在新编码保留。
//把它当作一场比赛。
      return true;
    }

    uint32_t h = hash_func_(key);
const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
    for (size_t j = 0; j < k; j++) {
      const uint32_t bitpos = h % bits;
      if ((array[bitpos/8] & (1 << (bitpos % 8))) == 0) return false;
      h += delta;
    }
    return true;
  }

  virtual FilterBitsBuilder* GetFilterBitsBuilder() const override {
    if (use_block_based_builder_) {
      return nullptr;
    }

    return new FullFilterBitsBuilder(bits_per_key_, num_probes_);
  }

  virtual FilterBitsReader* GetFilterBitsReader(const Slice& contents)
      const override {
    return new FullFilterBitsReader(contents);
  }

//如果选择使用基于块的生成器
  bool UseBlockBasedBuilder() { return use_block_based_builder_; }

 private:
  size_t bits_per_key_;
  size_t num_probes_;
  uint32_t (*hash_func_)(const Slice& key);

  const bool use_block_based_builder_;

  void initialize() {
//我们故意把探测成本降低一点
num_probes_ = static_cast<size_t>(bits_per_key_ * 0.69);  //0.69＝Ln（2）
    if (num_probes_ < 1) num_probes_ = 1;
    if (num_probes_ > 30) num_probes_ = 30;
  }
};

}  //命名空间

const FilterPolicy* NewBloomFilterPolicy(int bits_per_key,
                                         bool use_block_based_builder) {
  return new BloomFilterPolicy(bits_per_key, use_block_based_builder);
}

}  //命名空间rocksdb
