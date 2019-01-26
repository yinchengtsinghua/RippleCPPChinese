
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

#pragma once
#include <random>
#include <stdint.h>

namespace rocksdb {

//一个非常简单的随机数生成器。不是特别擅长
//生成真正的随机位，但足以满足我们的需求
//包裹。
class Random {
 private:
  enum : uint32_t {
M = 2147483647L  //2 ^ 31 -1
  };
  enum : uint64_t {
A = 16807  //位14、8、7、5、2、1、0
  };

  uint32_t seed_;

  static uint32_t GoodSeed(uint32_t s) { return (s & M) != 0 ? (s & M) : 1; }

 public:
//这是可以从next（）返回的最大值。
  enum : uint32_t { kMaxNext = M };

  explicit Random(uint32_t s) : seed_(GoodSeed(s)) {}

  void Reset(uint32_t s) { seed_ = GoodSeed(s); }

  uint32_t Next() {
//我们正在计算
//Seed_uu=（Seed_uux）%m，其中m=2^31-1
//
//种子_u不能为零或m，否则所有后续计算值
//分别为0或m。对于所有其他值，种子将结束
//在[1，m-1]中的每个数字上循环
    uint64_t product = seed_ * A;

//使用（（x<<31）=x这一事实计算（积%m）。
    seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
//第一个约简可能溢出1位，因此我们可能需要
//重复。mod==m不可能；使用>允许更快
//符号位测试。
    if (seed_ > M) {
      seed_ -= M;
    }
    return seed_;
  }

//返回范围[0..n-1]内均匀分布的值
//要求：n＞0
  uint32_t Uniform(int n) { return Next() % n; }

//随机返回时间的真~“1/n”，否则返回假。
//要求：n＞0
  bool OneIn(int n) { return (Next() % n) == 0; }

//倾斜：从范围[0，max_log]中均匀地选取“base”，然后
//返回“基”随机位。效果是在
//范围[0,2^max_log-1]，指数偏向较小的数字。
  uint32_t Skewed(int max_log) {
    return Uniform(1 << Uniform(max_log + 1));
  }

//返回当前线程使用的随机实例
//附加锁定
  static Random* GetTLSInstance();
};

//基于std:：mt19937_64的简单64位随机数生成器
class Random64 {
 private:
  std::mt19937_64 generator_;

 public:
  explicit Random64(uint64_t s) : generator_(s) { }

//生成下一个随机数
  uint64_t Next() { return generator_(); }

//返回范围[0..n-1]内均匀分布的值
//要求：n＞0
  uint64_t Uniform(uint64_t n) {
    return std::uniform_int_distribution<uint64_t>(0, n - 1)(generator_);
  }

//随机返回时间的真~“1/n”，否则返回假。
//要求：n＞0
  bool OneIn(uint64_t n) { return Uniform(n) == 0; }

//倾斜：从范围[0，max_log]中均匀地选取“base”，然后
//返回“基”随机位。效果是在
//范围[0,2^max_log-1]，指数偏向较小的数字。
  uint64_t Skewed(int max_log) {
    return Uniform(uint64_t(1) << Uniform(max_log + 1));
  }
};

}  //命名空间rocksdb
