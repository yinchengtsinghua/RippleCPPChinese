
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

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/TxFlags.h>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <mutex>

namespace ripple {

static
std::vector<std::pair<uint256, std::string>>
parseSection (Section const& section)
{
    static boost::regex const re1 (
"^"                       //起点线
"(?:\\s*)"                //空白（可选）
"([abcdefABCDEF0-9]{64})" //<十六进制修正ID>
"(?:\\s+)"                //空白区
"(\\S+)"                  //<描述>
        , boost::regex_constants::optimize
    );

    std::vector<std::pair<uint256, std::string>> names;

    for (auto const& line : section.lines ())
    {
        boost::smatch match;

        if (!boost::regex_match (line, match, re1))
            Throw<std::runtime_error> (
                "Invalid entry '" + line +
                "' in [" + section.name () + "]");

        uint256 id;

        if (!id.SetHexExact (match[1]))
            Throw<std::runtime_error> (
                "Invalid amendment ID '" + match[1] +
                "' in [" + section.name () + "]");

        names.push_back (std::make_pair (id, match[2]));
    }

    return names;
}

/*修正案的当前状态。
    指示是否支持、启用或否决修订。被否决的修正案
    意味着节点永远不会宣布其支持。
**/

struct AmendmentState
{
    /*如果修正被否决，服务器将不支持它。*/
    bool vetoed = false;

    /*指示修正已启用。
        这是单向开关：启用修正后
        它不能被禁用，但可以被取代
        随后的修正案。
    **/

    bool enabled = false;

    /*指示此服务器支持的代码修正。*/
    bool supported = false;

    /*此修订的名称，可能为空。*/
    std::string name;

    explicit AmendmentState () = default;
};

/*给定窗口中请求的所有修订的状态。*/
struct AmendmentSet
{
private:
//每项修正案获得多少赞成票
    hash_map<uint256, int> votes_;

public:
//可信验证数
    int mTrustedValidations = 0;

//所需票数
    int mThreshold = 0;

    AmendmentSet () = default;

    void tally (std::set<uint256> const& amendments)
    {
        ++mTrustedValidations;

        for (auto const& amendment : amendments)
            ++votes_[amendment];
    }

    int votes (uint256 const& amendment) const
    {
        auto const& it = votes_.find (amendment);

        if (it == votes_.end())
            return 0;

        return it->second;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*跟踪“修订”列表

   “修正”是一个可以影响交易处理规则的选项。
   修正案被提出，然后被网络采纳或拒绝。安
   修正是由修正的256位密钥唯一标识的。
**/

class AmendmentTableImpl final
    : public AmendmentTable
{
protected:
    std::mutex mutex_;

    hash_map<uint256, AmendmentState> amendmentMap_;
    std::uint32_t lastUpdateSeq_;

//修正案必须获得多数通过的时间
    std::chrono::seconds const majorityTime_;

//修正案必须得到的支持金额
//0=0%，256=100%
    int const majorityFraction_;

//最后一轮投票的结果-如果
//我们还没有参加。
    std::unique_ptr <AmendmentSet> lastVote_;

//如果启用了不支持的修正，则为true
    bool unsupportedEnabled_;

    beast::Journal j_;

//查找或创建状态
    AmendmentState* add (uint256 const& amendment);

//查找现有状态
    AmendmentState* get (uint256 const& amendment);

    void setJson (Json::Value& v, uint256 const& amendment, const AmendmentState&);

public:
    AmendmentTableImpl (
        std::chrono::seconds majorityTime,
        int majorityFraction,
        Section const& supported,
        Section const& enabled,
        Section const& vetoed,
        beast::Journal journal);

    uint256 find (std::string const& name) override;

    bool veto (uint256 const& amendment) override;
    bool unVeto (uint256 const& amendment) override;

    bool enable (uint256 const& amendment) override;
    bool disable (uint256 const& amendment) override;

    bool isEnabled (uint256 const& amendment) override;
    bool isSupported (uint256 const& amendment) override;

    bool hasUnsupportedEnabled () override;

    Json::Value getJson (int) override;
    Json::Value getJson (uint256 const&) override;

    bool needValidatedLedger (LedgerIndex seq) override;

    void doValidatedLedger (
        LedgerIndex seq,
        std::set<uint256> const& enabled) override;

    std::vector <uint256>
    doValidation (std::set<uint256> const& enabledAmendments) override;

    std::vector <uint256>
    getDesired () override;

    std::map <uint256, std::uint32_t>
    doVoting (
        NetClock::time_point closeTime,
        std::set<uint256> const& enabledAmendments,
        majorityAmendments_t const& majorityAmendments,
        std::vector<STValidation::pointer> const& validations) override;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

AmendmentTableImpl::AmendmentTableImpl (
        std::chrono::seconds majorityTime,
        int majorityFraction,
        Section const& supported,
        Section const& enabled,
        Section const& vetoed,
        beast::Journal journal)
    : lastUpdateSeq_ (0)
    , majorityTime_ (majorityTime)
    , majorityFraction_ (majorityFraction)
    , unsupportedEnabled_ (false)
    , j_ (journal)
{
    assert (majorityFraction_ != 0);

    std::lock_guard <std::mutex> sl (mutex_);

    for (auto const& a : parseSection(supported))
    {
        if (auto s = add (a.first))
        {
            JLOG (j_.debug()) <<
                "Amendment " << a.first << " is supported.";

            if (!a.second.empty ())
                s->name = a.second;

            s->supported = true;
        }
    }

    for (auto const& a : parseSection (enabled))
    {
        if (auto s = add (a.first))
        {
            JLOG (j_.debug()) <<
                "Amendment " << a.first << " is enabled.";

            if (!a.second.empty ())
                s->name = a.second;

            s->supported = true;
            s->enabled = true;
        }
    }

    for (auto const& a : parseSection (vetoed))
    {
//未知修正案已被有效否决
        if (auto s = get (a.first))
        {
            JLOG (j_.info()) <<
                "Amendment " << a.first << " is vetoed.";

            if (!a.second.empty ())
                s->name = a.second;

            s->vetoed = true;
        }
    }
}

AmendmentState*
AmendmentTableImpl::add (uint256 const& amendmentHash)
{
//保持互斥的呼叫
    return &amendmentMap_[amendmentHash];
}

AmendmentState*
AmendmentTableImpl::get (uint256 const& amendmentHash)
{
//保持互斥的呼叫
    auto ret = amendmentMap_.find (amendmentHash);

    if (ret == amendmentMap_.end())
        return nullptr;

    return &ret->second;
}

uint256
AmendmentTableImpl::find (std::string const& name)
{
    std::lock_guard <std::mutex> sl (mutex_);

    for (auto const& e : amendmentMap_)
    {
        if (name == e.second.name)
            return e.first;
    }

    return {};
}

bool
AmendmentTableImpl::veto (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = add (amendment);

    if (s->vetoed)
        return false;
    s->vetoed = true;
    return true;
}

bool
AmendmentTableImpl::unVeto (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = get (amendment);

    if (!s || !s->vetoed)
        return false;
    s->vetoed = false;
    return true;
}

bool
AmendmentTableImpl::enable (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = add (amendment);

    if (s->enabled)
        return false;

    s->enabled = true;

    if (! s->supported)
    {
        JLOG (j_.error()) <<
            "Unsupported amendment " << amendment << " activated.";
        unsupportedEnabled_ = true;
    }

    return true;
}

bool
AmendmentTableImpl::disable (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = get (amendment);

    if (!s || !s->enabled)
        return false;

    s->enabled = false;
    return true;
}

bool
AmendmentTableImpl::isEnabled (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = get (amendment);
    return s && s->enabled;
}

bool
AmendmentTableImpl::isSupported (uint256 const& amendment)
{
    std::lock_guard <std::mutex> sl (mutex_);
    auto s = get (amendment);
    return s && s->supported;
}

bool
AmendmentTableImpl::hasUnsupportedEnabled ()
{
    std::lock_guard <std::mutex> sl (mutex_);
    return unsupportedEnabled_;
}

std::vector <uint256>
AmendmentTableImpl::doValidation (
    std::set<uint256> const& enabled)
{
//获取我们支持但不支持的修订列表
//否决，但尚未启用
    std::vector <uint256> amendments;
    amendments.reserve (amendmentMap_.size());

    {
        std::lock_guard <std::mutex> sl (mutex_);
        for (auto const& e : amendmentMap_)
        {
            if (e.second.supported && ! e.second.vetoed &&
                (enabled.count (e.first) == 0))
            {
                amendments.push_back (e.first);
            }
        }
    }

    if (! amendments.empty())
        std::sort (amendments.begin (), amendments.end ());

    return amendments;
}

std::vector <uint256>
AmendmentTableImpl::getDesired ()
{
//获取我们支持的修订列表，不要否决
    return doValidation({});
}

std::map <uint256, std::uint32_t>
AmendmentTableImpl::doVoting (
    NetClock::time_point closeTime,
    std::set<uint256> const& enabledAmendments,
    majorityAmendments_t const& majorityAmendments,
    std::vector<STValidation::pointer> const& valSet)
{
    JLOG (j_.trace()) <<
        "voting at " << closeTime.time_since_epoch().count() <<
        ": " << enabledAmendments.size() <<
        ", " << majorityAmendments.size() <<
        ", " << valSet.size();

    auto vote = std::make_unique <AmendmentSet> ();

//在标记分类帐之前处理分类帐的验证
    for (auto const& val : valSet)
    {
        if (val->isTrusted ())
        {
            std::set<uint256> ballot;

            if (val->isFieldPresent (sfAmendments))
            {
                auto const choices =
                    val->getFieldV256 (sfAmendments);
                ballot.insert (choices.begin (), choices.end ());
            }

            vote->tally (ballot);
        }
    }

    vote->mThreshold = std::max(1,
        (vote->mTrustedValidations * majorityFraction_) / 256);

    JLOG (j_.debug()) <<
        "Received " << vote->mTrustedValidations <<
        " trusted validations, threshold is: " << vote->mThreshold;

//每项行动的修正图。行动是
//伪事务中标志的值
    std::map <uint256, std::uint32_t> actions;

    {
        std::lock_guard <std::mutex> sl (mutex_);

//处理我们所知的所有修订
        for (auto const& entry : amendmentMap_)
        {
            NetClock::time_point majorityTime = {};

            bool const hasValMajority =
                (vote->votes (entry.first) >= vote->mThreshold);

            {
                auto const it = majorityAmendments.find (entry.first);
                if (it != majorityAmendments.end ())
                    majorityTime = it->second;
            }

            if (enabledAmendments.count (entry.first) != 0)
            {
                JLOG (j_.debug()) <<
                    entry.first << ": amendment already enabled";
            }
            else  if (hasValMajority &&
                (majorityTime == NetClock::time_point{}) &&
                ! entry.second.vetoed)
            {
//莱杰说不多数，验证者说是。
                JLOG (j_.debug()) <<
                    entry.first << ": amendment got majority";
                actions[entry.first] = tfGotMajority;
            }
            else if (! hasValMajority &&
                (majorityTime != NetClock::time_point{}))
            {
//莱杰说多数，验证者说不。
                JLOG (j_.debug()) <<
                    entry.first << ": amendment lost majority";
                actions[entry.first] = tfLostMajority;
            }
            else if ((majorityTime != NetClock::time_point{}) &&
                ((majorityTime + majorityTime_) <= closeTime) &&
                ! entry.second.vetoed)
            {
//莱杰说大多数人持有
                JLOG (j_.debug()) <<
                    entry.first << ": amendment majority held";
                actions[entry.first] = 0;
            }
        }

//用于报告的库存
        lastVote_ = std::move(vote);
    }

    return actions;
}

bool
AmendmentTableImpl::needValidatedLedger (LedgerIndex ledgerSeq)
{
    std::lock_guard <std::mutex> sl (mutex_);

//是否有可以启用修订的分类帐？
//在这两个分类帐序列之间？

    return ((ledgerSeq - 1) / 256) != ((lastUpdateSeq_ - 1) / 256);
}

void
AmendmentTableImpl::doValidatedLedger (
    LedgerIndex ledgerSeq,
    std::set<uint256> const& enabled)
{
    for (auto& e : enabled)
        enable(e);
}

void
AmendmentTableImpl::setJson (Json::Value& v, const uint256& id, const AmendmentState& fs)
{
    if (!fs.name.empty())
        v[jss::name] = fs.name;

    v[jss::supported] = fs.supported;
    v[jss::vetoed] = fs.vetoed;
    v[jss::enabled] = fs.enabled;

    if (!fs.enabled && lastVote_)
    {
        auto const votesTotal = lastVote_->mTrustedValidations;
        auto const votesNeeded = lastVote_->mThreshold;
        auto const votesFor = lastVote_->votes (id);

        v[jss::count] = votesFor;
        v[jss::validations] = votesTotal;

        if (votesNeeded)
        {
            v[jss::vote] = votesFor * 256 / votesNeeded;
            v[jss::threshold] = votesNeeded;
        }
    }
}

Json::Value
AmendmentTableImpl::getJson (int)
{
    Json::Value ret(Json::objectValue);
    {
        std::lock_guard <std::mutex> sl(mutex_);
        for (auto const& e : amendmentMap_)
        {
            setJson (ret[to_string (e.first)] = Json::objectValue,
                e.first, e.second);
        }
    }
    return ret;
}

Json::Value
AmendmentTableImpl::getJson (uint256 const& amendmentID)
{
    Json::Value ret = Json::objectValue;
    Json::Value& jAmendment = (ret[to_string (amendmentID)] = Json::objectValue);

    {
        std::lock_guard <std::mutex> sl(mutex_);
        auto a = add (amendmentID);
        setJson (jAmendment, amendmentID, *a);
    }

    return ret;
}

std::unique_ptr<AmendmentTable> make_AmendmentTable (
    std::chrono::seconds majorityTime,
    int majorityFraction,
    Section const& supported,
    Section const& enabled,
    Section const& vetoed,
    beast::Journal journal)
{
    return std::make_unique<AmendmentTableImpl> (
        majorityTime,
        majorityFraction,
        supported,
        enabled,
        vetoed,
        journal);
}

}  //涟漪
