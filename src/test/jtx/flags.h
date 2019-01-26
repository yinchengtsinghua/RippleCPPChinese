
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

#ifndef RIPPLE_TEST_JTX_FLAGS_H_INCLUDED
#define RIPPLE_TEST_JTX_FLAGS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/basics/contract.h>

namespace ripple {
namespace test {
namespace jtx {

//JSON发生器

/*添加和/或删除标志。*/
Json::Value
fset (Account const& account,
    std::uint32_t on, std::uint32_t off = 0);

/*删除帐户标志。*/
inline
Json::Value
fclear (Account const& account,
    std::uint32_t off)
{
    return fset(account, 0, off);
}

namespace detail {

class flags_helper
{
protected:
    std::uint32_t mask_;

private:
    inline
    void
    set_args()
    {
    }

    void
    set_args (std::uint32_t flag)
    {
        switch(flag)
        {
        case asfRequireDest:    mask_ |= lsfRequireDestTag; break;
        case asfRequireAuth:    mask_ |= lsfRequireAuth;    break;
        case asfDisallowXRP:    mask_ |= lsfDisallowXRP;    break;
        case asfDisableMaster:  mask_ |= lsfDisableMaster;  break;
//案例asfaccounttxnid:/？？？？
        case asfNoFreeze:       mask_ |= lsfNoFreeze;       break;
        case asfGlobalFreeze:   mask_ |= lsfGlobalFreeze;   break;
        case asfDefaultRipple:  mask_ |= lsfDefaultRipple;  break;
        case asfDepositAuth:    mask_ |= lsfDepositAuth;    break;
        default:
        Throw<std::runtime_error> (
            "unknown flag");
        }
    }

    template <class Flag,
        class... Args>
    void
    set_args (std::uint32_t flag,
        Args... args)
    {
        set_args(flag, args...);
    }

protected:
    template <class... Args>
    flags_helper (Args... args)
        : mask_(0)
    {
        set_args(args...);
    }
};

} //细节

/*匹配集帐户标志*/
class flags : private detail::flags_helper
{
private:
    Account account_;

public:
    template <class... Args>
    flags (Account const& account,
            Args... args)
        : flags_helper(args...)
        , account_(account)
    {
    }

    void
    operator()(Env& env) const;
};

/*匹配清除帐户标志*/
class nflags : private detail::flags_helper
{
private:
    Account account_;

public:
    template <class... Args>
    nflags (Account const& account,
            Args... args)
        : flags_helper(args...)
        , account_(account)
    {
    }

    void
    operator()(Env& env) const;
};

} //JTX
} //测试
} //涟漪

#endif
