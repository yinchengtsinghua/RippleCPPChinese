
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

#ifndef BEAST_INTRUSIVE_LOCKFREESTACK_H_INCLUDED
#define BEAST_INTRUSIVE_LOCKFREESTACK_H_INCLUDED

#include <atomic>
#include <iterator>
#include <type_traits>

namespace beast {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Container, bool IsConst>
class LockFreeStackIterator
    : public std::iterator <
        std::forward_iterator_tag,
        typename Container::value_type,
        typename Container::difference_type,
        typename std::conditional <IsConst,
            typename Container::const_pointer,
            typename Container::pointer>::type,
        typename std::conditional <IsConst,
            typename Container::const_reference,
            typename Container::reference>::type>
{
protected:
    using Node = typename Container::Node;
    using NodePtr = typename std::conditional <
        IsConst, Node const*, Node*>::type;

public:
    using value_type = typename Container::value_type;
    using pointer = typename std::conditional <IsConst,
        typename Container::const_pointer,
        typename Container::pointer>::type;
    using reference = typename std::conditional <IsConst,
        typename Container::const_reference,
        typename Container::reference>::type;

    LockFreeStackIterator ()
        : m_node ()
    {
    }

    LockFreeStackIterator (NodePtr node)
        : m_node (node)
    {
    }

    template <bool OtherIsConst>
    explicit LockFreeStackIterator (LockFreeStackIterator <Container, OtherIsConst> const& other)
        : m_node (other.m_node)
    {
    }

    LockFreeStackIterator& operator= (NodePtr node)
    {
        m_node = node;
        return static_cast <LockFreeStackIterator&> (*this);
    }

    LockFreeStackIterator& operator++ ()
    {
        m_node = m_node->m_next.load ();
        return static_cast <LockFreeStackIterator&> (*this);
    }

    LockFreeStackIterator operator++ (int)
    {
        LockFreeStackIterator result (*this);
        m_node = m_node->m_next;
        return result;
    }

    NodePtr node() const
    {
        return m_node;
    }

    reference operator* () const
    {
        return *this->operator-> ();
    }

    pointer operator-> () const
    {
        return static_cast <pointer> (m_node);
    }

private:
    NodePtr m_node;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Container, bool LhsIsConst, bool RhsIsConst>
bool operator== (LockFreeStackIterator <Container, LhsIsConst> const& lhs,
                 LockFreeStackIterator <Container, RhsIsConst> const& rhs)
{
    return lhs.node() == rhs.node();
}

template <class Container, bool LhsIsConst, bool RhsIsConst>
bool operator!= (LockFreeStackIterator <Container, LhsIsConst> const& lhs,
                 LockFreeStackIterator <Container, RhsIsConst> const& rhs)
{
    return lhs.node() != rhs.node();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*多生产者、多消费者（MPMC）侵入式堆栈。

    这个堆栈是使用与list相同的侵入式接口实现的。
    所有的突变都是无锁的。

    呼叫方负责防止“ABA”问题：
        http://en.wikipedia.org/wiki/aba_问题

    @param tag用于区分列表和节点的类型名，用于
                将对象放入多个列表中。如果此参数是
                省略，将使用默认标记。
**/

template <class Element, class Tag = void>
class LockFreeStack
{
public:
    class Node
    {
    public:
        Node ()
            : m_next (nullptr)
        { }

        explicit Node (Node* next)
            : m_next (next)
        { }

        Node(Node const&) = delete;
        Node& operator= (Node const&) = delete;

    private:
        friend class LockFreeStack;

        template <class Container, bool IsConst>
        friend class LockFreeStackIterator;

        std::atomic <Node*> m_next;
    };

public:
    using value_type      = Element;
    using pointer         = Element*;
    using reference       = Element&;
    using const_pointer   = Element const*;
    using const_reference = Element const&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator        = LockFreeStackIterator <
        LockFreeStack <Element, Tag>, false>;
    using const_iterator  = LockFreeStackIterator <
        LockFreeStack <Element, Tag>, true>;

    LockFreeStack ()
        : m_end (nullptr)
        , m_head (&m_end)
    {
    }

    LockFreeStack(LockFreeStack const&) = delete;
    LockFreeStack& operator= (LockFreeStack const&) = delete;

    /*如果堆栈为空，则返回true。*/
    bool empty() const
    {
        return m_head.load () == &m_end;
    }

    /*将节点推到堆栈上。
        呼叫方负责防止ABA问题。
        此操作无锁。
        线程安全：
            从任何线程调用都是安全的。

        @param node要推的节点。

        @return`true`如果堆栈以前是空的。如果多线程
                正在尝试推送，只有一个将收到“true”。
    **/

//vvalco note修复了这个问题，它不应该是一个类似于侵入列表的引用吗？
    bool push_front (Node* node)
    {
        bool first;
        Node* old_head = m_head.load (std::memory_order_relaxed);
        do
        {
            first = (old_head == &m_end);
            node->m_next = old_head;
        }
        while (!m_head.compare_exchange_strong (old_head, node,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        return first;
    }

    /*从堆栈中弹出一个元素。
        呼叫方负责防止ABA问题。
        此操作无锁。
        线程安全：
            从任何线程调用都是安全的。

        @返回弹出的元素，如果堆栈
                是空的。
    **/

    Element* pop_front ()
    {
        Node* node = m_head.load ();
        Node* new_head;
        do
        {
            if (node == &m_end)
                return nullptr;
            new_head = node->m_next.load ();
        }
        while (!m_head.compare_exchange_strong (node, new_head,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
        return static_cast <Element*> (node);
    }

    /*将前向迭代器返回到堆栈的开头或结尾。
        如果调用push-front或pop-front，将导致未定义的行为结果
        当迭代正在进行时。
        线程安全：
            调用者负责同步。
    **/

    /*@ {*/
    iterator begin ()
    {
        return iterator (m_head.load ());
    }

    iterator end ()
    {
        return iterator (&m_end);
    }

    const_iterator begin () const
    {
        return const_iterator (m_head.load ());
    }

    const_iterator end () const
    {
        return const_iterator (&m_end);
    }

    const_iterator cbegin () const
    {
        return const_iterator (m_head.load ());
    }

    const_iterator cend () const
    {
        return const_iterator (&m_end);
    }
    /*@ }*/

private:
    Node m_end;
    std::atomic <Node*> m_head;
};

}

#endif
