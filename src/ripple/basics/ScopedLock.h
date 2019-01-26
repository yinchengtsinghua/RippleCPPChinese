
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

    这个文件的一部分来自JUCE。
    版权所有（c）2013-原材料软件有限公司
    请访问http://www.juce.com

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

#ifndef RIPPLE_BASICS_SCOPEDLOCK_H_INCLUDED
#define RIPPLE_BASICS_SCOPEDLOCK_H_INCLUDED

namespace ripple
{

//==============================================================
/*
    自动解锁并重新锁定互斥对象。

    这与std：：lock_guard对象相反-而不是锁定互斥对象
    在这个对象的生命周期中，它会解锁它。

    确保不要尝试解锁实际上未锁定的互斥体！

    例如@代码

    STD：互斥MUT；

    （；）
    {
        std:：lock_guard<std:：mutex>myscopedlock_mut_
        //MUT现在被锁定

        …把它锁上做些事情……

        而（XYZ）
        {
            …把它锁上做些事情……

            GenericScopedUnlock<std:：mutex>解锁器mut

            //该块其余部分的mut现在已解锁，
            //最后重新锁定。

            …打开它做一些事情…
        //mut在这里被锁定。

    //mut在这里解锁
    @端码

**/

template <class MutexType>
class GenericScopedUnlock
{
    MutexType& lock_;
public:
    /*创建GenericScopedUnlock。

        一旦它被创建，这将解锁关键部分，并且
        删除Scopedlock对象时，CriticalSection将
        重新锁定。

        确保此对象由同一线程创建和删除，
        否则就无法保证会发生什么！最好只是使用它
        作为本地堆栈对象，而不是使用new（）运算符创建一个。
    **/

    explicit GenericScopedUnlock (MutexType& lock) noexcept
        : lock_ (lock)
    {
        lock.unlock();
    }

    GenericScopedUnlock (GenericScopedUnlock const&) = delete;
    GenericScopedUnlock& operator= (GenericScopedUnlock const&) = delete;

    /*Destructor。

        调用析构函数时，CriticalSection将被解锁。

        确保此对象由同一线程创建和删除，
        否则就无法保证会发生什么！
    **/

    ~GenericScopedUnlock() noexcept(false)
    {
        lock_.lock();
    }
};

} //涟漪
#endif

