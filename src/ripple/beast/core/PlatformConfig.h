
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

#ifndef BEAST_CONFIG_PLATFORMCONFIG_H_INCLUDED
#define BEAST_CONFIG_PLATFORMCONFIG_H_INCLUDED

//==============================================================
/*这个文件找出正在构建的平台，并定义了一些宏
    其他代码可以用于特定于操作系统的编译。

    将在此处设置的宏有：

    -Beast_Windows、Beast_Mac Beast_Linux、Beast_iOS、Beast_Android等之一。
    -根据体系结构，可以是Beast32bit或Beast64bit。
    -无论是小野兽还是大野兽。
    -Beast_Intel或Beast_PPC
    -Beast_GCC或Beast_MSVC
**/


//==============================================================
#if (defined (_WIN32) || defined (_WIN64))
  #define       BEAST_WIN32 1
  #define       BEAST_WINDOWS 1
#elif defined (BEAST_ANDROID)
  #undef        BEAST_ANDROID
  #define       BEAST_ANDROID 1
#elif defined (LINUX) || defined (__linux__)
  #define     BEAST_LINUX 1
#elif defined (__APPLE_CPP__) || defined(__APPLE_CC__)
#define Point CarbonDummyPointName //（解决方法是避免旧的碳集管定义“点”）。
  #define Component CarbonDummyCompName
#include <CoreFoundation/CoreFoundation.h> //（需要了解我们使用的平台）
  #undef Point
  #undef Component

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #define     BEAST_IPHONE 1
    #define     BEAST_IOS 1
  #else
    #define     BEAST_MAC 1
  #endif
#elif defined (__FreeBSD__)
  #define BEAST_BSD 1
#else
  #error "Unknown platform!"
#endif

//==============================================================
#if BEAST_WINDOWS
  #ifdef _MSC_VER
    #ifdef _WIN64
      #define BEAST_64BIT 1
    #else
      #define BEAST_32BIT 1
    #endif
  #endif

  #ifdef _DEBUG
    #define BEAST_DEBUG 1
  #endif

  #ifdef __MINGW32__
    #define BEAST_MINGW 1
    #ifdef __MINGW64__
      #define BEAST_64BIT 1
    #else
      #define BEAST_32BIT 1
    #endif
  #endif

  /*如果定义了，这表示处理器是小endian。*/
  #define BEAST_LITTLE_ENDIAN 1

  #define BEAST_INTEL 1
#endif

//==============================================================
#if BEAST_MAC || BEAST_IOS

  #if defined (DEBUG) || defined (_DEBUG) || ! (defined (NDEBUG) || defined (_NDEBUG))
    #define BEAST_DEBUG 1
  #endif

  #ifdef __LITTLE_ENDIAN__
    #define BEAST_LITTLE_ENDIAN 1
  #else
    #define BEAST_BIG_ENDIAN 1
  #endif
#endif

#if BEAST_MAC

  #if defined (__ppc__) || defined (__ppc64__)
    #define BEAST_PPC 1
  #elif defined (__arm__)
    #define BEAST_ARM 1
  #else
    #define BEAST_INTEL 1
  #endif

  #ifdef __LP64__
    #define BEAST_64BIT 1
  #else
    #define BEAST_32BIT 1
  #endif

  #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    #error "Building for OSX 10.3 is no longer supported!"
  #endif

  #ifndef MAC_OS_X_VERSION_10_5
    #error "To build with 10.4 compatibility, use a 10.5 or 10.6 SDK and set the deployment target to 10.4"
  #endif

#endif

//==============================================================
#if BEAST_LINUX || BEAST_ANDROID || BEAST_BSD

  #ifdef _DEBUG
    #define BEAST_DEBUG 1
  #endif

//允许覆盖big endian Linux平台
  #if defined (__LITTLE_ENDIAN__) || ! defined (BEAST_BIG_ENDIAN)
    #define BEAST_LITTLE_ENDIAN 1
    #undef BEAST_BIG_ENDIAN
  #else
    #undef BEAST_LITTLE_ENDIAN
    #define BEAST_BIG_ENDIAN 1
  #endif

  #if defined (__LP64__) || defined (_LP64)
    #define BEAST_64BIT 1
  #else
    #define BEAST_32BIT 1
  #endif

  #if __MMX__ || __SSE__ || __amd64__
    #ifdef __arm__
      #define BEAST_ARM 1
    #else
      #define BEAST_INTEL 1
    #endif
  #endif
#endif

//==============================================================
//编译器类型宏。

#ifdef __clang__
 #define BEAST_CLANG 1
#elif defined (__GNUC__)
  #define BEAST_GCC 1
#elif defined (_MSC_VER)
  #define BEAST_MSVC 1

  #if _MSC_VER < 1500
    #define BEAST_VC8_OR_EARLIER 1

    #if _MSC_VER < 1400
      #define BEAST_VC7_OR_EARLIER 1

      #if _MSC_VER < 1300
        #warning "MSVC 6.0 is no longer supported!"
      #endif
    #endif
  #endif

  #if BEAST_64BIT || ! BEAST_VC7_OR_EARLIER
    #define BEAST_USE_INTRINSICS 1
  #endif
#else
  #error unknown compiler
#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//允许在输出窗口中单击pragma警告的简便宏
//
//用法：pragma message（beast_fileandline_u“advertise here！”）
//
//注意，对于C++ 11，宏之后的空间是强制性的。
//
//这里是这样的，所以它可以在直接包含它的C编译中使用。
//
#define BEAST_PP_STR2_(x) #x
#define BEAST_PP_STR1_(x) BEAST_PP_STR2_(x)
#define BEAST_FILEANDLINE_ __FILE__ "(" BEAST_PP_STR1_(__LINE__) "): warning:"

#endif
