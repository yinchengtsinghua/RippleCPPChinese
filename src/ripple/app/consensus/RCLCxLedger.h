
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
    版权所有（c）2012-2016 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_CONSENSUS_RCLCXLEDGER_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXLEDGER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <memory>

namespace ripple {

/*表示rclcommersion中的分类帐。

    rclcxleger是对'std:：shared_ptr<ledger const>
**/

class RCLCxLedger
{
public:
//！分类帐的唯一标识符
    using ID = LedgerHash;
//！分类帐的序号
    using Seq = LedgerIndex;

    /*默认构造函数

        托多：如果我们确保向RCLconsension提交一份有效的
        其构造函数中的分类帐。现在很糟糕，因为其他成员
        正在检查分类帐是否有效。
    **/

    RCLCxLedger() = default;

    /*构造函数

        @param l要包装的分类帐。
    **/

    RCLCxLedger(std::shared_ptr<Ledger const> const& l) : ledger_{l}
    {
    }

//！分类帐的序列号。
    Seq const&
    seq() const
    {
        return ledger_->info().seq;
    }

//！此分类帐的唯一标识符（哈希）。
    ID const&
    id() const
    {
        return ledger_->info().hash;
    }

//！此分类帐父级的唯一标识符（哈希）。
    ID const&
    parentID() const
    {
        return ledger_->info().parentHash;
    }

//！计算此分类帐的关闭时间时使用的解决方案。
    NetClock::duration
    closeTimeResolution() const
    {
        return ledger_->info().closeTimeResolution;
    }

//！协商过程是否同意分类账的关闭时间。
    bool
    closeAgree() const
    {
        return ripple::getCloseAgree(ledger_->info());
    }

//！这个分类帐的关闭时间
    NetClock::time_point
    closeTime() const
    {
        return ledger_->info().closeTime;
    }

//！此分类帐父级的关闭时间。
    NetClock::time_point
    parentCloseTime() const
    {
        return ledger_->info().parentCloseTime;
    }

//！此分类帐的JSON表示。
    Json::Value
    getJson() const
    {
        return ripple::getJson(*ledger_);
    }

    /*分类帐实例。

        TODO:将此共享资源设为\ptr<readview const>。需要创建
        阅读视图中的新分类帐？
    **/

    std::shared_ptr<Ledger const> ledger_;
};
}
#endif
