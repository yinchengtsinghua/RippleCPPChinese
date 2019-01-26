
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

#ifndef RIPPLE_PROTOCOL_KNOWNFORMATS_H_INCLUDED
#define RIPPLE_PROTOCOL_KNOWNFORMATS_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SOTemplate.h>
#include <memory>

namespace ripple {

/*管理已知格式的列表。

    每种格式都有一个名称、一个关联的键类型（通常是枚举）。
    以及一个预定义的@ref soelement。

    @tparam key type标识格式的密钥类型。
**/

template <class KeyType>
class KnownFormats
{
public:
    /*一种已知的格式。
    **/

    class Item
    {
    public:
        Item (char const* name, KeyType type)
            : m_name (name)
            , m_type (type)
        {
        }

        Item& operator<< (SOElement const& el)
        {
            elements.push_back (el);

            return *this;
        }

        /*检索格式的名称。
        **/

        std::string const& getName () const noexcept
        {
            return m_name;
        }

        /*检索此格式表示的事务类型。
        **/

        KeyType getType () const noexcept
        {
            return m_type;
        }

    public:
//vfalc todo为此创建访问器
        SOTemplate elements;

    private:
        std::string const m_name;
        KeyType const m_type;
    };

private:
    using NameMap = std::map <std::string, Item*>;
    using TypeMap = std::map <KeyType, Item*>;

public:
    /*创建已知格式对象。

        派生类将加载对象的所有已知格式。
    **/

    KnownFormats () = default;

    /*销毁已知格式对象。

        将删除定义的格式。
    **/

    virtual ~KnownFormats () = default;
    KnownFormats(KnownFormats const&) = delete;
    KnownFormats& operator=(KnownFormats const&) = delete;

    /*检索由名称指定的格式的类型。

        如果格式名未知，则引发异常。

        @param name类型的名称。
        @返回类型。
    **/

    KeyType findTypeByName (std::string const name) const
    {
        Item const* const result = findByName (name);

        if (result != nullptr)
            return result->getType ();
        Throw<std::runtime_error> ("Unknown format name");
return {}; //静默编译器警告。
    }

    /*根据其类型检索格式。
    **/

//vfalc todo我们能不能只返回元素&？
    Item const* findByType (KeyType type) const noexcept
    {
        Item* result = nullptr;

        typename TypeMap::const_iterator const iter = m_types.find (type);

        if (iter != m_types.end ())
        {
            result = iter->second;
        }

        return result;
    }

protected:
    /*根据其名称检索格式。
    **/

    Item const* findByName (std::string const& name) const noexcept
    {
        Item* result = nullptr;

        typename NameMap::const_iterator const iter = m_names.find (name);

        if (iter != m_names.end ())
        {
            result = iter->second;
        }

        return result;
    }

    /*添加新格式。

        新格式已经添加了一组公用字段。

        @param name此格式的名称。
        @param键入此格式的类型。

        @返回创建的格式。
    **/

    Item& add (char const* name, KeyType type)
    {
        m_formats.emplace_back (
            std::make_unique <Item> (name, type));
        auto& item (*m_formats.back());

        addCommonFields (item);

        m_types [item.getType ()] = &item;
        m_names [item.getName ()] = &item;

        return item;
    }

    /*添加公用字段。

        每个新项目都需要这样做。
    **/

    virtual void addCommonFields (Item& item) = 0;

private:
    std::vector <std::unique_ptr <Item>> m_formats;
    NameMap m_names;
    TypeMap m_types;
};

} //涟漪

#endif
