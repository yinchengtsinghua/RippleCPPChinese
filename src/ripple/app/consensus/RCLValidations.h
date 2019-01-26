
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

#ifndef RIPPLE_APP_CONSENSUSS_VALIDATIONS_H_INCLUDED
#define RIPPLE_APP_CONSENSUSS_VALIDATIONS_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/ScopedLock.h>
#include <ripple/consensus/Validations.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <vector>

namespace ripple {

class Application;

/*通用验证代码的包装覆盖stvalidation

    包装一个stvalidation：：指针以与常规验证兼容
    代码。
**/

class RCLValidation
{
    STValidation::pointer val_;
public:
    using NodeKey = ripple::PublicKey;
    using NodeID = ripple::NodeID;

    /*构造函数

        @param v要包装的验证。
    **/

    RCLValidation(STValidation::pointer const& v) : val_{v}
    {
    }

///已验证分类帐哈希
    uint256
    ledgerID() const
    {
        return val_->getLedgerHash();
    }

///已验证分类帐的序列号（如果没有，则为0）
    std::uint32_t
    seq() const
    {
        return val_->getFieldU32(sfLedgerSequence);
    }

///验证的签名时间
    NetClock::time_point
    signTime() const
    {
        return val_->getSignTime();
    }

///验证了分类帐的第一次看到时间
    NetClock::time_point
    seenTime() const
    {
        return val_->getSeenTime();
    }

///发布验证的验证程序的公钥
    PublicKey
    key() const
    {
        return val_->getSignerPublic();
    }

///nodeid发布验证的验证程序
    NodeID
    nodeID() const
    {
        return val_->getNodeID();
    }

///验证是否可信。
    bool
    trusted() const
    {
        return val_->isTrusted();
    }

    void
    setTrusted()
    {
        val_->setTrusted();
    }

    void
    setUntrusted()
    {
        val_->setUntrusted();
    }

///验证是否已满（不是部分验证）
    bool
    full() const
    {
        return val_->isFull();
    }

///获取验证的加载费用（如果存在）
    boost::optional<std::uint32_t>
    loadFee() const
    {
        return ~(*val_)[~sfLoadFee];
    }

///extract正在包装的基础stvalidation
    STValidation::pointer
    unwrap() const
    {
        return val_;
    }

};

/*包装分类帐实例以用于一般验证分类帐。

    Ledgertrie将分类帐的历史建模为seq->id.any中的映射。
    给定序列具有相同ID的两个分类帐具有相同的ID
    所有早期序列（例如共享祖先）。实际上，只有分类帐
    方便地提供了先前的256个祖先哈希。为了
    RCLvalidatedLedger，我们将任何由256个序列分隔的分类账视为
    独特的
**/

class RCLValidatedLedger
{
public:
    using ID = LedgerHash;
    using Seq = LedgerIndex;
    struct MakeGenesis
    {
        explicit MakeGenesis() = default;
    };

    RCLValidatedLedger(MakeGenesis);

    RCLValidatedLedger(
        std::shared_ptr<Ledger const> const& ledger,
        beast::Journal j);

///分类帐的顺序（索引）
    Seq
    seq() const;

///分类帐的ID（哈希）
    ID
    id() const;

    /*查找上级分类帐的ID

        @param s祖先的序列（索引）
        @返回带有序列号的该分类帐祖先的ID或
                ID 0如果未确定
    **/

    ID operator[](Seq const& s) const;

///查找最早不匹配祖先的序列号
    friend Seq
    mismatch(RCLValidatedLedger const& a, RCLValidatedLedger const& b);

    Seq
    minSeq() const;

private:
    ID ledgerID_;
    Seq ledgerSeq_;
    std::vector<uint256> ancestors_;
    beast::Journal j_;
};

/*RCL的通用验证适配器类

    管理向sqlite db存储和写入过时的rclvalidations，以及
    从网络中获取已验证的分类帐。
**/

class RCLValidationsAdaptor
{
public:
//通用验证的类型定义
    using Mutex = std::mutex;
    using Validation = RCLValidation;
    using Ledger = RCLValidatedLedger;

    RCLValidationsAdaptor(Application& app, beast::Journal j);

    /*用于确定验证是否过期的当前时间。
     **/

    NetClock::time_point
    now() const;

    /*处理新过时的验证。

        @param v新失效的验证

        @警告：这应该只做最少的工作，因为它应该被调用
                 当它可能持有
                 内锁
    **/

    void
    onStale(RCLValidation&& v);

    /*关闭前将当前验证刷新到磁盘。

        @param剩余的验证要刷新
    **/

    void
    flush(hash_map<NodeID, RCLValidation>&& remaining);

    /*尝试从网络获取具有给定ID的分类帐*/
    boost::optional<RCLValidatedLedger>
    acquire(LedgerHash const & id);

    beast::Journal
    journal() const
    {
        return j_;
    }

private:
    using ScopedLockType = std::lock_guard<Mutex>;
    using ScopedUnlockType = GenericScopedUnlock<Mutex>;

    Application& app_;
    beast::Journal j_;

//用于管理过时验证和写入的锁
    std::mutex staleLock_;
    std::vector<RCLValidation> staleValidations_;
    bool staleWriting_ = false;

//将过时的验证写入sqlite db，scoped lock参数
//用于提醒呼叫者在
//打电话
    void
    doStaleWrite(ScopedLockType&);
};

///alias用于通用验证的RCL特定实例
using RCLValidations = Validations<RCLValidationsAdaptor>;


/*处理新的验证

    还根据验证节点的
    公钥和此节点的当前UNL。

    包含验证和LedgerMaster的@param app application对象
    @param val要添加的验证
    @param source name与日志中使用的验证关联

    @返回是否应中继验证
**/

bool
handleNewValidation(
    Application& app,
    STValidation::ref val,
    std::string const& source);

}  //命名空间波纹

#endif
