
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

#pragma once
#include <string>
#include <stdint.h>
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

#include "options/cf_options.h"
#include "port/port.h"  //不除外
#include "table/persistent_cache_options.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class Block;
class RandomAccessFile;
struct ReadOptions;

extern bool ShouldReportDetailedTime(Env* env, Statistics* stats);

//幻数的长度（字节）。
const int kMagicNumberLengthByte = 8;

//blockhandle是指向存储数据的文件范围的指针。
//块或元块。
class BlockHandle {
 public:
  BlockHandle();
  BlockHandle(uint64_t offset, uint64_t size);

//文件中块的偏移量。
  uint64_t offset() const { return offset_; }
  void set_offset(uint64_t _offset) { offset_ = _offset; }

//存储块的大小
  uint64_t size() const { return size_; }
  void set_size(uint64_t _size) { size_ = _size; }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);

//返回包含句柄副本的字符串。
  std::string ToString(bool hex = true) const;

//如果块句柄的偏移量和大小都为“0”，我们将查看它
//作为指向no-where的空块句柄。
  bool IsNull() const {
    return offset_ == 0 && size_ == 0;
  }

  static const BlockHandle& NullBlockHandle() {
    return kNullBlockHandle;
  }

//块句柄的最大编码长度
  enum { kMaxEncodedLength = 10 + 10 };

 private:
  uint64_t offset_;
  uint64_t size_;

  static const BlockHandle kNullBlockHandle;
};

inline uint32_t GetCompressFormatForVersion(CompressionType compression_type,
                                            uint32_t version) {
//Snappy没有版本控制
  assert(compression_type != kSnappyCompression &&
         compression_type != kXpressCompression &&
         compression_type != kNoCompression);
//从版本2开始，我们用
//压缩_格式_版本==2。在此之前，版本是1。
//不要更改此功能，它会影响磁盘格式
  return version >= 2 ? 2 : 1;
}

inline bool BlockBasedTableSupportedVersion(uint32_t version) {
  return version <= 2;
}

//页脚封装存储在尾部的固定信息
//每个表文件的结尾。
class Footer {
 public:
//构造页脚而不指定其表幻数。
//在这种情况下，此类页脚的表幻数应为
//已通过@readfooterfromfile（）初始化。
//当计划使用decodeFrom（）加载页脚时，请使用此选项。永远不要使用这个
//当你计划编码的时候。
  Footer() : Footer(kInvalidTableMagicNumber, 0) {}

//当计划使用
//编码（）。不要将此构造函数与decodeFrom（）一起使用。
  Footer(uint64_t table_magic_number, uint32_t version);

//此文件中页脚的版本
  uint32_t version() const { return version_; }

//此文件中使用的校验和类型
  ChecksumType checksum() const { return checksum_; }
  void set_checksum(const ChecksumType c) { checksum_ = c; }

//表的metaindex块的块句柄
  const BlockHandle& metaindex_handle() const { return metaindex_handle_; }
  void set_metaindex_handle(const BlockHandle& h) { metaindex_handle_ = h; }

//表的索引块的块句柄
  const BlockHandle& index_handle() const { return index_handle_; }

  void set_index_handle(const BlockHandle& h) { index_handle_ = h; }

  uint64_t table_magic_number() const { return table_magic_number_; }

  void EncodeTo(std::string* dst) const;

//基于输入切片设置当前页脚。
//
//要求：未设置表“魔力”编号（即，
//HasInitializedTableMagicNumber（）为真）。函数将初始化
//幻数
  Status DecodeFrom(Slice* input);

//页脚的编码长度。请注意，页脚的序列化将
//始终至少占用kminencodedlength字节。如果字段已更改
//版本号应递增，kmaxencodedlength应为
//相应增加。
  enum {
//页脚版本0（旧版）将始终占用这么多字节。
//它由两个块句柄、填充和一个幻数组成。
    kVersion0EncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8,
//版本1及更高版本的页脚将始终占据这许多
//字节。它由校验和类型、两个块句柄、填充、
//版本号（大于1）和幻数
    kNewVersionsEncodedLength = 1 + 2 * BlockHandle::kMaxEncodedLength + 4 + 8,
    kMinEncodedLength = kVersion0EncodedLength,
    kMaxEncodedLength = kNewVersionsEncodedLength,
  };

  static const uint64_t kInvalidTableMagicNumber = 0;

//将此对象转换为人类可读的形式
  std::string ToString() const;

 private:
//要求：幻数未初始化。
  void set_table_magic_number(uint64_t magic_number) {
    assert(!HasInitializedTableMagicNumber());
    table_magic_number_ = magic_number;
  }

//如果@table_magic_number_设置为其他值，则返回true
//来自@kinvalidtablemagicnumber。
  bool HasInitializedTableMagicNumber() const {
    return (table_magic_number_ != kInvalidTableMagicNumber);
  }

  uint32_t version_;
  ChecksumType checksum_;
  BlockHandle metaindex_handle_;
  BlockHandle index_handle_;
  uint64_t table_magic_number_ = 0;
};

//从文件读取页脚
//如果强制执行表格的魔法号码！=0，readfooterfromfile（）将返回
//如果表\u幻数不等于强制表\u幻数，则损坏
Status ReadFooterFromFile(RandomAccessFileReader* file,
                          FilePrefetchBuffer* prefetch_buffer,
                          uint64_t file_size, Footer* footer,
                          uint64_t enforce_table_magic_number = 0);

//1字节类型+32位CRC
static const size_t kBlockTrailerSize = 5;

struct BlockContents {
Slice data;           //实际数据内容
bool cachable;        //可以缓存真正的iff数据
  CompressionType compression_type;
  std::unique_ptr<char[]> allocation;

  BlockContents() : cachable(false), compression_type(kNoCompression) {}

  BlockContents(const Slice& _data, bool _cachable,
                CompressionType _compression_type)
      : data(_data), cachable(_cachable), compression_type(_compression_type) {}

  BlockContents(std::unique_ptr<char[]>&& _data, size_t _size, bool _cachable,
                CompressionType _compression_type)
      : data(_data.get(), _size),
        cachable(_cachable),
        compression_type(_compression_type),
        allocation(std::move(_data)) {}

  BlockContents(BlockContents&& other) ROCKSDB_NOEXCEPT { *this = std::move(other); }

  BlockContents& operator=(BlockContents&& other) {
    data = std::move(other.data);
    cachable = other.cachable;
    compression_type = other.compression_type;
    allocation = std::move(other.allocation);
    return *this;
  }
};

//从“文件”中读取由“句柄”标识的块。关于失败
//返回非OK。成功后填写*结果并返回OK。
extern Status ReadBlockContents(
    RandomAccessFileReader* file, FilePrefetchBuffer* prefetch_buffer,
    const Footer& footer, const ReadOptions& options, const BlockHandle& handle,
    BlockContents* contents, const ImmutableCFOptions& ioptions,
    bool do_uncompress = true, const Slice& compression_dict = Slice(),
    const PersistentCacheOptions& cache_options = PersistentCacheOptions());

//“data”指向从文件中读取的原始块内容。
//此方法分配新的堆缓冲区和原始块
//内容被解压缩到这个缓冲区。这个缓冲器是
//通过“result”返回，由调用方
//释放这个缓冲区。
//有关压缩格式版本和可能值的说明，请参见
//利用/压缩.h
extern Status UncompressBlockContents(const char* data, size_t n,
                                      BlockContents* contents,
                                      uint32_t compress_format_version,
                                      const Slice& compression_dict,
                                      const ImmutableCFOptions &ioptions);

//这是对接受的UncompressBlockContents的扩展
//特定的压缩类型。这是未包装的块使用的
//没有压缩头。
extern Status UncompressBlockContentsForCompressionType(
    const char* data, size_t n, BlockContents* contents,
    uint32_t compress_format_version, const Slice& compression_dict,
    CompressionType compression_type, const ImmutableCFOptions &ioptions);

//实施细节如下。客户应忽略，

//TODO（andrewkr）：我们应该使用一种表示空值/未初始化值的方法
//封锁句柄。目前，我们使用零作为空值，使用零的负数作为
//未初始化。
inline BlockHandle::BlockHandle()
    : BlockHandle(~static_cast<uint64_t>(0),
                  ~static_cast<uint64_t>(0)) {
}

inline BlockHandle::BlockHandle(uint64_t _offset, uint64_t _size)
    : offset_(_offset), size_(_size) {}

}  //命名空间rocksdb
