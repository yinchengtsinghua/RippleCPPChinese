
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

#ifndef RIPPLE_PROTOCOL_HASHPREFIX_H_INCLUDED
#define RIPPLE_PROTOCOL_HASHPREFIX_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <cstdint>

namespace ripple {

/*哈希函数的前缀。

    这些前缀插入到用于生成的源材质之前
    各种散列。这样做是为了将每个散列放入自己的“空间”。这样，
    具有相同二进制数据的两种不同类型的对象将产生
    不同的哈希。

    每个前缀都是一个4字节的值，最后一个字节设置为零，第一个字节设置为
    由某个任意字符串的ASCII等价物形成的三个字节。为了
    例如“TXN”。

    @注意哈希前缀是Ripple协议的一部分。

    @ingroup协议
**/

class HashPrefix
{
private:
    std::uint32_t m_prefix;

    HashPrefix (char a, char b, char c)
        : m_prefix (0)
    {
        m_prefix = a;
        m_prefix = (m_prefix << 8) + b;
        m_prefix = (m_prefix << 8) + c;
        m_prefix = m_prefix << 8;
    }

public:
    HashPrefix(HashPrefix const&) = delete;
    HashPrefix& operator=(HashPrefix const&) = delete;

    /*返回与此对象关联的哈希前缀*/
    operator std::uint32_t () const
    {
        return m_prefix;
    }

//vvalco todo将描述扩展为完整、简洁的句子。
//

    /*事务加签名以提供事务ID*/
    static HashPrefix const transactionID;

    /*事务加元数据*/
    static HashPrefix const txNode;

    /*帐户状态*/
    static HashPrefix const leafNode;

    /*v1树中的内部节点*/
    static HashPrefix const innerNode;

    /*v2树中的内部节点*/
    static HashPrefix const innerNodeV2;

    /*用于签名的分类帐主数据*/
    static HashPrefix const ledgerMaster;

    /*要签名的内部事务*/
    static HashPrefix const txSign;

    /*内部事务到多重签名*/
    static HashPrefix const txMultiSign;

    /*签名验证*/
    static HashPrefix const validation;

    /*签署建议书*/
    static HashPrefix const proposal;

    /*显示*/
    static HashPrefix const manifest;

    /*支付渠道索赔*/
    static HashPrefix const paymentChannelClaim;
};

template <class Hasher>
void
hash_append (Hasher& h, HashPrefix const& hp) noexcept
{
    using beast::hash_append;
    hash_append(h,
        static_cast<std::uint32_t>(hp));
}

} //涟漪

#endif
