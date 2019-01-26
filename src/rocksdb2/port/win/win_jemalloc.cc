
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码是根据在
//此源树根目录中的许可证文件。额外补助金
//的专利权可以在同一目录下的专利文件中找到。
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef ROCKSDB_JEMALLOC
# error This file can only be part of jemalloc aware build
#endif

#include <stdexcept>
#include "jemalloc/jemalloc.h"

//当文件为
//建筑的一部分

void* operator new(size_t size) {
  void* p = je_malloc(size);
  if (!p) {
    throw std::bad_alloc();
  }
  return p;
}

void* operator new[](size_t size) {
  void* p = je_malloc(size);
  if (!p) {
    throw std::bad_alloc();
  }
  return p;
}

void operator delete(void* p) {
  if (p) {
    je_free(p);
  }
}

void operator delete[](void* p) {
  if (p) {
    je_free(p);
  }
}

