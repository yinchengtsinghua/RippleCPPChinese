
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
#include <ripple/protocol/digest.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/beast/unit_test.h>
#include <test/shamap/common.h>
#include <test/unit_test/SuiteJournal.h>
#include <functional>
#include <stdexcept>

namespace ripple {
namespace tests {

class FetchPack_test : public beast::unit_test::suite
{
public:
    enum
    {
        tableItems = 100,
        tableItemsExtra = 20
    };

    using Map   = hash_map <SHAMapHash, Blob>;
    using Table = SHAMap;
    using Item  = SHAMapItem;

    struct Handler
    {
        void operator()(std::uint32_t refNum) const
        {
            Throw<std::runtime_error> ("missing node");
        }
    };

    struct TestFilter : SHAMapSyncFilter
    {
        TestFilter (Map& map, beast::Journal journal) : mMap (map), mJournal (journal)
        {
        }

        void
        gotNode(bool fromFilter, SHAMapHash const& nodeHash,
            std::uint32_t ledgerSeq, Blob&& nodeData,
                SHAMapTreeNode::TNType type) const override
        {
        }

        boost::optional<Blob>
        getNode (SHAMapHash const& nodeHash) const override
        {
            Map::iterator it = mMap.find (nodeHash);
            if (it == mMap.end ())
            {
                JLOG(mJournal.fatal()) << "Test filter missing node";
                return boost::none;
            }
            return it->second;
        }

        Map& mMap;
        beast::Journal mJournal;
    };

    std::shared_ptr <Item>
    make_random_item (beast::xor_shift_engine& r)
    {
        Serializer s;
        for (int d = 0; d < 3; ++d)
            s.add32 (ripple::rand_int<std::uint32_t>(r));
        return std::make_shared <Item> (
            s.getSHA512Half(), s.peekData ());
    }

    void
    add_random_items (
        std::size_t n,
        Table& t,
        beast::xor_shift_engine& r)
    {
        while (n--)
        {
            std::shared_ptr <SHAMapItem> item (
                make_random_item (r));
            auto const result (t.addItem (std::move(*item), false, false));
            assert (result);
            (void) result;
        }
    }

    void on_fetch (Map& map, SHAMapHash const& hash, Blob const& blob)
    {
        BEAST_EXPECT(sha512Half(makeSlice(blob)) == hash.as_uint256());
        map.emplace (hash, blob);
    }

    void run () override
    {
        using namespace beast::severities;
        test::SuiteJournal journal ("FetchPack_test", *this);

        TestFamily f(journal);
        std::shared_ptr <Table> t1 (std::make_shared <Table> (
            SHAMapType::FREE, f, SHAMap::version{2}));

        pass ();

//野兽：随机R；
//添加随机项（tableitems，*t1，r）；
//std:：shared_ptr<table>t2（t1->snapshot（true））；
//
//添加随机项（tableitemsetra，*t1，r）；
//添加随机项（tableitemsetra，*t2，r）；

//把T1变成T2
//地图地图；
//t2->getfetchpack（t1.get（），true，1000000，std:：bind（
//&fetchpack_test:：on_fetch，this，std:：ref（map），std:：placeholders:：_1，std:：placeholders:：_2））；
//T1->GetFetchPack（nullptr，true，1000000，std:：bind（
//&fetchpack_test:：on_fetch，this，std:：ref（map），std:：placeholders:：_1，std:：placeholders:：_2））；

//尝试从获取包重新生成t2
//std:：shared_ptr<table>t3；
//尝试
//{
//testfilter过滤器（map，beast:：journal（））；
//
//t3=std:：make_shared<table>（shamaptype:：free，t2->gethash（），
//fullblowcache）；
//
//beast-expect（t3->fetchroot（t2->gethash（），&filter），“无法获取根”）；
//
////所有内容都应该在包中，不需要哈希值
//std:：vector<uint256>hashes=t3->getNeededHashes（1，&filter）；
//Beast_Expect（hashes.empty（），“缺少散列”）；
//
//beast_expect（t3->gethash（）==t2->gethash（），“根哈希不匹配”）；
//Beast_Expect（t3->DeepCompare（*t2），“失败比较”）；
//}
//catch（std:：exception const&）
//{
//fail（“未处理的异常”）；
//}
    }
};

BEAST_DEFINE_TESTSUITE(FetchPack,shamap,ripple);

} //测验
} //涟漪

