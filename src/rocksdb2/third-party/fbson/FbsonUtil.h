
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

/*
 *这个头文件定义了各种实用程序类。
 *
 *@author tian xia<tianx@fb.com>
 **/


#ifndef FBSON_FBSONUTIL_H
#define FBSON_FBSONUTIL_H

#include <sstream>
#include "FbsonDocument.h"

namespace fbson {

#define OUT_BUF_SIZE 1024

/*
 *fbsonTojson将fbsonValue对象转换为json字符串。
 **/

class FbsonToJson {
 public:
  FbsonToJson() : os_(buffer_, OUT_BUF_SIZE) {}

//获取JSON字符串
  const char* json(const FbsonValue* pval) {
    os_.clear();
    os_.seekp(0);

    if (pval) {
      intern_json(pval);
    }

    os_.put(0);
    return os_.getBuffer();
  }

 private:
//递归转换fbsonValue
  void intern_json(const FbsonValue* val) {
    switch (val->type()) {
    case FbsonType::T_Null: {
      os_.write("null", 4);
      break;
    }
    case FbsonType::T_True: {
      os_.write("true", 4);
      break;
    }
    case FbsonType::T_False: {
      os_.write("false", 5);
      break;
    }
    case FbsonType::T_Int8: {
      os_.write(((Int8Val*)val)->val());
      break;
    }
    case FbsonType::T_Int16: {
      os_.write(((Int16Val*)val)->val());
      break;
    }
    case FbsonType::T_Int32: {
      os_.write(((Int32Val*)val)->val());
      break;
    }
    case FbsonType::T_Int64: {
      os_.write(((Int64Val*)val)->val());
      break;
    }
    case FbsonType::T_Double: {
      os_.write(((DoubleVal*)val)->val());
      break;
    }
    case FbsonType::T_String: {
      os_.put('"');
      os_.write(((StringVal*)val)->getBlob(), ((StringVal*)val)->getBlobLen());
      os_.put('"');
      break;
    }
    case FbsonType::T_Binary: {
      os_.write("\"<BINARY>", 9);
      os_.write(((BinaryVal*)val)->getBlob(), ((BinaryVal*)val)->getBlobLen());
      os_.write("<BINARY>\"", 9);
      break;
    }
    case FbsonType::T_Object: {
      object_to_json((ObjectVal*)val);
      break;
    }
    case FbsonType::T_Array: {
      array_to_json((ArrayVal*)val);
      break;
    }
    default:
      break;
    }
  }

//转换对象
  void object_to_json(const ObjectVal* val) {
    os_.put('{');

    auto iter = val->begin();
    auto iter_fence = val->end();

    while (iter < iter_fence) {
//写入键
      if (iter->klen()) {
        os_.put('"');
        os_.write(iter->getKeyStr(), iter->klen());
        os_.put('"');
      } else {
        os_.write(iter->getKeyId());
      }
      os_.put(':');

//转换值
      intern_json(iter->value());

      ++iter;
      if (iter != iter_fence) {
        os_.put(',');
      }
    }

    assert(iter == iter_fence);

    os_.put('}');
  }

//将数组转换为JSON
  void array_to_json(const ArrayVal* val) {
    os_.put('[');

    auto iter = val->begin();
    auto iter_fence = val->end();

    while (iter != iter_fence) {
//转换值
      intern_json((const FbsonValue*)iter);
      ++iter;
      if (iter != iter_fence) {
        os_.put(',');
      }
    }

    assert(iter == iter_fence);

    os_.put(']');
  }

 private:
  FbsonOutStream os_;
  char buffer_[OUT_BUF_SIZE];
};

} //命名空间FBSON

#endif //fbson？fbsonUtil？h
