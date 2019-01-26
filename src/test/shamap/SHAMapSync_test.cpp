
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

#include <ripple/shamap/SHAMap.h>
#include <ripple/shamap/SHAMapItem.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/xor_shift_engine.h>
#include <test/shamap/common.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {
namespace tests {

class sync_test : public beast::unit_test::suite
{
public:
    beast::xor_shift_engine eng_;

    std::shared_ptr<SHAMapItem> makeRandomAS ()
    {
        Serializer s;

        for (int d = 0; d < 3; ++d)
            s.add32 (rand_int<std::uint32_t>(eng_));

        return std::make_shared<SHAMapItem>(
            s.getSHA512Half(), s.peekData ());
    }

    bool confuseMap (SHAMap& map, int count)
    {
//向地图中添加一组随机状态，然后删除它们
//地图应该是一样的
        SHAMapHash beforeHash = map.getHash ();

        std::list<uint256> items;

        for (int i = 0; i < count; ++i)
        {
            std::shared_ptr<SHAMapItem> item = makeRandomAS ();
            items.push_back (item->key());

            if (!map.addItem (std::move(*item), false, false))
            {
                log << "Unable to add item to map\n";
                return false;
            }
        }

        for (auto const& item : items)
        {
            if (!map.delItem (item))
            {
                log << "Unable to remove item from map\n";
                return false;
            }
        }

        if (beforeHash != map.getHash ())
        {
            log <<
                "Hashes do not match " << beforeHash <<
                " " << map.getHash () << std::endl;
            return false;
        }

        return true;
    }

    void run() override
    {
        using namespace beast::severities;
        test::SuiteJournal journal ("SHAMapSync_test", *this);

        log << "Run, version 1\n" << std::endl;
        run(SHAMap::version{1}, journal);

        log << "Run, version 2\n" << std::endl;
        run(SHAMap::version{2}, journal);
    }

    void run(SHAMap::version v, beast::Journal const& journal)
    {
        TestFamily f(journal), f2(journal);
        SHAMap source (SHAMapType::FREE, f, v);
        SHAMap destination (SHAMapType::FREE, f2, v);

        int items = 10000;
        for (int i = 0; i < items; ++i)
        {
            source.addItem (std::move(*makeRandomAS ()), false, false);
            if (i % 100 == 0)
                source.invariants();
        }

        source.invariants();
        BEAST_EXPECT(confuseMap (source, 500));
        source.invariants();

        source.setImmutable ();

        int count = 0;
        source.visitLeaves([&count](auto const& item)
            {
                ++count;
            });
        BEAST_EXPECT(count == items);

        std::vector<SHAMapMissingNode> missingNodes;
        source.walkMap(missingNodes, 2048);
        BEAST_EXPECT(missingNodes.empty());

        std::vector<SHAMapNodeID> nodeIDs, gotNodeIDs;
        std::vector< Blob > gotNodes;
        std::vector<uint256> hashes;

        destination.setSynching ();

        {
            std::vector<SHAMapNodeID> gotNodeIDs_a;
            std::vector<Blob> gotNodes_a;

            BEAST_EXPECT(source.getNodeFat (
                SHAMapNodeID (),
                gotNodeIDs_a,
                gotNodes_a,
                rand_bool(eng_),
                rand_int(eng_, 2)));

            unexpected (gotNodes_a.size () < 1, "NodeSize");

            BEAST_EXPECT(destination.addRootNode (
                source.getHash(),
                makeSlice(*gotNodes_a.begin ()),
                snfWIRE,
                nullptr).isGood());
        }

        do
        {
            f.clock().advance(std::chrono::seconds(1));

//获取我们知道需要的节点列表
            auto nodesMissing = destination.getMissingNodes (2048, nullptr);

            if (nodesMissing.empty ())
                break;

//根据此信息获取尽可能多的节点
            std::vector<SHAMapNodeID> gotNodeIDs_b;
            std::vector<Blob> gotNodes_b;

            for (auto& it : nodesMissing)
            {
//不要使用Beast，这里是B/C，它将被称为非确定性次数
//运行的测试数量应该是确定的
                if (!source.getNodeFat(
                        it.first,
                        gotNodeIDs_b,
                        gotNodes_b,
                        rand_bool(eng_),
                        rand_int(eng_, 2)))
                    fail("", __FILE__, __LINE__);
            }

//不要使用Beast，这里是B/C，它将被称为非确定性次数
//运行的测试数量应该是确定的
            if (gotNodeIDs_b.size() != gotNodes_b.size() ||
                gotNodeIDs_b.empty())
                fail("", __FILE__, __LINE__);

            for (std::size_t i = 0; i < gotNodeIDs_b.size(); ++i)
            {
//不要使用Beast，这里是B/C，它将被称为非确定性次数
//运行的测试数量应该是确定的
                if (!destination
                         .addKnownNode(
                             gotNodeIDs_b[i], makeSlice(gotNodes_b[i]), nullptr)
                         .isUseful())
                    fail("", __FILE__, __LINE__);
            }
        }
        while (true);

        destination.clearSynching ();

        BEAST_EXPECT(source.deepCompare (destination));

        log << "Checking destination invariants..." << std::endl;
        destination.invariants();
    }
};

BEAST_DEFINE_TESTSUITE(sync,shamap,ripple);

} //测验
} //涟漪
