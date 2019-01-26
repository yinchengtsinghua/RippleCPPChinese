
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

#ifndef RIPPLE_LEDGER_APPLYVIEWIMPL_H_INCLUDED
#define RIPPLE_LEDGER_APPLYVIEWIMPL_H_INCLUDED

#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/detail/ApplyViewBase.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/TER.h>
#include <boost/optional.hpp>

namespace ripple {

/*可编辑、可丢弃的视图，可以为一个Tx生成元数据。

    将Tx映射迭代委托给基础。

    @说明显示为客户端的ApplyView。
**/

class ApplyViewImpl final
    : public detail::ApplyViewBase
{
public:
    ApplyViewImpl() = delete;
    ApplyViewImpl (ApplyViewImpl const&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl&&) = delete;
    ApplyViewImpl& operator= (ApplyViewImpl const&) = delete;

    ApplyViewImpl (ApplyViewImpl&&) = default;
    ApplyViewImpl(
        ReadView const* base, ApplyFlags flags);

    /*应用事务。

        调用“apply”后，唯一有效的
        此对象的操作是调用
        析构函数。
    **/

    void
    apply (OpenView& to,
        STTx const& tx, TER ter,
            beast::Journal j);

    /*设置已交货的货币金额。

        此值在生成元数据时使用
        对于付款，设置DeliveredAmount字段。
        如果未指定金额，则字段为
        从生成的元数据中排除。
    **/

    void
    deliver (STAmount const& amount)
    {
        deliver_ = amount;
    }

    /*获取修改的条目数
    **/

    std::size_t
    size ();

    /*访问修改的条目
    **/

    void
    visit (
        OpenView& target,
        std::function <void (
            uint256 const& key,
            bool isDelete,
            std::shared_ptr <SLE const> const& before,
            std::shared_ptr <SLE const> const& after)> const& func);
private:
    boost::optional<STAmount> deliver_;
};

} //涟漪

#endif
