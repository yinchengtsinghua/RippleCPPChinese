
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

#ifndef BEAST_INTRUSIVE_LIST_H_INCLUDED
#define BEAST_INTRUSIVE_LIST_H_INCLUDED

#include <ripple/beast/core/Config.h>

#include <iterator>
#include <type_traits>

namespace beast {

template <typename, typename>
class List;

namespace detail {

/*将“const”属性从t复制到u（如果存在）。*/
/*@ {*/
template <typename T, typename U>
struct CopyConst
{
    explicit CopyConst() = default;

    using type = typename std::remove_const <U>::type;
};

template <typename T, typename U>
struct CopyConst <T const, U>
{
    explicit CopyConst() = default;

    using type = typename std::remove_const <U>::type const;
};
/*@ }*/

//这是双重链接列表的侵入部分。
//对象可能出现在每个列表中的一个派生
//必须同时进行。
//
template <typename T, typename Tag>
class ListNode
{
private:
    using value_type = T;

    friend class List<T, Tag>;

    template <typename>
    friend class ListIterator;

    ListNode* m_next;
    ListNode* m_prev;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <typename N>
class ListIterator : public std::iterator <
    std::bidirectional_iterator_tag, std::size_t>
{
public:
    using value_type = typename beast::detail::CopyConst <
        N, typename N::value_type>::type;
    using pointer = value_type*;
    using reference = value_type&;
    using size_type = std::size_t;

    ListIterator (N* node = nullptr) noexcept
        : m_node (node)
    {
    }

    template <typename M>
    ListIterator (ListIterator <M> const& other) noexcept
        : m_node (other.m_node)
    {
    }

    template <typename M>
    bool operator== (ListIterator <M> const& other) const noexcept
    {
        return m_node == other.m_node;
    }

    template <typename M>
    bool operator!= (ListIterator <M> const& other) const noexcept
    {
        return ! ((*this) == other);
    }

    reference operator* () const noexcept
    {
        return dereference ();
    }

    pointer operator-> () const noexcept
    {
        return &dereference ();
    }

    ListIterator& operator++ () noexcept
    {
        increment ();
        return *this;
    }

    ListIterator operator++ (int) noexcept
    {
        ListIterator result (*this);
        increment ();
        return result;
    }

    ListIterator& operator-- () noexcept
    {
        decrement ();
        return *this;
    }

    ListIterator operator-- (int) noexcept
    {
        ListIterator result (*this);
        decrement ();
        return result;
    }

private:
    reference dereference () const noexcept
    {
        return static_cast <reference> (*m_node);
    }

    void increment () noexcept
    {
        m_node = m_node->m_next;
    }

    void decrement () noexcept
    {
        m_node = m_node->m_prev;
    }

    N* m_node;
};

}

/*侵入式双链接列表。

    此侵入列表是一个容器，其操作与中的std:：list类似。
    标准模板库（STL）。像所有@ref侵入性容器一样，列出
    要求首先从list<>：：node派生类：

    @代码

    结构对象：list<object>：：node
    {
        显式对象（int值）：m_值（值）
        {
        }

        int值；
    }；

    @端码

    现在我们定义列表，并添加一些项。

    @代码

    list<object>list；

    list.push_back（*（新对象（1））；
    list.push_back（*（新对象（2））；

    @端码

    为了与标准容器兼容，push_back（）需要
    对对象的引用。但是，与标准容器不同的是，将_向后推（）。
    将实际对象放在列表中，而不是复制构造的副本。

    对列表的迭代遵循与STL相同的习惯用法：

    @代码

    for（list<object>：：迭代器iter=list.begin（）；iter！=list.end；+iter）
        std：：cout<<iter->m_value；

    @端码

    您甚至可以使用Boost-ForEach，或基于范围的循环：

    @代码

    boost_foreach（object&object，list）//仅限boost
        std：：cout<<object.m_value；

    仅用于（对象和对象：列表）//c++ 11
        std：：cout<<object.m_value；

    @端码

    因为列表主要是符合STL的，所以它可以传递到STL算法中：
    例如，“std:：foreach（）”或“std:：find_first_of（）”。

    通常，放置在列表中的对象应该动态分配
    尽管这在编译时不能强制执行。因为呼叫者提供
    对象的存储，调用方还负责删除
    对象。对象从列表中删除后仍存在，直到
    呼叫者删除它。这意味着元素可以从一个列表移动到
    另一个几乎没有开销。

    与标准容器不同，对象只能存在于
    时间，除非有特别的准备。标记模板参数为
    用于区分同一对象的不同列表类型，
    允许对象同时存在于多个列表中。

    例如，考虑一个参与者系统，其中的全局参与者列表是
    维护，以便它们可以定期接收处理
    时间。我们还希望维护一个需要
    与域相关的更新。为了实现这一点，我们声明了两个标签，
    关联的列表类型和列表元素：

    @代码

    struct actor；//需要forward声明

    结构流程标记
    结构更新标记

    使用processlist=list<actor，processtag>；
    使用updateList=list<actor，updateTag>；

    //从两个节点类型派生，这样我们就可以同时出现在每个列表中。
    / /
    结构参与者：processList:：node、updateList:：node
    {
        bool process（）；//如果需要更新，则返回true
        无效更新（）；
    }；

    @端码

    @tparam t列表将存储的元素的基类型
                    指向…的指针

    @tparam tag用于区分列表和节点的可选唯一类型名，
                当对象可以同时存在于多个列表中时。

    @Ingroup Beast_核心侵入式
**/

template <typename T, typename Tag = void>
class List
{
public:
    using Node = typename detail::ListNode <T, Tag>;

    using value_type      = T;
    using pointer         = value_type*;
    using reference       = value_type&;
    using const_pointer   = value_type const*;
    using const_reference = value_type const&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator       = detail::ListIterator <Node>;
    using const_iterator = detail::ListIterator <Node const>;

    /*创建空列表。*/
    List ()
    {
m_head.m_prev = nullptr; //识别头部
m_tail.m_next = nullptr; //识别尾部
        clear ();
    }

    List(List const&) = delete;
    List& operator= (List const&) = delete;

    /*确定列表是否为空。
        @如果列表为空，返回'true'。
    **/

    bool empty () const noexcept
    {
        return size () == 0;
    }

    /*返回列表中的元素数。*/
    size_type size () const noexcept
    {
        return m_size;
    }

    /*获取对第一个元素的引用。
        @invariant列表不能为空。
        @返回对第一个元素的引用。
    **/

    reference front () noexcept
    {
        return element_from (m_head.m_next);
    }

    /*获取对第一个元素的常量引用。
        @invariant列表不能为空。
        @返回对第一个元素的常量引用。
    **/

    const_reference front () const noexcept
    {
        return element_from (m_head.m_next);
    }

    /*获取对最后一个元素的引用。
        @invariant列表不能为空。
        @返回对最后一个元素的引用。
    **/

    reference back () noexcept
    {
        return element_from (m_tail.m_prev);
    }

    /*获取最后一个元素的常量引用。
        @invariant列表不能为空。
        @返回对最后一个元素的常量引用。
    **/

    const_reference back () const noexcept
    {
        return element_from (m_tail.m_prev);
    }

    /*获取列表开头的迭代器。
        @返回指向列表开头的迭代器。
    **/

    iterator begin () noexcept
    {
        return iterator (m_head.m_next);
    }

    /*获取列表开头的常量迭代器。
        @返回指向列表开头的常量迭代器。
    **/

    const_iterator begin () const noexcept
    {
        return const_iterator (m_head.m_next);
    }

    /*获取列表开头的常量迭代器。
        @返回指向列表开头的常量迭代器。
    **/

    const_iterator cbegin () const noexcept
    {
        return const_iterator (m_head.m_next);
    }

    /*获取列表末尾的迭代器。
        @返回指向列表结尾的迭代器。
    **/

    iterator end () noexcept
    {
        return iterator (&m_tail);
    }

    /*获取列表末尾的常量迭代器。
        @返回一个指向列表末尾的常量。
    **/

    const_iterator end () const noexcept
    {
        return const_iterator (&m_tail);
    }

    /*获取列表末尾的常量迭代器
        @返回一个指向列表末尾的常量。
    **/

    const_iterator cend () const noexcept
    {
        return const_iterator (&m_tail);
    }

    /*清除清单。
        @注意，这不会释放元素。
    **/

    void clear () noexcept
    {
        m_head.m_next = &m_tail;
        m_tail.m_prev = &m_head;
        m_size = 0;
    }

    /*插入元素。
        @invariant元素不能已经在列表中。
        @param pos在后面插入的位置。
        @param element要插入的元素。
        @返回指向新插入元素的迭代器。
    **/

    iterator insert (iterator pos, T& element) noexcept
    {
        Node* node = static_cast <Node*> (&element);
        node->m_next = &*pos;
        node->m_prev = node->m_next->m_prev;
        node->m_next->m_prev = node;
        node->m_prev->m_next = node;
        ++m_size;
        return iterator (node);
    }

    /*在此列表中插入另一个列表。
        另一个列表被清除。
        @param pos在后面插入的位置。
        @param other要插入的列表。
    **/

    void insert (iterator pos, List& other) noexcept
    {
        if (!other.empty ())
        {
            Node* before = &*pos;
            other.m_head.m_next->m_prev = before->m_prev;
            before->m_prev->m_next = other.m_head.m_next;
            other.m_tail.m_prev->m_next = before;
            before->m_prev = other.m_tail.m_prev;
            m_size += other.m_size;
            other.clear ();
        }
    }

    /*拆下一个元件。
        @invariant元素必须存在于列表中。
        @param pos指向要删除元素的迭代器。
        @返回一个迭代器，该迭代器指向删除后的下一个元素。
    **/

    iterator erase (iterator pos) noexcept
    {
        Node* node = &*pos;
        ++pos;
        node->m_next->m_prev = node->m_prev;
        node->m_prev->m_next = node->m_next;
        --m_size;
        return pos;
    }

    /*在列表的开头插入元素。
        @invariant元素不能存在于列表中。
        @param element要插入的元素。
    **/

    iterator push_front (T& element) noexcept
    {
        return insert (begin (), element);
    }

    /*删除列表开头的元素。
        @invariant列表不能为空。
        @返回对弹出元素的引用。
    **/

    T& pop_front () noexcept
    {
        T& element (front ());
        erase (begin ());
        return element;
    }

    /*在列表末尾附加一个元素。
        @invariant元素不能存在于列表中。
        @param element要追加的元素。
    **/

    iterator push_back (T& element) noexcept
    {
        return insert (end (), element);
    }

    /*删除列表末尾的元素。
        @invariant列表不能为空。
        @返回对弹出元素的引用。
    **/

    T& pop_back () noexcept
    {
        T& element (back ());
        erase (--end ());
        return element;
    }

    /*与其他列表交换内容。*/
    void swap (List& other) noexcept
    {
        List temp;
        temp.append (other);
        other.append (*this);
        append (temp);
    }

    /*在此列表的开头插入另一个列表。
        另一个列表被清除。
        @param列出要插入的其他列表。
    **/

    iterator prepend (List& list) noexcept
    {
        return insert (begin (), list);
    }

    /*在此列表末尾附加另一个列表。
        另一个列表被清除。
        @param列出要附加的其他列表。
    **/

    iterator append (List& list) noexcept
    {
        return insert (end (), list);
    }

    /*从元素获取迭代器。
        @invariant元素必须存在于列表中。
        @param element要获取迭代器的元素。
        @返回元素的迭代器。
    **/

    iterator iterator_to (T& element) const noexcept
    {
        return iterator (static_cast <Node*> (&element));
    }

    /*从元素获取常量迭代器。
        @invariant元素必须存在于列表中。
        @param element要获取迭代器的元素。
        @向元素返回常量迭代器。
    **/

    const_iterator const_iterator_to (T const& element) const noexcept
    {
        return const_iterator (static_cast <Node const*> (&element));
    }

private:
    reference element_from (Node* node) noexcept
    {
        return *(static_cast <pointer> (node));
    }

    const_reference element_from (Node const* node) const noexcept
    {
        return *(static_cast <const_pointer> (node));
    }

private:
    size_type m_size;
    Node m_head;
    Node m_tail;
};

}

#endif
