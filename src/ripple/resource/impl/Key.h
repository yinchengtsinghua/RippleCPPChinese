
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
    版权所有（c）2012，2013 Ripple Labs Inc.

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

#ifndef RIPPLE_RESOURCE_KEY_H_INCLUDED
#define RIPPLE_RESOURCE_KEY_H_INCLUDED

#include <ripple/resource/impl/Kind.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <cassert>

namespace ripple {
namespace Resource {

//消费者密钥
struct Key
{
    Kind kind;
    beast::IP::Endpoint address;
    std::string name;

    Key () = delete;

//入站和出站（非无限）密钥的构造函数
    Key (Kind k, beast::IP::Endpoint const& addr)
        : kind(k)
        , address(addr)
    {
        assert(kind != kindUnlimited);
    }

//无限制键的构造函数
    Key (std::string const& n)
        : kind(kindUnlimited)
        , name(n)
    {}

    struct hasher
    {
        std::size_t operator() (Key const& v) const
        {
            switch (v.kind)
            {
            case kindInbound:
            case kindOutbound:
                return m_addr_hash (v.address);

            case kindUnlimited:
                return m_name_hash (v.name);

            default:
                assert(false);
            };

            return 0;
        }

    private:
        beast::uhash <> m_addr_hash;
        beast::uhash <> m_name_hash;
    };

    struct key_equal
    {
        explicit key_equal() = default;

        bool operator() (Key const& lhs, Key const& rhs) const
        {
            if (lhs.kind != rhs.kind)
                return false;

            switch (lhs.kind)
            {
            case kindInbound:
            case kindOutbound:
                return lhs.address == rhs.address;

            case kindUnlimited:
                return lhs.name == rhs.name;

            default:
                assert(false);
            };

            return false;
        }

    private:
    };
};

}
}

#endif
