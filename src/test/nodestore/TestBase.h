
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

#ifndef RIPPLE_NODESTORE_BASE_H_INCLUDED
#define RIPPLE_NODESTORE_BASE_H_INCLUDED

#include <ripple/nodestore/Database.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/beast/xor_shift_engine.h>
#include <ripple/nodestore/Backend.h>
#include <ripple/nodestore/Types.h>
#include <boost/algorithm/string.hpp>
#include <iomanip>

namespace ripple {
namespace NodeStore {

/*满足严格弱序要求的二元函数。

    这将比较两个对象的哈希值，并返回true if
    第一个哈希被认为先于第二个哈希。

    参见STD：：排序
**/

struct LessThan
{
    bool
    operator()(
        std::shared_ptr<NodeObject> const& lhs,
            std::shared_ptr<NodeObject> const& rhs) const noexcept
    {
        return lhs->getHash () < rhs->getHash ();
    }
};

/*如果对象相同，则返回“true”。*/
inline
bool isSame (std::shared_ptr<NodeObject> const& lhs,
    std::shared_ptr<NodeObject> const& rhs)
{
    return
        (lhs->getType() == rhs->getType()) &&
        (lhs->getHash() == rhs->getHash()) &&
        (lhs->getData() == rhs->getData());
}

//单元测试的一些常见代码
//
class TestBase : public beast::unit_test::suite
{
public:
//可调参数
//
    static std::size_t const minPayloadBytes = 1;
    static std::size_t const maxPayloadBytes = 2000;
    static int const numObjectsToTest = 2000;

public:
//创建可预测的一批对象
    static
    Batch createPredictableBatch(
        int numObjects, std::uint64_t seed)
    {
        Batch batch;
        batch.reserve (numObjects);

        beast::xor_shift_engine rng (seed);

        for (int i = 0; i < numObjects; ++i)
        {
            NodeObjectType type;

            switch (rand_int(rng, 3))
            {
            case 0: type = hotLEDGER; break;
            case 1: type = hotACCOUNT_NODE; break;
            case 2: type = hotTRANSACTION_NODE; break;
            case 3: type = hotUNKNOWN; break;
            }

            uint256 hash;
            beast::rngfill (hash.begin(), hash.size(), rng);

            Blob blob (
                rand_int(rng,
                    minPayloadBytes, maxPayloadBytes));
            beast::rngfill (blob.data(), blob.size(), rng);

            batch.push_back (
                NodeObject::createObject(
                    type, std::move(blob), hash));
        }

        return batch;
    }

//比较两批是否相等
    static bool areBatchesEqual (Batch const& lhs, Batch const& rhs)
    {
        bool result = true;

        if (lhs.size () == rhs.size ())
        {
            for (int i = 0; i < lhs.size (); ++i)
            {
                if (! isSame(lhs[i], rhs[i]))
                {
                    result = false;
                    break;
                }
            }
        }
        else
        {
            result = false;
        }

        return result;
    }

//将批存储在后端
    void storeBatch (Backend& backend, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            backend.store (batch [i]);
        }
    }

//在后端获取批处理的副本
    void fetchCopyOfBatch (Backend& backend, Batch* pCopy, Batch const& batch)
    {
        pCopy->clear ();
        pCopy->reserve (batch.size ());

        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object;

            Status const status = backend.fetch (
                batch [i]->getHash ().cbegin (), &object);

            BEAST_EXPECT(status == ok);

            if (status == ok)
            {
                BEAST_EXPECT(object != nullptr);

                pCopy->push_back (object);
            }
        }
    }

    void fetchMissing(Backend& backend, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object;

            Status const status = backend.fetch (
                batch [i]->getHash ().cbegin (), &object);

            BEAST_EXPECT(status == notFound);
        }
    }

//批量存储所有对象
    static void storeBatch (Database& db, Batch const& batch)
    {
        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> const object (batch [i]);

            Blob data (object->getData ());

            db.store (object->getType (),
                      std::move (data),
                      object->getHash (),
                      db.earliestSeq());
        }
    }

//将一个批次中的所有哈希值提取到另一个批次中。
    static void fetchCopyOfBatch (Database& db,
                                  Batch* pCopy,
                                  Batch const& batch)
    {
        pCopy->clear ();
        pCopy->reserve (batch.size ());

        for (int i = 0; i < batch.size (); ++i)
        {
            std::shared_ptr<NodeObject> object = db.fetch (
                batch [i]->getHash (), 0);

            if (object != nullptr)
                pCopy->push_back (object);
        }
    }
};

}
}

#endif
