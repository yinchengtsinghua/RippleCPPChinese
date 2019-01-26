
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

#include <ripple/app/ledger/ConsensusTransSetSF.h>
#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/digest.h>
#include <ripple/core/JobQueue.h>
#include <ripple/nodestore/Database.h>
#include <ripple/protocol/HashPrefix.h>

namespace ripple {

ConsensusTransSetSF::ConsensusTransSetSF (Application& app, NodeCache& nodeCache)
    : app_ (app)
    , m_nodeCache (nodeCache)
    , j_ (app.journal ("TransactionAcquire"))
{
}

void
ConsensusTransSetSF::gotNode(bool fromFilter, SHAMapHash const& nodeHash,
    std::uint32_t, Blob&& nodeData, SHAMapTreeNode::TNType type) const
{
    if (fromFilter)
        return;

    m_nodeCache.insert (nodeHash, nodeData);

    if ((type == SHAMapTreeNode::tnTRANSACTION_NM) && (nodeData.size () > 16))
    {
//这是一笔交易，我们没有
        JLOG (j_.debug())
                << "Node on our acquiring TX set is TXN we may not have";

        try
        {
//跳过前缀
            Serializer s (nodeData.data() + 4, nodeData.size() - 4);
            SerialIter sit (s.slice());
            auto stx = std::make_shared<STTx const> (std::ref (sit));
            assert (stx->getTransactionID () == nodeHash.as_uint256());
            auto const pap = &app_;
            app_.getJobQueue ().addJob (
                jtTRANSACTION, "TXS->TXN",
                [pap, stx] (Job&) {
                    pap->getOPs().submitTransaction(stx);
                });
        }
        catch (std::exception const&)
        {
            JLOG (j_.warn())
                    << "Fetched invalid transaction in proposed set";
        }
    }
}

boost::optional<Blob>
ConsensusTransSetSF::getNode (SHAMapHash const& nodeHash) const
{
    Blob nodeData;
    if (m_nodeCache.retrieve (nodeHash, nodeData))
        return nodeData;

    auto txn = app_.getMasterTransaction().fetch(nodeHash.as_uint256(), false);

    if (txn)
    {
//这是一笔交易，我们有它
        JLOG (j_.trace())
                << "Node in our acquiring TX set is TXN we have";
        Serializer s;
        s.add32 (HashPrefix::transactionID);
        txn->getSTransaction ()->add (s);
        assert(sha512Half(s.slice()) == nodeHash.as_uint256());
        nodeData = s.peekData ();
        return nodeData;
    }

    return boost::none;
}

} //涟漪
