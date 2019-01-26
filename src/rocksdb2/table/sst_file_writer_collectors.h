
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
#include "rocksdb/types.h"
#include "util/string_util.h"

namespace rocksdb {

//特定于sstfilewriter创建的表的表属性。
struct ExternalSstFilePropertyNames {
//此属性的值是一个固定的uint32数字。
  static const std::string kVersion;
//此属性的值是一个固定的uint64数字。
  static const std::string kGlobalSeqno;
};

//PropertiesCollector用于添加特定于表的属性
//由sstfilewriter生成
class SstFileWriterPropertiesCollector : public IntTblPropCollector {
 public:
  explicit SstFileWriterPropertiesCollector(int32_t version,
                                            SequenceNumber global_seqno)
      : version_(version), global_seqno_(global_seqno) {}

  virtual Status InternalAdd(const Slice& key, const Slice& value,
                             uint64_t file_size) override {
//故意留空。对收集统计数据没有兴趣
//单个键/值对。
    return Status::OK();
  }

  virtual Status Finish(UserCollectedProperties* properties) override {
//文件版本
    std::string version_val;
    PutFixed32(&version_val, static_cast<uint32_t>(version_));
    properties->insert({ExternalSstFilePropertyNames::kVersion, version_val});

//全局序列号
    std::string seqno_val;
    PutFixed64(&seqno_val, static_cast<uint64_t>(global_seqno_));
    properties->insert({ExternalSstFilePropertyNames::kGlobalSeqno, seqno_val});

    return Status::OK();
  }

  virtual const char* Name() const override {
    return "SstFileWriterPropertiesCollector";
  }

  virtual UserCollectedProperties GetReadableProperties() const override {
    return {{ExternalSstFilePropertyNames::kVersion, ToString(version_)}};
  }

 private:
  int32_t version_;
  SequenceNumber global_seqno_;
};

class SstFileWriterPropertiesCollectorFactory
    : public IntTblPropCollectorFactory {
 public:
  explicit SstFileWriterPropertiesCollectorFactory(int32_t version,
                                                   SequenceNumber global_seqno)
      : version_(version), global_seqno_(global_seqno) {}

  virtual IntTblPropCollector* CreateIntTblPropCollector(
      uint32_t column_family_id) override {
    return new SstFileWriterPropertiesCollector(version_, global_seqno_);
  }

  virtual const char* Name() const override {
    return "SstFileWriterPropertiesCollector";
  }

 private:
  int32_t version_;
  SequenceNumber global_seqno_;
};

}  //命名空间rocksdb
