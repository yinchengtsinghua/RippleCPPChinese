
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
    版权所有（c）2012-2014 Ripple Labs Inc.

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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//{
//事务：<hex>
//}

static
bool
isHexTxID (std::string const& txid)
{
    if (txid.size () != 64)
        return false;

    auto const ret = std::find_if (txid.begin (), txid.end (),
        [](std::string::value_type c)
        {
            return !std::isxdigit (static_cast<unsigned char>(c));
        });

    return (ret == txid.end ());
}

static
bool
isValidated (RPC::Context& context, std::uint32_t seq, uint256 const& hash)
{
    if (!context.ledgerMaster.haveLedger (seq))
        return false;

    if (seq > context.ledgerMaster.getValidatedLedger ()->info().seq)
        return false;

    return context.ledgerMaster.getHashBySeq (seq) == hash;
}

bool
getMetaHex (Ledger const& ledger,
    uint256 const& transID, std::string& hex)
{
    SHAMapTreeNode::TNType type;
    auto const item =
        ledger.txMap().peekItem (transID, type);

    if (!item)
        return false;

    if (type != SHAMapTreeNode::tnTRANSACTION_MD)
        return false;

    SerialIter it (item->slice());
it.getVL (); //跳过事务
    hex = strHex (makeSlice(it.getVL ()));
    return true;
}

Json::Value doTx (RPC::Context& context)
{
    if (!context.params.isMember (jss::transaction))
        return rpcError (rpcINVALID_PARAMS);

    bool binary = context.params.isMember (jss::binary)
            && context.params[jss::binary].asBool ();

    auto const txid  = context.params[jss::transaction].asString ();

    if (!isHexTxID (txid))
        return rpcError (rpcNOT_IMPL);

    auto txn = context.app.getMasterTransaction ().fetch (
        from_hex_text<uint256>(txid), true);

    if (!txn)
        return rpcError (rpcTXN_NOT_FOUND);

    Json::Value ret = txn->getJson (1, binary);

    if (txn->getLedger () == 0)
        return ret;

    if (auto lgr = context.ledgerMaster.getLedgerBySeq (txn->getLedger ()))
    {
        bool okay = false;

        if (binary)
        {
            std::string meta;

            if (getMetaHex (*lgr, txn->getID (), meta))
            {
                ret[jss::meta] = meta;
                okay = true;
            }
        }
        else
        {
            auto rawMeta = lgr->txRead (txn->getID()).second;
            if (rawMeta)
            {
                auto txMeta = std::make_shared<TxMeta> (txn->getID (),
                    lgr->seq (), *rawMeta, context.app.journal ("TxMeta"));
                okay = true;
                auto meta = txMeta->getJson (0);
                addPaymentDeliveredAmount (meta, context, txn, txMeta);
                ret[jss::meta] = meta;
            }
        }

        if (okay)
            ret[jss::validated] = isValidated (
                context, lgr->info().seq, lgr->info().hash);
    }

    return ret;
}

} //涟漪
