
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

#ifndef RIPPLE_PROTOCOL_STLEDGERENTRY_H_INCLUDED
#define RIPPLE_PROTOCOL_STLEDGERENTRY_H_INCLUDED

#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STObject.h>

namespace ripple {

class Invariants_test;

class STLedgerEntry final
    : public STObject
    , public CountedObject <STLedgerEntry>
{
friend Invariants_test; //此测试需要访问私有类型\u

public:
    static char const* getCountedObjectName () { return "STLedgerEntry"; }

    using pointer = std::shared_ptr<STLedgerEntry>;
    using ref     = const std::shared_ptr<STLedgerEntry>&;

    /*使用给定的键和类型创建空对象。*/
    explicit
    STLedgerEntry (Keylet const& k);

    STLedgerEntry (LedgerEntryType type,
            uint256 const& key)
        : STLedgerEntry(Keylet(type, key))
    {
    }

    STLedgerEntry (SerialIter & sit, uint256 const& index);
    STLedgerEntry(SerialIter&& sit, uint256 const& index)
        : STLedgerEntry(sit, index) {}

    STLedgerEntry (STObject const& object, uint256 const& index);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

    SerializedTypeID getSType () const override
    {
        return STI_LEDGERENTRY;
    }

    std::string getFullText () const override;

    std::string getText () const override;

    Json::Value getJson (int options) const override;

    /*返回此项的“key”（或“index”）。
        键标识此条目在中的位置
        shamap关联容器。
    **/

    uint256 const&
    key() const
    {
        return key_;
    }

    LedgerEntryType getType () const
    {
        return type_;
    }

//这是可以穿线的分类帐分录吗
    bool isThreadedType() const;

    bool thread (
        uint256 const& txID,
        std::uint32_t ledgerSeq,
        uint256 & prevTxID,
        std::uint32_t & prevLedgerID);

private:
    /*使Stobject符合此SLE类型的模板
        可以扔
    **/

    void setSLEType ();

private:
    uint256 key_;
    LedgerEntryType type_;
};

using SLE = STLedgerEntry;

} //涟漪

#endif
