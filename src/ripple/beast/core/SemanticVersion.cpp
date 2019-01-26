
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/beast/core/LexicalCast.h>

#include <algorithm>
#include <cassert>
#include <locale>

namespace beast {

std::string print_identifiers (SemanticVersion::identifier_list const& list)
{
    std::string ret;

    for (auto const& x : list)
    {
        if (!ret.empty ())
            ret += ".";
        ret += x;
    }

    return ret;
}

bool isNumeric (std::string const& s)
{
    int n;

//必须可转换为整数
    if (!lexicalCastChecked (n, s))
        return false;

//不能有前导零
    return std::to_string (n) == s;
}

bool chop (std::string const& what, std::string& input)
{
    auto ret = input.find (what);

    if (ret != 0)
        return false;

    input.erase (0, what.size ());
    return true;
}

bool chopUInt (int& value, int limit, std::string& input)
{
//不能为空
    if (input.empty ())
        return false;

    auto left_iter = std::find_if_not (input.begin (), input.end (),
        [](std::string::value_type c)
        {
            return std::isdigit (c, std::locale::classic());
        });

    std::string item (input.begin (), left_iter);

//不能为空
    if (item.empty ())
        return false;

    int n;

//必须可转换为整数
    if (!lexicalCastChecked (n, item))
        return false;

//不能有前导零
    if (std::to_string (n) != item)
        return false;

//不能超出范围
    if (n < 0 || n > limit)
        return false;

    input.erase (input.begin (), left_iter);
    value = n;

    return true;
}

bool extract_identifier (
    std::string& value, bool allowLeadingZeroes, std::string& input)
{
//不能为空
    if (input.empty ())
        return false;

//不能有前导0
    if (!allowLeadingZeroes && input [0] == '0')
        return false;

    auto last = input.find_first_not_of (
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-");

//不能为空
    if (last == 0)
        return false;

    value = input.substr (0, last);
    input.erase (0, last);
    return true;
}

bool extract_identifiers (
    SemanticVersion::identifier_list& identifiers,
    bool allowLeadingZeroes,
    std::string& input)
{
    if (input.empty ())
        return false;

    do {
        std::string s;

        if (!extract_identifier (s, allowLeadingZeroes, input))
            return false;
        identifiers.push_back (s);
    } while (chop (".", input));

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

SemanticVersion::SemanticVersion ()
    : majorVersion (0)
    , minorVersion (0)
    , patchVersion (0)
{
}

SemanticVersion::SemanticVersion (std::string const& version)
    : SemanticVersion ()
{
    if (!parse (version))
        throw std::invalid_argument ("invalid version string");
}

bool SemanticVersion::parse (std::string const& input)
{
//可能没有前导空格或尾随空格
    auto left_iter = std::find_if_not (input.begin (), input.end (),
        [](std::string::value_type c)
        {
            return std::isspace (c, std::locale::classic());
        });

    auto right_iter = std::find_if_not (input.rbegin (), input.rend (),
        [](std::string::value_type c)
        {
            return std::isspace (c, std::locale::classic());
        }).base ();

//不能为空！
    if (left_iter >= right_iter)
        return false;

    std::string version (left_iter, right_iter);

//可能没有前导空格或尾随空格
    if (version != input)
        return false;

//必须有主版本号
    if (! chopUInt (majorVersion, std::numeric_limits <int>::max (), version))
        return false;
    if (! chop (".", version))
        return false;

//必须有次要版本号
    if (! chopUInt (minorVersion, std::numeric_limits <int>::max (), version))
        return false;
    if (! chop (".", version))
        return false;

//必须具有修补程序版本号
    if (! chopUInt (patchVersion, std::numeric_limits <int>::max (), version))
        return false;

//可能有预发布标识符列表
    if (chop ("-", version))
    {
        if (!extract_identifiers (preReleaseIdentifiers, false, version))
            return false;

//不能为空
        if (preReleaseIdentifiers.empty ())
            return false;
    }

//可能有元数据标识符列表
    if (chop ("+", version))
    {
        if (!extract_identifiers (metaData, true, version))
            return false;

//不能为空
        if (metaData.empty ())
            return false;
    }

    return version.empty ();
}

std::string SemanticVersion::print () const
{
    std::string s;

    s = std::to_string (majorVersion) + "." +
        std::to_string (minorVersion) + "." +
        std::to_string (patchVersion);

    if (!preReleaseIdentifiers.empty ())
    {
        s += "-";
        s += print_identifiers (preReleaseIdentifiers);
    }

    if (!metaData.empty ())
    {
        s += "+";
        s += print_identifiers (metaData);
    }

    return s;
}

int compare (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    if (lhs.majorVersion > rhs.majorVersion)
        return 1;
    else if (lhs.majorVersion < rhs.majorVersion)
        return -1;

    if (lhs.minorVersion > rhs.minorVersion)
        return 1;
    else if (lhs.minorVersion < rhs.minorVersion)
        return -1;

    if (lhs.patchVersion > rhs.patchVersion)
        return 1;
    else if (lhs.patchVersion < rhs.patchVersion)
        return -1;

    if (lhs.isPreRelease () || rhs.isPreRelease ())
    {
//预发布具有较低的优先级
        if (lhs.isRelease () && rhs.isPreRelease ())
            return 1;
        else if (lhs.isPreRelease () && rhs.isRelease ())
            return -1;

//比较预发布标识符
        for (int i = 0; i < std::max (lhs.preReleaseIdentifiers.size (), rhs.preReleaseIdentifiers.size ()); ++i)
        {
//较大的标识符列表具有较高的优先级
            if (i >= rhs.preReleaseIdentifiers.size ())
                return 1;
            else if (i >= lhs.preReleaseIdentifiers.size ())
                return -1;

            std::string const& left (lhs.preReleaseIdentifiers [i]);
            std::string const& right (rhs.preReleaseIdentifiers [i]);

//数字标识符的优先级较低
            if (! isNumeric (left) && isNumeric (right))
                return 1;
            else if (isNumeric (left) && ! isNumeric (right))
                return -1;

            if (isNumeric (left))
            {
                assert(isNumeric (right));

                int const iLeft (lexicalCastThrow <int> (left));
                int const iRight (lexicalCastThrow <int> (right));

                if (iLeft > iRight)
                    return 1;
                else if (iLeft < iRight)
                    return -1;
            }
            else
            {
                assert (! isNumeric (right));

                int result = left.compare (right);

                if (result != 0)
                    return result;
            }
        }
    }

//忽略元数据

    return 0;
}

} //野兽
