
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

#include <ripple/overlay/impl/TrafficCount.h>

namespace ripple {

const char* TrafficCount::getName (category c)
{
    switch (c)
    {
        case category::CT_base:
            return "overhead";
        case category::CT_overlay:
            return "overlay";
        case category::CT_transaction:
            return "transactions";
        case category::CT_proposal:
            return "proposals";
        case category::CT_validation:
            return "validations";
        case category::CT_get_ledger:
            return "ledger_get";
        case category::CT_share_ledger:
            return "ledger_share";
        case category::CT_get_trans:
            return "transaction_set_get";
        case category::CT_share_trans:
            return "transaction_set_share";
        case category::CT_unknown:
            assert (false);
            return "unknown";
        default:
            assert (false);
            return "truly_unknow";
    }
}

TrafficCount::category TrafficCount::categorize (
    ::google::protobuf::Message const& message,
    int type, bool inbound)
{
    if ((type == protocol::mtHELLO) ||
            (type == protocol::mtPING) ||
            (type == protocol::mtCLUSTER) ||
            (type == protocol::mtSTATUS_CHANGE))
        return TrafficCount::category::CT_base;

    if ((type == protocol::mtMANIFESTS) ||
            (type == protocol::mtENDPOINTS) ||
            (type == protocol::mtGET_SHARD_INFO) ||
            (type == protocol::mtSHARD_INFO) ||
            (type == protocol::mtPEERS) ||
            (type == protocol::mtGET_PEERS))
        return TrafficCount::category::CT_overlay;

    if (type == protocol::mtTRANSACTION)
        return TrafficCount::category::CT_transaction;

    if (type == protocol::mtVALIDATION)
        return TrafficCount::category::CT_validation;

    if (type == protocol::mtPROPOSE_LEDGER)
        return TrafficCount::category::CT_proposal;

    if (type == protocol::mtHAVE_SET)
        return inbound ? TrafficCount::category::CT_get_trans :
            TrafficCount::category::CT_share_trans;

    {
        auto msg = dynamic_cast
            <protocol::TMLedgerData const*> (&message);
        if (msg)
        {
//我们已收到分类帐数据
            if (msg->type() == protocol::liTS_CANDIDATE)
                return (inbound && !msg->has_requestcookie()) ?
                    TrafficCount::category::CT_get_trans :
                    TrafficCount::category::CT_share_trans;
            return (inbound && !msg->has_requestcookie()) ?
                TrafficCount::category::CT_get_ledger :
                TrafficCount::category::CT_share_ledger;
        }
    }

    {
        auto msg =
            dynamic_cast <protocol::TMGetLedger const*>
                (&message);
        if (msg)
        {
            if (msg->itype() == protocol::liTS_CANDIDATE)
                return (inbound || msg->has_requestcookie()) ?
                    TrafficCount::category::CT_share_trans :
                    TrafficCount::category::CT_get_trans;
            return (inbound || msg->has_requestcookie()) ?
                TrafficCount::category::CT_share_ledger :
                TrafficCount::category::CT_get_ledger;
        }
    }

    {
        auto msg =
            dynamic_cast <protocol::TMGetObjectByHash const*>
                (&message);
        if (msg)
        {
//正在共享入站查询和出站响应
//出站查询和入站响应正在获取
            return (msg->query() == inbound) ?
                TrafficCount::category::CT_share_ledger :
                TrafficCount::category::CT_get_ledger;
        }
    }

    assert (false);
    return TrafficCount::category::CT_unknown;
}

} //涟漪
