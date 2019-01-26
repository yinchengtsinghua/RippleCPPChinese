
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
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_NET_IPADDRESSV6_H_INCLUDED
#define BEAST_NET_IPADDRESSV6_H_INCLUDED

#include <cassert>
#include <cstdint>
#include <functional>
#include <ios>
#include <string>
#include <utility>
#include <boost/asio/ip/address_v6.hpp>

namespace beast {
namespace IP {

using AddressV6 = boost::asio::ip::address_v6;

/*如果地址是私人不可路由地址，则返回“true”。*/
bool is_private (AddressV6 const& addr);

/*如果地址是公共可路由地址，则返回“true”。*/
bool is_public (AddressV6 const& addr);

}
}

#endif
