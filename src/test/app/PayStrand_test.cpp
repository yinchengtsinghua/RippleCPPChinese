
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

#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/Sandbox.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>
#include <test/jtx/PathSet.h>

namespace ripple {
namespace test {

struct DirectStepInfo
{
    AccountID src;
    AccountID dst;
    Currency currency;
};

struct XRPEndpointStepInfo
{
    AccountID acc;
};

enum class TrustFlag {freeze, auth, noripple};

/*onstexpr*/std:：uint32_t trustflag（trustflag f，bool usehigh）
{
    开关（F）
    {
        case trustflag：：冻结：
            如果（使用高）
                返回lsfhighfreeze；
            返回lsflowfreeze；
        case trustflag：：身份验证：
            如果（使用高）
                返回lsfhighauth；
            返回lsflowauth；
        case trustflag:：noripple:
            如果（使用高）
                返回lsfhighnoriple；
            返回lsflownoripple；
    }
    返回0；//关于非void函数结束的静默警告
}

布尔
获取信任标志
    jtx:：env常量和env，
    jtx:：account const&src，
    JTX：账户常量和DST，
    货币常量和当前值，
    信任标志标志
{
    if（auto sle=env.le（keylet:：line（src，dst，cur）））
    {
        auto const usehigh=src.id（）>dst.id（）；
        返回sle->isflag（trustflag（flag，usehigh））；
    }
    throw<std:：runtime_error>（“gettrustflag中没有行”）；
    返回false；//静音警告
}

布尔
相等（std:：unique_ptr<step>const&s1，directstepinfo const&dsi）
{
    如果（！）S1）
        返回错误；
    返回测试：：DirectStepEqual（*s1，dsi.src，dsi.dst，dsi.currency）；
}

布尔
等于（std:：unique_ptr<step>const&s1，xrpendpointstepinfo const&xrpsi）
{
    如果（！）S1）
        返回错误；
    返回测试：：xrpendpointstepequal（*s1，xrpsi.acc）；
}

布尔
等于（std:：unique_ptr<step>const&s1，ripple:：book const&bsi）
{
    如果（！）S1）
        返回错误；
    返回BookStepEqual（*s1，bsi）；
}

模板<class iter>
布尔
StrandeQualHelper（ITER一）
{
    //基本情况。所有参数都已处理并发现相等。
    回归真实；
}

template<class iter，class stepinfo，class…ARG>
布尔
StrandeQualHelper（ITER I、StepInfo和Si、Args和…ARGS）
{
    如果（！）等于（*i，std:：forward<stepinfo>（si）））
        返回错误；
    返回strandEqualHelper（++I，std:：forward<args>（args）…）；
}

模板<类…ARG>
布尔
相等（股结构和股结构，args和&……ARGS）
{
    如果（strend.size（）！=sizeof…（参数）
        返回错误；
    if（strend.empty（））
        回归真实；
    返回strandEqualHelper（strand.begin（），std:：forward<args>（args）…；
}

STATEP元素
APE（accountID const&a）
{
    返回stpathElement（
        stpathElement:：typeaccount，a，xrpcurrency（），xrpaccount（））；
}；

//发出路径元素
STATEP元素
IPE（发布结构和ISS）
{
    返回stpathElement（
        stpathElement:：typecurrency stpathElement:：typeIssuer，
        xrpCub（）
        ISS货币
        国际会计准则；
}；

//颁发者路径元素
STATEP元素
IAPE（accountID const&account）
{
    返回stpathElement（
        stpathElement:：typeIssuer，xrpaccount（），xrpcurrency（），account）；
}；

//货币路径元素
STATEP元素
CPE（货币常量和C）
{
    返回stpathElement（
        stpathElement:：typecurrency，xrpacCount（），c，xrpacCount（））；
}；

//所有路径元素
STATEP元素
ALLPE（accountID const&a、issue const&iss）
{
    返回stpathElement（
        stPathElement:：typeAccount stPathElement:：typeCurrency
            stPathElement：：类型颁发者，
        A
        ISS货币
        国际会计准则；
}；

类元素组合表
{
    枚举类sb/*状态bi*/

        : std::uint16_t
    { acc,
      iss,
      cur,
      rootAcc,
      rootIss,
      xrp,
      sameAccIss,
      existingAcc,
      existingCur,
      existingIss,
      prevAcc,
      prevCur,
      prevIss,
      boundary,
      last };

    std::uint16_t state_ = 0;
    static_assert(safe_cast<size_t>(SB::last) <= sizeof(decltype(state_)) * 8, "");
    STPathElement const* prev_ = nullptr;
//不允许使用ACC指定ISS和CUR（简化某些测试）
    bool const allowCompound_ = false;

    bool
    has(SB s) const
    {
        return state_ & (1 << safe_cast<int>(s));
    }

    bool
    hasAny(std::initializer_list<SB> sb) const
    {
        for (auto const s : sb)
            if (has(s))
                return true;
        return false;
    }

    size_t
    count(std::initializer_list<SB> sb) const
    {
        size_t result=0;

        for (auto const s : sb)
            if (has(s))
                result++;
        return result;
    }

public:
    explicit ElementComboIter(STPathElement const* prev = nullptr) : prev_(prev)
    {
    }

    bool
    valid() const
    {
        return
            (allowCompound_ || !(has(SB::acc) && hasAny({SB::cur, SB::iss}))) &&
            (!hasAny({SB::prevAcc, SB::prevCur, SB::prevIss}) || prev_) &&
            (!hasAny({SB::rootAcc, SB::sameAccIss, SB::existingAcc, SB::prevAcc}) || has(SB::acc)) &&
            (!hasAny({SB::rootIss, SB::sameAccIss, SB::existingIss, SB::prevIss}) || has(SB::iss)) &&
            (!hasAny({SB::xrp, SB::existingCur, SB::prevCur}) || has(SB::cur)) &&
//这些是复制品
            (count({SB::xrp, SB::existingCur, SB::prevCur}) <= 1) &&
            (count({SB::rootAcc, SB::existingAcc, SB::prevAcc}) <= 1) &&
            (count({SB::rootIss, SB::existingIss, SB::rootIss}) <= 1);
    }
    bool
    next()
    {
        if (!(has(SB::last)))
        {
            do
            {
                ++state_;
            } while (!valid());
        }
        return !has(SB::last);
    }

    template <
        class Col,
        class AccFactory,
        class IssFactory,
        class CurrencyFactory>
    void
    emplace_into(
        Col& col,
        AccFactory&& accF,
        IssFactory&& issF,
        CurrencyFactory&& currencyF,
        boost::optional<AccountID> const& existingAcc,
        boost::optional<Currency> const& existingCur,
        boost::optional<AccountID> const& existingIss)
    {
        assert(!has(SB::last));

        auto const acc = [&]() -> boost::optional<AccountID> {
            if (!has(SB::acc))
                return boost::none;
            if (has(SB::rootAcc))
                return xrpAccount();
            if (has(SB::existingAcc) && existingAcc)
                return existingAcc;
            return accF().id();
        }();
        auto const iss = [&]() -> boost::optional<AccountID> {
            if (!has(SB::iss))
                return boost::none;
            if (has(SB::rootIss))
                return xrpAccount();
            if (has(SB::sameAccIss))
                return acc;
            if (has(SB::existingIss) && existingIss)
                return *existingIss;
            return issF().id();
        }();
        auto const cur = [&]() -> boost::optional<Currency> {
            if (!has(SB::cur))
                return boost::none;
            if (has(SB::xrp))
                return xrpCurrency();
            if (has(SB::existingCur) && existingCur)
                return *existingCur;
            return currencyF();
        }();
        if (!has(SB::boundary))
            col.emplace_back(acc, cur, iss);
        else
            col.emplace_back(
                STPathElement::Type::typeBoundary,
                acc.value_or(AccountID{}),
                cur.value_or(Currency{}),
                iss.value_or(AccountID{}));
    }
};

struct ExistingElementPool
{
    std::vector<jtx::Account> accounts;
    std::vector<ripple::Currency> currencies;
    std::vector<std::string> currencyNames;

    jtx::Account
    getAccount(size_t id)
    {
        assert(id < accounts.size());
        return accounts[id];
    }

    ripple::Currency
    getCurrency(size_t id)
    {
        assert(id < currencies.size());
        return currencies[id];
    }

//从0到（nextavail-1）的ID已用于
//路径
    size_t nextAvailAccount = 0;
    size_t nextAvailCurrency = 0;

    using ResetState = std::tuple<size_t, size_t>;
    ResetState
    getResetState() const
    {
        return std::make_tuple(nextAvailAccount, nextAvailCurrency);
    }

    void
    resetTo(ResetState const& s)
    {
        std::tie(nextAvailAccount, nextAvailCurrency) = s;
    }

    struct StateGuard
    {
        ExistingElementPool& p_;
        ResetState state_;

        explicit StateGuard(ExistingElementPool& p) : p_{p}, state_{p.getResetState()}
        {
        }
        ~StateGuard()
        {
            p_.resetTo(state_);
        }
    };

//创建给定数量的帐户，并添加信任行，以便
//帐户以每种货币互相信任
//从每种货币/帐户向其他货币/帐户创建报价
//货币/帐户；报价所有者是指定的
//账户或“承购人获得”账户的发行人
    void
    setupEnv(
        jtx::Env& env,
        size_t numAct,
        size_t numCur,
        boost::optional<size_t> const& offererIndex)
    {
        using namespace jtx;

        assert(!offererIndex || offererIndex < numAct);

        accounts.clear();
        accounts.reserve(numAct);
        currencies.clear();
        currencies.reserve(numCur);
        currencyNames.clear();
        currencyNames.reserve(numCur);

        constexpr size_t bufSize = 32;
        char buf[bufSize];

        for (size_t id = 0; id < numAct; ++id)
        {
            snprintf(buf, bufSize, "A%zu", id);
            accounts.emplace_back(buf);
        }

        for (size_t id = 0; id < numCur; ++id)
        {
            if (id < 10)
                snprintf(buf, bufSize, "CC%zu", id);
            else if (id < 100)
                snprintf(buf, bufSize, "C%zu", id);
            else
                snprintf(buf, bufSize, "%zu", id);
            currencies.emplace_back(to_currency(buf));
            currencyNames.emplace_back(buf);
        }

        for (auto const& a : accounts)
            env.fund(XRP(100000), a);

//每个帐户都用每种货币信任其他帐户
        for (auto ai1 = accounts.begin(), aie = accounts.end(); ai1 != aie;
             ++ai1)
        {
            for (auto ai2 = accounts.begin(); ai2 != aie; ++ai2)
            {
                if (ai1 == ai2)
                    continue;
                for (auto const& cn : currencyNames)
                {
                    env.trust((*ai1)[cn](1'000'000), *ai2);
                    if (ai1 > ai2)
                    {
//指数较低的账户持有
//账户
//索引更高
                        auto const& src = *ai1;
                        auto const& dst = *ai2;
                        env(pay(src, dst, src[cn](500000)));
                    }
                }
                env.close();
            }
        }

        std::vector<IOU> ious;
        ious.reserve(numAct * numCur);
        for (auto const& a : accounts)
            for (auto const& cn : currencyNames)
                ious.emplace_back(a[cn]);

//创建从每种货币到每种其他货币的报价
        for (auto takerPays = ious.begin(), ie = ious.end(); takerPays != ie;
             ++takerPays)
        {
            for (auto takerGets = ious.begin(); takerGets != ie; ++takerGets)
            {
                if (takerPays == takerGets)
                    continue;
                auto const owner =
                    offererIndex ? accounts[*offererIndex] : takerGets->account;
                if (owner.id() != takerGets->account.id())
                    env(pay(takerGets->account, owner, (*takerGets)(1000)));

                env(offer(owner, (*takerPays)(1000), (*takerGets)(1000)),
                    txflags(tfPassive));
            }
            env.close();
        }

//为每个其他借据创建往来于XRP的报价
        for (auto const& iou : ious)
        {
            auto const owner =
                offererIndex ? accounts[*offererIndex] : iou.account;
            env(offer(owner, iou(1000), XRP(1000)), txflags(tfPassive));
            env(offer(owner, XRP(1000), iou(1000)), txflags(tfPassive));
            env.close();
        }
    }

    std::int64_t
    totalXRP(ReadView const& v, bool incRoot)
    {
        std::uint64_t totalXRP = 0;
        auto add = [&](auto const& a) {
//XRP天平
            auto const sle = v.read(keylet::account(a));
            if (!sle)
                return;
            auto const b = (*sle)[sfBalance];
            totalXRP += b.mantissa();
        };
        for (auto const& a : accounts)
            add(a);
        if (incRoot)
            add(xrpAccount());
        return totalXRP;
    }

//检查所有货币和xrp的所有账户余额是否为
//相同的
    bool
    checkBalances(ReadView const& v1, ReadView const& v2)
    {
        std::vector<std::tuple<STAmount, STAmount, AccountID, AccountID>> diffs;

        auto xrpBalance = [](ReadView const& v, ripple::Keylet const& k) {
            auto const sle = v.read(k);
            if (!sle)
                return STAmount{};
            return (*sle)[sfBalance];
        };
        auto lineBalance = [](ReadView const& v, ripple::Keylet const& k) {
            auto const sle = v.read(k);
            if (!sle)
                return STAmount{};
            return (*sle)[sfBalance];
        };
        std::uint64_t totalXRP[2];
        for (auto ai1 = accounts.begin(), aie = accounts.end(); ai1 != aie;
             ++ai1)
        {
            {
//XRP天平
                auto const ak = keylet::account(*ai1);
                auto const b1 = xrpBalance(v1, ak);
                auto const b2 = xrpBalance(v2, ak);
                totalXRP[0] += b1.mantissa();
                totalXRP[1] += b2.mantissa();
                if (b1 != b2)
                    diffs.emplace_back(b1, b2, xrpAccount(), *ai1);
            }
            for (auto ai2 = accounts.begin(); ai2 != aie; ++ai2)
            {
                if (ai1 >= ai2)
                    continue;
                for (auto const& c : currencies)
                {
//线路平衡
                    auto const lk = keylet::line(*ai1, *ai2, c);
                    auto const b1 = lineBalance(v1, lk);
                    auto const b2 = lineBalance(v2, lk);
                    if (b1 != b2)
                        diffs.emplace_back(b1, b2, *ai1, *ai2);
                }
            }
        }
        return diffs.empty();
    }

    jtx::Account
    getAvailAccount()
    {
        return getAccount(nextAvailAccount++);
    }

    ripple::Currency
    getAvailCurrency()
    {
        return getCurrency(nextAvailCurrency++);
    }

    template <class F>
    void
    for_each_element_pair(
        STAmount const& sendMax,
        STAmount const& deliver,
        std::vector<STPathElement> const& prefix,
        std::vector<STPathElement> const& suffix,
        boost::optional<AccountID> const& existingAcc,
        boost::optional<Currency> const& existingCur,
        boost::optional<AccountID> const& existingIss,
        F&& f)
    {
        auto accF = [&] { return this->getAvailAccount(); };
        auto issF = [&] { return this->getAvailAccount(); };
        auto currencyF = [&] { return this->getAvailCurrency(); };

        STPathElement const* prevOuter =
            prefix.empty() ? nullptr : &prefix.back();
        ElementComboIter outer(prevOuter);

        std::vector<STPathElement> outerResult;
        std::vector<STPathElement> result;
        auto const resultSize = prefix.size() + suffix.size() + 2;
        outerResult.reserve(resultSize);
        result.reserve(resultSize);
        while (outer.next())
        {
            StateGuard og{*this};
            outerResult = prefix;
            outer.emplace_into(
                outerResult,
                accF,
                issF,
                currencyF,
                existingAcc,
                existingCur,
                existingIss);
            STPathElement const* prevInner = &outerResult.back();
            ElementComboIter inner(prevInner);
            while (inner.next())
            {
                StateGuard ig{*this};
                result = outerResult;
                inner.emplace_into(
                    result,
                    accF,
                    issF,
                    currencyF,
                    existingAcc,
                    existingCur,
                    existingIss);
                result.insert(result.end(), suffix.begin(), suffix.end());
                f(sendMax, deliver, result);
            }
        };
    }
};

struct PayStrandAllPairs_test : public beast::unit_test::suite
{
//在路径上测试元素类型对的每个组合
    void
    testAllPairs(FeatureBitset features)
    {
        testcase("All pairs");
        using namespace jtx;
        using RippleCalc = ::ripple::path::RippleCalc;

        ExistingElementPool eep;
        Env env(*this, features);

        auto const closeTime = fix1298Time() +
            100 * env.closed()->info().closeTimeResolution;
        env.close(closeTime);
        /*.setupenv（env，/*numacc*/9，/*numcur*/6，boost:：none）；
        关闭（）；

        auto const src=eep.getavailaccount（）；
        auto const dst=eep.getAvailaccount（）；

        RippleCalc：：输入；
        inputs.defaultPathSallowed=false；

        自动回调=[&]
            Stamount常量和sendmax，
            组装和交付，
            std:：vector<stpathelement>const&p）
            std:：array<paymentsandbox，2>sbs_
                paymentSandbox env.current（）.get（），tapnone，
                 paymentSandbox env.current（）.get（），tapnone _
            std:：array<ripplecalc:：output，2>r输出；
            //同时使用env1和env2支付
            //检查所有结果和帐户余额是否匹配
            //保存结果，以便查看资金是否用完或
            stpathset路径；
            路径。放回原处（P）；
            对于（auto i=0；i<2；++i）
            {
                如果（i＝0）
                    env.app（）.config（）.features.insert（功能流）；
                其他的
                    env.app（）.config（）.features.erase（功能流）；

                尝试
                {
                    rcoutputs[i]=波纹计算：：波纹计算（
                        SBS [Ⅰ]，
                        森达马克斯
                        交付，
                        DST
                        SRC，
                        路径，
                        env.app（）.logs（），
                        和输入）；
                }
                接住（…）
                {
                    这个->失败（）；
                }
            }

            //检查SRC和DST货币组合（包括XRP）
            //检查结果
            auto const termatch=[&]
                if（rcoutputs[0].result（）==rcoutputs[1].result（））
                    回归真实；

                //处理一些已知错误代码不匹配
                if（p.empty（）
                    ！（rcoutputs[0].result（）==tembad_path_
                      rcoutputs[0].result（）==tembad_path_loop））。
                    返回错误；

                if（rcoutputs[1].result（）==tembad_path）
                    回归真实；

                if（rcoutputs[1].result（）==terno_line）
                    回归真实；

                用于（auto const&pe:p）
                {
                    auto const t=pe.getNodeType（）；
                    if（（t&stPathElement:：typeAccount）&&
                        T！=stPathElement：：类型帐户）
                    {
                        回归真实；
                    }
                }

                //xrp，后跟不同时指定货币和
                //发行人（货币不是xrp，如果指定）
                如果（isxrp（sendmax）&&
                    ！（p[0].hascurrency（）&&isxrp（p[0].getcurrency（））&&
                    ！（p[0].hasCurrency（）&&p[0].hasIssuer（）））
                {
                    回归真实；
                }

                对于（尺寸t i=0；i<p.尺寸（）-1；++i）
                {
                    auto const tcur=p[i].getNodeType（）；
                    auto const tnext=p[i+1].getNodeType（）；
                    if（（tcur&stpathelement:：typecurrency）&&
                        isxrp（p[i].getcurrency（））和&
                        （下一个&stPathElement:：typeAccount）&&
                        ！isxrp（p[i+1].getAccountID（）））
                    {
                        回归真实；
                    }
                }
                返回错误；
            }（）；

            这个->Beast_Expect（
                termatch&（rcoutputs[0].result（）==tessuccess_
                             rcoutputs[0].result（）==tembad_path_
                             rcoutputs[0].result（）==tembad_path_loop））；
            if（termatch和rcoutputs[0].result（）==tessuccess）
                this->beast_expect（eep.checkbalances（sbs[0]，sbs[1]）；
        }；

        std:：vector<stpathelement>prefix；
        std:：vector<stpathelement>后缀；

        for（自动常量srcamtisxrp:假，真）
        {
            for（自动常量dstamtisxrp:假，真）
            {
                for（自动常量hasPrefix：假，真）
                {
                    现有元素池：：Stateguard ESG EEP
                    前缀.Car（）；
                    后缀.Car（）；

                    stamount const sendmax_
                        SRCAMISTXRP？xrpissue（）：发布eep.getavailcurrency（），
                                                         eep.getavailaccount（），
                        -1，//（-1==无限制）
                        0 }；

                    Stamount const deliver_
                        DSTATISXRP？xrpissue（）：发布eep.getavailcurrency（），
                                                         eep.getavailaccount（），
                        1，
                        0 }；

                    如果（HASPROFIX）
                    {
                        for（自动常量e0isaccount:假，真）
                        {
                            for（自动常量e1isaccount:假，真）
                            {
                                现有元素池：：Stateguard Presg eep
                                前缀.Car（）；
                                自动推动元素=
                                    [&prefix，&eep]（bool isaccount）可变
                                        如果（ISCART）
                                            prefix.emplace_back（
                                                eep.getAvailaccount（）.id（），
                                                没有：
                                                Boo:：没有）；
                                        其他的
                                            prefix.emplace_back（
                                                没有：
                                                eep.getavailcurrency（），
                                                eep.getAvailaccount（）.id（））；
                                    }；
                                pushElement（e0isaccount）；
                                pushElement（e1isaccount）；
                                boost：：可选<accountid>existingacc；
                                boost：：可选<currency>existingcur；
                                boost：：可选<accountid>existingiss；
                                如果（e0isaccount）
                                {
                                    existingAcc=前缀[0].getAccountID（）；
                                }
                                其他的
                                {
                                    existingiss=前缀[0].getIssuerID（）；
                                    existingcur=前缀[0].getcurrency（）；
                                }
                                如果（e1isaccount）
                                {
                                    如果（！）现有ACC）
                                        existingAcc=前缀[1].getAccountID（）；
                                }
                                其他的
                                {
                                    如果（！）存在的ISS）
                                        existingiss=前缀[1].getIssuerID（）；
                                    如果（！）存在的CURR）
                                        existingcur=前缀[1].getcurrency（）；
                                }
                                eep.for_each_element_pair（
                                    森达马克斯
                                    交付，
                                    前缀，
                                    后缀，
                                    现有ACC，
                                    现存的
                                    存在的，
                                    回调）；
                            }
                        }
                    }
                    其他的
                    {
                        eep.for_each_element_pair（
                            森达马克斯
                            交付，
                            前缀，
                            后缀，
                            /*存在AC*/ boost::none,

                            /*xistingcur*/boost：：无，
                            /*存在*/ boost::none,

                            callback);
                    }
                }
            }
        }
    }
    void
    run() override
    {
        auto const sa = jtx::supported_amendments();
        testAllPairs(sa - featureFlowCross);
        testAllPairs(sa);
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(PayStrandAllPairs, app, ripple, 12);

struct PayStrand_test : public beast::unit_test::suite
{
    void
    testToStrand(FeatureBitset features)
    {
        testcase("To Strand");

        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const eurC = EUR.currency;
        auto const usdC = USD.currency;

        using D = DirectStepInfo;
        using B = ripple::Book;
        using XRPS = XRPEndpointStepInfo;

        auto test = [&, this](
            jtx::Env& env,
            Issue const& deliver,
            boost::optional<Issue> const& sendMaxIssue,
            STPath const& path,
            TER expTer,
            auto&&... expSteps) {
            auto r = toStrand(
                *env.current(),
                alice,
                bob,
                deliver,
                boost::none,
                sendMaxIssue,
                path,
                true,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == expTer);
            if (sizeof...(expSteps) !=0 )
                BEAST_EXPECT(equal(
                    r.second, std::forward<decltype(expSteps)>(expSteps)...));
        };

        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env(pay(gw, alice, EUR(100)));

            {
                STPath const path =
                    STPath({ipe(bob["USD"]), cpe(EUR.currency)});
                auto r = toStrand(
                    *env.current(),
                    alice,
                    alice,
                    /*eliver*/xrpissue（），
                    /LimQualQuITIT*/ boost::none,

                    /*endmaxissue*/eur.issue（），
                    路径，
                    真的，
                    错误的，
                    env.app（）.logs（）.journal（“流”））；
                Beast_Expect（r.first==Tessuccess）；
            }
            {
                stpath const path=stpath（ipe（美元），cpe（xrpcurrency（）））；
                自动R=Tostrand（
                    ＊Env.Currn（）
                    爱丽丝，
                    爱丽丝，
                    /*交付*/ xrpIssue(),

                    /*imitquality*/boost：：无，
                    /*SDENDMAX SUSCR*/ xrpIssue(),

                    path,
                    true,
                    false,
                    env.app().logs().journal("Flow"));
                BEAST_EXPECT(r.first == tesSUCCESS);
            }
            return;
        };

        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, carol, gw);

            test(env, USD, boost::none, STPath(), terNO_LINE);

            env.trust(USD(1000), alice, bob, carol);
            test(env, USD, boost::none, STPath(), tecPATH_DRY);

            env(pay(gw, alice, USD(100)));
            env(pay(gw, carol, USD(100)));

//插入隐含帐户
            test(
                env,
                USD,
                boost::none,
                STPath(),
                tesSUCCESS,
                D{alice, gw, usdC},
                D{gw, bob, usdC});
            env.trust(EUR(1000), alice, bob);

//插入暗示要约
            test(
                env,
                EUR,
                USD.issue(),
                STPath(),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, EUR},
                D{gw, bob, eurC});

//有明确报价的路径
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(EUR)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, EUR},
                D{gw, bob, eurC});

//仅更改发行者的报价路径
            env.trust(carol["USD"](1000), bob);
            test(
                env,
                carol["USD"],
                USD.issue(),
                STPath({iape(carol)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, carol["USD"]},
                D{carol, bob, usdC});

//使用xrp-src-currency的路径
            test(
                env,
                USD,
                xrpIssue(),
                STPath({ipe(USD)}),
                tesSUCCESS,
                XRPS{alice},
                B{XRP, USD},
                D{gw, bob, usdC});

//使用xrp dst currency的路径
            test(
                env,
                xrpIssue(),
                USD.issue(),
                STPath({ipe(XRP)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, XRP},
                XRPS{bob});

//xrp跨货币桥接支付路径
            test(
                env,
                EUR,
                USD.issue(),
                STPath({cpe(xrpCurrency())}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, XRP},
                B{XRP, EUR},
                D{gw, bob, eurC});

//xrp->xrp事务不能包含路径
            test(env, XRP, boost::none, STPath({ape(carol)}), temBAD_PATH);

            {
//根帐户不能是SRC或DST
                auto flowJournal = env.app().logs().journal("Flow");
                {
//根帐户不能是DST
                    auto r = toStrand(
                        *env.current(),
                        alice,
                        xrpAccount(),
                        XRP,
                        boost::none,
                        USD.issue(),
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == temBAD_PATH);
                }
                {
//根帐户不能是SRC
                    auto r = toStrand(
                        *env.current(),
                        xrpAccount(),
                        alice,
                        XRP,
                        boost::none,
                        boost::none,
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == temBAD_PATH);
                }
                {
//根帐户不能是SRC
                    auto r = toStrand(
                        *env.current(),
                        noAccount(),
                        bob,
                        USD,
                        boost::none,
                        boost::none,
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == terNO_ACCOUNT);
                }
            }

//创建具有相同输入/输出问题的报价
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(USD), ipe(EUR)}),
                temBAD_PATH);

//类型为零的路径元素
            test(
                env,
                USD,
                boost::none,
                STPath({STPathElement(
                    0, xrpAccount(), xrpCurrency(), xrpAccount())}),
                temBAD_PATH);

//同一帐户不能在路径上出现多次
//“gw”将在alice->carol中使用，并在carol之间隐含。
//鲍伯
            test(
                env,
                USD,
                boost::none,
                STPath({ape(gw), ape(carol)}),
                temBAD_PATH_LOOP);

//同一优惠不能在一条路径上出现多次
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(EUR), ipe(USD), ipe(EUR)}),
                temBAD_PATH_LOOP);
        }

        {
//不能有多个具有相同输出问题的报价

            using namespace jtx;
            Env env(*this, features);

            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);
            env.trust(EUR(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, bob, EUR(100)));

            env(offer(bob, XRP(100), USD(100)));
            env(offer(bob, USD(100), EUR(100)), txflags(tfPassive));
            env(offer(bob, EUR(100), USD(100)), txflags(tfPassive));

//付款路径：xrp->xrp/usd->usd/eur->eur/usd
            env(pay(alice, carol, USD(100)),
                path(~USD, ~EUR, ~USD),
                sendmax(XRP(200)),
                txflags(tfNoRippleDirect),
                ter(temBAD_PATH_LOOP));
        }

        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, noripple(gw));
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));
            test(env, USD, boost::none, STPath(), terNO_RIPPLE);
        }

        {
//检查全局冻结
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));

//帐户仍可以发放付款
            env(fset(alice, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
            env(fclear(alice, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);

//账户不能发行资金
            env(fset(gw, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
            env(fclear(gw, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);

//帐户无法接收资金
            env(fset(bob, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
            env(fclear(bob, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
        }
        {
//在GW和Alice之间冻结
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
            env(trust(gw, alice["USD"](0), tfSetFreeze));
            BEAST_EXPECT(getTrustFlag(env, gw, alice, usdC, TrustFlag::freeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
        }
        {
//检查否
//帐户可能需要授权从
//发行人
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env(fset(gw, asfRequireAuth));
            env.trust(USD(1000), alice, bob);
//授权Alice，但不授权Bob
            env(trust(gw, alice["USD"](1000), tfSetfAuth));
            BEAST_EXPECT(getTrustFlag(env, gw, alice, usdC, TrustFlag::auth));
            env(pay(gw, alice, USD(100)));
            env.require(balance(alice, USD(100)));
            test(env, USD, boost::none, STPath(), terNO_AUTH);

//检查纯发行赎回仍然有效
            auto r = toStrand(
                *env.current(),
                alice,
                gw,
                USD,
                boost::none,
                boost::none,
                STPath(),
                true,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == tesSUCCESS);
            BEAST_EXPECT(equal(r.second, D{alice, gw, usdC}));
        }
        {
//检查已设置正确sendmax的路径和节点
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env(pay(gw, alice, EUR(100)));
            auto const path = STPath({STPathElement(
                STPathElement::typeAll,
                EUR.account,
                EUR.currency,
                EUR.account)});
            test(env, USD, EUR.issue(), path, tesSUCCESS);
        }

        {
//报价的最后一步xrp
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));

//Alice->USD/XRP->Bob
            STPath path;
            path.emplace_back(boost::none, USD.currency, USD.account.id());
            path.emplace_back(boost::none, xrpCurrency(), boost::none);

            auto r = toStrand(
                *env.current(),
                alice,
                bob,
                XRP,
                boost::none,
                USD.issue(),
                path,
                false,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == tesSUCCESS);
            BEAST_EXPECT(equal(r.second, D{alice, gw, usdC}, B{USD.issue(), xrpIssue()}, XRPS{bob}));
        }
    }

    void
    testRIPD1373(FeatureBitset features)
    {
        using namespace jtx;
        testcase("RIPD1373");

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        if (features[fix1373])
        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, gw);

            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env.trust(bob["USD"](1000), alice, gw);
            env.trust(bob["EUR"](1000), alice, gw);

            env(offer(bob, XRP(100), bob["USD"](100)), txflags(tfPassive));
            env(offer(gw, XRP(100), USD(100)), txflags(tfPassive));

            env(offer(bob, bob["USD"](100), bob["EUR"](100)),
                txflags(tfPassive));
            env(offer(gw, USD(100), EUR(100)), txflags(tfPassive));

            Path const p = [&] {
                Path result;
                result.push_back(allpe(gw, bob["USD"]));
                result.push_back(cpe(EUR.currency));
                return result;
            }();

            PathSet paths(p);

            env(pay(alice, alice, EUR(1)),
                json(paths.json()),
                sendmax(XRP(10)),
                txflags(tfNoRippleDirect | tfPartialPayment),
                ter(temBAD_PATH));
        }

        {
            Env env(*this, features);

            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));

            env(offer(bob, XRP(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), XRP(100)), txflags(tfPassive));

//付款路径：xrp->xrp/usd->usd/xrp
            env(pay(alice, carol, XRP(100)),
                path(~USD, ~XRP),
                txflags(tfNoRippleDirect),
                ter(temBAD_SEND_XRP_PATHS));
        }

        {
            Env env(*this, features);

            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));

            env(offer(bob, XRP(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), XRP(100)), txflags(tfPassive));

//付款路径：xrp->xrp/usd->usd/xrp
            env(pay(alice, carol, XRP(100)),
                path(~USD, ~XRP),
                sendmax(XRP(200)),
                txflags(tfNoRippleDirect),
                ter(temBAD_SEND_XRP_MAX));
        }
    }

    void
    testLoop(FeatureBitset features)
    {
        testcase("test loop");
        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];
        auto const CNY = gw["CNY"];

        {
            Env env(*this, features);

            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, alice, USD(100)));

            env(offer(bob, XRP(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), XRP(100)), txflags(tfPassive));

            auto const expectedResult = [&] () -> TER {
                if (features[featureFlow] && !features[fix1373])
                    return tesSUCCESS;
                return temBAD_PATH_LOOP;
            }();
//付款路径：美元->美元/xrp->xrp/美元
            env(pay(alice, carol, USD(100)),
                sendmax(USD(100)),
                path(~XRP, ~USD),
                txflags(tfNoRippleDirect),
                ter(expectedResult));
        }
        {
            Env env(*this, features);

            env.fund(XRP(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);
            env.trust(EUR(10000), alice, bob, carol);
            env.trust(CNY(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, bob, EUR(100)));
            env(pay(gw, bob, CNY(100)));

            env(offer(bob, XRP(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), EUR(100)), txflags(tfPassive));
            env(offer(bob, EUR(100), CNY(100)), txflags(tfPassive));

//付款路径：xrp->xrp/usd->usd/eur->usd/cny
            env(pay(alice, carol, CNY(100)),
                sendmax(XRP(100)),
                path(~USD, ~EUR, ~USD, ~CNY),
                txflags(tfNoRippleDirect),
                ter(temBAD_PATH_LOOP));
        }
    }

    void
    testNoAccount(FeatureBitset features)
    {
        testcase("test no account");
        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];

        Env env(*this, features);
        env.fund(XRP(10000), alice, bob, gw);

        STAmount sendMax{USD.issue(), 100, 1};
        STAmount noAccountAmount{Issue{USD.currency, noAccount()}, 100, 1};
        STAmount deliver;
        AccountID const srcAcc = alice.id();
        AccountID dstAcc = bob.id();
        STPathSet pathSet;
        ::ripple::path::RippleCalc::Input inputs;
        inputs.defaultPathsAllowed = true;
        try
        {
            PaymentSandbox sb{env.current().get(), tapNONE};
            {
                auto const r = ::ripple::path::RippleCalc::rippleCalculate(
                    sb, sendMax, deliver, dstAcc, noAccount(), pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::ripple::path::RippleCalc::rippleCalculate(
                    sb, sendMax, deliver, noAccount(), srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::ripple::path::RippleCalc::rippleCalculate(
                    sb, noAccountAmount, deliver, dstAcc, srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::ripple::path::RippleCalc::rippleCalculate(
                    sb, sendMax, noAccountAmount, dstAcc, srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
        }
        catch (...)
        {
            this->fail();
        }
    }

    void
    run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        testToStrand(sa - fix1373 - featureFlowCross);
        testToStrand(sa           - featureFlowCross);
        testToStrand(sa);

        testRIPD1373(sa - featureFlow - fix1373 - featureFlowCross);
        testRIPD1373(sa                         - featureFlowCross);
        testRIPD1373(sa);

        testLoop(sa - featureFlow - fix1373 - featureFlowCross);
        testLoop(sa               - fix1373 - featureFlowCross);
        testLoop(sa                         - featureFlowCross);
        testLoop(sa);

        testNoAccount(sa);
    }
};

BEAST_DEFINE_TESTSUITE(PayStrand, app, ripple);

}  //测试
}  //涟漪
