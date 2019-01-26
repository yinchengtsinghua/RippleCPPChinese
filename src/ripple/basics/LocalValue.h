
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

#ifndef RIPPLE_BASICS_LOCALVALUE_H_INCLUDED
#define RIPPLE_BASICS_LOCALVALUE_H_INCLUDED

#include <boost/thread/tss.hpp>
#include <memory>
#include <unordered_map>

namespace ripple {

namespace detail {

struct LocalValues
{
    explicit LocalValues() = default;

    bool onCoro = true;

    struct BasicValue
    {
        virtual ~BasicValue() = default;
        virtual void* get() = 0;
    };

    template <class T>
    struct Value : BasicValue
    {
        T t_;

        Value() = default;
        explicit Value(T const& t) : t_(t) {}

        void* get() override
        {
            return &t_;
        }
    };

//键是本地值的地址。
    std::unordered_map<void const*, std::unique_ptr<BasicValue>> values;

    static
    inline
    void
    cleanup(LocalValues* lvs)
    {
        if (lvs && ! lvs->onCoro)
            delete lvs;
    }
};

template<class = void>
boost::thread_specific_ptr<detail::LocalValues>&
getLocalValues()
{
    static boost::thread_specific_ptr<
        detail::LocalValues> tsp(&detail::LocalValues::cleanup);
    return tsp;
}

} //细节

template <class T>
class LocalValue
{
public:
    template <class... Args>
    LocalValue(Args&&... args)
        : t_(std::forward<Args>(args)...)
    {
    }

    /*存储特定于调用协同程序或线程的T实例。*/
    T& operator*();

    /*存储特定于调用协同程序或线程的T实例。*/
    T* operator->()
    {
        return &**this;
    }

private:
    T t_;
};

template <class T>
T&
LocalValue<T>::operator*()
{
    auto lvs = detail::getLocalValues().get();
    if (! lvs)
    {
        lvs = new detail::LocalValues();
        lvs->onCoro = false;
        detail::getLocalValues().reset(lvs);
    }
    else
    {
        auto const iter = lvs->values.find(this);
        if (iter != lvs->values.end())
            return *reinterpret_cast<T*>(iter->second->get());
    }

    return *reinterpret_cast<T*>(lvs->values.emplace(this,
        std::make_unique<detail::LocalValues::Value<T>>(t_)).
            first->second->get());
}
} //涟漪

#endif
