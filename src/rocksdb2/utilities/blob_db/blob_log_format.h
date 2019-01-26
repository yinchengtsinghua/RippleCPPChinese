
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
//读写器共享的日志格式信息。

#pragma once

#ifndef ROCKSDB_LITE

#include <limits>
#include <utility>
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"

namespace rocksdb {
namespace blob_db {

constexpr uint32_t kMagicNumber = 2395959;  //0x24248F37
constexpr uint32_t kVersion1 = 1;
constexpr uint64_t kNoExpiration = std::numeric_limits<uint64_t>::max();

using ExpirationRange = std::pair<uint64_t, uint64_t>;
using SequenceRange = std::pair<uint64_t, uint64_t>;

//blob日志文件头的格式（30字节）：
//
//+————————+——————+————————+——————+————+——————+————+————+————++
//幻数版本CF ID标志压缩到期范围
//+————————+——————+————————+——————+————+——————+————+————+————++
//固定32固定32固定32字符固定64固定64
//+————————+——————+————————+——————+————+——————+————+————+————++
//
//旗帜列表：
//has_ttl:文件是否包含ttl数据。
//
//标题中的过期范围是基于
//blob_db_options.ttl_range_秒。
struct BlobLogHeader {
  static constexpr size_t kSize = 30;

  uint32_t version = kVersion1;
  uint32_t column_family_id = 0;
  CompressionType compression = kNoCompression;
  bool has_ttl = false;
  ExpirationRange expiration_range = std::make_pair(0, 0);

  void EncodeTo(std::string* dst);

  Status DecodeFrom(Slice slice);
};

//blob日志文件页脚格式（48字节）：
//
//+————————————+——————————+——————————+——————————+——————————+————————+————————+————————+————————+————————
//幻数斑点计数过期范围序列范围页脚CRC
//+————————————+——————————+——————————+——————————+——————————+————————+————————+————————+————————+————————
//固定32固定64固定64+固定64固定64+固定64固定32
//+————————————+——————————+——————————+——————————+——————————+————————+————————+————————+————————+————————
//
//只有当BLOB文件正确关闭时，才会显示页脚。
//
//与文件头中的相同字段不同，页脚中的过期范围是
//此文件中数据的最小和最大过期范围。
struct BlobLogFooter {
  static constexpr size_t kSize = 48;

  uint64_t blob_count = 0;
  ExpirationRange expiration_range = std::make_pair(0, 0);
  SequenceRange sequence_range = std::make_pair(0, 0);
  uint32_t crc = 0;

  void EncodeTo(std::string* dst);

  Status DecodeFrom(Slice slice);
};

//Blob记录格式（32字节头+键+值）：
//
//+——————+——————+——————+——————+——————+————+————+————+——————+——————+
//密钥长度值长度到期头CRC blob CRC密钥值
//+——————+——————+——————+——————+——————+————+————+————+——————+——————+
//固定64固定64固定64固定32固定32键长度值长度
//+——————+——————+——————+——————+——————+————+————+————+——————+——————+
//
//如果文件的ttl=false，则expiration字段始终为0，而blob
//没有过期。
//
//还要注意，如果使用压缩，则值是压缩值和值
//长度是压缩值长度。
//
//头CRC是（key_len+val_len+expiration）的校验和，而
//blob crc是（key+value）的校验和。
//
//我们可以使用可变长度编码（varint64）来节省更多的空间，但是
//使读者更加复杂。
struct BlobLogRecord {
//头包含blob crc之前的字段
  static constexpr size_t kHeaderSize = 32;

  uint64_t key_size = 0;
  uint64_t value_size = 0;
  uint64_t expiration = 0;
  uint32_t header_crc = 0;
  uint32_t blob_crc = 0;
  Slice key;
  Slice value;
  std::string key_buf;
  std::string value_buf;

  void EncodeHeaderTo(std::string* dst);

  Status DecodeHeaderFrom(Slice src);

  Status CheckBlobCRC() const;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
