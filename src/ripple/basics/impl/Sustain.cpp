
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

#include <ripple/basics/safe_cast.h>
#include <ripple/basics/Sustain.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <boost/format.hpp>

//用于支持Linux变体
//vfalc todo重写sustain以使用beast:：process
#ifdef __linux__
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace ripple {

#ifdef __unix__

static auto const sleepBeforeWaiting = 10;
static auto const sleepBetweenWaits = 1;

static pid_t pManager = safe_cast<pid_t> (0);
static pid_t pChild = safe_cast<pid_t> (0);

static void pass_signal (int a)
{
    kill (pChild, a);
}

static void stop_manager (int)
{
    kill (pChild, SIGINT);
    _exit (0);
}

bool HaveSustain ()
{
    return true;
}

std::string StopSustain ()
{
    if (getppid () != pManager)
        return "";

    kill (pManager, SIGHUP);
    return "Terminating monitor";
}

static
bool checkChild(pid_t pid, int options)
{
    int i;

    if (waitpid (pChild, &i, options) == -1)
        return false;

    return kill (pChild, 0) == 0;
}

std::string DoSustain ()
{
    pManager = getpid ();
    signal (SIGINT, stop_manager);
    signal (SIGHUP, stop_manager);
    signal (SIGUSR1, pass_signal);
    signal (SIGUSR2, pass_signal);

//孩子退出的次数少于
//15秒。
    int fastExit = 0;

    for (auto childCount = 1; ; ++childCount)
    {
        pChild = fork ();

        if (pChild == -1)
            _exit (0);

        auto cc = std::to_string (childCount);
        if (pChild == 0)
        {
            beast::setCurrentThreadName ("rippled: main");
            signal (SIGINT, SIG_DFL);
            signal (SIGHUP, SIG_DFL);
            signal (SIGUSR1, SIG_DFL);
            signal (SIGUSR2, SIG_DFL);
            return "Launching child " + cc;
        }

        beast::setCurrentThreadName (("rippled: #" + cc).c_str());

        sleep (sleepBeforeWaiting);

//如果孩子已经终止，则将此计数
//作为一个快速的出口和一个指示
//出了差错：
        if (!checkChild (pChild, WNOHANG))
        {
            if (++fastExit == 5)
                _exit (0);
        }
        else
        {
            fastExit = 0;

            while (checkChild (pChild, 0))
                sleep(sleepBetweenWaits);

            (void)rename ("core",
                ("core." + std::to_string(pChild)).c_str());
        }
    }
}

#else

bool HaveSustain ()
{
    return false;
}

std::string DoSustain ()
{
    return "";
}

std::string StopSustain ()
{
    return "";
}

#endif

} //涟漪
