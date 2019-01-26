
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012-2018 Ripple Labs Inc.

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

//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

/*
   部分来自http://www.adp-gmbH.ch/cpp/common/base64.html
   版权声明：

   base64.cpp和base64.h

   版权所有（c）2004-2008 Ren_nyffenegger

   此源代码按原样提供，没有任何明示或暗示
   担保。在任何情况下，作者都不承担任何损害赔偿责任。
   因使用本软件而产生。

   允许任何人将本软件用于任何目的，
   包括商业应用程序，并对其进行更改和重新分发
   自由，受以下限制：

   1。不得歪曲此源代码的来源；不得
      声明您编写了原始源代码。如果使用此源代码
      在产品中，产品文档中的确认将是
      感谢但不是必需的。

   2。更改的源版本必须清楚地标记为这样，并且不能
      误传为原始源代码。

   三。不得从任何源分发中删除或更改此通知。

   Ren_nyffeegger rene.nyffeegger@adp-gmbH.ch

**/


#ifndef RIPPLE_BASICS_BASE64_H_INCLUDED
#define RIPPLE_BASICS_BASE64_H_INCLUDED

#include <string>

namespace ripple {

std::string
base64_encode (std::uint8_t const* data,
    std::size_t len);

inline
std::string
base64_encode(std::string const& s)
{
    return base64_encode (reinterpret_cast <
        std::uint8_t const*> (s.data()), s.size());
}

std::string
base64_decode(std::string const& data);

} //涟漪

#endif
