
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

#ifndef BEAST_CONFIG_STANDARDCONFIG_H_INCLUDED
#define BEAST_CONFIG_STANDARDCONFIG_H_INCLUDED

//现在我们将包括一些常见的操作系统头。
#if BEAST_MSVC
#pragma warning (push)
#pragma warning (disable: 4514 4245 4100)
#endif

#if BEAST_USE_INTRINSICS
#include <intrin.h>
#endif

#if BEAST_MAC || BEAST_IOS
#include <libkern/OSAtomic.h>
#endif

#if BEAST_LINUX
#include <signal.h>
# if __INTEL_COMPILER
#  if __ia64__
#include <ia64intrin.h>
#  else
#include <ia32intrin.h>
#  endif
# endif
#endif

#if BEAST_MSVC && BEAST_DEBUG
#include <crtdbg.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#if BEAST_MSVC
#pragma warning (pop)
#endif

#if BEAST_ANDROID
 #include <sys/atomics.h>
 #include <byteswap.h>
#endif

//有时由错误引导的第三方标题设置的UNdef符号。
#undef check
#undef TYPE_BOOL
#undef max
#undef min

#endif
