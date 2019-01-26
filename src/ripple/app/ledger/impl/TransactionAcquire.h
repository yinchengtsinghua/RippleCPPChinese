
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

#ifndef RIPPLE_APP_LEDGER_TRANSACTIONACQUIRE_H_INCLUDED
#define RIPPLE_APP_LEDGER_TRANSACTIONACQUIRE_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {

//vfalc todo重命名为peertxrequest
//我们试图获取的事务集
class TransactionAcquire
    : public PeerSet
    , public std::enable_shared_from_this <TransactionAcquire>
    , public CountedObject <TransactionAcquire>
{
public:
    static char const* getCountedObjectName () { return "TransactionAcquire"; }

    using pointer = std::shared_ptr<TransactionAcquire>;

public:
    TransactionAcquire (Application& app, uint256 const& hash, clock_type& clock);
    ~TransactionAcquire () = default;

    std::shared_ptr<SHAMap> const& getMap ()
    {
        return mMap;
    }

    SHAMapAddNode takeNodes (const std::list<SHAMapNodeID>& IDs,
                             const std::list< Blob >& data, std::shared_ptr<Peer> const&);

    void init (int startPeers);

    void stillNeed ();

private:

    std::shared_ptr<SHAMap> mMap;
    bool                    mHaveRoot;
    beast::Journal          j_;

    void execute () override;

    void onTimer (bool progress, ScopedLockType& peerSetLock) override;


    void newPeer (std::shared_ptr<Peer> const& peer) override
    {
        trigger (peer);
    }

    void done ();

//尝试添加指定数量的对等方
    void addPeers (int num);

    void trigger (std::shared_ptr<Peer> const&);
    std::weak_ptr<PeerSet> pmDowncast () override;
};

} //涟漪

#endif
