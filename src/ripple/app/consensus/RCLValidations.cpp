
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

#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/chrono.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/core/JobQueue.h>
#include <ripple/core/TimeKeeper.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

RCLValidatedLedger::RCLValidatedLedger(MakeGenesis)
    : ledgerID_{0}, ledgerSeq_{0}, j_{beast::Journal::getNullSink()}
{
}

RCLValidatedLedger::RCLValidatedLedger(
    std::shared_ptr<Ledger const> const& ledger,
    beast::Journal j)
    : ledgerID_{ledger->info().hash}, ledgerSeq_{ledger->seq()}, j_{j}
{
    auto const hashIndex = ledger->read(keylet::skip());
    if (hashIndex)
    {
        assert(hashIndex->getFieldU32(sfLastLedgerSequence) == (seq() - 1));
        ancestors_ = hashIndex->getFieldV256(sfHashes).value();
    }
    else
        JLOG(j_.warn()) << "Ledger " << ledgerSeq_ << ":" << ledgerID_
                        << " missing recent ancestor hashes";
}

auto
RCLValidatedLedger::minSeq() const -> Seq
{
    return seq() - std::min(seq(), static_cast<Seq>(ancestors_.size()));
}

auto
RCLValidatedLedger::seq() const -> Seq
{
    return ledgerSeq_;
}
auto
RCLValidatedLedger::id() const -> ID
{
    return ledgerID_;
}

auto RCLValidatedLedger::operator[](Seq const& s) const -> ID
{
    if (s >= minSeq() && s <= seq())
    {
        if (s == seq())
            return ledgerID_;
        Seq const diff = seq() - s;
        return ancestors_[ancestors_.size() - diff];
    }

    JLOG(j_.warn()) << "Unable to determine hash of ancestor seq=" << s
                    << " from ledger hash=" << ledgerID_
                    << " seq=" << ledgerSeq_;
//小于所有其他ID的默认ID
    return ID{0};
}

//返回最早可能不匹配祖先的序列号
RCLValidatedLedger::Seq
mismatch(RCLValidatedLedger const& a, RCLValidatedLedger const& b)
{
    using Seq = RCLValidatedLedger::Seq;

//查找分类帐已知序列的重叠间隔
    Seq const lower = std::max(a.minSeq(), b.minSeq());
    Seq const upper = std::min(a.seq(), b.seq());

    Seq curr = upper;
    while (curr != Seq{0} && a[curr] != b[curr] && curr >= lower)
        --curr;

//如果可搜索的间隔完全不匹配，那么我们必须
//假设分类帐不匹配，开始创建后分类帐
    return (curr < lower) ? Seq{1} : (curr + Seq{1});
}

RCLValidationsAdaptor::RCLValidationsAdaptor(Application& app, beast::Journal j)
    : app_(app),  j_(j)
{
    staleValidations_.reserve(512);
}

NetClock::time_point
RCLValidationsAdaptor::now() const
{
    return app_.timeKeeper().closeTime();
}

boost::optional<RCLValidatedLedger>
RCLValidationsAdaptor::acquire(LedgerHash const & hash)
{
    auto ledger = app_.getLedgerMaster().getLedgerByHash(hash);
    if (!ledger)
    {
        JLOG(j_.debug())
            << "Need validated ledger for preferred ledger analysis " << hash;

        Application * pApp = &app_;

        app_.getJobQueue().addJob(
            jtADVANCE, "getConsensusLedger", [pApp, hash](Job&) {
                pApp ->getInboundLedgers().acquire(
                    hash, 0, InboundLedger::Reason::CONSENSUS);
            });
        return boost::none;
    }

    assert(!ledger->open() && ledger->isImmutable());
    assert(ledger->info().hash == hash);

    return RCLValidatedLedger(std::move(ledger), j_);
}

void
RCLValidationsAdaptor::onStale(RCLValidation&& v)
{
//存储新过时的验证；不要在其中做重要的工作
//函数，因为这是来自验证的回调，可能是
//做其他工作。

    ScopedLockType sl(staleLock_);
    staleValidations_.emplace_back(std::move(v));
    if (staleWriting_)
        return;

//addjob（）在关闭时可能返回false（未添加作业）。
    staleWriting_ = app_.getJobQueue().addJob(
        jtWRITE, "Validations::doStaleWrite", [this](Job&) {
            auto event =
                app_.getJobQueue().makeLoadEvent(jtDISK, "ValidationWrite");
            ScopedLockType sl(staleLock_);
            doStaleWrite(sl);
        });
}

void
RCLValidationsAdaptor::flush(hash_map<NodeID, RCLValidation>&& remaining)
{
    bool anyNew = false;
    {
        ScopedLockType sl(staleLock_);

        for (auto const& keyVal : remaining)
        {
            staleValidations_.emplace_back(std::move(keyVal.second));
            anyNew = true;
        }

//如果我们有新的验证要写入但没有写入
//已经取得进展，然后同步写入数据库。
        if (anyNew && !staleWriting_)
        {
            staleWriting_ = true;
            doStaleWrite(sl);
        }

//在预先安排异步DostaleWrite的情况下，
//在刷新所有验证之前，此循环将被阻塞。
//这样可以确保在从返回时写入所有验证
//这个函数。

        while (staleWriting_)
        {
            ScopedUnlockType sul(staleLock_);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

//注意：必须使用staleLock_*locked*调用dostaleWrite（）。通过
//ScopedlockType&提醒未来的维护人员。
void
RCLValidationsAdaptor::doStaleWrite(ScopedLockType&)
{
    static const std::string insVal(
        "INSERT INTO Validations "
        "(InitialSeq, LedgerSeq, LedgerHash,NodePubKey,SignTime,RawData) "
        "VALUES (:initialSeq, :ledgerSeq, "
        ":ledgerHash,:nodePubKey,:signTime,:rawData);");
    static const std::string findSeq(
        "SELECT LedgerSeq FROM Ledgers WHERE Ledgerhash=:ledgerHash;");

    assert(staleWriting_);

    while (!staleValidations_.empty())
    {
        std::vector<RCLValidation> currentStale;
        currentStale.reserve(512);
        staleValidations_.swap(currentStale);

        {
            ScopedUnlockType sul(staleLock_);
            {
                auto db = app_.getLedgerDB().checkoutDb();

                Serializer s(1024);
                soci::transaction tr(*db);
                for (RCLValidation const& wValidation : currentStale)
                {
//仅在更新架构之前保存完整验证
                    if(!wValidation.full())
                        continue;
                    s.erase();
                    STValidation::pointer const& val = wValidation.unwrap();
                    val->add(s);

                    auto const ledgerHash = to_string(val->getLedgerHash());

                    boost::optional<std::uint64_t> ledgerSeq;
                    *db << findSeq, soci::use(ledgerHash),
                        soci::into(ledgerSeq);

                    auto const initialSeq = ledgerSeq.value_or(
                        app_.getLedgerMaster().getCurrentLedgerIndex());
                    auto const nodePubKey = toBase58(
                        TokenType::NodePublic, val->getSignerPublic());
                    auto const signTime =
                        val->getSignTime().time_since_epoch().count();

                    soci::blob rawData(*db);
                    rawData.append(
                        reinterpret_cast<const char*>(s.peekData().data()),
                        s.peekData().size());
                    assert(rawData.get_len() == s.peekData().size());

                    *db << insVal, soci::use(initialSeq), soci::use(ledgerSeq),
                        soci::use(ledgerHash), soci::use(nodePubKey),
                        soci::use(signTime), soci::use(rawData);
                }

                tr.commit();
            }
        }
    }

    staleWriting_ = false;
}

bool
handleNewValidation(Application& app,
    STValidation::ref val,
    std::string const& source)
{
    PublicKey const& signingKey = val->getSignerPublic();
    uint256 const& hash = val->getLedgerHash();

//如果签名者当前受信任，请确保验证标记为受信任
    boost::optional<PublicKey> masterKey =
        app.validators().getTrustedKey(signingKey);
    if (!val->isTrusted() && masterKey)
        val->setTrusted();

//如果当前不受信任，请查看当前是否列出了签名者
    if (!masterKey)
        masterKey = app.validators().getListedKey(signingKey);

    bool shouldRelay = false;
    RCLValidations& validations = app.getValidations();
    beast::Journal j = validations.adaptor().journal();

    auto dmp = [&](beast::Journal::Stream s, std::string const& msg) {
        s << "Val for " << hash
          << (val->isTrusted() ? " trusted/" : " UNtrusted/")
          << (val->isFull() ? "full" : "partial") << " from "
          << (masterKey ? toBase58(TokenType::NodePublic, *masterKey)
                        : "unknown")
          << " signing key "
          << toBase58(TokenType::NodePublic, signingKey) << " " << msg
          << " src=" << source;
    };

    if(!val->isFieldPresent(sfLedgerSequence))
    {
        if(j.error())
            dmp(j.error(), "missing ledger sequence field");
        return false;
    }

//仅当验证程序可信或已列出时，主密钥才位于
    if (masterKey)
    {
        ValStatus const outcome = validations.add(calcNodeID(*masterKey), val);
        if(j.debug())
            dmp(j.debug(), to_string(outcome));

        if (outcome == ValStatus::badSeq && j.warn())
        {
            auto const seq = val->getFieldU32(sfLedgerSequence);
            dmp(j.warn(),
                "already validated sequence at or past " + to_string(seq));
        }

        if (val->isTrusted() && outcome == ValStatus::current)
        {
            app.getLedgerMaster().checkAccept(
                hash, val->getFieldU32(sfLedgerSequence));
            shouldRelay = true;
        }
    }
    else
    {
        JLOG(j.debug()) << "Val for " << hash << " from "
                    << toBase58(TokenType::NodePublic, signingKey)
                    << " not added UNlisted";
    }

//尽管我们可能
//以后再考虑。来自@ JoelKatz：
//我们的想法是有一定数量的验证槽
//我们信任的验证器的优先级。剩余的插槽可能是
//分配给我们信任的发布者列出的验证程序，但
//我们没有选择信任。短期计划只是向前看
//不可信的验证如果同龄人想要它们或者如果我们有
//能力/带宽。这些都没有实施。
    return shouldRelay;
}


}  //命名空间波纹
