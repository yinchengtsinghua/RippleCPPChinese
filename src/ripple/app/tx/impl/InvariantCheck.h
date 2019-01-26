
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_TX_INVARIANTCHECK_H_INCLUDED
#define RIPPLE_APP_TX_INVARIANTCHECK_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/utility/Journal.h>
#include <map>
#include <tuple>
#include <utility>
#include <cstdint>

namespace ripple {

#if GENERATING_DOCS
/*
 *@brief原型用于不变量检查实现。
 *
 *此类不存在，或者更确切地说，它只存在于文档中
 *通信任何不变检查器所需的接口。任何不变量
 *检查实施情况应执行此处记录的公共方法。
 *
 **/

class InvariantChecker_PROTOTYPE
{
public:
    explicit InvariantChecker_PROTOTYPE() = default;

    /*
     *@brief调用了当前交易中的每个分类账分录。
     *
     *@param index分类账分录的键（标识符）
     *@param isdelete true如果删除SLE
     *@param在交易记录修改前的分类账分录前
     *@param交易修改后的分类账分录后
     **/

    void
    visitEntry(
        uint256 const& index,
        bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after);

    /*
     访问所有分类账条目后调用了*@brief以确定
     *支票的最终状态
     *
     *@param tx正在应用的事务
     *@param tec事务的当前ter结果
     *@param fee本次交易实际收取的费用
     *@param j日志
     *
     *@check通过返回true，失败返回false
     **/

    bool
    finalize(
        STTx const& tx,
        TER const tec,
        XRPAmount const fee,
        beast::Journal const& j);
};
#endif

/*
 *@brief invariant:我们不应该向交易收取负费用或
 *费用大于交易本身规定的费用。
 *
 *在某些情况下，我们可以降低收费。
 **/

class TransactionFeeCheck
{
public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:事务不能创建xrp，应该只销毁
 ＊XRP费。
 *
 *我们迭代所有账户根、支付渠道和托管条目。
 *修改后，计算由
 *交易。
 **/

class XRPNotCreated
{
    std::int64_t drops_ = 0;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:我们无法删除会计分录
 *
 *我们迭代所有已修改的帐户根，并确保
 *在应用交易前存在继续存在
 *之后。
 **/

class AccountRootsNotDeleted
{
    bool accountDeleted_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:帐户xrp余额必须在xrp中并取一个值
 *从0到系统货币开始下降，包括。
 *
 *我们迭代该事务修改的所有帐户根，并确保
 *他们的XRP余额是合理的。
 **/

class XRPBalanceChecks
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:相应修改的分类账条目应在类型中匹配
 *并且添加的条目应该是有效的类型。
 **/

class LedgerEntryTypesMatch
{
    bool typeMismatch_ = false;
    bool invalidTypeAdded_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:不允许使用xrp的信任行。
 *
 *我们迭代此事务创建的所有信任行，并确保
 *他们反对有效的发行人。
 **/

class NoXRPTrustLines
{
    bool xrpTrustLine_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);
};

/*
 *@brief invariant:优惠金额应为非负数，不得
 *从xrp到xrp。
 *
 *检查交易修改的所有报价，并确保
 *没有包含负金额或将XRP兑换为XRP的优惠。
 **/

class NoBadOffers
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);

};

/*
 *@brief invariant:托管条目的值必须介于0和之间
 *系统\货币\开始以独占方式删除。
 **/

class NoZeroEscrow
{
    bool bad_ = false;

public:
    void
    visitEntry(
        uint256 const&,
        bool,
        std::shared_ptr<SLE const> const&,
        std::shared_ptr<SLE const> const&);

    bool
    finalize(STTx const&, TER const, XRPAmount const, beast::Journal const&);

};

//可以在上面声明其他不变检查，然后将其添加到
//元组
using InvariantChecks = std::tuple<
    TransactionFeeCheck,
    AccountRootsNotDeleted,
    LedgerEntryTypesMatch,
    XRPBalanceChecks,
    XRPNotCreated,
    NoXRPTrustLines,
    NoBadOffers,
    NoZeroEscrow
>;

/*
 *@brief获取所有不变检查的元组
 *
 *@return std：：实现所需不变检查的实例的元组
 ＊方法
 *
 *@参见Ripple:：InvariantChecker_原型
 **/

inline
InvariantChecks
getInvariantChecks()
{
    return InvariantChecks{};
}

} //涟漪

#endif
