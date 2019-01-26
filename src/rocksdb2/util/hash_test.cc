
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include <vector>

#include "util/hash.h"
#include "util/testharness.h"

//哈希算法是文件格式的一部分，例如bloom
//过滤器。测试一组随机字符串的哈希值是否稳定
//长度不同。
TEST(HashTest, Values) {
  using rocksdb::Hash;
constexpr uint32_t kSeed = 0xbc9f1d34;  //和bloomhash一样。

  EXPECT_EQ(Hash("", 0, kSeed), 3164544308);
  EXPECT_EQ(Hash("\x08", 1, kSeed), 422599524);
  EXPECT_EQ(Hash("\x17", 1, kSeed), 3168152998);
  EXPECT_EQ(Hash("\x9a", 1, kSeed), 3195034349);
  EXPECT_EQ(Hash("\x1c", 1, kSeed), 2651681383);
  EXPECT_EQ(Hash("\x4d\x76", 2, kSeed), 2447836956);
  EXPECT_EQ(Hash("\x52\xd5", 2, kSeed), 3854228105);
  EXPECT_EQ(Hash("\x91\xf7", 2, kSeed), 31066776);
  EXPECT_EQ(Hash("\xd6\x27", 2, kSeed), 1806091603);
  EXPECT_EQ(Hash("\x30\x46\x0b", 3, kSeed), 3808221797);
  EXPECT_EQ(Hash("\x56\xdc\xd6", 3, kSeed), 2157698265);
  EXPECT_EQ(Hash("\xd4\x52\x33", 3, kSeed), 1721992661);
  EXPECT_EQ(Hash("\x6a\xb5\xf4", 3, kSeed), 2469105222);
  EXPECT_EQ(Hash("\x67\x53\x81\x1c", 4, kSeed), 118283265);
  EXPECT_EQ(Hash("\x69\xb8\xc0\x88", 4, kSeed), 3416318611);
  EXPECT_EQ(Hash("\x1e\x84\xaf\x2d", 4, kSeed), 3315003572);
  EXPECT_EQ(Hash("\x46\xdc\x54\xbe", 4, kSeed), 447346355);
  EXPECT_EQ(Hash("\xd0\x7a\x6e\xea\x56", 5, kSeed), 4255445370);
  EXPECT_EQ(Hash("\x86\x83\xd5\xa4\xd8", 5, kSeed), 2390603402);
  EXPECT_EQ(Hash("\xb7\x46\xbb\x77\xce", 5, kSeed), 2048907743);
  EXPECT_EQ(Hash("\x6c\xa8\xbc\xe5\x99", 5, kSeed), 2177978500);
  EXPECT_EQ(Hash("\x5c\x5e\xe1\xa0\x73\x81", 6, kSeed), 1036846008);
  EXPECT_EQ(Hash("\x08\x5d\x73\x1c\xe5\x2e", 6, kSeed), 229980482);
  EXPECT_EQ(Hash("\x42\xfb\xf2\x52\xb4\x10", 6, kSeed), 3655585422);
  EXPECT_EQ(Hash("\x73\xe1\xff\x56\x9c\xce", 6, kSeed), 3502708029);
  EXPECT_EQ(Hash("\x5c\xbe\x97\x75\x54\x9a\x52", 7, kSeed), 815120748);
  EXPECT_EQ(Hash("\x16\x82\x39\x49\x88\x2b\x36", 7, kSeed), 3056033698);
  EXPECT_EQ(Hash("\x59\x77\xf0\xa7\x24\xf4\x78", 7, kSeed), 587205227);
  EXPECT_EQ(Hash("\xd3\xa5\x7c\x0e\xc0\x02\x07", 7, kSeed), 2030937252);
  EXPECT_EQ(Hash("\x31\x1b\x98\x75\x96\x22\xd3\x9a", 8, kSeed), 469635402);
  EXPECT_EQ(Hash("\x38\xd6\xf7\x28\x20\xb4\x8a\xe9", 8, kSeed), 3530274698);
  EXPECT_EQ(Hash("\xbb\x18\x5d\xf4\x12\x03\xf7\x99", 8, kSeed), 1974545809);
  EXPECT_EQ(Hash("\x80\xd4\x3b\x3b\xae\x22\xa2\x78", 8, kSeed), 3563570120);
  EXPECT_EQ(Hash("\x1a\xb5\xd0\xfe\xab\xc3\x61\xb2\x99", 9, kSeed), 2706087434);
  EXPECT_EQ(Hash("\x8e\x4a\xc3\x18\x20\x2f\x06\xe6\x3c", 9, kSeed), 1534654151);
  EXPECT_EQ(Hash("\xb6\xc0\xdd\x05\x3f\xc4\x86\x4c\xef", 9, kSeed), 2355554696);
  EXPECT_EQ(Hash("\x9a\x5f\x78\x0d\xaf\x50\xe1\x1f\x55", 9, kSeed), 1400800912);
  EXPECT_EQ(Hash("\x22\x6f\x39\x1f\xf8\xdd\x4f\x52\x17\x94", 10, kSeed),
            3420325137);
  EXPECT_EQ(Hash("\x32\x89\x2a\x75\x48\x3a\x4a\x02\x69\xdd", 10, kSeed),
            3427803584);
  EXPECT_EQ(Hash("\x06\x92\x5c\xf4\x88\x0e\x7e\x68\x38\x3e", 10, kSeed),
            1152407945);
  EXPECT_EQ(Hash("\xbd\x2c\x63\x38\xbf\xe9\x78\xb7\xbf\x15", 10, kSeed),
            3382479516);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
