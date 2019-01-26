
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include <string.h>
#include "util/coding.h"
#include "util/hash.h"

namespace rocksdb {

uint32_t Hash(const char* data, size_t n, uint32_t seed) {
//类似于杂音杂音杂音
  const uint32_t m = 0xc6a4a793;
  const uint32_t r = 24;
  const char* limit = data + n;
  uint32_t h = static_cast<uint32_t>(seed ^ (n * m));

//一次接收四个字节
  while (data + 4 <= limit) {
    uint32_t w = DecodeFixed32(data);
    data += 4;
    h += w;
    h *= m;
    h ^= (h >> 16);
  }

//提取剩余字节
  switch (limit - data) {
//注意：最初的哈希实现使用了数据[i]<<shift，其中
//将char提升为int，然后执行移位。如果字符是
//否定，移位是C++中未定义的行为。哈希算法是
//格式定义的一部分，因此我们不能更改它；以获取相同的
//在法律上的行为，我们只是投给uint32，这会
//延长签名。确保与chars架构的兼容性
//是无符号的，我们首先将字符转换为int8_t。
    case 3:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[2])) << 16;
//跌倒
    case 2:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[1])) << 8;
//跌倒
    case 1:
      h += static_cast<uint32_t>(static_cast<int8_t>(data[0]));
      h *= m;
      h ^= (h >> r);
      break;
  }
  return h;
}

}  //命名空间rocksdb
