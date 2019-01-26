
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

#include <stdint.h>

#include <limits>
#include <string>
#include <vector>

#include "rocksdb/types.h"

namespace rocksdb {
struct ColumnFamilyMetaData;
struct LevelMetaData;
struct SstFileMetaData;

//描述列族的元数据。
struct ColumnFamilyMetaData {
  ColumnFamilyMetaData() : size(0), name("") {}
  ColumnFamilyMetaData(const std::string& _name, uint64_t _size,
                       const std::vector<LevelMetaData>&& _levels) :
      size(_size), name(_name), levels(_levels) {}

//此列族的大小（以字节为单位），等于
//其“级别”的文件大小。
  uint64_t size;
//此列族中的文件数。
  size_t file_count;
//列族的名称。
  std::string name;
//此列族中所有级别的元数据。
  std::vector<LevelMetaData> levels;
};

//描述级别的元数据。
struct LevelMetaData {
  LevelMetaData(int _level, uint64_t _size,
                const std::vector<SstFileMetaData>&& _files) :
      level(_level), size(_size),
      files(_files) {}

//此元数据描述的级别。
  const int level;
//此级别的大小（以字节为单位），等于
//其“文件”的文件大小。
  const uint64_t size;
//此级别中所有sst文件的元数据。
  const std::vector<SstFileMetaData> files;
};

//描述SST文件的元数据。
struct SstFileMetaData {
  SstFileMetaData() {}
  SstFileMetaData(const std::string& _file_name, const std::string& _path,
                  uint64_t _size, SequenceNumber _smallest_seqno,
                  SequenceNumber _largest_seqno,
                  const std::string& _smallestkey,
                  const std::string& _largestkey, uint64_t _num_reads_sampled,
                  bool _being_compacted)
      : size(_size),
        name(_file_name),
        db_path(_path),
        smallest_seqno(_smallest_seqno),
        largest_seqno(_largest_seqno),
        smallestkey(_smallestkey),
        largestkey(_largestkey),
        num_reads_sampled(_num_reads_sampled),
        being_compacted(_being_compacted) {}

//文件大小（字节）。
  uint64_t size;
//文件名。
  std::string name;
//文件所在的完整路径。
  std::string db_path;

SequenceNumber smallest_seqno;  //文件中最小的序列号。
SequenceNumber largest_seqno;   //文件中的最大序列号。
std::string smallestkey;     //文件中最小的用户定义密钥。
std::string largestkey;      //文件中最大的用户定义密钥。
uint64_t num_reads_sampled;  //读取文件的次数。
bool being_compacted;  //如果文件当前正在压缩，则为true。
};

//与每个sst文件关联的完整元数据集。
struct LiveFileMetaData : SstFileMetaData {
std::string column_family_name;  //列族的名称
int level;               //此文件所在的级别。
};
}  //命名空间rocksdb
