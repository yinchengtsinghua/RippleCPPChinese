
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_VALIDATION_H_INCLUDED
#define RIPPLE_TEST_CSF_VALIDATION_H_INCLUDED

#include <boost/optional.hpp>
#include <memory>
#include <ripple/basics/tagged_integer.h>
#include <test/csf/ledgers.h>
#include <utility>

namespace ripple {
namespace test {
namespace csf {


struct PeerIDTag;
//<唯一标识对等机
using PeerID = tagged_integer<std::uint32_t, PeerIDTag>;

/*对等机的当前密钥

    最后，可以使用对中的第二个条目来模拟短暂的
    钥匙。现在，约定将第二个条目0作为
    万能钥匙。
**/

using PeerKey =  std::pair<PeerID, std::uint32_t>;

/*由特定对等方验证特定分类帐。
**/

class Validation
{
    Ledger::ID ledgerID_{0};
    Ledger::Seq seq_{0};

    NetClock::time_point signTime_;
    NetClock::time_point seenTime_;
    PeerKey key_;
    PeerID nodeID_{0};
    bool trusted_ = false;
    bool full_ = false;
    boost::optional<std::uint32_t> loadFee_;

public:
    using NodeKey = PeerKey;
    using NodeID = PeerID;

    Validation(Ledger::ID id,
        Ledger::Seq seq,
        NetClock::time_point sign,
        NetClock::time_point seen,
        PeerKey key,
        PeerID nodeID,
        bool full,
        boost::optional<std::uint32_t> loadFee = boost::none)
        : ledgerID_{id}
        , seq_{seq}
        , signTime_{sign}
        , seenTime_{seen}
        , key_{key}
        , nodeID_{nodeID}
        , full_{full}
        , loadFee_{loadFee}
    {
    }

    Ledger::ID
    ledgerID() const
    {
        return ledgerID_;
    }

    Ledger::Seq
    seq() const
    {
        return seq_;
    }

    NetClock::time_point
    signTime() const
    {
        return signTime_;
    }

    NetClock::time_point
    seenTime() const
    {
        return seenTime_;
    }

    PeerKey
    key() const
    {
        return key_;
    }

    PeerID
    nodeID() const
    {
        return nodeID_;
    }

    bool
    trusted() const
    {
        return trusted_;
    }

    bool
    full() const
    {
        return full_;
    }


    boost::optional<std::uint32_t>
    loadFee() const
    {
        return loadFee_;
    }

    Validation const&
    unwrap() const
    {
//对于包含RCLvalidation的波纹实现
//stvalidation，csf:：validation没有更具体的类型它
//包装，使csf:：validation展开到自身
        return *this;
    }

    auto
    asTie() const
    {
//受信任是由接收者设置的状态，因此它不是联系的一部分。
        return std::tie(
            ledgerID_,
            seq_,
            signTime_,
            seenTime_,
            key_,
            nodeID_,
            loadFee_,
            full_);
    }
    bool
    operator==(Validation const& o) const
    {
        return asTie() == o.asTie();
    }

    bool
    operator<(Validation const& o) const
    {
        return asTie() < o.asTie();
    }

    void
    setTrusted()
    {
        trusted_ = true;
    }

    void
    setUntrusted()
    {
        trusted_ = false;
    }

    void
    setSeen(NetClock::time_point seen)
    {
        seenTime_ = seen;
    }
};

}  //涟漪
}  //测试
}  //脑脊液
#endif