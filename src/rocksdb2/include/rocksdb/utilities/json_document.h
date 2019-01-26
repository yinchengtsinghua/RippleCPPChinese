
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

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "rocksdb/slice.h"

//我们对documentdb api使用jsondocument
//实现灵感来源于Folly:：Dynamic、RapidJSon和FBSON

namespace fbson {
  class FbsonValue;
  class ObjectVal;
  template <typename T>
  class FbsonWriterT;
  class FbsonOutStream;
  typedef FbsonWriterT<FbsonOutStream> FbsonWriter;
}  //命名空间FBSON

namespace rocksdb {

//注意：这些都不是线程安全的
class JSONDocument {
 public:
//分析失败时返回nullptr
  static JSONDocument* ParseJSON(const char* json);

  enum Type {
    kNull,
    kArray,
    kBool,
    kDouble,
    kInt64,
    kObject,
    kString,
  };

  /*隐式*/jsondocument（）；//空
  /*隐式*/ JSONDocument(bool b);

  /*隐式*/jsondocument（double d）；
  /*隐式*/ JSONDocument(int8_t i);

  /*隐式*/jsondocument（int16_t i）；
  /*隐式*/ JSONDocument(int32_t i);

  /*隐式*/jsondocument（int64_t i）；
  /*隐式*/ JSONDocument(const std::string& s);

  /*隐式*/jsondocument（const char*s）；
  //用默认值构造特定类型的jsondocument
  显式jsondocument（type _type）；

  json document（const json document&json_文件）；

  json document（json document&json_文档）；

  type type（）常量；

  //需要：isObject（））
  bool包含（const std:：string&key）const；
  //需要：isObject（））
  //返回非所有者对象
  jsondocument operator[]（const std:：string&key）const；

  //需要：isarray（）==true isObject（）==true
  size_t count（）常量；

  //需要：isarray（）
  //返回非所有者对象
  jsondocument operator[]（大小写i）const；

  jsondocument&operator=（jsondocument jsondocument）；

  bool isnull（）常量；
  bool isarray（）常量；
  bool isbool（）常量；
  bool isdouble（）常量；
  bool isint64（）常量；
  bool isobject（）常量；
  bool isstring（）常量；

  //需要：isbool（）==true
  bool getbool（）常量；
  //需要：isDouble（）==true
  double getdouble（）常量；
  //需要：isint64（）==true
  int64_t getInt64（）常量；
  //需要：isstring（）==true
  std:：string getstring（）常量；

  bool operator==（const jsondocument&rhs）const；

  布尔操作员！=（const jsondocument&rhs）const；

  jsondocument copy（）常量；

  bool isowner（）常量；

  std:：string debugString（）常量；

 私人：
  类itemsiteratorgenerator；

 公众：
  //需要：isObject（））
  itemsiteratorGenerator items（）常量；

  //将序列化对象追加到DST
  void serialize（std:：string*dst）常量；
  //如果slice不表示有效的序列化jsondocument，则返回nullptr
  静态jsondocument*反序列化（const slice&src）；

 私人：
  friend类jsondocumentbuilder；

  jsondocument（fbson:：fbsonvalue*val，bool makecopy）；

  void initfromvalue（const fbson:：fbsonvalue*val）；

  //对象迭代
  类const_item_迭代器_
   私人：
    IMPL类；
   公众：
    typedef std:：pair<std:：string，jsondocument>value_type；
    显式常量项迭代器（impl*impl）；
    const_item_迭代器（const_item_迭代器&）
    常量项迭代器&运算符++（）；
    布尔操作员！=（const const_item_迭代器&其他）；
    值_类型运算符*（）；
    ~ const_item_itector（）；
   私人：
    友元类itemsiteratorgenerator；
    std:：unique_ptr<impl>it_uu；
  }；

  类itemsiteratorgenerator
   公众：
    显式itemsiteratorGenerator（const fbson:：objectVal&object）；
    const_item_迭代器begin（）const；

    const_item_迭代器end（）const；

   私人：
    const fbson：：对象值&object_uu；
  }；

  std:：unique_ptr<char[]>数据\；
  可变fbson：：fbson value*value_uu；

  //序列化格式的第一个字节指定编码版本。那
  //这样，我们可以轻松地更改格式，同时向后提供
  //兼容性。此常量指定
  //序列化格式
  静态const char kserializationformationversion；
}；

类jsondocumentbuilder
 公众：
  jsonDocumentBuilder（）；

  显式jsonDocumentBuilder（fbson:：fbsonoutstream*out）；

  空复位（）；

  bool writeStartArray（）；

  bool writeendarray（）；

  bool writeStartObject（）；

  bool writeendObject（）；

  bool writekeyvalue（const std:：string&key，const jsondocument&value）；

  bool writejsondocument（const jsondocument&value）；

  jsondocument getjsondocument（）；

  ~jsonDocumentBuilder（）；

 私人：
  std:：unique_ptr<fbson:：fbsonWriter>writer_uu；
}；

//命名空间rocksdb

endif//rocksdb_lite
