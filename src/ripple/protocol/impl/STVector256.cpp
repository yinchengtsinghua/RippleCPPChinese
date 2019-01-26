
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

#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/STVector256.h>

namespace ripple {

STVector256::STVector256(SerialIter& sit, SField const& name)
    : STBase(name)
{
    Blob data = sit.getVL ();
    auto const count = data.size () / (256 / 8);
    mValue.reserve (count);
    Blob::iterator begin = data.begin ();
    unsigned int uStart  = 0;
    for (unsigned int i = 0; i != count; i++)
    {
        unsigned int uEnd = uStart + (256 / 8);
//下一行可以优化以构造默认值
//向量中的uint256，然后复制到其中
        mValue.push_back (uint256 (Blob (begin + uStart, begin + uEnd)));
        uStart  = uEnd;
    }
}

void
STVector256::add (Serializer& s) const
{
    assert (fName->isBinary ());
    assert (fName->fieldType == STI_VECTOR256);
    s.addVL (mValue.begin(), mValue.end(), mValue.size () * (256 / 8));
}

bool
STVector256::isEquivalent (const STBase& t) const
{
    const STVector256* v = dynamic_cast<const STVector256*> (&t);
    return v && (mValue == v->mValue);
}

Json::Value
STVector256::getJson (int) const
{
    Json::Value ret (Json::arrayValue);

    for (auto const& vEntry : mValue)
        ret.append (to_string (vEntry));

    return ret;
}

} //涟漪
