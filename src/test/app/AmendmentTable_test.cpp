
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


#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/unit_test.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/TxFlags.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple
{

class AmendmentTable_test final : public beast::unit_test::suite
{
private:
//204/256，大约80%（我们向下取整，因为实现向上取整）
    static int const majorityFraction{204};

    static
    uint256
    amendmentId (std::string in)
    {
        sha256_hasher h;
        using beast::hash_append;
        hash_append(h, in);
        auto const d = static_cast<sha256_hasher::result_type>(h);
        uint256 result;
        std::memcpy(result.data(), d.data(), d.size());
        return result;
    }

    static
    std::vector<std::string>
    createSet (int group, int count)
    {
        std::vector<std::string> amendments;
        for (int i = 0; i < count; i++)
            amendments.push_back (
                "Amendment" + std::to_string ((1000000 * group) + i));
        return amendments;
    }

    static
    Section
    makeSection (std::vector<std::string> const& amendments)
    {
        Section section ("Test");
        for (auto const& a : amendments)
            section.append (to_string(amendmentId (a)) + " " + a);
        return section;
    }

    static
    Section
    makeSection (uint256 const& amendment)
    {
        Section section ("Test");
        section.append (to_string (amendment) + " " + to_string(amendment));
        return section;
    }

    std::vector<std::string> const m_set1;
    std::vector<std::string> const m_set2;
    std::vector<std::string> const m_set3;
    std::vector<std::string> const m_set4;

    Section const emptySection;

    test::SuiteJournal journal;

public:
    AmendmentTable_test ()
        : m_set1 (createSet (1, 12))
        , m_set2 (createSet (2, 12))
        , m_set3 (createSet (3, 12))
        , m_set4 (createSet (4, 12))
        , journal ("AmendmentTable_test", *this)
    {
    }

    std::unique_ptr<AmendmentTable>
    makeTable(
        int w,
        Section const supported,
        Section const enabled,
        Section const vetoed)
    {
        return make_AmendmentTable (
            weeks (w),
            majorityFraction,
            supported,
            enabled,
            vetoed,
            journal);
    }

    std::unique_ptr<AmendmentTable>
    makeTable (int w)
    {
        return makeTable (
            w,
            makeSection (m_set1),
            makeSection (m_set2),
            makeSection (m_set3));
    }

    void testConstruct ()
    {
        testcase ("Construction");

        auto table = makeTable(1);

        for (auto const& a : m_set1)
        {
            BEAST_EXPECT(table->isSupported (amendmentId (a)));
            BEAST_EXPECT(!table->isEnabled (amendmentId (a)));
        }

        for (auto const& a : m_set2)
        {
            BEAST_EXPECT(table->isSupported (amendmentId (a)));
            BEAST_EXPECT(table->isEnabled (amendmentId (a)));
        }

        for (auto const& a : m_set3)
        {
            BEAST_EXPECT(!table->isSupported (amendmentId (a)));
            BEAST_EXPECT(!table->isEnabled (amendmentId (a)));
        }
    }

    void testGet ()
    {
        testcase ("Name to ID mapping");

        auto table = makeTable (1);

        for (auto const& a : m_set1)
            BEAST_EXPECT(table->find (a) == amendmentId (a));
        for (auto const& a : m_set2)
            BEAST_EXPECT(table->find (a) == amendmentId (a));

        for (auto const& a : m_set3)
            BEAST_EXPECT(!table->find (a));
        for (auto const& a : m_set4)
            BEAST_EXPECT(!table->find (a));
    }

    void testBadConfig ()
    {
        auto const section = makeSection (m_set1);
        auto const id = to_string (amendmentId (m_set2[0]));

        testcase ("Bad Config");

{ //需要两个参数-我们传递一个
            Section test = section;
            test.append (id);

            try
            {
                if (makeTable (2, test, emptySection, emptySection))
                    fail ("Accepted only amendment ID");
            }
            catch (...)
            {
                pass();
            }
        }

{ //需要两个参数-我们传递三个
            Section test = section;
            test.append (id + " Test Name");

            try
            {
                if (makeTable (2, test, emptySection, emptySection))
                    fail ("Accepted extra arguments");
            }
            catch (...)
            {
                pass();
            }
        }

        {
            auto sid = id;
            sid.resize (sid.length() - 1);

            Section test = section;
            test.append (sid + " Name");

            try
            {
                if (makeTable (2, test, emptySection, emptySection))
                    fail ("Accepted short amendment ID");
            }
            catch (...)
            {
                pass();
            }
        }

        {
            auto sid = id;
            sid.resize (sid.length() + 1, '0');

            Section test = section;
            test.append (sid + " Name");

            try
            {
                if (makeTable (2, test, emptySection, emptySection))
                    fail ("Accepted long amendment ID");
            }
            catch (...)
            {
                pass();
            }
        }

        {
            auto sid = id;
            sid.resize (sid.length() - 1);
            sid.push_back ('Q');

            Section test = section;
            test.append (sid + " Name");

            try
            {
                if (makeTable (2, test, emptySection, emptySection))
                    fail ("Accepted non-hex amendment ID");
            }
            catch (...)
            {
                pass();
            }
        }
    }

    std::map<uint256, bool>
    getState (
        AmendmentTable *table,
        std::set<uint256> const& exclude)
    {
        std::map<uint256, bool> state;

        auto track = [&state,table](std::vector<std::string> const& v)
        {
            for (auto const& a : v)
            {
                auto const id = amendmentId(a);
                state[id] = table->isEnabled (id);
            }
        };

        track (m_set1);
        track (m_set2);
        track (m_set3);
        track (m_set4);

        for (auto const& a : exclude)
            state.erase(a);

        return state;
    }

    void testEnableDisable ()
    {
        testcase ("enable & disable");

        auto const testAmendment = amendmentId("TestAmendment");
        auto table = makeTable (2);

//要启用的修订子集
        std::set<uint256> enabled;
        enabled.insert (testAmendment);
        enabled.insert (amendmentId(m_set1[0]));
        enabled.insert (amendmentId(m_set2[0]));
        enabled.insert (amendmentId(m_set3[0]));
        enabled.insert (amendmentId(m_set4[0]));

//获取之前的状态，不包括将要更改的项：
        auto const pre_state = getState (table.get(), enabled);

//启用子集并验证
        for (auto const& a : enabled)
            table->enable (a);

        for (auto const& a : enabled)
            BEAST_EXPECT(table->isEnabled (a));

//禁用子集并验证
        for (auto const& a : enabled)
            table->disable (a);

        for (auto const& a : enabled)
            BEAST_EXPECT(!table->isEnabled (a));

//之后获取状态，不包括我们更改的项：
        auto const post_state = getState (table.get(), enabled);

//确保状态相同
        auto ret = std::mismatch(
            pre_state.begin(), pre_state.end(),
            post_state.begin(), post_state.end());

        BEAST_EXPECT(ret.first == pre_state.end());
        BEAST_EXPECT(ret.second == post_state.end());
    }

    std::vector<std::pair<PublicKey,SecretKey>> makeValidators (int num)
    {
        std::vector <std::pair<PublicKey, SecretKey>> ret;
        ret.reserve (num);
        for (int i = 0; i < num; ++i)
        {
            ret.emplace_back(randomKeyPair(KeyType::secp256k1));
        }
        return ret;
    }

    static NetClock::time_point weekTime (weeks w)
    {
        return NetClock::time_point{w};
    }

//为一个标志分类帐执行一个假装的共识回合
    void doRound(
        AmendmentTable& table,
        weeks week,
        std::vector <std::pair<PublicKey, SecretKey>> const& validators,
        std::vector <std::pair <uint256, int>> const& votes,
        std::vector <uint256>& ourVotes,
        std::set <uint256>& enabled,
        majorityAmendments_t& majority)
    {
//在规定的时间做一轮
//返回我们投票赞成的修正案

//参数：
//表：我们的已知和否决修正案表
//验证器：我们信任的验证器的addreses
//投票：修正案和投票给他们的验证者的数量
//我们的投票：我们在验证中投票赞成的修正案
//启用：启用/禁用的修正
//多数：在/我们的多数修正案中（当他们获得多数时）

        auto const roundTime = weekTime (week);

//生成验证
        std::vector<STValidation::pointer> validations;
        validations.reserve (validators.size ());

        int i = 0;
        for (auto const& val : validators)
        {
            ++i;
            std::vector<uint256> field;

            for (auto const& amendment : votes)
            {
                if ((256 * i) < (validators.size() * amendment.second))
                {
//我们对修正案投赞成票
                    field.push_back (amendment.first);
                }
            }

            auto v = std::make_shared<STValidation>(
                uint256(),
                i,
                uint256(),
                roundTime,
                val.first,
                val.second,
                calcNodeID(val.first),
                true,
                STValidation::FeeSettings{},
                field);

            validations.emplace_back(v);
        }

        ourVotes = table.doValidation (enabled);

        auto actions = table.doVoting (
            roundTime, enabled, majority, validations);
        for (auto const& action : actions)
        {
//此代码假定其他验证器和我们一样

            auto const& hash = action.first;
            switch (action.second)
            {
                case 0:
//修正案由多数改为启用
                    if (enabled.find (hash) != enabled.end ())
                        Throw<std::runtime_error> ("enabling already enabled");
                    if (majority.find (hash) == majority.end ())
                        Throw<std::runtime_error> ("enabling without majority");
                    enabled.insert (hash);
                    majority.erase (hash);
                    break;

                case tfGotMajority:
                    if (majority.find (hash) != majority.end ())
                        Throw<std::runtime_error> ("got majority while having majority");
                    majority[hash] = roundTime;
                    break;

                case tfLostMajority:
                    if (majority.find (hash) == majority.end ())
                        Throw<std::runtime_error> ("lost majority without majority");
                    majority.erase (hash);
                    break;

                default:
                    Throw<std::runtime_error> ("unknown action");
            }
        }
    }

//对未知修正案不投票
    void testNoOnUnknown ()
    {
        testcase ("Vote NO on unknown");

        auto const testAmendment = amendmentId("TestAmendment");
        auto const validators = makeValidators (10);

        auto table = makeTable (2,
            emptySection,
            emptySection,
            emptySection);

        std::vector <std::pair <uint256, int>> votes;
        std::vector <uint256> ourVotes;
        std::set <uint256> enabled;
        majorityAmendments_t majority;

        doRound (*table, weeks{1},
            validators,
            votes,
            ourVotes,
            enabled,
            majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());
        BEAST_EXPECT(majority.empty());

        votes.emplace_back (testAmendment, 256);

        doRound (*table, weeks{2},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());

        majority[testAmendment] = weekTime(weeks{1});

//注意，模拟代码假定其他代码的行为与我们相同，
//所以修正案不会生效
        doRound (*table, weeks{5},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());
    }

//否决修正案无表决权
    void testNoOnVetoed ()
    {
        testcase ("Vote NO on vetoed");

        auto const testAmendment = amendmentId ("vetoedAmendment");

        auto table = makeTable (2,
            emptySection,
            emptySection,
            makeSection (testAmendment));

        auto const validators = makeValidators (10);

        std::vector <std::pair <uint256, int>> votes;
        std::vector <uint256> ourVotes;
        std::set <uint256> enabled;
        majorityAmendments_t majority;

        doRound (*table, weeks{1},
            validators,
            votes,
            ourVotes,
            enabled,
            majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());
        BEAST_EXPECT(majority.empty());

        votes.emplace_back (testAmendment, 256);

        doRound (*table, weeks{2},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());

        majority[testAmendment] = weekTime(weeks{1});

        doRound (*table, weeks{5},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(ourVotes.empty());
        BEAST_EXPECT(enabled.empty());
    }

//投票并启用已知的、未启用的修订
    void testVoteEnable ()
    {
        testcase ("voteEnable");

        auto table = makeTable (
            2,
            makeSection (m_set1),
            emptySection,
            emptySection);

        auto const validators = makeValidators (10);
        std::vector <std::pair <uint256, int>> votes;
        std::vector <uint256> ourVotes;
        std::set <uint256> enabled;
        majorityAmendments_t majority;

//第一周：我们应该对所有未启用的已知修正案进行投票。
        doRound (*table, weeks{1},
            validators,
            votes,
            ourVotes,
            enabled,
            majority);
        BEAST_EXPECT(ourVotes.size() == m_set1.size());
        BEAST_EXPECT(enabled.empty());
        for (auto const& i : m_set1)
            BEAST_EXPECT(majority.find(amendmentId (i)) == majority.end());

//现在，每个人都投票支持这个功能
        for (auto const& i : m_set1)
            votes.emplace_back (amendmentId(i), 256);

//第二周：我们应该承认多数
        doRound (*table, weeks{2},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(ourVotes.size() == m_set1.size());
        BEAST_EXPECT(enabled.empty());

        for (auto const& i : m_set1)
            BEAST_EXPECT(majority[amendmentId (i)] == weekTime(weeks{2}));

//第五周：我们应该启用修正案
        doRound (*table, weeks{5},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(enabled.size() == m_set1.size());

//第六周：我们应该把它从我们的选票中除掉，不再获得多数票。
        doRound (*table, weeks{6},
                validators,
                votes,
                ourVotes,
                enabled,
                majority);
        BEAST_EXPECT(enabled.size() == m_set1.size());
        BEAST_EXPECT(ourVotes.empty());
        for (auto const& i : m_set1)
            BEAST_EXPECT(majority.find(amendmentId (i)) == majority.end());
    }

//以80%检测多数，稍后启用
    void testDetectMajority ()
    {
        testcase ("detectMajority");

        auto const testAmendment = amendmentId ("detectMajority");
        auto table = makeTable (
            2,
            makeSection (testAmendment),
            emptySection,
            emptySection);

        auto const validators = makeValidators (16);

        std::set <uint256> enabled;
        majorityAmendments_t majority;

        for (int i = 0; i <= 17; ++i)
        {
            std::vector <std::pair <uint256, int>> votes;
            std::vector <uint256> ourVotes;

            if ((i > 0) && (i < 17))
                votes.emplace_back (testAmendment, i * 16);

            doRound (*table, weeks{i},
                validators, votes, ourVotes, enabled, majority);

            if (i < 13)
            {
//我们投票赞成，不赞成，不赞成
                BEAST_EXPECT(!ourVotes.empty());
                BEAST_EXPECT(enabled.empty());
                BEAST_EXPECT(majority.empty());
            }
            else if (i < 15)
            {
//我们有多数票，没有启用，继续投票
                BEAST_EXPECT(!ourVotes.empty());
                BEAST_EXPECT(!majority.empty());
                BEAST_EXPECT(enabled.empty());
            }
            else if (i == 15)
            {
//启用、继续投票、从多数中删除
                BEAST_EXPECT(!ourVotes.empty());
                BEAST_EXPECT(majority.empty());
                BEAST_EXPECT(!enabled.empty());
            }
            else
            {
//完成了，我们应该被启用而不是投票
                BEAST_EXPECT(ourVotes.empty());
                BEAST_EXPECT(majority.empty());
                BEAST_EXPECT(!enabled.empty());
            }
        }
    }

//检测多数丢失
    void testLostMajority ()
    {
        testcase ("lostMajority");

        auto const testAmendment = amendmentId ("lostMajority");
        auto const validators = makeValidators (16);

        auto table = makeTable (
            8,
            makeSection (testAmendment),
            emptySection,
            emptySection);

        std::set <uint256> enabled;
        majorityAmendments_t majority;

        {
//建立多数
            std::vector <std::pair <uint256, int>> votes;
            std::vector <uint256> ourVotes;

            votes.emplace_back (testAmendment, 250);

            doRound (*table, weeks{1},
                validators, votes, ourVotes, enabled, majority);

            BEAST_EXPECT(enabled.empty());
            BEAST_EXPECT(!majority.empty());
        }

        for (int i = 1; i < 16; ++i)
        {
            std::vector <std::pair <uint256, int>> votes;
            std::vector <uint256> ourVotes;

//逐渐减少支撑
            votes.emplace_back (testAmendment, 256 - i * 8);

            doRound (*table, weeks{i + 1},
                validators, votes, ourVotes, enabled, majority);

            if (i < 8)
            {
//我们投赞成票，不赞成票，多数票
                BEAST_EXPECT(!ourVotes.empty());
                BEAST_EXPECT(enabled.empty());
                BEAST_EXPECT(!majority.empty());
            }
            else
            {
//没有多数，没有启用，继续投票
                BEAST_EXPECT(!ourVotes.empty());
                BEAST_EXPECT(majority.empty());
                BEAST_EXPECT(enabled.empty());
            }
        }
    }

    void testHasUnsupported ()
    {
        testcase ("hasUnsupportedEnabled");

        auto table = makeTable(1);
        BEAST_EXPECT(! table->hasUnsupportedEnabled());

        std::set <uint256> enabled;
        std::for_each(m_set4.begin(), m_set4.end(),
            [&enabled](auto const &s){ enabled.insert(amendmentId(s)); });
        table->doValidatedLedger(1, enabled);
        BEAST_EXPECT(table->hasUnsupportedEnabled());
    }

    void run () override
    {
        testConstruct();
        testGet ();
        testBadConfig ();
        testEnableDisable ();
        testNoOnUnknown ();
        testNoOnVetoed ();
        testVoteEnable ();
        testDetectMajority ();
        testLostMajority ();
        testHasUnsupported ();
    }
};

BEAST_DEFINE_TESTSUITE (AmendmentTable, app, ripple);

}  //涟漪
