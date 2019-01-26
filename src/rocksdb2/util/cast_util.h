
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

namespace rocksdb {
//断言从动态强制转换到的移动的helper函数
//静态_cast<>是正确的。此函数用于处理遗留代码。
//建议不要添加新代码来发布类强制转换。首选
//解决方案是在不需要强制转换的情况下实现功能。
template <class DestClass, class SrcClass>
inline DestClass* static_cast_with_check(SrcClass* x) {
  DestClass* ret = static_cast<DestClass*>(x);
#ifdef ROCKSDB_USE_RTTI
  assert(ret == dynamic_cast<DestClass*>(x));
#endif
  return ret;
}
}  //命名空间rocksdb
