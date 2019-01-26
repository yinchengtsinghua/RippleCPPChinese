
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

#ifndef RIPPLE_APP_MISC_AMENDMENTTABLE_H_INCLUDED
#define RIPPLE_APP_MISC_AMENDMENTTABLE_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/Protocol.h>

namespace ripple {

/*修正表存储已启用和潜在修正的列表。
    个人修正案在协商一致期间由验证者投票表决。
    过程。
**/

class AmendmentTable
{
public:
    virtual ~AmendmentTable() = default;

    virtual uint256 find (std::string const& name) = 0;

    virtual bool veto (uint256 const& amendment) = 0;
    virtual bool unVeto (uint256 const& amendment) = 0;

    virtual bool enable (uint256 const& amendment) = 0;
    virtual bool disable (uint256 const& amendment) = 0;

    virtual bool isEnabled (uint256 const& amendment) = 0;
    virtual bool isSupported (uint256 const& amendment) = 0;

    /*
     *@brief如果网络上有一个或多个修改，则返回true
     *已启用，但此服务器不支持
     *
     *@如果网络上启用了不支持的功能，则返回true
     **/

    virtual bool hasUnsupportedEnabled () = 0;

    virtual Json::Value getJson (int) = 0;

    /*返回json:：objectValue。*/
    virtual Json::Value getJson (uint256 const& ) = 0;

    /*当接受新的完全验证的分类帐时调用。*/
    void doValidatedLedger (std::shared_ptr<ReadView const> const& lastValidatedLedger)
    {
        if (needValidatedLedger (lastValidatedLedger->seq ()))
            doValidatedLedger (lastValidatedLedger->seq (),
                getEnabledAmendments (*lastValidatedLedger));
    }

    /*调用以确定是否需要处理修正逻辑
        新的已验证的分类帐。（如果它能改变一切的话。）
    **/

    virtual bool
    needValidatedLedger (LedgerIndex seq) = 0;

    virtual void
    doValidatedLedger (
        LedgerIndex ledgerSeq,
        std::set <uint256> const& enabled) = 0;

//当我们需要
//注入伪事务
    virtual std::map <uint256, std::uint32_t>
    doVoting (
        NetClock::time_point closeTime,
        std::set <uint256> const& enabledAmendments,
        majorityAmendments_t const& majorityAmendments,
        std::vector<STValidation::pointer> const& valSet) = 0;

//当我们需要
//向验证添加功能项
    virtual std::vector <uint256>
    doValidation (std::set <uint256> const& enabled) = 0;

//在创世纪分类账中启用的一组修订
//这将返回所有已知的、未被否决的修正案。
//如果我们有两个修正案，那不应该同时是
//在启用的同时，要确保一个被否决。
    virtual std::vector <uint256>
    getDesired () = 0;

//下面的函数将API调用方期望的
//内部修订表API。这允许修正
//表实现独立于分类帐
//实施。当视图代码
//支持完整的分类帐API

    void
    doVoting (
        std::shared_ptr <ReadView const> const& lastClosedLedger,
        std::vector<STValidation::pointer> const& parentValidations,
        std::shared_ptr<SHAMap> const& initialPosition)
    {
//询问实施部门要做什么
        auto actions = doVoting (
            lastClosedLedger->parentCloseTime(),
            getEnabledAmendments(*lastClosedLedger),
            getMajorityAmendments(*lastClosedLedger),
            parentValidations);

//注入适当的伪事务
        for (auto const& it : actions)
        {
            STTx amendTx (ttAMENDMENT,
                [&it, seq = lastClosedLedger->seq() + 1](auto& obj)
                {
                    obj.setAccountID (sfAccount, AccountID());
                    obj.setFieldH256 (sfAmendment, it.first);
                    obj.setFieldU32 (sfLedgerSequence, seq);

                    if (it.second != 0)
                        obj.setFieldU32 (sfFlags, it.second);
                });

            Serializer s;
            amendTx.add (s);

            initialPosition->addGiveItem (
                std::make_shared <SHAMapItem> (
                    amendTx.getTransactionID(),
                    s.peekData()),
                true,
                false);
        }
    }

};

std::unique_ptr<AmendmentTable> make_AmendmentTable (
    std::chrono::seconds majorityTime,
    int majorityFraction,
    Section const& supported,
    Section const& enabled,
    Section const& vetoed,
    beast::Journal journal);

}  //涟漪

#endif
