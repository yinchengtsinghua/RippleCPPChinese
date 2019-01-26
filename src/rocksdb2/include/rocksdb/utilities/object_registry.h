
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#ifndef ROCKSDB_LITE

#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "rocksdb/env.h"

namespace rocksdb {

//使用用模式注册的工厂函数创建新的t
//根据std:：regex_match匹配提供的“target”字符串。
//
//如果没有匹配的注册函数，则返回nullptr。如果多个函数
//匹配，未指定使用的工厂函数。
//
//如果授予调用方所有权，则使用结果指针填充Res-Guard。
template <typename T>
T* NewCustomObject(const std::string& target, std::unique_ptr<T>* res_guard);

//使用字符串调用时返回新的T。填充唯一的参数
//如果授予呼叫者所有权。
template <typename T>
using FactoryFunc = std::function<T*(const std::string&, std::unique_ptr<T>*)>;

//要为T类型注册工厂函数，请初始化Registrar对象
//静态存储时间。例如：
//
//静态注册器<env>hdfs_reg（“hdfs:/.*”，&createhdfsnv）；
//
//然后，调用newCustomObject<env>（“hdfs://some_path”，…）将匹配
//上面提供了regex，因此它返回调用createhdfsenv的结果。
template <typename T>
class Registrar {
 public:
  explicit Registrar(std::string pattern, FactoryFunc<T> factory);
};

//实施细节如下。

namespace internal {

template <typename T>
struct RegistryEntry {
  std::regex pattern;
  FactoryFunc<T> factory;
};

template <typename T>
struct Registry {
  static Registry* Get() {
    static Registry<T> instance;
    return &instance;
  }
  std::vector<RegistryEntry<T>> entries;

 private:
  Registry() = default;
};

}  //命名空间内部

template <typename T>
T* NewCustomObject(const std::string& target, std::unique_ptr<T>* res_guard) {
  res_guard->reset();
  for (const auto& entry : internal::Registry<T>::Get()->entries) {
    if (std::regex_match(target, entry.pattern)) {
      return entry.factory(target, res_guard);
    }
  }
  return nullptr;
}

template <typename T>
Registrar<T>::Registrar(std::string pattern, FactoryFunc<T> factory) {
  internal::Registry<T>::Get()->entries.emplace_back(internal::RegistryEntry<T>{
      std::regex(std::move(pattern)), std::move(factory)});
}

}  //命名空间rocksdb
#endif  //摇滚乐
