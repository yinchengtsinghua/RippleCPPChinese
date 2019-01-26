
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

#ifndef RIPPLE_PROTOCOL_SIGN_H_INCLUDED
#define RIPPLE_PROTOCOL_SIGN_H_INCLUDED

#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/STObject.h>
#include <utility>

namespace ripple {

/*签署一个主题

    @param st要签名的对象
    哈希时要在序列化对象之前插入的@param prefix前缀
    用于派生公钥的@param type签名密钥类型
    @param sk签名密钥
    @param sigfield在其中存储对象签名的字段。
    如果未指定，则值默认为“sfsignature”。

    @注意：如果签名已经存在，它将被覆盖。
**/

void
sign (STObject& st, HashPrefix const& prefix,
    KeyType type, SecretKey const& sk,
        SF_Blob const& sigField = sfSignature);

/*如果stobject包含有效签名，则返回“true”

    @param st签名对象
    哈希时在序列化对象之前插入了@param prefix前缀
    用于验证签名的@param pk公钥
    @param sigfield对象包含签名的字段。
    如果未指定，则值默认为“sfsignature”。
**/

bool
verify (STObject const& st, HashPrefix const& prefix,
    PublicKey const& pk,
        SF_Blob const& sigField = sfSignature);

/*返回一个适用于计算多点火txnsignature的序列化程序。*/
Serializer
buildMultiSigningData (STObject const& obj, AccountID const& signingID);

/*将多签名哈希计算分成两部分进行优化。

    我们可以通过分割
    数据构建分为两部分；
     o所有计算共享的大部分。
     o多签名中每个签名者独有的小部分。

    以下方法支持该优化：
     1。StartMultiSigningData提供了可以共享的大部分内容。
     2。finishmuiltisigndata用每个
        签名者的唯一数据。
**/

Serializer
startMultiSigningData (STObject const& obj);

inline void
finishMultiSigningData (AccountID const& signingID, Serializer& s)
{
    s.add160 (signingID);
}

} //涟漪

#endif
