
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

#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/beast/core/Config.h>
#include <boost/thread/tss.hpp>
#include <ripple/beast/core/BasicNativeHeaders.h>
#include <ripple/beast/core/StandardIncludes.h>

namespace beast {
namespace detail {

static boost::thread_specific_ptr<std::string> threadName;

void saveThreadName (std::string name)
{
    threadName.reset (new std::string {std::move(name)});
}

} //细节

boost::optional<std::string> getCurrentThreadName ()
{
    if (auto r = detail::threadName.get())
        return *r;
    return boost::none;
}

} //野兽

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#if BEAST_WINDOWS

#include <windows.h>
#include <process.h>
#include <tchar.h>

namespace beast {
namespace detail {

void setCurrentThreadNameImpl (std::string const& name)
{
   #if BEAST_DEBUG && BEAST_MSVC
    struct
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    } info;

    info.dwType = 0x1000;
    info.szName = name.c_str ();
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try
    {
        /*请参见异常（0x406D1388/*ms_vc_exception*/，0，sizeof（info）/sizeof（ulong_ptr），（ulong_ptr*）和info）；
    }
    _uuu except（异常\继续执行）
    {}
   否则
    （无效）名称；
   第二节
}

} / /细节
} /兽

埃尔夫·贝斯塔姆

include<pthread.h>

命名空间Beast
命名空间详细信息

void setcurrenthreadnameimpl（std:：string const&name）
{
    pthread_setname_np（name.c_str（））；
}

} / /细节
} /兽

else//Beast覕Linux

include<pthread.h>

命名空间Beast
命名空间详细信息

void setcurrenthreadnameimpl（std:：string const&name）
{
    pthread_setname_np（pthread_self（），name.c_str（））；
}

} / /细节
} /兽

endif//beast_Linux

命名空间Beast

void setCurrentThreadName（标准：：字符串名称）
{
    详细信息：setCurrentThreadNameImpl（name）；
    详细信息：：savethreadname（std:：move（name））；
}

} /兽
