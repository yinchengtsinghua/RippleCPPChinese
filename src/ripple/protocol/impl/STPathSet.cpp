
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

#include <ripple/protocol/STPathSet.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/strHex.h>
#include <ripple/basics/StringUtilities.h>
#include <cstddef>

namespace ripple {

std::size_t
STPathElement::get_hash (STPathElement const& element)
{
    std::size_t hash_account  = 2654435761;
    std::size_t hash_currency = 2654435761;
    std::size_t hash_issuer   = 2654435761;

//尼克：这不一定是一个安全的哈希，因为速度更快。
//重要的。我们甚至不需要把整个
//在这里输入基值，就像我们使用的几个字节一样。

    for (auto const x : element.getAccountID ())
        hash_account += (hash_account * 257) ^ x;

    for (auto const x : element.getCurrency ())
        hash_currency += (hash_currency * 509) ^ x;

    for (auto const x : element.getIssuerID ())
        hash_issuer += (hash_issuer * 911) ^ x;

    return (hash_account ^ hash_currency ^ hash_issuer);
}

STPathSet::STPathSet (SerialIter& sit, SField const& name)
    : STBase(name)
{
    std::vector<STPathElement> path;
    for(;;)
    {
        int iType = sit.get8 ();

        if (iType == STPathElement::typeNone ||
            iType == STPathElement::typeBoundary)
        {
            if (path.empty ())
            {
                JLOG (debugLog().error())
                    << "Empty path in pathset";
                Throw<std::runtime_error> ("empty path");
            }

            push_back (path);
            path.clear ();

            if (iType == STPathElement::typeNone)
                return;
        }
        else if (iType & ~STPathElement::typeAll)
        {
            JLOG (debugLog().error())
                << "Bad path element " << iType << " in pathset";
            Throw<std::runtime_error> ("bad path element");
        }
        else
        {
            auto hasAccount = iType & STPathElement::typeAccount;
            auto hasCurrency = iType & STPathElement::typeCurrency;
            auto hasIssuer = iType & STPathElement::typeIssuer;

            AccountID account;
            Currency currency;
            AccountID issuer;

            if (hasAccount)
                account.copyFrom (sit.get160 ());

            if (hasCurrency)
                currency.copyFrom (sit.get160 ());

            if (hasIssuer)
                issuer.copyFrom (sit.get160 ());

            path.emplace_back (account, currency, issuer, hasCurrency);
        }
    }
}

bool
STPathSet::assembleAdd(STPath const& base, STPathElement const& tail)
{ //组装底部+尾部，如果不是重复的，则将其添加到集合中
    value.push_back (base);

    std::vector<STPath>::reverse_iterator it = value.rbegin ();

    STPath& newPath = *it;
    newPath.push_back (tail);

    while (++it != value.rend ())
    {
        if (*it == newPath)
        {
            value.pop_back ();
            return false;
        }
    }
    return true;
}

bool
STPathSet::isEquivalent (const STBase& t) const
{
    const STPathSet* v = dynamic_cast<const STPathSet*> (&t);
    return v && (value == v->value);
}

bool
STPath::hasSeen (
    AccountID const& account, Currency const& currency,
    AccountID const& issuer) const
{
    for (auto& p: mPath)
    {
        if (p.getAccountID () == account
            && p.getCurrency () == currency
            && p.getIssuerID () == issuer)
            return true;
    }

    return false;
}

Json::Value
STPath::getJson (int) const
{
    Json::Value ret (Json::arrayValue);

    for (auto it: mPath)
    {
        Json::Value elem (Json::objectValue);
        auto const iType   = it.getNodeType ();

        elem[jss::type]      = iType;
        elem[jss::type_hex]  = strHex (iType);

        if (iType & STPathElement::typeAccount)
            elem[jss::account]  = to_string (it.getAccountID ());

        if (iType & STPathElement::typeCurrency)
            elem[jss::currency] = to_string (it.getCurrency ());

        if (iType & STPathElement::typeIssuer)
            elem[jss::issuer]   = to_string (it.getIssuerID ());

        ret.append (elem);
    }

    return ret;
}

Json::Value
STPathSet::getJson (int options) const
{
    Json::Value ret (Json::arrayValue);
    for (auto it: value)
        ret.append (it.getJson (options));

    return ret;
}

void
STPathSet::add (Serializer& s) const
{
    assert (fName->isBinary ());
    assert (fName->fieldType == STI_PATHSET);
    bool first = true;

    for (auto const& spPath : value)
    {
        if (!first)
            s.add8 (STPathElement::typeBoundary);

        for (auto const& speElement : spPath)
        {
            int iType = speElement.getNodeType ();

            s.add8 (iType);

            if (iType & STPathElement::typeAccount)
                s.add160 (speElement.getAccountID ());

            if (iType & STPathElement::typeCurrency)
                s.add160 (speElement.getCurrency ());

            if (iType & STPathElement::typeIssuer)
                s.add160 (speElement.getIssuerID ());
        }

        first = false;
    }

    s.add8 (STPathElement::typeNone);
}

} //涟漪
