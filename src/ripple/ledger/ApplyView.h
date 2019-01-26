
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

#ifndef RIPPLE_LEDGER_APPLYVIEW_H_INCLUDED
#define RIPPLE_LEDGER_APPLYVIEW_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/ledger/RawView.h>
#include <ripple/ledger/ReadView.h>
#include <boost/optional.hpp>

namespace ripple {

enum ApplyFlags
    : std::uint32_t
{
    tapNONE             = 0x00,

//这不是交易的最后通行证
//可以重试事务，允许软故障
    tapRETRY            = 0x20,

//交易记录必须支付超过两个未结分类帐的金额
//费用和队列中的所有交易进入
//开式分类帐
    tapPREFER_QUEUE     = 0x40,

//事务来自特权源
    tapUNLIMITED        = 0x400,
};

constexpr
ApplyFlags
operator|(ApplyFlags const& lhs,
    ApplyFlags const& rhs)
{
    return safe_cast<ApplyFlags>(
        safe_cast<std::underlying_type_t<ApplyFlags>>(lhs) |
            safe_cast<std::underlying_type_t<ApplyFlags>>(rhs));
}

static_assert((tapPREFER_QUEUE | tapRETRY) == safe_cast<ApplyFlags>(0x60u),
    "ApplyFlags operator |");
static_assert((tapRETRY | tapPREFER_QUEUE) == safe_cast<ApplyFlags>(0x60u),
    "ApplyFlags operator |");

constexpr
ApplyFlags
operator&(ApplyFlags const& lhs,
    ApplyFlags const& rhs)
{
    return safe_cast<ApplyFlags>(
        safe_cast<std::underlying_type_t<ApplyFlags>>(lhs) &
            safe_cast<std::underlying_type_t<ApplyFlags>>(rhs));
}

static_assert((tapPREFER_QUEUE & tapRETRY) == tapNONE,
    "ApplyFlags operator &");
static_assert((tapRETRY & tapPREFER_QUEUE) == tapNONE,
    "ApplyFlags operator &");

constexpr
ApplyFlags
operator~(ApplyFlags const& flags)
{
    return safe_cast<ApplyFlags>(
        ~safe_cast<std::underlying_type_t<ApplyFlags>>(flags));
}

static_assert(~tapRETRY == safe_cast<ApplyFlags>(0xFFFFFFDFu),
    "ApplyFlags operator ~");

inline
ApplyFlags
operator|=(ApplyFlags & lhs,
    ApplyFlags const& rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

inline
ApplyFlags
operator&=(ApplyFlags& lhs,
    ApplyFlags const& rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*用于应用交易的分类帐的可写视图。

    这种对readview的改进提供了一个接口，
    SLE可以“签出”进行修改并放置
    恢复到更新或删除状态。还添加了一个
    提供必要的上下文信息的接口
    要计算事务处理的结果，
    如果视图稍后应用于
    父级（使用派生类中的接口）。
    上下文信息还包括基值
    分类帐，如序列号和网络时间。

    这允许实现记录对
    分类帐中的状态项，可选择应用
    对基础所做的更改或在没有
    影响基础。

    典型的用法是调用read（）进行非变异
    操作。

    对于突变操作，顺序如下：

        //添加新值
        插入（SLE）；

        //签出要修改的值
        sle=v.peek（k）；

        //指示进行了更改
        V.UPDATE（SLE）

        //或者，删除该值
        擦除（SLE）

    不变量是插入、更新和擦除可能不会
    使用属于不同视图的任何SLE调用。
**/

class ApplyView
    : public ReadView
{
private:
    /*使用指定的插入策略向目录添加条目*/
    boost::optional<std::uint64_t>
    dirAdd (
        bool preserveOrder,
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe);

public:
    ApplyView () = default;

    /*返回Tx应用标志。

        标志会影响事务的结果
        处理。例如，应用的事务
        到打开的分类帐生成“本地”故障，
        而交易适用于共识
        分类帐会产生严重的故障（并收取费用）。
    **/

    virtual
    ApplyFlags
    flags() const = 0;

    /*准备修改与键关联的SLE。

        影响：

            为调用方提供可修改的
            与指定密钥关联的SLE。

        返回的SLE可用于后续
        调用以擦除或更新。

        SLE不能传递给任何其他ApplyView。

        @return`nullptr`如果键不存在
    **/

    virtual
    std::shared_ptr<SLE>
    peek (Keylet const& k) = 0;

    /*移除窥视的SLE。

        要求：

            'sle'是从先前调用peek（）获得的
            在rawview的这个实例上。

        影响：

            密钥不再与SLE关联。
    **/

    virtual
    void
    erase (std::shared_ptr<SLE> const& sle) = 0;

    /*插入新的状态SLE

        要求：

            'sle'未从任何对
            对rawview的任何实例进行peek（）。

            SLE的密钥必须不存在。

        影响：

            状态映射中的键已关联
            用SLE。

            rawview获得共享资源的所有权。

        @注意，钥匙取自SLE
    **/

    virtual
    void
    insert (std::shared_ptr<SLE> const& sle) = 0;

    /*显示对窥视SLE的更改

        要求：

            SLE的密钥必须存在。

            'sle'是从先前调用peek（）获得的
            在rawview的这个实例上。

        影响：

            SLE已更新

        @注意，钥匙取自SLE
    **/

    /*@ {*/
    virtual
    void
    update (std::shared_ptr<SLE> const& sle) = 0;

//——————————————————————————————————————————————————————————————

//向帐户贷记时调用
//这是支持PaymentSandbox所必需的
    virtual void
    creditHook (AccountID const& from,
        AccountID const& to,
        STAmount const& amount,
        STAmount const& preCreditBalance)
    {
    }

//当所有者计数更改时调用
//这是支持PaymentSandbox所必需的
    virtual
    void adjustOwnerCountHook (AccountID const& account,
        std::uint32_t cur, std::uint32_t next)
    {}

    /*将条目附加到目录

        目录中的条目将按插入顺序存储，即新条目
        条目将始终添加到最后一页的末尾。

        @param directory目录的基目录
        @param键要插入的项
        @param描述回调以将所需条目添加到新页面

        @返回一个\c boost：：可选，如果插入成功，
                将包含存储项的页码。

        @注意：如果没有，这个函数可以创建一个页面（包括根页面）。
              
              页计数器超过协议定义的最大数目
              允许的页面。
    **/

    /*@ {*/
    boost::optional<std::uint64_t>
    dirAppend (
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAdd (true, directory, key, describe);
    }

    boost::optional<std::uint64_t>
    dirAppend (
        Keylet const& directory,
        Keylet const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAppend (directory, key.key, describe);
    }
    /*@ }*/

    /*将条目插入目录

        目录中的条目将以半随机顺序存储，但是
        每一页将按排序顺序维护。

        @param directory目录的基目录
        @param键要插入的项
        @param描述回调以将所需条目添加到新页面

        @返回一个\c boost：：可选，如果插入成功，
                将包含存储项的页码。

        @注意：如果没有，这个函数可以创建一个页面（包括根页面）。
              有空格的页可用。只有在
              页计数器超过协议定义的最大数目
              允许的页面。
    **/

    /*@ {*/
    boost::optional<std::uint64_t>
    dirInsert (
        Keylet const& directory,
        uint256 const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirAdd (false, directory, key, describe);
    }

    boost::optional<std::uint64_t>
    dirInsert (
        Keylet const& directory,
        Keylet const& key,
        std::function<void(std::shared_ptr<SLE> const&)> const& describe)
    {
        return dirInsert (directory, key.key, describe);
    }
    /*@ }*/

    /*从目录中删除条目

        @param directory目录的基目录
        @param page此页的页码
        @param键要删除的项
        @param keeproot如果删除最后一个条目，不要
                        删除根页面（即目录本身）。

        @返回\c如果找到并删除了条目
                \c否则为false。

        @注意：此函数将从目录中删除零页或更多页；
              即使根页面为空，也不会删除，除非
              \p未设置keeprot，目录为空。
    **/

    /*@ {*/
    bool
    dirRemove (
        Keylet const& directory,
        std::uint64_t page,
        uint256 const& key,
        bool keepRoot);

    bool
    dirRemove (
        Keylet const& directory,
        std::uint64_t page,
        Keylet const& key,
        bool keepRoot)
    {
        return dirRemove (directory, page, key.key, keepRoot);
    }
    /*@ }*/
};

} //涟漪

#endif
