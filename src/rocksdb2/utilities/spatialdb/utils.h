
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
#include <string>
#include <algorithm>

#include "rocksdb/utilities/spatial_db.h"

namespace rocksdb {
namespace spatial {

//索引思想来自http://msdn.microsoft.com/en-us/library/bb259689.aspx
inline uint64_t GetTileFromCoord(double x, double start, double end,
                                 uint32_t tile_bits) {
  if (x < start) {
    return 0;
  }
  uint64_t tiles = 1ull << tile_bits;
  uint64_t r = static_cast<uint64_t>(((x - start) / (end - start)) * tiles);
  return std::min(r, tiles - 1);
}

inline uint64_t GetQuadKeyFromTile(uint64_t tile_x, uint64_t tile_y,
                                   uint32_t tile_bits) {
  uint64_t quad_key = 0;
  for (uint32_t i = 0; i < tile_bits; ++i) {
    uint64_t mask = (1ull << i);
    quad_key |= (tile_x & mask) << i;
    quad_key |= (tile_y & mask) << (i + 1);
  }
  return quad_key;
}

inline BoundingBox<uint64_t> GetTileBoundingBox(
    const SpatialIndexOptions& spatial_index, BoundingBox<double> bbox) {
  return BoundingBox<uint64_t>(
      GetTileFromCoord(bbox.min_x, spatial_index.bbox.min_x,
                       spatial_index.bbox.max_x, spatial_index.tile_bits),
      GetTileFromCoord(bbox.min_y, spatial_index.bbox.min_y,
                       spatial_index.bbox.max_y, spatial_index.tile_bits),
      GetTileFromCoord(bbox.max_x, spatial_index.bbox.min_x,
                       spatial_index.bbox.max_x, spatial_index.tile_bits),
      GetTileFromCoord(bbox.max_y, spatial_index.bbox.min_y,
                       spatial_index.bbox.max_y, spatial_index.tile_bits));
}

//使用memcpy可以比较big endian
inline void PutFixed64BigEndian(std::string* dst, uint64_t value) {
  char buf[sizeof(value)];
  buf[0] = (value >> 56) & 0xff;
  buf[1] = (value >> 48) & 0xff;
  buf[2] = (value >> 40) & 0xff;
  buf[3] = (value >> 32) & 0xff;
  buf[4] = (value >> 24) & 0xff;
  buf[5] = (value >> 16) & 0xff;
  buf[6] = (value >> 8) & 0xff;
  buf[7] = value & 0xff;
  dst->append(buf, sizeof(buf));
}

//使用memcpy可以比较big endian
inline bool GetFixed64BigEndian(const Slice& input, uint64_t* value) {
  if (input.size() < sizeof(uint64_t)) {
    return false;
  }
  auto ptr = input.data();
  *value = (static_cast<uint64_t>(static_cast<unsigned char>(ptr[0])) << 56) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[1])) << 48) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[2])) << 40) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[3])) << 32) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[4])) << 24) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[5])) << 16) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[6])) << 8) |
           static_cast<uint64_t>(static_cast<unsigned char>(ptr[7]));
  return true;
}

inline void PutDouble(std::string* dst, double d) {
  dst->append(reinterpret_cast<char*>(&d), sizeof(double));
}

inline bool GetDouble(Slice* input, double* d) {
  if (input->size() < sizeof(double)) {
    return false;
  }
  memcpy(d, input->data(), sizeof(double));
  input->remove_prefix(sizeof(double));
  return true;
}

}  //命名空间空间
}  //命名空间rocksdb
