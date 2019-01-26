
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
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/LedgerTrie.h>
#include <random>
#include <test/csf/ledgers.h>
#include <unordered_map>

namespace ripple {
namespace test {

class LedgerTrie_test : public beast::unit_test::suite
{
    void
    testInsert()
    {
        using namespace csf;
//单独输入
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);

            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }
//现有后缀（扩展树）
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());
//无兄弟姐妹扩展
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);

//用现有同级扩展
            t.insert(h["abce"]);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abce"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abce"]) == 1);
        }
//未提交现有节点
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
//没有兄弟姐妹的不承诺
            t.insert(h["abcdf"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcdf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcdf"]) == 1);

//未与现有子项提交
            t.insert(h["abc"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcdf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcdf"]) == 1);
        }
//现有节点的后缀+未提交
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abce"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abce"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abce"]) == 1);
        }
//后缀+未提交现有子项
        {
//ABCD：ABCD E，ABCF

            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcde"]);
            BEAST_EXPECT(t.checkInvariants());
            t.insert(h["abcf"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcf"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcf"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abcde"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcde"]) == 1);
        }

//多重计数
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"], 4);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["a"]) == 4);

            t.insert(h["abc"], 2);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 4);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 6);
            BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["a"]) == 6);
        }
    }

    void
    testRemove()
    {
        using namespace csf;
//不在特里
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);

            BEAST_EXPECT(!t.remove(h["ab"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(!t.remove(h["a"]));
            BEAST_EXPECT(t.checkInvariants());
        }
//在Trie中，但有0个尖端支撑
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"]);
            t.insert(h["abce"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(!t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }
//在具有大于1个尖端支撑的Trie中
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"], 2);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);

            t.insert(h["abc"], 1);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.remove(h["abc"], 2));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);

            t.insert(h["abc"], 3);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 3);
            BEAST_EXPECT(t.remove(h["abc"], 300));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
        }
//在trie中，=1个尖端支持，没有孩子
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);

            BEAST_EXPECT(t.tipSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 0);
        }
//在trie中，=1个尖端支撑，1个孩子
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abcd"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abcd"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abcd"]) == 1);
        }
//在TIE中，=1个尖端支持，>1个儿童
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"]);

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 3);

            BEAST_EXPECT(t.remove(h["abc"]));
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["abc"]) == 2);
        }

//在具有=1个尖端支撑的Trie中，父级压缩
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["abc"]);
            t.insert(h["abd"]);
            BEAST_EXPECT(t.checkInvariants());
            t.remove(h["ab"]);
            BEAST_EXPECT(t.checkInvariants());
            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["abd"]) == 1);
            BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);

            t.remove(h["abd"]);
            BEAST_EXPECT(t.checkInvariants());

            BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
            BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        }
    }

    void
    testEmpty()
    {
        using namespace csf;
        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(t.empty());

        Ledger genesis = h[""];
        t.insert(genesis);
        BEAST_EXPECT(!t.empty());
        t.remove(genesis);
        BEAST_EXPECT(t.empty());

        t.insert(h["abc"]);
        BEAST_EXPECT(!t.empty());
        t.remove(h["abc"]);
        BEAST_EXPECT(t.empty());
    }

    void
    testSupport()
    {
        using namespace csf;
        using Seq = Ledger::Seq;

        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["axy"]) == 0);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 0);
        BEAST_EXPECT(t.branchSupport(h["axy"]) == 0);

        t.insert(h["abc"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.tipSupport(h["abcd"]) == 0);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abcd"]) == 0);

        t.insert(h["abe"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.tipSupport(h["abe"]) == 1);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 2);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 2);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abe"]) == 1);

        t.remove(h["abc"]);
        BEAST_EXPECT(t.tipSupport(h["a"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["ab"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abc"]) == 0);
        BEAST_EXPECT(t.tipSupport(h["abe"]) == 1);

        BEAST_EXPECT(t.branchSupport(h["a"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["ab"]) == 1);
        BEAST_EXPECT(t.branchSupport(h["abc"]) == 0);
        BEAST_EXPECT(t.branchSupport(h["abe"]) == 1);
    }

    void
    testGetPreferred()
    {
        using namespace csf;
        using Seq = Ledger::Seq;
//空的
        {
            LedgerTrie<Ledger> t;
            BEAST_EXPECT(t.getPreferred(Seq{0}) == boost::none);
            BEAST_EXPECT(t.getPreferred(Seq{2}) == boost::none);
        }
//创世支持不是空的
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            Ledger genesis = h[""];
            t.insert(genesis);
            BEAST_EXPECT(t.getPreferred(Seq{0})->id == genesis.id());
            BEAST_EXPECT(t.remove(genesis));
            BEAST_EXPECT(t.getPreferred(Seq{0}) == boost::none);
            BEAST_EXPECT(!t.remove(genesis));
        }
//单节点无子节点
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
        }
//单节点小子支持
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }
//单个节点较大的子节点
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }
//单节点小子支持
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());

            t.insert(h["abc"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }
//单个节点较大的子节点
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            t.insert(h["abce"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());

            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }
//按ID划分的断线器
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abcd"], 2);
            t.insert(h["abce"], 2);

            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abce"].id());

            t.insert(h["abcd"]);
            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcd"].id());
        }

//不需要连接断路器
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"]);
            t.insert(h["abce"], 2);
//ABCE的利润率只有1，但它拥有打破平局的优势。
            BEAST_EXPECT(h["abce"].id() > h["abcd"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abce"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abce"].id());

//将支持从ABCE切换到ABCD，现在需要连接断路器
            t.remove(h["abce"]);
            t.insert(h["abcd"]);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
        }

//单节点大孙子
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcd"], 2);
            t.insert(h["abcde"], 4);
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abcde"].id());
        }

//来自竞争分支机构的过多未承诺支持
        {
            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["abc"]);
            t.insert(h["abcde"], 2);
            t.insert(h["abcfg"], 2);
//“de”和“fg”并列，没有“abc”投票。
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abc"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abc"].id());

            t.remove(h["abc"]);
            t.insert(h["abcd"]);

//“de”分支有3票对2票，因此早期序列将其视为
//首选
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abcde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["abcde"].id());

//但是，如果您使用seq 5验证了分类账，则可能在
//另一个分支，您还不知道他们是否选择ABCD
//或者是因为你的原因，所以ABC仍然是首选
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["abc"].id());
        }

//更改largestseq透视图更改首选分支
        {
            /*用注释的初始提示支持构建下面的树
                   一
                  / \
               B（1）C（1）
              /*
             H D F（1）
                 γ
                 e（2）
                 γ
                 G
            **/

            LedgerTrie<Ledger> t;
            LedgerHistoryHelper h;
            t.insert(h["ab"]);
            t.insert(h["ac"]);
            t.insert(h["acf"]);
            t.insert(h["abde"], 2);

//B有更多的分支支持
            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
//但如果你上次确认D，F或E，你还不知道
//如果有人使用验证提交给B或C
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());

            /*其中一个向G前进不会改变任何东西
                   一
                  / \
               B（1）C（1）
              /*
             H D F（1）
                 γ
                 e（1）
                 γ
                 G（1）
            **/

            t.remove(h["abde"]);
            t.insert(h["abdeg"]);

            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["a"].id());

            /*C前进到H前进到Seq 3优先分类账
                   一
                  / \
               B（1）C
              /*
             H（1）D F（1）
                 γ
                 e（1）
                 γ
                 G（1）
            **/

            t.remove(h["ac"]);
            t.insert(h["abh"]);
            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["a"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["a"].id());

            /*F前进到E也将首选分类账向前移动
                   一
                  / \
               B（1）C
              /*
             H（1）D f
                 γ
                 e（2）
                 γ
                 G（1）
            **/

            t.remove(h["acf"]);
            t.insert(h["abde"]);
            BEAST_EXPECT(t.getPreferred(Seq{1})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{2})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{3})->id == h["abde"].id());
            BEAST_EXPECT(t.getPreferred(Seq{4})->id == h["ab"].id());
            BEAST_EXPECT(t.getPreferred(Seq{5})->id == h["ab"].id());
        }
    }

    void
    testRootRelated()
    {
        using namespace csf;
        using Seq = Ledger::Seq;
//因为根是一个特殊的节点，它不会破坏任何单个子节点
//不变量，做一些测试来练习它。

        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;
        BEAST_EXPECT(!t.remove(h[""]));
        BEAST_EXPECT(t.branchSupport(h[""]) == 0);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        t.insert(h["a"]);
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 1);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        t.insert(h["e"]);
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 2);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);

        BEAST_EXPECT(t.remove(h["e"]));
        BEAST_EXPECT(t.checkInvariants());
        BEAST_EXPECT(t.branchSupport(h[""]) == 1);
        BEAST_EXPECT(t.tipSupport(h[""]) == 0);
    }

    void
    testStress()
    {
        using namespace csf;
        LedgerTrie<Ledger> t;
        LedgerHistoryHelper h;

//测试准随机添加/删除不同分类账的支持
//从分支历史。

//分类帐有序列1、2、3、4
        std::uint32_t const depth = 4;
//每个分类帐有4个可能的子分类帐
        std::uint32_t const width = 4;

        std::uint32_t const iterations = 10000;

//使用显式种子对CI产生相同的结果
        std::mt19937 gen{42};
        std::uniform_int_distribution<> depthDist(0, depth - 1);
        std::uniform_int_distribution<> widthDist(0, width - 1);
        std::uniform_int_distribution<> flip(0, 1);
        for (std::uint32_t i = 0; i < iterations; ++i)
        {
//随机选取分类帐历史记录
            std::string curr = "";
            char depth = depthDist(gen);
            char offset = 0;
            for (char d = 0; d < depth; ++d)
            {
                char a = offset + widthDist(gen);
                curr += a;
                offset = (a + 1) * width;
            }

//50-50添加删除
            if (flip(gen) == 0)
                t.insert(h[curr]);
            else
                t.remove(h[curr]);
            if (!BEAST_EXPECT(t.checkInvariants()))
                return;
        }
    }

    void
    run() override
    {
        testInsert();
        testRemove();
        testEmpty();
        testSupport();
        testGetPreferred();
        testRootRelated();
        testStress();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerTrie, consensus, ripple);
}  //命名空间测试
}  //命名空间波纹
