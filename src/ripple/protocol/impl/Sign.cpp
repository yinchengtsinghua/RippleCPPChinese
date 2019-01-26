
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

#include <ripple/protocol/Sign.h>

namespace ripple {

void
sign (STObject& st, HashPrefix const& prefix,
    KeyType type, SecretKey const& sk,
    SF_Blob const& sigField)
{
    Serializer ss;
    ss.add32(prefix);
    st.addWithoutSigningFields(ss);
    set(st, sigField,
        sign(type, sk, ss.slice()));
}

bool
verify (STObject const& st, HashPrefix const& prefix,
    PublicKey const& pk, SF_Blob const& sigField)
{
    auto const sig = get(st, sigField);
    if (! sig)
        return false;
    Serializer ss;
    ss.add32(prefix);
    st.addWithoutSigningFields(ss);
    return verify(pk,
        Slice(ss.data(), ss.size()),
            Slice(sig->data(), sig->size()));
}

//有关建筑物多重点火数据的问题：
//
//为什么要在要签名的blob中包含signer.account？
//
//除非您在签名blob中包含正在签名的帐户，
//你可以换掉任何签名者。帐户换成任何其他的，也可以
//在签名者列表中，并有一个与
//signer.signingpubkey。
//
//可以将该RegularKey设置为允许某些第三方签署事务
//以客户的名义，RegularKey可能在所有人中都很常见
//第三方用户。这只是一个分享的例子
//各种帐户之间的RegularKey，只有一个漏洞。
//
//“当你有一件容易做的事情时，整个类
//攻击显然是不可能的，你需要一个很好的理由
//不要这样做。”——大卫·施瓦茨
//
//为什么要在待签名的blob中包含对帐户的签名？
//
//在当前签名方案中，签名者正在签名的帐户
//代表`是txjson.帐户。
//
//稍后，我们可能支持更多级别的签名。假设Bob是签名者
//对爱丽丝来说，卡罗尔是鲍勃的签名人，所以卡罗尔可以为鲍勃签名。
//爱丽丝的标志。但是假设爱丽丝有两个签名者：鲍勃和戴夫。如果
//卡罗尔是鲍勃和戴夫的签名人，那么签名需要
//区分卡罗尔为鲍勃签名和卡罗尔为戴夫签名。
//
//因此，如果我们支持多个级别的签名，那么我们需要
//将“签到”账户也纳入签到数据中。
Serializer
buildMultiSigningData (STObject const& obj, AccountID const& signingID)
{
    Serializer s {startMultiSigningData (obj)};
    finishMultiSigningData (signingID, s);
    return s;
}

Serializer
startMultiSigningData (STObject const& obj)
{
    Serializer s;
    s.add32 (HashPrefix::txMultiSign);
    obj.addWithoutSigningFields (s);
    return s;
}

} //涟漪
