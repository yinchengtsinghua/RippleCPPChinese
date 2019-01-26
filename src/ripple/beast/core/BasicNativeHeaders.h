
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

#ifndef BEAST_MODULE_CORE_NATIVE_BASICNATIVEHEADERS_H_INCLUDED
#define BEAST_MODULE_CORE_NATIVE_BASICNATIVEHEADERS_H_INCLUDED

#include <ripple/beast/core/Config.h>

#undef T

//==============================================================
#if BEAST_MAC || BEAST_IOS

 #if BEAST_IOS
  #ifdef __OBJC__
    #import <Foundation/Foundation.h>
    #import <UIKit/UIKit.h>
    #import <CoreData/CoreData.h>
    #import <MobileCoreServices/MobileCoreServices.h>
  #endif
  #include <sys/fcntl.h>
 #else
  #ifdef __OBJC__
    #define Point CarbonDummyPointName
    #define Component CarbonDummyCompName
    #import <Cocoa/Cocoa.h>
    #import <CoreAudio/HostTime.h>
    #undef Point
    #undef Component
  #endif
  #include <sys/dir.h>
 #endif

 #include <sys/socket.h>
 #include <sys/sysctl.h>
 #include <sys/stat.h>
 #include <sys/param.h>
 #include <sys/mount.h>
 #include <sys/utsname.h>
 #include <sys/mman.h>
 #include <fnmatch.h>
 #include <utime.h>
 #include <dlfcn.h>
 #include <ifaddrs.h>
 #include <net/if_dl.h>
 #include <mach/mach_time.h>
 #include <mach-o/dyld.h>
 #include <objc/runtime.h>
 #include <objc/objc.h>
 #include <objc/message.h>

//==============================================================
#elif BEAST_WINDOWS
 #if BEAST_MSVC
  #ifndef _CPPRTTI
   #error "Beast requires RTTI!"
  #endif

  #ifndef _CPPUNWIND
   #error "Beast requires RTTI!"
  #endif

  #pragma warning (push)
  #pragma warning (disable : 4100 4201 4514 4312 4995)
 #endif

 #define STRICT 1
 #define WIN32_LEAN_AND_MEAN 1
 #ifndef _WIN32_WINNT
  #if BEAST_MINGW
   #define _WIN32_WINNT 0x0501
  #else
   #define _WIN32_WINNT 0x0600
  #endif
 #endif
 #define _UNICODE 1
 #define UNICODE 1
 #ifndef _WIN32_IE
  #define _WIN32_IE 0x0400
 #endif

 #include <windows.h>
 #include <shellapi.h>
 #include <tchar.h>
 #include <stddef.h>
 #include <ctime>
 #include <wininet.h>
 #include <nb30.h>
 #include <iphlpapi.h>
 #include <mapi.h>
 #include <float.h>
 #include <process.h>
 #pragma warning ( push )
 #pragma warning ( disable: 4091 )
 #include <shlobj.h>
 #pragma warning ( pop )
 #include <shlwapi.h>
 #include <mmsystem.h>

 #if BEAST_MINGW
  #include <basetyps.h>
 #else
  #include <crtdbg.h>
  #include <comutil.h>
 #endif

 #undef PACKED

 #if BEAST_MSVC
  #pragma warning (pop)
  /*AGMA警告（4:4511 4512 4100/*4365*/）/（启用VC8中关闭的某些警告）
 第二节

 如果是野兽Beast不自动链接到win32库
  pragma comment（lib，“kernel32.lib”）。
  pragma comment（lib，“user32.lib”）。
  pragma comment（lib，“wininet.lib”）。
  pragma comment（lib，“advapi32.lib”）。
  pragma comment（lib，“ws2_32.lib”）。
  pragma comment（lib，“version.lib”）。
  pragma comment（lib，“shlwapi.lib”）。
  pragma comment（lib，“winmm.lib”）。

  定义
   μIFDEF调试
    pragma comment（lib，“comsuppwd.lib”）。
   否则
    pragma comment（lib，“comsupp.lib”）。
   第二节
  否则
   μIFDEF调试
    pragma comment（lib，“comsuppd.lib”）。
   否则
    pragma comment（lib，“comsupp.lib”）。
   第二节
  第二节
 第二节

//==============================================================
elif beast_linux_beast_bsd
 include<sched.h>
 include<pthread.h>
 包括<sys/time.h>
 include<errno.h>
 包括<sys/stat.h>
 include<sys/ptrace.h>
 include<sys/wait.h>
 include<sys/mman.h>
 include<fnmatch.h>
 包括<utime.h>
 include<pwd.h>
 include<fcntl.h>
 include<dlfcn.h>
 include<netdb.h>
 包括<arpa/inet.h>
 include<netinet/in.h>
 包括<sys/types.h>
 include<sys/ioctl.h>
 include<sys/socket.h>
 include<net/if.h>
 include<sys/file.h>
 include<signal.h>
 include<stddef.h>

 如果BASASTH-BSD
  include<dirent.h>
  包括<ifaddrs.h>
  include<net/if_dl.h>
  包括<kvm.h>
  include<langinfo.h>
  include<sys/cdefs.h>
  include<sys/param.h>
  包括<sys/mount.h>
  包括<sys/types.h>
  include<sys/sysctl.h>

  //必须在全局命名空间中
  外部字符**环境；

 否则
  include<sys/dir.h>
  包括<sys/vfs.h>
  include<sys/sysinfo.h>
  include<sys/prctl.h>
 第二节

//==============================================================
伊利夫野兽机器人
 包括<jni.h>
 include<pthread.h>
 include<sched.h>
 包括<sys/time.h>
 包括<utime.h>
 include<errno.h>
 include<fcntl.h>
 include<dlfcn.h>
 包括<sys/stat.h>
 include<sys/statfs.h>
 include<sys/ptrace.h>
 include<sys/sysinfo.h>
 include<sys/mman.h>
 include<pwd.h>
 include<dirent.h>
 include<fnmatch.h>
 include<sys/wait.h>
第二节

//需要清除系统头文件所做的各种复杂重新定义。
最大值
min
直接指令
检验检验

//订单很重要，因为头没有自己的include行。
//在底部添加新的include。

include<ripple/beast/core/lexicalcast.h>
include<ripple/beast/core/semanticversion.h>

include<locale>
include<cctype>

如果有的话！比斯塔BSD
 include<sys/timeb.h>
第二节

如果有的话！百思达安卓
 include<cwtype>
第二节

如果是野兽窗口
 包括<ctime>
 include<winsock2.h>
 include<ws2tcpip.h>

 如果有的话！比斯塔明
  pragma警告（push）
  杂注警告（禁用：4091）
  include<dbghelp.h>
  pragma警告（pop）

  如果有的话！Beast不自动链接到win32库
   pragma comment（lib，“dbghelp.lib”）。
  第二节
 第二节

 如果是
  include<ws2spi.h>
 第二节

否则
 如果野兽Linux野兽Android
  包括<sys/types.h>
  include<sys/socket.h>
  include<unistd.h>
  include<netinet/in.h>
 第二节

 如果BESASTLinux
  include<langinfo.h>
 第二节

 include<pwd.h>
 include<fcntl.h>
 include<netdb.h>
 包括<arpa/inet.h>
 include<netnet/tcp.h>
 包括<sys/time.h>
 include<net/if.h>
 include<sys/ioctl.h>
第二节

如果野兽Mac野兽iOS
 include<xlocale.h>
 包括<mach/mach.h>
第二节

如果野兽是安卓
 include<android/log.h>
第二节

endif//包含Beast_basicNativeHeaders_h_
