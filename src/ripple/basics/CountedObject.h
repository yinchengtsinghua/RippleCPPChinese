
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

#ifndef RIPPLE_BASICS_COUNTEDOBJECT_H_INCLUDED
#define RIPPLE_BASICS_COUNTEDOBJECT_H_INCLUDED

#include <atomic>
#include <string>
#include <utility>
#include <vector>

namespace ripple {

/*管理所有计数的对象类型。*/
class CountedObjects
{
public:
    static CountedObjects& getInstance () noexcept;

    using Entry = std::pair <std::string, int>;
    using List = std::vector <Entry>;

    List getCounts (int minimumThreshold) const;

public:
    /*@ref countedObject的实现。

        @内部
    **/

    class CounterBase
    {
    public:
        CounterBase () noexcept;

        virtual ~CounterBase () noexcept;

        int increment () noexcept
        {
            return ++m_count;
        }

        int decrement () noexcept
        {
            return --m_count;
        }

        int getCount () const noexcept
        {
            return m_count.load ();
        }

        CounterBase* getNext () const noexcept
        {
            return m_next;
        }

        virtual char const* getName () const = 0;

    private:
        virtual void checkPureVirtual () const = 0;

    protected:
        std::atomic <int> m_count;
        CounterBase* m_next;
    };

private:
    CountedObjects () noexcept;
    ~CountedObjects () noexcept = default;

private:
    std::atomic <int> m_count;
    std::atomic <CounterBase*> m_head;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*跟踪对象的实例数。

    派生类自动计算其实例。这是用的
    用于报告目的。

    @InGroup Ripple_基础
**/

template <class Object>
class CountedObject
{
public:
    CountedObject () noexcept
    {
        getCounter ().increment ();
    }

    CountedObject (CountedObject const&) noexcept
    {
        getCounter ().increment ();
    }

    CountedObject& operator=(CountedObject const&) noexcept = default;

    ~CountedObject () noexcept
    {
        getCounter ().decrement ();
    }

private:
    class Counter : public CountedObjects::CounterBase
    {
    public:
        Counter () noexcept { }

        char const* getName () const override
        {
            return Object::getCountedObjectName ();
        }

        void checkPureVirtual () const override { }
    };

private:
    static Counter& getCounter() noexcept
    {
        static_assert(std::is_nothrow_constructible<Counter>{}, "");
        static Counter c;
        return c;
    }
};

} //涟漪

#endif
