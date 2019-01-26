
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

#include <test/nodestore/TestBase.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/nodestore/impl/DecodedBlob.h>
#include <ripple/nodestore/impl/EncodedBlob.h>

namespace ripple {
namespace NodeStore {

//测试可预测的批和nodeObject blob编码
//
class NodeStoreBasic_test : public TestBase
{
public:
//确保可预测的对象生成有效！
    void testBatches (std::uint64_t const seedValue)
    {
        testcase ("batch");

        auto batch1 = createPredictableBatch (
            numObjectsToTest, seedValue);

        auto batch2 = createPredictableBatch (
            numObjectsToTest, seedValue);

        BEAST_EXPECT(areBatchesEqual (batch1, batch2));

        auto batch3 = createPredictableBatch (
            numObjectsToTest, seedValue + 1);

        BEAST_EXPECT(! areBatchesEqual (batch1, batch3));
    }

//检查编码/解码块
    void testBlobs (std::uint64_t const seedValue)
    {
        testcase ("encoding");

        auto batch = createPredictableBatch (
            numObjectsToTest, seedValue);

        EncodedBlob encoded;
        for (int i = 0; i < batch.size (); ++i)
        {
            encoded.prepare (batch [i]);

            DecodedBlob decoded (encoded.getKey (), encoded.getData (), encoded.getSize ());

            BEAST_EXPECT(decoded.wasOk ());

            if (decoded.wasOk ())
            {
                std::shared_ptr<NodeObject> const object (decoded.createObject ());

                BEAST_EXPECT(isSame(batch[i], object));
            }
        }
    }

    void run () override
    {
        std::uint64_t const seedValue = 50;

        testBatches (seedValue);

        testBlobs (seedValue);
    }
};

BEAST_DEFINE_TESTSUITE(NodeStoreBasic,ripple_core,ripple);

}
}
