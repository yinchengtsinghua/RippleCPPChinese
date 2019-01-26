
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

#ifndef RIPPLE_APP_CONSENSUS_RCLCXPEERPOS_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCXPEERPOS_H_INCLUDED

#include <ripple/basics/CountedObject.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <chrono>
#include <cstdint>
#include <string>

namespace ripple {

/*一位同行签署的，建议在RCLconsument中使用的职位。

    携带由同伴签署的同意书。提供值语义
    但在内部管理对等位置的共享存储。
**/

class RCLCxPeerPos
{
public:
//<提议职位的类型
    using Proposal = ConsensusProposal<NodeID, uint256, uint256>;

    /*构造函数

        构造有符号的对等位置。

        对等机的@param public key公钥
        随建议书提供的@param签名
        @param suppress用于哈希路由器抑制的唯一ID
        @param proposal协商一致的建议
    **/


    RCLCxPeerPos(
        PublicKey const& publicKey,
        Slice const& signature,
        uint256 const& suppress,
        Proposal&& proposal);

//！为建议创建签名哈希
    uint256
    signingHash() const;

//！验证提案的签名哈希
    bool
    checkSign() const;

//！建议书签字（不一定经过核实）
    Slice
    signature() const
    {
        return data_->signature_;
    }

//！发送建议的对等方的公钥
    PublicKey const&
    publicKey() const
    {
        return data_->publicKey_;
    }

//！哈希路由器用于禁止重复的唯一ID
    uint256 const&
    suppressionID() const
    {
        return data_->suppression_;
    }

    Proposal const &
    proposal() const
    {
        return data_->proposal_;
    }

//！JSON提案陈述
    Json::Value
    getJson() const;

private:

    struct Data : public CountedObject<Data>
    {
        PublicKey publicKey_;
        Buffer signature_;
        uint256 suppression_;
        Proposal proposal_;

        Data(
            PublicKey const& publicKey,
            Slice const& signature,
            uint256 const& suppress,
            Proposal&& proposal);

        static char const*
        getCountedObjectName()
        {
            return "RCLCxPeerPos::Data";
        }
    };

    std::shared_ptr<Data> data_;

    template <class Hasher>
    void
    hash_append(Hasher& h) const
    {
        using beast::hash_append;
        hash_append(h, HashPrefix::proposal);
        hash_append(h, std::uint32_t(proposal().proposeSeq()));
        hash_append(h, proposal().closeTime());
        hash_append(h, proposal().prevLedger());
        hash_append(h, proposal().position());
    }

};

/*计算已签名建议的唯一标识符。

    标识符基于所有参与签名的字段，
    以及签名本身。“上次关闭的分类帐”字段可能是
    省略，但签名者将计算签名，就像此字段是
    现在。提案的接收者将在
    命令验证签名。如果最后一个已关闭的分类帐被删除，则
    出于签名的目的，它被视为全部零。

    @param proposeHash建议位置的哈希值
    @param previousleger建议所基于的分类帐的哈希
    @param proposeseq建议的序列号
    @param close time建议的关闭时间
    
    @param签名建议签名
**/

uint256
proposalUniqueId(
    uint256 const& proposeHash,
    uint256 const& previousLedger,
    std::uint32_t proposeSeq,
    NetClock::time_point closeTime,
    Slice const& publicKey,
    Slice const& signature);

}  //涟漪

#endif
