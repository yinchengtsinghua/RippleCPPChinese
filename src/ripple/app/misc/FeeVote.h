
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

#ifndef RIPPLE_APP_MISC_FEEVOTE_H_INCLUDED
#define RIPPLE_APP_MISC_FEEVOTE_H_INCLUDED

#include <ripple/ledger/ReadView.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/protocol/SystemParameters.h>

namespace ripple {

/*经理处理费用投票。*/
class FeeVote
{
public:
    /*投票费用表。
        在投票分类账中，feevote逻辑将尝试向
        这些值在注入费用设置交易时。
        默认构造的设置包含建议的值。
    **/

    struct Setup
    {
        /*参考交易记录的下降成本。*/
        std::uint64_t reference_fee = 10;

        /*以费用单位表示的参考交易记录的成本。*/
        std::uint32_t const reference_fee_units = 10;

        /*存款准备金要求下降。*/
        std::uint64_t account_reserve = 20 * SYSTEM_CURRENCY_PARTS;

        /*每件拥有的物品储备需求下降。*/
        std::uint64_t owner_reserve = 5 * SYSTEM_CURRENCY_PARTS;
    };

    virtual ~FeeVote () = default;

    /*将本地费用首选项添加到验证中。

        @param lastclosedledger参数
        @param basevalidation参数验证
    **/

    virtual
    void
    doValidation (std::shared_ptr<ReadView const> const& lastClosedLedger,
        STValidation::FeeSettings & fees) = 0;

    /*对费用进行本地投票。

        @param lastclosedledger参数
        @参数初始位置
    **/

    virtual
    void
    doVoting (std::shared_ptr<ReadView const> const& lastClosedLedger,
        std::vector<STValidation::pointer> const& parentValidations,
            std::shared_ptr<SHAMap> const& initialPosition) = 0;
};

/*从配置部分生成feevote:：setup。*/
FeeVote::Setup
setup_FeeVote (Section const& section);

/*创建feevote逻辑的实例。
    @param设置要投票的费用计划。
    @param journal记录到哪里。
**/

std::unique_ptr <FeeVote>
make_FeeVote (FeeVote::Setup const& setup, beast::Journal journal);

} //涟漪

#endif
