
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
#include <assert.h>
#include <limits>
#include <stdint.h>
#include "rocksdb/utilities/json_document.h"
#include "third-party/fbson/FbsonWriter.h"

namespace rocksdb {
JSONDocumentBuilder::JSONDocumentBuilder()
: writer_(new fbson::FbsonWriter()) {
}

JSONDocumentBuilder::JSONDocumentBuilder(fbson::FbsonOutStream* out)
: writer_(new fbson::FbsonWriter(*out)) {
}

void JSONDocumentBuilder::Reset() {
  writer_->reset();
}

bool JSONDocumentBuilder::WriteStartArray() {
  return writer_->writeStartArray();
}

bool JSONDocumentBuilder::WriteEndArray() {
  return writer_->writeEndArray();
}

bool JSONDocumentBuilder::WriteStartObject() {
  return writer_->writeStartObject();
}

bool JSONDocumentBuilder::WriteEndObject() {
  return writer_->writeEndObject();
}

bool JSONDocumentBuilder::WriteKeyValue(const std::string& key,
                                        const JSONDocument& value) {
  assert(key.size() <= std::numeric_limits<uint8_t>::max());
  size_t bytesWritten = writer_->writeKey(key.c_str(),
    static_cast<uint8_t>(key.size()));
  if (bytesWritten == 0) {
    return false;
  }
  return WriteJSONDocument(value);
}

bool JSONDocumentBuilder::WriteJSONDocument(const JSONDocument& value) {
  switch (value.type()) {
    case JSONDocument::kNull:
      return writer_->writeNull() != 0;
    case JSONDocument::kInt64:
      return writer_->writeInt64(value.GetInt64());
    case JSONDocument::kDouble:
      return writer_->writeDouble(value.GetDouble());
    case JSONDocument::kBool:
      return writer_->writeBool(value.GetBool());
    case JSONDocument::kString:
    {
      bool res = writer_->writeStartString();
      if (!res) {
        return false;
      }
      const std::string& str = value.GetString();
      res = writer_->writeString(str.c_str(),
                  static_cast<uint32_t>(str.size()));
      if (!res) {
        return false;
      }
      return writer_->writeEndString();
    }
    case JSONDocument::kArray:
    {
      bool res = WriteStartArray();
      if (!res) {
        return false;
      }
      for (size_t i = 0; i < value.Count(); ++i) {
        res = WriteJSONDocument(value[i]);
        if (!res) {
          return false;
        }
      }
      return WriteEndArray();
    }
    case JSONDocument::kObject:
    {
      bool res = WriteStartObject();
      if (!res) {
        return false;
      }
      for (auto keyValue : value.Items()) {
        WriteKeyValue(keyValue.first, keyValue.second);
      }
      return WriteEndObject();
    }
    default:
      assert(false);
  }
  return false;
}

JSONDocument JSONDocumentBuilder::GetJSONDocument() {
  fbson::FbsonValue* value =
      fbson::FbsonDocument::createValue(writer_->getOutput()->getBuffer(),
                       static_cast<uint32_t>(writer_->getOutput()->getSize()));
  return JSONDocument(value, true);
}

JSONDocumentBuilder::~JSONDocumentBuilder() {
}

}  //命名空间rocksdb

#endif  //摇滚乐
