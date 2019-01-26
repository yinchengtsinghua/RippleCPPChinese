
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

#ifndef RIPPLE_RESOURCE_ENTRY_H_INCLUDED
#define RIPPLE_RESOURCE_ENTRY_H_INCLUDED

#include <ripple/basics/DecayingSample.h>
#include <ripple/resource/impl/Key.h>
#include <ripple/resource/impl/Tuning.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/core/List.h>
#include <cassert>

namespace ripple {
namespace Resource {

using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

//表中的一个条目
//vfalc不赞成使用boost:：invigative list
struct Entry
    : public beast::List <Entry>::Node
{
    Entry () = delete;

    /*
       @param现在进入施工时间。
    **/

    explicit Entry(clock_type::time_point const now)
        : refcount (0)
        , local_balance (now)
        , remote_balance (0)
        , lastWarningTime ()
        , whenExpires ()
    {
    }

    std::string to_string() const
    {
        switch (key->kind)
        {
        case kindInbound:   return key->address.to_string();
        case kindOutbound:  return key->address.to_string();
        case kindUnlimited: return std::string ("\"") + key->name + "\"";
        default:
            assert(false);
        }

        return "(undefined)";
    }

    /*
     *如果此连接没有
     *应用了资源限制--某些rpc命令仍然可以使用。
     *被禁止，但这取决于角色。
     **/

    bool isUnlimited () const
    {
        return key->kind == kindUnlimited;
    }

//包括远程捐款的余额
    int balance (clock_type::time_point const now)
    {
        return local_balance.value (now) + remote_balance;
    }

//添加费用并返回标准余额
//包括进口捐款。
    int add (int charge, clock_type::time_point const now)
    {
        return local_balance.add (charge, now) + remote_balance;
    }

//返回指向地图键的指针（这里有点黑）
    Key const* key;

//消费者证明资料的数量
    int refcount;

//资源消耗指数衰减平衡
    DecayingSample <decayWindowSeconds, clock_type> local_balance;

//来自进口的标准化余额贡献
    int remote_balance;

//最后一次警告的时间
    clock_type::time_point lastWarningTime;

//对于非活动项，清除此项的时间
    clock_type::time_point whenExpires;
};

inline std::ostream& operator<< (std::ostream& os, Entry const& v)
{
    os << v.to_string();
    return os;
}

}
}

#endif
