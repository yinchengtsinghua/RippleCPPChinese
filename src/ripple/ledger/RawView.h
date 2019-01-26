
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

#ifndef RIPPLE_LEDGER_RAWVIEW_H_INCLUDED
#define RIPPLE_LEDGER_RAWVIEW_H_INCLUDED

#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <memory>
#include <utility>

namespace ripple {

/*分类帐分录更改的接口。

    子类允许对分类账分录进行原始修改。
**/

class RawView
{
public:
    virtual ~RawView() = default;
    RawView() = default;
    RawView(RawView const&) = default;
    RawView& operator=(RawView const&) = delete;

    /*删除现有状态项。

        提供了SLE，因此实现
        可以计算元数据。
    **/

    virtual
    void
    rawErase (std::shared_ptr<SLE> const& sle) = 0;

    /*无条件插入状态项。

        要求：
            该键不能已经存在。

        影响：

            该键与SLE关联。

        @注意，钥匙取自SLE
    **/

    virtual
    void
    rawInsert (std::shared_ptr<SLE> const& sle) = 0;

    /*无条件替换状态项。

        要求：

            密钥必须存在。

        影响：

            该键与SLE关联。

        @注意，钥匙取自SLE
    **/

    virtual
    void
    rawReplace (std::shared_ptr<SLE> const& sle) = 0;

    /*销毁XRP。

        用于支付交易费用。
    **/

    virtual
    void
    rawDestroyXRP (XRPAmount const& fee) = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*用于更改带有交易记录的分录的接口。

    允许原始修改分类账分录和插入
    将事务映射到事务映射中。
**/

class TxsRawView : public RawView
{
public:
    /*向Tx映射添加事务。

        已关闭的分类帐必须具有元数据，
        打开分类帐时忽略元数据。
    **/

    virtual
    void
    rawTxInsert (ReadView::key_type const& key,
        std::shared_ptr<Serializer const>
            const& txn, std::shared_ptr<
                Serializer const> const& metaData) = 0;
};

} //涟漪

#endif
