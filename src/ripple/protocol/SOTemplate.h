
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

#ifndef RIPPLE_PROTOCOL_SOTEMPLATE_H_INCLUDED
#define RIPPLE_PROTOCOL_SOTEMPLATE_H_INCLUDED

#include <ripple/protocol/SField.h>
#include <boost/range.hpp>
#include <memory>

namespace ripple {

/*sotemplate中元素的标志。*/
//vvalco注意到这些看起来不像位标记…
enum SOE_Flags
{
    SOE_INVALID  = -1,
SOE_REQUIRED = 0,   //必修的
SOE_OPTIONAL = 1,   //可选，可以使用默认值显示
SOE_DEFAULT  = 2,   //可选，如果存在，则不能有默认值
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*sotemplate中的元素。*/
class SOElement
{
public:
    SField const&     e_field;
    SOE_Flags const   flags;

    SOElement (SField const& fieldName, SOE_Flags flags)
        : e_field (fieldName)
        , flags (flags)
    {
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*定义对象中的字段及其属性。
    serializedObject的每个子类都将提供自己的模板
    描述可用字段及其元数据属性。
**/

class SOTemplate
{
public:
    using list_type = std::vector <std::unique_ptr <SOElement const>>;
    using iterator_range = boost::iterator_range<list_type::const_iterator>;

    /*创建空模板。
        创建模板后，使用
        期望字段。
        “见推后背”
    **/

    SOTemplate () = default;

    SOTemplate(SOTemplate&& other)
        : mTypes(std::move(other.mTypes))
        , mIndex(std::move(other.mIndex))
    {
    }

    /*提供字段的枚举*/
    iterator_range all () const
    {
        return boost::make_iterator_range(mTypes);
    }

    /*此模板中的条目数*/
    std::size_t size () const
    {
        return mTypes.size ();
    }

    /*向模板中添加元素。*/
    void push_back (SOElement const& r);

    /*检索命名字段的位置。*/
    int getIndex (SField const&) const;

    SOE_Flags
    style(SField const& sf) const
    {
        return mTypes[mIndex[sf.getNum()]]->flags;
    }

private:
    list_type mTypes;

std::vector <int> mIndex;       //字段编号->索引
};

} //涟漪

#endif
