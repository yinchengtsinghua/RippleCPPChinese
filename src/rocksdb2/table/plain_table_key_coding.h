
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

#include <array>
#include "rocksdb/slice.h"
#include "db/dbformat.h"
#include "table/plain_table_reader.h"

namespace rocksdb {

class WritableFile;
struct ParsedInternalKey;
struct PlainTableReaderFileInfo;
enum PlainTableEntryType : unsigned char;

//Helper类，用于写出输出文件的键
//密钥的实际数据格式记录在plain_table_factory.h中。
class PlainTableKeyEncoder {
 public:
  explicit PlainTableKeyEncoder(EncodingType encoding_type,
                                uint32_t user_key_len,
                                const SliceTransform* prefix_extractor,
                                size_t index_sparseness)
      : encoding_type_((prefix_extractor != nullptr) ? encoding_type : kPlain),
        fixed_user_key_len_(user_key_len),
        prefix_extractor_(prefix_extractor),
        index_sparseness_((index_sparseness > 1) ? index_sparseness : 1),
        key_count_for_prefix_(0) {}
//密钥：以内部密钥的格式写出的密钥。
//文件：要写出的输出文件
//偏移：文件中的偏移。需要在附加字节后更新
//为了钥匙
//meta_bytes_buf：额外meta bytes的缓冲区
//meta_bytes_buf_size：附加额外meta字节的偏移量。将更新
//如果更新了meta_bytes_buf。
  Status AppendKey(const Slice& key, WritableFileWriter* file, uint64_t* offset,
                   char* meta_bytes_buf, size_t* meta_bytes_buf_size);

//返回要选取的实际编码类型
  EncodingType GetEncodingType() { return encoding_type_; }

 private:
  EncodingType encoding_type_;
  uint32_t fixed_user_key_len_;
  const SliceTransform* prefix_extractor_;
  const size_t index_sparseness_;
  size_t key_count_for_prefix_;
  IterKey pre_prefix_;
};

class PlainTableFileReader {
 public:
  explicit PlainTableFileReader(const PlainTableReaderFileInfo* _file_info)
      : file_info_(_file_info), num_buf_(0) {}
//在mmaped模式下，结果指向文件的mmaped区域，该区域
//表示在关闭文件之前始终有效。
//在非mmap模式下，结果指向一个内部缓冲区。如果呼叫者
//进行另一个读取调用，结果可能无效。所以打电话的人应该
//必要时复印一份。
//为了保存对文件的读取调用，我们保留了两个内部缓冲区：
//第一次读和最近一次读。这是有效的，因为它
//列这两个常见用例：
//（1）哈希索引只标识一个位置，我们读取密钥进行验证
//位置，如果位置正确，则读取键和值。
//（2）哈希索引检查后，我们确定了两个位置（因为
//散列桶冲突），我们二进制搜索两个位置以查看
//哪一个是我们需要的，并开始从位置读取。
//这两个最常见的用例将由两个缓冲区覆盖
//这样我们就不需要重新读取相同的位置。
//目前我们保留一个固定大小的缓冲区。如果读数不完全符合
//缓冲区，我们用用户读取的位置替换第二个缓冲区。
//
//如果返回false，则状态代码存储在状态_uu中。
  bool Read(uint32_t file_offset, uint32_t len, Slice* out) {
    if (file_info_->is_mmap_mode) {
      assert(file_offset + len <= file_info_->data_end_offset);
      *out = Slice(file_info_->file_data.data() + file_offset, len);
      return true;
    } else {
      return ReadNonMmap(file_offset, len, out);
    }
  }

//如果返回false，则状态代码存储在状态_uu中。
  bool ReadNonMmap(uint32_t file_offset, uint32_t len, Slice* output);

//*bytes_read=0表示eof。错误表示保存了故障和状态
//在StasuSu.不直接返回状态保存复制状态
//对象以映射mmap模式的以前性能。
  inline bool ReadVarint32(uint32_t offset, uint32_t* output,
                           uint32_t* bytes_read);

  bool ReadVarint32NonMmap(uint32_t offset, uint32_t* output,
                           uint32_t* bytes_read);

  Status status() const { return status_; }

  const PlainTableReaderFileInfo* file_info() { return file_info_; }

 private:
  const PlainTableReaderFileInfo* file_info_;

  struct Buffer {
    Buffer() : buf_start_offset(0), buf_len(0), buf_capacity(0) {}
    std::unique_ptr<char[]> buf;
    uint32_t buf_start_offset;
    uint32_t buf_len;
    uint32_t buf_capacity;
  };

//保留两次最近读取的缓冲区。
  std::array<unique_ptr<Buffer>, 2> buffers_;
  uint32_t num_buf_;
  Status status_;

  Slice GetFromBuffer(Buffer* buf, uint32_t file_offset, uint32_t len);
};

//用于从输入缓冲区解码键的帮助程序类
//密钥的实际数据格式记录在plain_table_factory.h中。
class PlainTableKeyDecoder {
 public:
  explicit PlainTableKeyDecoder(const PlainTableReaderFileInfo* file_info,
                                EncodingType encoding_type,
                                uint32_t user_key_len,
                                const SliceTransform* prefix_extractor)
      : file_reader_(file_info),
        encoding_type_(encoding_type),
        prefix_len_(0),
        fixed_user_key_len_(user_key_len),
        prefix_extractor_(prefix_extractor),
        in_prefix_(false) {}
//找到下一个键。
//start：键开始的字符数组。
//限制：字符数组的边界
//已解析\键：结果键的输出
//内部\键：如果不是空值，则用结果键的输出填充
//未解析格式
//字节读取：从开始读取的字节数。产量
//可查找：是否可以从此处读取密钥。建筑时使用
//索引。输出。
  Status NextKey(uint32_t start_offset, ParsedInternalKey* parsed_key,
                 Slice* internal_key, Slice* value, uint32_t* bytes_read,
                 bool* seekable = nullptr);

  Status NextKeyNoValue(uint32_t start_offset, ParsedInternalKey* parsed_key,
                        Slice* internal_key, uint32_t* bytes_read,
                        bool* seekable = nullptr);

  PlainTableFileReader file_reader_;
  EncodingType encoding_type_;
  uint32_t prefix_len_;
  uint32_t fixed_user_key_len_;
  Slice saved_user_key_;
  IterKey cur_key_;
  const SliceTransform* prefix_extractor_;
  bool in_prefix_;

 private:
  Status NextPlainEncodingKey(uint32_t start_offset,
                              ParsedInternalKey* parsed_key,
                              Slice* internal_key, uint32_t* bytes_read,
                              bool* seekable = nullptr);
  Status NextPrefixEncodingKey(uint32_t start_offset,
                               ParsedInternalKey* parsed_key,
                               Slice* internal_key, uint32_t* bytes_read,
                               bool* seekable = nullptr);
  Status ReadInternalKey(uint32_t file_offset, uint32_t user_key_size,
                         ParsedInternalKey* parsed_key, uint32_t* bytes_read,
                         bool* internal_key_valid, Slice* internal_key);
  inline Status DecodeSize(uint32_t start_offset,
                           PlainTableEntryType* entry_type, uint32_t* key_size,
                           uint32_t* bytes_read);
};

}  //命名空间rocksdb

#endif  //摇滚乐
