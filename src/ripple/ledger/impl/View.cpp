
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

#include <ripple/basics/chrono.h>
#include <ripple/ledger/BookDirs.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/View.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/Quality.h>
#include <boost/algorithm/string.hpp>
#include <cassert>

namespace ripple {

NetClock::time_point const& fix1141Time ()
{
    using namespace std::chrono_literals;
//2016年7月1日星期五10:00:00 PDT
    static NetClock::time_point const soTime{520707600s};
    return soTime;
}

bool fix1141 (NetClock::time_point const closeTime)
{
    return closeTime > fix1141Time();
}

NetClock::time_point const& fix1274Time ()
{
    using namespace std::chrono_literals;
//2016年9月30日星期五10:00:00 PDT
    static NetClock::time_point const soTime{528570000s};

    return soTime;
}

bool fix1274 (NetClock::time_point const closeTime)
{
    return closeTime > fix1274Time();
}

NetClock::time_point const& fix1298Time ()
{
    using namespace std::chrono_literals;
//2016年12月21日星期三，太平洋标准时间上午10:00:00
    static NetClock::time_point const soTime{535658400s};

    return soTime;
}

bool fix1298 (NetClock::time_point const closeTime)
{
    return closeTime > fix1298Time();
}

NetClock::time_point const& fix1443Time ()
{
    using namespace std::chrono_literals;
//2017年3月11日星期六太平洋标准时间下午5:00:00
    static NetClock::time_point const soTime{542595600s};

    return soTime;
}

bool fix1443 (NetClock::time_point const closeTime)
{
    return closeTime > fix1443Time();
}

NetClock::time_point const& fix1449Time ()
{
    using namespace std::chrono_literals;
//2017年3月30日星期四下午1:00:00 PDT
    static NetClock::time_point const soTime{544219200s};

    return soTime;
}

bool fix1449 (NetClock::time_point const closeTime)
{
    return closeTime > fix1449Time();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//观察员
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void addRaw (LedgerInfo const& info, Serializer& s)
{
    s.add32 (info.seq);
    s.add64 (info.drops.drops ());
    s.add256 (info.parentHash);
    s.add256 (info.txHash);
    s.add256 (info.accountHash);
    s.add32 (info.parentCloseTime.time_since_epoch().count());
    s.add32 (info.closeTime.time_since_epoch().count());
    s.add8 (info.closeTimeResolution.count());
    s.add8 (info.closeFlags);
}

bool
isGlobalFrozen (ReadView const& view,
    AccountID const& issuer)
{
//vfalc也许应该断言
    if (isXRP (issuer))
        return false;
    auto const sle =
        view.read(keylet::account(issuer));
    if (sle && sle->isFlag (lsfGlobalFreeze))
        return true;
    return false;
}

//指定帐户是否可以使用由
//指定的颁发者还是冻结标志禁止它？
bool
isFrozen (ReadView const& view, AccountID const& account,
    Currency const& currency, AccountID const& issuer)
{
    if (isXRP (currency))
        return false;
    auto sle =
        view.read(keylet::account(issuer));
    if (sle && sle->isFlag (lsfGlobalFreeze))
        return true;
    if (issuer != account)
    {
//检查发卡行是否冻结行
        sle = view.read(keylet::line(
            account, issuer, currency));
        if (sle && sle->isFlag(
            (issuer > account) ?
                lsfHighFreeze : lsfLowFreeze))
            return true;
    }
    return false;
}

STAmount
accountHolds (ReadView const& view,
    AccountID const& account, Currency const& currency,
        AccountID const& issuer, FreezeHandling zeroIfFrozen,
              beast::Journal j)
{
    STAmount amount;
    if (isXRP(currency))
    {
        return {xrpLiquid (view, account, 0, j)};
    }

//IOU：托管行模冻结返回余额
    auto const sle = view.read(keylet::line(
        account, issuer, currency));
    if (! sle)
    {
        amount.clear ({currency, issuer});
    }
    else if ((zeroIfFrozen == fhZERO_IF_FROZEN) &&
        isFrozen(view, account, currency, issuer))
    {
        amount.clear (Issue (currency, issuer));
    }
    else
    {
        amount = sle->getFieldAmount (sfBalance);
        if (account > issuer)
        {
//把余额记在账上。
            amount.negate();
        }
        amount.setIssuer (issuer);
    }
    JLOG (j.trace()) << "accountHolds:" <<
        " account=" << to_string (account) <<
        " amount=" << amount.getFullText ();

    return view.balanceHook(
        account, issuer, amount);
}

STAmount
accountFunds (ReadView const& view, AccountID const& id,
    STAmount const& saDefault, FreezeHandling freezeHandling,
        beast::Journal j)
{
    STAmount saFunds;

    if (!saDefault.native () &&
        saDefault.getIssuer () == id)
    {
        saFunds = saDefault;
        JLOG (j.trace()) << "accountFunds:" <<
            " account=" << to_string (id) <<
            " saDefault=" << saDefault.getFullText () <<
            " SELF-FUNDED";
    }
    else
    {
        saFunds = accountHolds(view, id,
            saDefault.getCurrency(), saDefault.getIssuer(),
                freezeHandling, j);
        JLOG (j.trace()) << "accountFunds:" <<
            " account=" << to_string (id) <<
            " saDefault=" << saDefault.getFullText () <<
            " saFunds=" << saFunds.getFullText ();
    }
    return saFunds;
}

//防止所有者计数在错误情况下包装。
//
//调整允许所有者计数以多个步骤向上或向下调整。
//如果身份证！=boost.none，然后执行错误报告。
//
//返回调整后的所有者计数。
static
std::uint32_t
confineOwnerCount (std::uint32_t current, std::int32_t adjustment,
    boost::optional<AccountID> const& id = boost::none,
    beast::Journal j = beast::Journal {beast::Journal::getNullSink()})
{
    std::uint32_t adjusted {current + adjustment};
    if (adjustment > 0)
    {
//溢出在无符号上定义良好
        if (adjusted < current)
        {
            if (id)
            {
                JLOG (j.fatal()) <<
                    "Account " << *id <<
                    " owner count exceeds max!";
            }
            adjusted = std::numeric_limits<std::uint32_t>::max ();
        }
    }
    else
    {
//下溢在无符号上定义良好
        if (adjusted > current)
        {
            if (id)
            {
                JLOG (j.fatal()) <<
                    "Account " << *id <<
                    " owner count set below 0!";
            }
            adjusted = 0;
            assert(!id);
        }
    }
    return adjusted;
}

XRPAmount
xrpLiquid (ReadView const& view, AccountID const& id,
    std::int32_t ownerCountAdj, beast::Journal j)
{
    auto const sle = view.read(keylet::account(id));
    if (sle == nullptr)
        return beast::zero;

//返回余额减去准备金
    if (fix1141 (view.info ().parentCloseTime))
    {
        std::uint32_t const ownerCount = confineOwnerCount (
            view.ownerCountHook (id, sle->getFieldU32 (sfOwnerCount)),
                ownerCountAdj);

        auto const reserve =
            view.fees().accountReserve(ownerCount);

        auto const fullBalance =
            sle->getFieldAmount(sfBalance);

        auto const balance = view.balanceHook(id, xrpAccount(), fullBalance);

        STAmount amount = balance - reserve;
        if (balance < reserve)
            amount.clear ();

        JLOG (j.trace()) << "accountHolds:" <<
            " account=" << to_string (id) <<
            " amount=" << amount.getFullText() <<
            " fullBalance=" << fullBalance.getFullText() <<
            " balance=" << balance.getFullText() <<
            " reserve=" << to_string (reserve) <<
            " ownerCount=" << to_string (ownerCount) <<
            " ownerCountAdj=" << to_string (ownerCountAdj);

        return amount.xrp();
    }
    else
    {
//预切换
//xrp：返回余额减去准备金
        std::uint32_t const ownerCount =
            confineOwnerCount (sle->getFieldU32 (sfOwnerCount), ownerCountAdj);
        auto const reserve =
            view.fees().accountReserve(sle->getFieldU32(sfOwnerCount));
        auto const balance = sle->getFieldAmount(sfBalance);

        STAmount amount = balance - reserve;
        if (balance < reserve)
            amount.clear ();

        JLOG (j.trace()) << "accountHolds:" <<
            " account=" << to_string (id) <<
            " amount=" << amount.getFullText() <<
            " balance=" << balance.getFullText() <<
            " reserve=" << to_string (reserve) <<
            " ownerCount=" << to_string (ownerCount) <<
            " ownerCountAdj=" << to_string (ownerCountAdj);

        return view.balanceHook(id, xrpAccount(), amount).xrp();
    }
}

void
forEachItem (ReadView const& view, AccountID const& id,
    std::function<void(std::shared_ptr<SLE const> const&)> f)
{
    auto const root = keylet::ownerDir(id);
    auto pos = root;
    for(;;)
    {
        auto sle = view.read(pos);
        if (! sle)
            return;
//vfalc注：我们没有检查字段是否存在？
        for (auto const& key : sle->getFieldV256(sfIndexes))
            f(view.read(keylet::child(key)));
        auto const next =
            sle->getFieldU64 (sfIndexNext);
        if (! next)
            return;
        pos = keylet::page(root, next);
    }
}

bool
forEachItemAfter (ReadView const& view, AccountID const& id,
    uint256 const& after, std::uint64_t const hint,
        unsigned int limit, std::function<
            bool (std::shared_ptr<SLE const> const&)> f)
{
    auto const rootIndex = keylet::ownerDir(id);
    auto currentIndex = rootIndex;

//如果startafter不是零，请尝试使用提示跳到该页
    if (after.isNonZero ())
    {
        auto const hintIndex = keylet::page(rootIndex, hint);
        auto hintDir = view.read(hintIndex);
        if (hintDir)
        {
            for (auto const& key : hintDir->getFieldV256 (sfIndexes))
            {
                if (key == after)
                {
//我们找到了提示，我们可以从这里开始
                    currentIndex = hintIndex;
                    break;
                }
            }
        }

        bool found = false;
        for (;;)
        {
            auto const ownerDir = view.read(currentIndex);
            if (! ownerDir)
                return found;
            for (auto const& key : ownerDir->getFieldV256 (sfIndexes))
            {
                if (! found)
                {
                    if (key == after)
                        found = true;
                }
                else if (f (view.read(keylet::child(key))) && limit-- <= 1)
                {
                    return found;
                }
            }

            auto const uNodeNext =
                ownerDir->getFieldU64(sfIndexNext);
            if (uNodeNext == 0)
                return found;
            currentIndex = keylet::page(rootIndex, uNodeNext);
        }
    }
    else
    {
        for (;;)
        {
            auto const ownerDir = view.read(currentIndex);
            if (! ownerDir)
                return true;
            for (auto const& key : ownerDir->getFieldV256 (sfIndexes))
                if (f (view.read(keylet::child(key))) && limit-- <= 1)
                    return true;
            auto const uNodeNext =
                ownerDir->getFieldU64 (sfIndexNext);
            if (uNodeNext == 0)
                return true;
            currentIndex = keylet::page(rootIndex, uNodeNext);
        }
    }
}

Rate
transferRate (ReadView const& view,
    AccountID const& issuer)
{
    auto const sle = view.read(keylet::account(issuer));

    if (sle && sle->isFieldPresent (sfTransferRate))
        return Rate{ sle->getFieldU32 (sfTransferRate) };

    return parityRate;
}

bool
areCompatible (ReadView const& validLedger, ReadView const& testLedger,
    beast::Journal::Stream& s, const char* reason)
{
    bool ret = true;

    if (validLedger.info().seq < testLedger.info().seq)
    {
//有效->>试验
        auto hash = hashOfSeq (testLedger, validLedger.info().seq,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != validLedger.info().hash))
        {
            JLOG(s) << reason << " incompatible with valid ledger";

            JLOG(s) << "Hash(VSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if (validLedger.info().seq > testLedger.info().seq)
    {
//测试>…>有效
        auto hash = hashOfSeq (validLedger, testLedger.info().seq,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != testLedger.info().hash))
        {
            JLOG(s) << reason << " incompatible preceding ledger";

            JLOG(s) << "Hash(NSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if ((validLedger.info().seq == testLedger.info().seq) &&
         (validLedger.info().hash != testLedger.info().hash))
    {
//相同的序列号，不同的哈希
        JLOG(s) << reason << " incompatible ledger";

        ret = false;
    }

    if (! ret)
    {
        JLOG(s) << "Val: " << validLedger.info().seq <<
            " " << to_string (validLedger.info().hash);

        JLOG(s) << "New: " << testLedger.info().seq <<
            " " << to_string (testLedger.info().hash);
    }

    return ret;
}

bool areCompatible (uint256 const& validHash, LedgerIndex validIndex,
    ReadView const& testLedger, beast::Journal::Stream& s, const char* reason)
{
    bool ret = true;

    if (testLedger.info().seq > validIndex)
    {
//我们正在测试的分类帐遵循最后一个有效分类帐
        auto hash = hashOfSeq (testLedger, validIndex,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != validHash))
        {
            JLOG(s) << reason << " incompatible following ledger";
            JLOG(s) << "Hash(VSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if ((validIndex == testLedger.info().seq) &&
        (testLedger.info().hash != validHash))
    {
        JLOG(s) << reason << " incompatible ledger";

        ret = false;
    }

    if (! ret)
    {
        JLOG(s) << "Val: " << validIndex <<
            " " << to_string (validHash);

        JLOG(s) << "New: " << testLedger.info().seq <<
            " " << to_string (testLedger.info().hash);
    }

    return ret;
}

bool
dirIsEmpty (ReadView const& view,
    Keylet const& k)
{
    auto const sleNode = view.read(k);
    if (! sleNode)
        return true;
    if (! sleNode->getFieldV256 (sfIndexes).empty ())
        return false;
//如果有其他页面，它必须是非空的
    return sleNode->getFieldU64 (sfIndexNext) == 0;
}

bool
cdirFirst (ReadView const& view,
uint256 const& uRootIndex,  //>目录的根目录。
std::shared_ptr<SLE const>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j)
{
    sleNode = view.read(keylet::page(uRootIndex));
    uDirEntry   = 0;
assert (sleNode);           //从不探测目录。
    return cdirNext (view, uRootIndex, sleNode, uDirEntry, uEntryIndex, j);
}

bool
cdirNext (ReadView const& view,
uint256 const& uRootIndex,  //-->目录根目录
std::shared_ptr<SLE const>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<>下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j)
{
    auto const& svIndexes = sleNode->getFieldV256 (sfIndexes);
    assert (uDirEntry <= svIndexes.size ());
    if (uDirEntry >= svIndexes.size ())
    {
        auto const uNodeNext =
            sleNode->getFieldU64 (sfIndexNext);
        if (! uNodeNext)
        {
            uEntryIndex.zero ();
            return false;
        }
        auto const sleNext = view.read(
            keylet::page(uRootIndex, uNodeNext));
        uDirEntry = 0;
        if (!sleNext)
        {
//这不应该发生
            JLOG (j.fatal())
                    << "Corrupt directory: index:"
                    << uRootIndex << " next:" << uNodeNext;
            return false;
        }
        sleNode = sleNext;
        return cdirNext (view, uRootIndex,
            sleNode, uDirEntry, uEntryIndex, j);
    }
    uEntryIndex = svIndexes[uDirEntry++];
    JLOG (j.trace()) << "dirNext:" <<
        " uDirEntry=" << uDirEntry <<
        " uEntryIndex=" << uEntryIndex;
    return true;
}

std::set <uint256>
getEnabledAmendments (ReadView const& view)
{
    std::set<uint256> amendments;

    if (auto const sle = view.read(keylet::amendments()))
    {
        if (sle->isFieldPresent (sfAmendments))
        {
            auto const& v = sle->getFieldV256 (sfAmendments);
            amendments.insert (v.begin(), v.end());
        }
    }

    return amendments;
}

majorityAmendments_t
getMajorityAmendments (ReadView const& view)
{
    majorityAmendments_t ret;

    if (auto const sle = view.read(keylet::amendments()))
    {
        if (sle->isFieldPresent (sfMajorities))
        {
            using tp = NetClock::time_point;
            using d = tp::duration;

            auto const majorities = sle->getFieldArray (sfMajorities);

            for (auto const& m : majorities)
                ret[m.getFieldH256 (sfAmendment)] =
                    tp(d(m.getFieldU32(sfCloseTime)));
        }
    }

    return ret;
}

boost::optional<uint256>
hashOfSeq (ReadView const& ledger, LedgerIndex seq,
    beast::Journal journal)
{
//容易的情况…
    if (seq > ledger.seq())
    {
        JLOG (journal.warn()) <<
            "Can't get seq " << seq <<
            " from " << ledger.seq() << " future";
        return boost::none;
    }
    if (seq == ledger.seq())
        return ledger.info().hash;
    if (seq == (ledger.seq() - 1))
        return ledger.info().parentHash;

//在256以内…
    {
        int diff = ledger.seq() - seq;
        if (diff <= 256)
        {
            auto const hashIndex =
                ledger.read(keylet::skip());
            if (hashIndex)
            {
                assert (hashIndex->getFieldU32 (sfLastLedgerSequence) ==
                        (ledger.seq() - 1));
                STVector256 vec = hashIndex->getFieldV256 (sfHashes);
                if (vec.size () >= diff)
                    return vec[vec.size () - diff];
                JLOG (journal.warn()) <<
                    "Ledger " << ledger.seq() <<
                    " missing hash for " << seq <<
                    " (" << vec.size () << "," << diff << ")";
            }
            else
            {
                JLOG (journal.warn()) <<
                    "Ledger " << ledger.seq() <<
                    ":" << ledger.info().hash << " missing normal list";
            }
        }
        if ((seq & 0xff) != 0)
        {
            JLOG (journal.debug()) <<
                "Can't get seq " << seq <<
                " from " << ledger.seq() << " past";
            return boost::none;
        }
    }

//滑雪运动员
    auto const hashIndex =
        ledger.read(keylet::skip(seq));
    if (hashIndex)
    {
        auto const lastSeq =
            hashIndex->getFieldU32 (sfLastLedgerSequence);
        assert (lastSeq >= seq);
        assert ((lastSeq & 0xff) == 0);
        auto const diff = (lastSeq - seq) >> 8;
        STVector256 vec = hashIndex->getFieldV256 (sfHashes);
        if (vec.size () > diff)
            return vec[vec.size () - diff - 1];
    }
    JLOG (journal.warn()) <<
        "Can't get seq " << seq <<
        " from " << ledger.seq() << " error";
    return boost::none;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//修饰语
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
adjustOwnerCount (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        std::int32_t amount, beast::Journal j)
{
    assert(amount != 0);
    std::uint32_t const current {sle->getFieldU32 (sfOwnerCount)};
    AccountID const id = (*sle)[sfAccount];
    std::uint32_t const adjusted = confineOwnerCount (current, amount, id, j);
    view.adjustOwnerCountHook (id, current, adjusted);
    sle->setFieldU32 (sfOwnerCount, adjusted);
    view.update(sle);
}

bool
dirFirst (ApplyView& view,
uint256 const& uRootIndex,  //>目录的根目录。
std::shared_ptr<SLE>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j)
{
    sleNode = view.peek(keylet::page(uRootIndex));
    uDirEntry   = 0;
assert (sleNode);           //从不探测目录。
    return dirNext (view, uRootIndex, sleNode, uDirEntry, uEntryIndex, j);
}

bool
dirNext (ApplyView& view,
uint256 const& uRootIndex,  //-->目录根目录
std::shared_ptr<SLE>& sleNode,      //<->当前节点
unsigned int& uDirEntry,    //<>下一个条目
uint256& uEntryIndex,       //<--条目（如果可用）。否则，为零。
    beast::Journal j)
{
    auto const& svIndexes = sleNode->getFieldV256 (sfIndexes);
    assert (uDirEntry <= svIndexes.size ());
    if (uDirEntry >= svIndexes.size ())
    {
        auto const uNodeNext =
            sleNode->getFieldU64 (sfIndexNext);
        if (! uNodeNext)
        {
            uEntryIndex.zero ();
            return false;
        }
        auto const sleNext = view.peek(
            keylet::page(uRootIndex, uNodeNext));
        uDirEntry = 0;
        if (!sleNext)
        {
//这不应该发生
            JLOG (j.fatal())
                    << "Corrupt directory: index:"
                    << uRootIndex << " next:" << uNodeNext;
            return false;
        }
        sleNode = sleNext;
        return dirNext (view, uRootIndex,
            sleNode, uDirEntry, uEntryIndex, j);
    }
    uEntryIndex = svIndexes[uDirEntry++];
    JLOG (j.trace()) << "dirNext:" <<
        " uDirEntry=" << uDirEntry <<
        " uEntryIndex=" << uEntryIndex;
    return true;
}

std::function<void (SLE::ref)>
describeOwnerDir(AccountID const& account)
{
    return [&account](std::shared_ptr<SLE> const& sle)
    {
        (*sle)[sfOwner] = account;
    };
}

boost::optional<std::uint64_t>
dirAdd (ApplyView& view,
    Keylet const&                           dir,
    uint256 const&                          uLedgerIndex,
    bool                                    strictOrder,
    std::function<void (SLE::ref)>          fDescriber,
    beast::Journal j)
{
    if (view.rules().enabled(featureSortedDirectories))
    {
        if (strictOrder)
            return view.dirAppend(dir, uLedgerIndex, fDescriber);
        else
            return view.dirInsert(dir, uLedgerIndex, fDescriber);
    }

    JLOG (j.trace()) << "dirAdd:" <<
        " dir=" << to_string (dir.key) <<
        " uLedgerIndex=" << to_string (uLedgerIndex);

    auto sleRoot = view.peek(dir);
    std::uint64_t uNodeDir = 0;

    if (! sleRoot)
    {
//没有根，做吧。
        sleRoot = std::make_shared<SLE>(dir);
        sleRoot->setFieldH256 (sfRootIndex, dir.key);
        view.insert (sleRoot);
        fDescriber (sleRoot);

        STVector256 v;
        v.push_back (uLedgerIndex);
        sleRoot->setFieldV256 (sfIndexes, v);

        JLOG (j.trace()) <<
            "dirAdd: created root " << to_string (dir.key) <<
            " for entry " << to_string (uLedgerIndex);

        return uNodeDir;
    }

    SLE::pointer sleNode;
    STVector256 svIndexes;

uNodeDir = sleRoot->getFieldU64 (sfIndexPrevious);       //获取最后一个目录节点的索引。

    if (uNodeDir)
    {
//尝试添加到最后一个节点。
        sleNode = view.peek (keylet::page(dir, uNodeDir));
        assert (sleNode);
    }
    else
    {
//尝试添加到根目录。上一个节点没有上一个设置。
        sleNode     = sleRoot;
    }

    svIndexes = sleNode->getFieldV256 (sfIndexes);

    if (dirNodeMaxEntries != svIndexes.size ())
    {
//添加到当前节点。
        view.update(sleNode);
    }
//添加到新节点。
    else if (!++uNodeDir)
    {
        return boost::none;
    }
    else
    {
//将旧的最后一点指向新节点
        sleNode->setFieldU64 (sfIndexNext, uNodeDir);
        view.update(sleNode);

//使根点指向新节点。
        sleRoot->setFieldU64 (sfIndexPrevious, uNodeDir);
        view.update (sleRoot);

//创建新节点。
        sleNode = std::make_shared<SLE>(
            keylet::page(dir, uNodeDir));
        sleNode->setFieldH256 (sfRootIndex, dir.key);
        view.insert (sleNode);

        if (uNodeDir != 1)
            sleNode->setFieldU64 (sfIndexPrevious, uNodeDir - 1);

        fDescriber (sleNode);

        svIndexes   = STVector256 ();
    }

svIndexes.push_back (uLedgerIndex); //追加条目。
sleNode->setFieldV256 (sfIndexes, svIndexes);   //保存条目。

    JLOG (j.trace()) <<
        "dirAdd:   creating: root: " << to_string (dir.key);
    JLOG (j.trace()) <<
        "dirAdd:  appending: Entry: " << to_string (uLedgerIndex);
    JLOG (j.trace()) <<
        "dirAdd:  appending: Node: " << strHex (uNodeDir);

    return uNodeDir;
}

TER
trustCreate (ApplyView& view,
    const bool      bSrcHigh,
    AccountID const&  uSrcAccountID,
    AccountID const&  uDstAccountID,
uint256 const&  uIndex,             //-->Ripple状态条目
SLE::ref        sleAccount,         //-->正在设置的帐户。
const bool      bAuth,              //-->授权帐户。
const bool      bNoRipple,          //-->其他人无法通过
const bool      bFreeze,            //>资金不能离开
STAmount const& saBalance,          //-->正在设置的帐户余额。
//颁发者应为noaccount（）。
STAmount const& saLimit,            //>正在设置的帐户限制。
//颁发者应该是正在设置的帐户。
    std::uint32_t uQualityIn,
    std::uint32_t uQualityOut,
    beast::Journal j)
{
    JLOG (j.trace())
        << "trustCreate: " << to_string (uSrcAccountID) << ", "
        << to_string (uDstAccountID) << ", " << saBalance.getFullText ();

    auto const& uLowAccountID   = !bSrcHigh ? uSrcAccountID : uDstAccountID;
    auto const& uHighAccountID  =  bSrcHigh ? uSrcAccountID : uDstAccountID;

    auto const sleRippleState = std::make_shared<SLE>(
        ltRIPPLE_STATE, uIndex);
    view.insert (sleRippleState);

    auto lowNode = dirAdd (view, keylet::ownerDir (uLowAccountID),
        sleRippleState->key(), false, describeOwnerDir (uLowAccountID), j);

    if (!lowNode)
        return tecDIR_FULL;

    auto highNode = dirAdd (view, keylet::ownerDir (uHighAccountID),
        sleRippleState->key(), false, describeOwnerDir (uHighAccountID), j);

    if (!highNode)
        return tecDIR_FULL;

    const bool bSetDst = saLimit.getIssuer () == uDstAccountID;
    const bool bSetHigh = bSrcHigh ^ bSetDst;

    assert (sleAccount->getAccountID (sfAccount) ==
        (bSetHigh ? uHighAccountID : uLowAccountID));
    auto slePeer = view.peek (keylet::account(
        bSetHigh ? uLowAccountID : uHighAccountID));
    assert (slePeer);

//记住删除提示。
    sleRippleState->setFieldU64 (sfLowNode, *lowNode);
    sleRippleState->setFieldU64 (sfHighNode, *highNode);

    sleRippleState->setFieldAmount (
        bSetHigh ? sfHighLimit : sfLowLimit, saLimit);
    sleRippleState->setFieldAmount (
        bSetHigh ? sfLowLimit : sfHighLimit,
        STAmount ({saBalance.getCurrency (),
                   bSetDst ? uSrcAccountID : uDstAccountID}));

    if (uQualityIn)
        sleRippleState->setFieldU32 (
            bSetHigh ? sfHighQualityIn : sfLowQualityIn, uQualityIn);

    if (uQualityOut)
        sleRippleState->setFieldU32 (
            bSetHigh ? sfHighQualityOut : sfLowQualityOut, uQualityOut);

    std::uint32_t uFlags = bSetHigh ? lsfHighReserve : lsfLowReserve;

    if (bAuth)
    {
        uFlags |= (bSetHigh ? lsfHighAuth : lsfLowAuth);
    }
    if (bNoRipple)
    {
        uFlags |= (bSetHigh ? lsfHighNoRipple : lsfLowNoRipple);
    }
    if (bFreeze)
    {
        uFlags |= (!bSetHigh ? lsfLowFreeze : lsfHighFreeze);
    }

    if ((slePeer->getFlags() & lsfDefaultRipple) == 0)
    {
//另一方的默认值是没有涟漪
        uFlags |= (bSetHigh ? lsfLowNoRipple : lsfHighNoRipple);
    }

    sleRippleState->setFieldU32 (sfFlags, uFlags);
    adjustOwnerCount(view, sleAccount, 1, j);

//只有：创造涟漪平衡。
    sleRippleState->setFieldAmount (sfBalance, bSetHigh ? -saBalance : saBalance);

    view.creditHook (uSrcAccountID,
        uDstAccountID, saBalance, saBalance.zeroed());

    return tesSUCCESS;
}

TER
trustDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sleRippleState,
        AccountID const& uLowAccountID,
            AccountID const& uHighAccountID,
                 beast::Journal j)
{
//检测旧目录。
    std::uint64_t uLowNode    = sleRippleState->getFieldU64 (sfLowNode);
    std::uint64_t uHighNode   = sleRippleState->getFieldU64 (sfHighNode);

    JLOG (j.trace())
        << "trustDelete: Deleting ripple line: low";

    if (! view.dirRemove(
            keylet::ownerDir(uLowAccountID),
            uLowNode,
            sleRippleState->key(),
            false))
    {
        return tefBAD_LEDGER;
    }

    JLOG (j.trace())
            << "trustDelete: Deleting ripple line: high";

    if (! view.dirRemove(
            keylet::ownerDir(uHighAccountID),
            uHighNode,
            sleRippleState->key(),
            false))
    {
        return tefBAD_LEDGER;
    }

    JLOG (j.trace()) << "trustDelete: Deleting ripple line: state";
    view.erase(sleRippleState);

    return tesSUCCESS;
}

TER
offerDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
    beast::Journal j)
{
    if (! sle)
        return tesSUCCESS;
    auto offerIndex = sle->key();
    auto owner = sle->getAccountID  (sfAccount);

//检测旧目录。
    uint256 uDirectory = sle->getFieldH256 (sfBookDirectory);

    if (! view.dirRemove(
        keylet::ownerDir(owner),
        sle->getFieldU64(sfOwnerNode),
        offerIndex,
        false))
    {
        return tefBAD_LEDGER;
    }

    if (! view.dirRemove(
        keylet::page(uDirectory),
        sle->getFieldU64(sfBookNode),
        offerIndex,
        false))
    {
        return tefBAD_LEDGER;
    }

    adjustOwnerCount(view, view.peek(
        keylet::account(owner)), -1, j);

    view.erase(sle);

    return tesSUCCESS;
}

//直接发送，不收取费用：
//-偿还欠条和/或发送发送方自己的欠条。
//-根据需要创建信任线。
//-->BCheckIssuer：通常要求涉及到颁发者。
TER
rippleCredit (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, bool bCheckIssuer,
    beast::Journal j)
{
    auto issuer = saAmount.getIssuer ();
    auto currency = saAmount.getCurrency ();

//确保涉及发卡行。
    assert (
        !bCheckIssuer || uSenderID == issuer || uReceiverID == issuer);
    (void) issuer;

//不允许发送给自己。
    assert (uSenderID != uReceiverID);

    bool bSenderHigh = uSenderID > uReceiverID;
    uint256 uIndex = getRippleStateIndex (
        uSenderID, uReceiverID, saAmount.getCurrency ());
    auto sleRippleState  = view.peek (keylet::line(uIndex));

    TER terResult;

    assert (!isXRP (uSenderID) && uSenderID != noAccount());
    assert (!isXRP (uReceiverID) && uReceiverID != noAccount());

    if (!sleRippleState)
    {
        STAmount saReceiverLimit({currency, uReceiverID});
        STAmount saBalance = saAmount;

        saBalance.setIssuer (noAccount());

        JLOG (j.debug()) << "rippleCredit: "
            "create line: " << to_string (uSenderID) <<
            " -> " << to_string (uReceiverID) <<
            " : " << saAmount.getFullText ();

        auto const sleAccount =
            view.peek(keylet::account(uReceiverID));

        bool noRipple = (sleAccount->getFlags() & lsfDefaultRipple) == 0;

        terResult = trustCreate (view,
            bSenderHigh,
            uSenderID,
            uReceiverID,
            uIndex,
            sleAccount,
            false,
            noRipple,
            false,
            saBalance,
            saReceiverLimit,
            0,
            0,
            j);
    }
    else
    {
        STAmount saBalance   = sleRippleState->getFieldAmount (sfBalance);

        if (bSenderHigh)
saBalance.negate ();    //用寄件人的术语计算余额。

        view.creditHook (uSenderID,
            uReceiverID, saAmount, saBalance);

        STAmount    saBefore    = saBalance;

        saBalance   -= saAmount;

        JLOG (j.trace()) << "rippleCredit: " <<
            to_string (uSenderID) <<
            " -> " << to_string (uReceiverID) <<
            " : before=" << saBefore.getFullText () <<
            " amount=" << saAmount.getFullText () <<
            " after=" << saBalance.getFullText ();

        std::uint32_t const uFlags (sleRippleState->getFieldU32 (sfFlags));
        bool bDelete = false;

//如果反向波动，YYY可以跳过这一点。
        if (saBefore > beast::zero
//寄件人余额为正。
            && saBalance <= beast::zero
//发送方为零或负。
            && (uFlags & (!bSenderHigh ? lsfLowReserve : lsfHighReserve))
//已设置发件人保留。
            && static_cast <bool> (uFlags & (!bSenderHigh ? lsfLowNoRipple : lsfHighNoRipple)) !=
               static_cast <bool> (view.read (keylet::account(uSenderID))->getFlags() & lsfDefaultRipple)
            && !(uFlags & (!bSenderHigh ? lsfLowFreeze : lsfHighFreeze))
            && !sleRippleState->getFieldAmount (
                !bSenderHigh ? sfLowLimit : sfHighLimit)
//发件人信任限制为0。
            && !sleRippleState->getFieldU32 (
                !bSenderHigh ? sfLowQualityIn : sfHighQualityIn)
//中的发件人质量为0。
            && !sleRippleState->getFieldU32 (
                !bSenderHigh ? sfLowQualityOut : sfHighQualityOut))
//发件人质量输出为0。
        {
//清除发送方的保留，可能删除该行！
            adjustOwnerCount(view,
                view.peek(keylet::account(uSenderID)), -1, j);

//清除保留标志。
            sleRippleState->setFieldU32 (
                sfFlags,
                uFlags & (!bSenderHigh ? ~lsfLowReserve : ~lsfHighReserve));

//余额为零，接收者准备金为零。
bDelete = !saBalance        //余额为零。
                && !(uFlags & (bSenderHigh ? lsfLowReserve : lsfHighReserve));
//接收器储备已清除。
        }

        if (bSenderHigh)
            saBalance.negate ();

//希望将余额反映为零，即使我们正在删除行。
        sleRippleState->setFieldAmount (sfBalance, saBalance);
//仅限：调整纹波平衡。

        if (bDelete)
        {
            terResult   = trustDelete (view,
                sleRippleState,
                bSenderHigh ? uReceiverID : uSenderID,
                !bSenderHigh ? uReceiverID : uSenderID, j);
        }
        else
        {
            view.update (sleRippleState);
            terResult   = tesSUCCESS;
        }
    }

    return terResult;
}

//计算在双方之间转移借据资产所需的费用。
static
STAmount
rippleTransferFee (ReadView const& view,
    AccountID const& from,
    AccountID const& to,
    AccountID const& issuer,
    STAmount const& amount,
    beast::Journal j)
{
    if (from != issuer && to != issuer)
    {
        Rate const rate = transferRate (view, issuer);

        if (parityRate != rate)
        {
            auto const fee = multiply (amount, rate) - amount;

            JLOG (j.debug()) << "rippleTransferFee:" <<
                " amount=" << amount.getFullText () <<
                " fee=" << fee.getFullText ();

            return fee;
        }
    }

    return amount.zeroed();
}

//不受限制地发送。
//-->saamount：要交付给接收者的金额/货币/发行者。
//<--saactual：实际成本金额。寄件人支付费用。
static
TER
rippleSend (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, STAmount& saActual, beast::Journal j)
{
    auto const issuer   = saAmount.getIssuer ();

    assert (!isXRP (uSenderID) && !isXRP (uReceiverID));
    assert (uSenderID != uReceiverID);

    if (uSenderID == issuer || uReceiverID == issuer || issuer == noAccount())
    {
//直接发送：偿还欠条和/或发送自己的欠条。
        rippleCredit (view, uSenderID, uReceiverID, saAmount, false, j);
        saActual = saAmount;
        return tesSUCCESS;
    }

//发送第三方IOU：传输。

//计算要转移会计的金额
//任何转让费：
    if (!fix1141 (view.info ().parentCloseTime))
    {
        STAmount const saTransitFee = rippleTransferFee (
            view, uSenderID, uReceiverID, issuer, saAmount, j);

        saActual = !saTransitFee ? saAmount : saAmount + saTransitFee;
saActual.setIssuer (issuer);  //确保在+中完成。
    }
    else
    {
        saActual = multiply (saAmount,
            transferRate (view, issuer));
    }

    JLOG (j.debug()) << "rippleSend> " <<
        to_string (uSenderID) <<
        " - > " << to_string (uReceiverID) <<
        " : deliver=" << saAmount.getFullText () <<
        " cost=" << saActual.getFullText ();

    TER terResult   = rippleCredit (view, issuer, uReceiverID, saAmount, true, j);

    if (tesSUCCESS == terResult)
        terResult = rippleCredit (view, uSenderID, issuer, saActual, true, j);

    return terResult;
}

TER
accountSend (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, beast::Journal j)
{
    assert (saAmount >= beast::zero);

    /*如果我们不发送任何内容或发件人与
     *接收器，我们不需要做任何事情。
     **/

    if (!saAmount || (uSenderID == uReceiverID))
        return tesSUCCESS;

    if (!saAmount.native ())
    {
        STAmount saActual;

        JLOG (j.trace()) << "accountSend: " <<
            to_string (uSenderID) << " -> " << to_string (uReceiverID) <<
            " : " << saAmount.getFullText ();

        return rippleSend (view, uSenderID, uReceiverID, saAmount, saActual, j);
    }

    auto const fv2Switch = fix1141 (view.info ().parentCloseTime);
    if (!fv2Switch)
    {
        auto const dummyBalance = saAmount.zeroed();
        view.creditHook (uSenderID, uReceiverID, saAmount, dummyBalance);
    }

    /*xrp发送，不检查储备，可以做纯调整。
     *请注意，发送方或接收方可能为空，这不是错误；这是
     *在寻路过程中使用设置，并小心地将其控制为
     *确保转账平衡。
     **/


    TER terResult (tesSUCCESS);

    SLE::pointer sender = uSenderID != beast::zero
        ? view.peek (keylet::account(uSenderID))
        : SLE::pointer ();
    SLE::pointer receiver = uReceiverID != beast::zero
        ? view.peek (keylet::account(uReceiverID))
        : SLE::pointer ();

    if (auto stream = j.trace())
    {
        std::string sender_bal ("-");
        std::string receiver_bal ("-");

        if (sender)
            sender_bal = sender->getFieldAmount (sfBalance).getFullText ();

        if (receiver)
            receiver_bal = receiver->getFieldAmount (sfBalance).getFullText ();

       stream << "accountSend> " <<
            to_string (uSenderID) << " (" << sender_bal <<
            ") -> " << to_string (uReceiverID) << " (" << receiver_bal <<
            ") : " << saAmount.getFullText ();
    }

    if (sender)
    {
        if (sender->getFieldAmount (sfBalance) < saAmount)
        {
//vfalc必须改变
//基于所有参数
            terResult = view.open()
                ? TER {telFAILED_PROCESSING}
                : TER {tecFAILED_PROCESSING};
        }
        else
        {
            auto const sndBal = sender->getFieldAmount (sfBalance);
            if (fv2Switch)
                view.creditHook (uSenderID, xrpAccount (), saAmount, sndBal);

//减少xrp余额。
            sender->setFieldAmount (sfBalance, sndBal - saAmount);
            view.update (sender);
        }
    }

    if (tesSUCCESS == terResult && receiver)
    {
//增加xrp余额。
        auto const rcvBal = receiver->getFieldAmount (sfBalance);
        receiver->setFieldAmount (sfBalance, rcvBal + saAmount);

        if (fv2Switch)
            view.creditHook (xrpAccount (), uReceiverID, saAmount, -rcvBal);

        view.update (receiver);
    }

    if (auto stream = j.trace())
    {
        std::string sender_bal ("-");
        std::string receiver_bal ("-");

        if (sender)
            sender_bal = sender->getFieldAmount (sfBalance).getFullText ();

        if (receiver)
            receiver_bal = receiver->getFieldAmount (sfBalance).getFullText ();

        stream << "accountSend< " <<
            to_string (uSenderID) << " (" << sender_bal <<
            ") -> " << to_string (uReceiverID) << " (" << receiver_bal <<
            ") : " << saAmount.getFullText ();
    }

    return terResult;
}

static
bool
updateTrustLine (
    ApplyView& view,
    SLE::pointer state,
    bool bSenderHigh,
    AccountID const& sender,
    STAmount const& before,
    STAmount const& after,
    beast::Journal j)
{
    std::uint32_t const flags (state->getFieldU32 (sfFlags));

    auto sle = view.peek (keylet::account(sender));
    assert (sle);

//如果反向波动，YYY可以跳过这一点。
    if (before > beast::zero
//寄件人余额为正。
        && after <= beast::zero
//发送方为零或负。
        && (flags & (!bSenderHigh ? lsfLowReserve : lsfHighReserve))
//已设置发件人保留。
        && static_cast <bool> (flags & (!bSenderHigh ? lsfLowNoRipple : lsfHighNoRipple)) !=
           static_cast <bool> (sle->getFlags() & lsfDefaultRipple)
        && !(flags & (!bSenderHigh ? lsfLowFreeze : lsfHighFreeze))
        && !state->getFieldAmount (
            !bSenderHigh ? sfLowLimit : sfHighLimit)
//发件人信任限制为0。
        && !state->getFieldU32 (
            !bSenderHigh ? sfLowQualityIn : sfHighQualityIn)
//中的发件人质量为0。
        && !state->getFieldU32 (
            !bSenderHigh ? sfLowQualityOut : sfHighQualityOut))
//发件人质量输出为0。
    {
//vfalc删除的行在哪里？
//清除发送方的保留，可能删除该行！
        adjustOwnerCount(view, sle, -1, j);

//清除保留标志。
        state->setFieldU32 (sfFlags,
            flags & (!bSenderHigh ? ~lsfLowReserve : ~lsfHighReserve));

//余额为零，接收者准备金为零。
if (!after        //余额为零。
            && !(flags & (bSenderHigh ? lsfLowReserve : lsfHighReserve)))
            return true;
    }

    return false;
}

TER
issueIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount, Issue const& issue, beast::Journal j)
{
    assert (!isXRP (account) && !isXRP (issue.account));

//一致性检查
    assert (issue == amount.issue ());

//无法发送给自己！
    assert (issue.account != account);

    JLOG (j.trace()) << "issueIOU: " <<
        to_string (account) << ": " <<
        amount.getFullText ();

    bool bSenderHigh = issue.account > account;
    uint256 const index = getRippleStateIndex (
        issue.account, account, issue.currency);
    auto state = view.peek (keylet::line(index));

    if (!state)
    {
//NIKB TODO:限额使用接收者的账户作为发行人，并且
//这是不必要的效率低下，因为复制是可以避免的
//现在是必需的。考虑可用的选项。
        STAmount limit({issue.currency, account});
        STAmount final_balance = amount;

        final_balance.setIssuer (noAccount());

        auto receiverAccount = view.peek (keylet::account(account));

        bool noRipple = (receiverAccount->getFlags() & lsfDefaultRipple) == 0;

        return trustCreate (view, bSenderHigh, issue.account, account, index,
            receiverAccount, false, noRipple, false, final_balance, limit, 0, 0, j);
    }

    STAmount final_balance = state->getFieldAmount (sfBalance);

    if (bSenderHigh)
final_balance.negate ();    //用寄件人的术语计算余额。

    STAmount const start_balance = final_balance;

    final_balance -= amount;

    auto const must_delete = updateTrustLine(view, state, bSenderHigh, issue.account,
        start_balance, final_balance, j);

    view.creditHook (issue.account, account, amount, start_balance);

    if (bSenderHigh)
        final_balance.negate ();

//如有必要，调整信托额度上的余额。即使我们
//将删除该行以反映当时的正确余额
//删除的。
    state->setFieldAmount (sfBalance, final_balance);
    if (must_delete)
        return trustDelete (view, state,
            bSenderHigh ? account : issue.account,
            bSenderHigh ? issue.account : account, j);

    view.update (state);

    return tesSUCCESS;
}

TER
redeemIOU (ApplyView& view,
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue,
    beast::Journal j)
{
    assert (!isXRP (account) && !isXRP (issue.account));

//一致性检查
    assert (issue == amount.issue ());

//无法发送给自己！
    assert (issue.account != account);

    JLOG (j.trace()) << "redeemIOU: " <<
        to_string (account) << ": " <<
        amount.getFullText ();

    bool bSenderHigh = account > issue.account;
    uint256 const index = getRippleStateIndex (
        account, issue.account, issue.currency);
    auto state  = view.peek (keylet::line(index));

    if (!state)
    {
//为了持有IOU，必须存在一个信任行*来跟踪
//平衡。如果不是这样，那就大错特错了。不要尝试
//继续。
        JLOG (j.fatal()) << "redeemIOU: " <<
            to_string (account) << " attempts to redeem " <<
            amount.getFullText () << " but no trust line exists!";

        return tefINTERNAL;
    }

    STAmount final_balance = state->getFieldAmount (sfBalance);

    if (bSenderHigh)
final_balance.negate ();    //用寄件人的术语计算余额。

    STAmount const start_balance = final_balance;

    final_balance -= amount;

    auto const must_delete = updateTrustLine (view, state, bSenderHigh, account,
        start_balance, final_balance, j);

    view.creditHook (account, issue.account, amount, start_balance);

    if (bSenderHigh)
        final_balance.negate ();


//如有必要，调整信托额度上的余额。即使我们
//将删除该行以反映当时的正确余额
//删除的。
    state->setFieldAmount (sfBalance, final_balance);

    if (must_delete)
    {
        return trustDelete (view, state,
            bSenderHigh ? issue.account : account,
            bSenderHigh ? account : issue.account, j);
    }

    view.update (state);
    return tesSUCCESS;
}

TER
transferXRP (ApplyView& view,
    AccountID const& from,
    AccountID const& to,
    STAmount const& amount,
    beast::Journal j)
{
    assert (from != beast::zero);
    assert (to != beast::zero);
    assert (from != to);
    assert (amount.native ());

    SLE::pointer sender = view.peek (keylet::account(from));
    SLE::pointer receiver = view.peek (keylet::account(to));

    JLOG (j.trace()) << "transferXRP: " <<
        to_string (from) <<  " -> " << to_string (to) <<
        ") : " << amount.getFullText ();

    if (sender->getFieldAmount (sfBalance) < amount)
    {
//很不幸，我们必须
//到处变异
//Fixme：这个逻辑应该转移到调用者身上，也许？
        return view.open()
            ? TER {telFAILED_PROCESSING}
            : TER {tecFAILED_PROCESSING};
    }

//减少xrp余额。
    sender->setFieldAmount (sfBalance,
        sender->getFieldAmount (sfBalance) - amount);
    view.update (sender);

    receiver->setFieldAmount (sfBalance,
        receiver->getFieldAmount (sfBalance) + amount);
    view.update (receiver);

    return tesSUCCESS;
}

} //涟漪
