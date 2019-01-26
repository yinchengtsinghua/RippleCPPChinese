
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

#ifndef BEAST_MODULE_CORE_DIAGNOSTIC_SEMANTICVERSION_H_INCLUDED
#define BEAST_MODULE_CORE_DIAGNOSTIC_SEMANTICVERSION_H_INCLUDED

#include <vector>
#include <string>

namespace beast {

/*语义版本号。

    识别特定软件版本的构建，使用
    这里描述的语义版本规范：

    http://semver.org/
**/

class SemanticVersion
{
public:
    using identifier_list = std::vector<std::string>;

    int majorVersion;
    int minorVersion;
    int patchVersion;

    identifier_list preReleaseIdentifiers;
    identifier_list metaData;

    SemanticVersion ();

    SemanticVersion (std::string const& version);

    /*分析语义版本字符串。
        解析尽可能严格。
        @如果分析了字符串，返回'true'。
    **/

    bool parse (std::string const& input);

    /*从语义版本组件生成字符串。*/
    std::string print () const;

    inline bool isRelease () const noexcept
    {
        return preReleaseIdentifiers.empty();
    }
    inline bool isPreRelease () const noexcept
    {
        return !isRelease ();
    }
};

/*比较两个语义版本。
    按照规范进行比较。
**/

int compare (SemanticVersion const& lhs, SemanticVersion const& rhs);

inline bool
operator== (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) == 0;
}

inline bool
operator!= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) != 0;
}

inline bool
operator>= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) >= 0;
}

inline bool
operator<= (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) <= 0;
}

inline bool
operator>  (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) >  0;
}

inline bool
operator<  (SemanticVersion const& lhs, SemanticVersion const& rhs)
{
    return compare (lhs, rhs) <  0;
}

} //野兽

#endif
