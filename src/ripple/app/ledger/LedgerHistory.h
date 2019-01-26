
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

#ifndef RIPPLE_APP_LEDGER_LEDGERHISTORY_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERHISTORY_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/main/Application.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/beast/insight/Event.h>

namespace ripple {

//vvalco todo重命名为oldledgers？

/*保留历史分类账。*/
class LedgerHistory
{
public:
    LedgerHistory (beast::insight::Collector::ptr const& collector,
        Application& app);

    /*跟踪分类帐
        @return`true`如果分类帐已经被跟踪
    **/

    bool insert (std::shared_ptr<Ledger const> ledger,
        bool validated);

    /*按哈希缓存命中率获取分类帐
        @返回命中率
    **/

    float getCacheHitRate ()
    {
        return m_ledgers_by_hash.getHitRate ();
    }

    /*根据分类账的序列号获取分类账*/
    std::shared_ptr<Ledger const>
    getLedgerBySeq (LedgerIndex ledgerIndex);

    /*根据分类账的哈希值检索分类账*/
    std::shared_ptr<Ledger const>
    getLedgerByHash (LedgerHash const& ledgerHash);

    /*根据分类帐的序列号获取分类帐的哈希值
        @param ledgerindex所需分类帐的序列号
        @返回指定分类帐的哈希值
    **/

    LedgerHash getLedgerHash (LedgerIndex ledgerIndex);

    /*设置历史缓存的参数
        @param size缓存的目标大小
        @param age缓存的目标时间（秒）
    **/

    void tune (int size, std::chrono::seconds age);

    /*删除过时的缓存项
    **/

    void sweep ()
    {
        m_ledgers_by_hash.sweep ();
        m_consensus_validated.sweep ();
    }

    /*报告我们在本地建立了一个特定的分类帐*/
    void builtLedger (
        std::shared_ptr<Ledger const> const&,
        uint256 const& consensusHash,
        Json::Value);

    /*报告我们已经确认了一个特定的分类账*/
    void validatedLedger (
        std::shared_ptr<Ledger const> const&,
        boost::optional<uint256> const& consensusHash);

    /*修复哈希到索引的映射
        @param ledgerindex要修复其映射的索引
        @param ledgerhash要映射到的哈希
        @如果映射被修复，返回'true'
    **/

    bool fixIndex(LedgerIndex ledgerIndex, LedgerHash const& ledgerHash);

    void clearLedgerCachePrior (LedgerIndex seq);

private:

    /*在我们建立一个分类账的情况下记录详细信息，但是
        验证另一个。
        @param建立了我们建立的分类账的哈希
        @param valid我们认为完全有效的分类账哈希
        @param builtconnensusshash为
        我们建造的分类帐
        @param validatedconsureshash已验证分类帐的哈希
        共识交易集
        @param consensive协商回合的状态
    **/

    void
    handleMismatch(
        LedgerHash const& built,
        LedgerHash const& valid,
        boost::optional<uint256> const& builtConsensusHash,
        boost::optional<uint256> const& validatedConsensusHash,
        Json::Value const& consensus);

    Application& app_;
    beast::insight::Collector::ptr collector_;
    beast::insight::Counter mismatch_counter_;

    using LedgersByHash = TaggedCache <LedgerHash, Ledger const>;

    LedgersByHash m_ledgers_by_hash;

//将分类帐索引映射到相应的哈希
//用于调试和日志记录
    struct cv_entry
    {
//本地生成的分类帐的哈希
        boost::optional<LedgerHash> built;
//已验证分类帐的哈希
        boost::optional<LedgerHash> validated;
//本地接受的一致性事务集的哈希
        boost::optional<uint256> builtConsensusHash;
//已验证共识事务集的哈希
        boost::optional<uint256> validatedConsensusHash;
//已建账本共识元数据
        boost::optional<Json::Value> consensus;
    };
    using ConsensusValidated = TaggedCache <LedgerIndex, cv_entry>;
    ConsensusValidated m_consensus_validated;


//将分类帐索引映射到相应的哈希。
std::map <LedgerIndex, LedgerHash> mLedgersByIndex; //已验证的分类帐

    beast::Journal j_;
};

} //涟漪

#endif
