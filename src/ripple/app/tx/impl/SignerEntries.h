
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

#ifndef RIPPLE_TX_IMPL_SIGNER_ENTRIES_H_INCLUDED
#define RIPPLE_TX_IMPL_SIGNER_ENTRIES_H_INCLUDED

#include <ripple/app/tx/impl/Transactor.h>    //诺特克
#include <ripple/protocol/STTx.h>             //sttx：：最大多信号
#include <ripple/protocol/UintTypes.h>        //帐户编号
#include <ripple/protocol/TER.h>              //畸形的
#include <ripple/beast/utility/Journal.h>     //兽类：杂志

namespace ripple {

//向前声明
class STObject;

//一些交易人需要的签名支持
class SignerEntries
{
public:
    explicit SignerEntries() = default;

    struct SignerEntry
    {
        AccountID account;
        std::uint16_t weight;

        SignerEntry (AccountID const& inAccount, std::uint16_t inWeight)
        : account (inAccount)
        , weight (inWeight)
        { }

//用于排序以查找重复帐户
        friend bool operator< (SignerEntry const& lhs, SignerEntry const& rhs)
        {
            return lhs.account < rhs.account;
        }

        friend bool operator== (SignerEntry const& lhs, SignerEntry const& rhs)
        {
            return lhs.account == rhs.account;
        }
    };

//从网络或分类帐中反序列化Signerentries数组。
    static
    std::pair<std::vector<SignerEntry>, NotTEC>
    deserialize (
        STObject const& obj,
        beast::Journal journal,
        std::string const& annotation);
};

} //涟漪

#endif //Ripple_tx_impl_signer_entries_h_包括
