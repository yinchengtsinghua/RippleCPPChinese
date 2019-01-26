
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2014，howard hinnant<howard.hinnant@gmail.com>，
        vinnie falco<vinnie.falco@gmail.com

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#ifndef BEAST_HASH_ENDIAN_H_INCLUDED
#define BEAST_HASH_ENDIAN_H_INCLUDED

namespace beast {

//endian提供以下问题的答案：
//1。这个系统是大的还是小的？
//2。某个类或函数的“desired endian”是否与
//土著人？
enum class endian
{
#ifdef _MSC_VER
    big    = 1,
    little = 0,
    native = little
#else
    native = __BYTE_ORDER__,
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__
#endif
};

#ifndef __INTELLISENSE__
static_assert(endian::native == endian::little ||
              endian::native == endian::big,
              "endian::native shall be one of endian::little or endian::big");

static_assert(endian::big != endian::little,
              "endian::big and endian::little shall have different values");
#endif

} //野兽

#endif
