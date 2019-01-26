
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有2005，谷歌公司。
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：wan@google.com（zhanyong wan）
//
//谷歌C++测试框架（谷歌测试）
//
//这个头文件定义了Google测试的公共API。应该是
//包括在任何使用谷歌测试的测试程序中。
//
//重要提示：由于C++语言的局限性，我们不得不
//在此头文件中保留一些内部实现详细信息。
//他们清楚地用这样的评论来标记：
//
////内部实现-不要在用户程序中使用。
//
//这样的代码并不打算由用户直接使用，而是主题
//未经通知擅自更改。因此，不要在用户中依赖它
//程序！
//
//承认：谷歌测试借鉴了自动测试的思想
//从Barthelemy Dagenais注册（barthelemy@prologique.com）
//easyUnit框架。

#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_H_

#include <limits>
#include <ostream>
#include <vector>

//版权所有2005，谷歌公司。
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：wan@google.com（zhanyong wan），eefacm@gmail.com（sean mcafee）
//
//谷歌C++测试框架（谷歌测试）
//
//此头文件声明内部使用的函数和宏
//谷歌测试。如有更改，恕不另行通知。

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_INTERNAL_H_

//版权所有2005，谷歌公司。
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：wan@google.com（zhanyong wan）
//
//用于将Google测试移植到各种
//平台。所有以u结尾的宏和在中定义的符号
//内部命名空间可能随时更改，恕不另行通知。代码
//在谷歌测试之外，不能直接使用它们。宏没有
//结尾是Google测试的公共API的一部分，可以由
//谷歌测试之外的代码。
//
//这个文件是谷歌测试的基础。所有其他谷歌测试源
//文件应包括此内容。因此，不能包括
//任何其他谷歌测试头。

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PORT_H_

//描述宏的环境
//----------------------
//
//谷歌测试可以在许多不同的环境中使用。宏在
//这个部分告诉谷歌测试它是什么样的环境
//在中使用，这样Google测试可以提供特定于环境的
//特性和实现。
//
//谷歌测试试图自动检测其
//环境，所以用户通常不需要担心这些
//宏。然而，自动检测并不完美。
//有时用户需要定义以下内容
//构建脚本中的宏可以覆盖Google测试的决策。
//
//如果用户没有在列表中定义宏，Google测试将
//提供默认定义。在包含此标题之后，所有
//此列表中的宏将被定义为1或0。
//
//维护人员注意事项：
//-这里的每个宏都是一个用户可调整的旋钮；不要增加列表
//轻轻地。
//-使用if键关闭这些宏。不要使用ifdef或if
//已定义（…）”，它将无法工作，因为这些宏总是
//定义。
//
//gtest_has_clone-将其定义为1/0以指示克隆（2）
//不可用。
//gtest_有_异常-将其定义为1/0以指示异常
//启用。
//gtest_具有_global_string-将其定义为1/0以指示：：string
//是否可用（某些系统定义
//：：字符串，与std：：string不同）。
//gtest_has_global_wstring-将其定义为1/0以指示：：string
//是否可用（某些系统定义
//：：wstring，与std:：wstring不同）。
//gtest_已将posix_重新定义为1/0，以指示posix正则
//表达式可用/不可用。
//gtest_haspthread-define it to 1/0 to indicate that<pthread.h>
//不可用。
//gtest_has_rtti-将其定义为1/0以指示rtti是/不是
//启用。
//gtest_has_std_wstring-将其定义为1/0表示
//std:：wstring工作/不工作（Google测试可以
//当std:：wstring不可用时使用）。
//gtest_有\tr1_元组-将其定义为1/0以指示tr1:：tuple
//不可用。
//gtest_has_seh-将其定义为1/0以指示
//编译器支持Microsoft的“结构化”
//异常处理”。
//gtest_具有流重定向
//-将其定义为1/0，以指示
//平台支持使用
//dup（）和dup2（）。
//gtest使用自己的tuple-将其定义为1/0以指示Google
//测试自己的tr1元组实现应该是
//使用。用户设置时未使用
//gtest_的tuple为0。
//gtest_lang_cx11-将其定义为1/0以指示Google测试
//是在C++ 11／C++ 98模式下构建的。
//GTEST链接为共享库
//-编译使用的测试时定义为1
//作为共享库的Google测试（称为
//Windows上的dll）。
//创建共享库
//-编译Google测试时定义为1
//作为共享库。

//指示宏的平台
//---------------------
//
//宏指示使用Google测试的平台
//（如果在给定平台上编译，则宏定义为1；
//否则未定义——从未定义为0）。谷歌试验
//自动定义这些宏。谷歌测试之外的代码必须
//没有定义它们。
//
//GTEST_OS_AIX-IBM AIX
//Gtestou osou cygwin-cygwin公司
//gtest-os-freebsd-freebsd
//GTEST操作系统-HP-UX
//gtest_os_linux-Linux版
//GTEST操作系统Linux安卓-谷歌安卓
//GTEST操作系统Mac-Mac操作系统X
//GTEST操作系统-iOS
//gtest_os_nacl-谷歌本地客户端（nacl）
//gtest-os-openbsd-openbsd
//gtest_os_qnx-qnx
//gtest_os_solaris-Sun solaris
//GTEST-OS-Symbian-Symbian
//gtest_os_windows-Windows（桌面、Mingw或移动）
//GTEST操作系统Windows桌面-Windows桌面
//测试窗口
//GTEST操作系统Windows Mobile-Windows Mobile
//GTEST操作系统Windows电话-Windows电话
//gtest_os_windows_rt-Windows应用商店应用程序/winrt
//GTEST操作系统-z/OS
//
//在这些平台中，cygwin、linux、max os x和windows具有
//最稳定的支撑。因为谷歌测试项目的核心成员
//无法访问其他平台，对它们的支持可能会更少
//稳定的。如果您发现您的平台有任何问题，请通知
//googletestframework@googlegroups.com（修复它们的补丁是
//更受欢迎！）.
//
//可能没有定义任何gtest-os-ux宏。

//指示宏的功能
//---------------------
//
//宏指示哪些Google测试功能可用（宏
//如果支持相应功能，则定义为1；
//否则未定义——从未定义为0）。谷歌试验
//自动定义这些宏。谷歌测试之外的代码必须
//没有定义它们。
//
//这些宏是公共的，因此可以编写可移植的测试。
//此类测试通常使用带有if的功能包围代码
//它控制着代码。例如：
//
//如果GTEST覕U有覕死亡覕测试
//预期死亡（dosomethingdeadly（））；
//第二节
//
//gtest_具有_combine-combine（）函数（对于参数化的值
//测试）
//GTEST有死亡测试-死亡测试
//gtest_具有参数化测试值
//GTest_有_型测试-型测试
//gtest_有\u型\u测试\u p型参数化测试
//gtest_is_threasafe-Google测试是线程安全的。
//gtest使用了增强的posix regex。不要混淆
//GTEST有用户可以
//定义自己。
//gtest使用简单的regex-使用我们自己的简单regex；
//以上两项是相互排斥的。
//gtest_can_compare_null-在expect_eq（）中接受非类型化的空值。

//其他公共宏
//------------
//
//gtest_flag（flag_name）-引用对应于
//给定的Google测试标志。

//内部公用设施
//------------
//
//以下宏和实用程序适用于Google测试的内部
//仅使用。谷歌测试之外的代码不能直接使用它们。
//
//用于基本C++编码的宏：
//用于禁用GCC警告的gtest_migulary_else_blocker_uuu。
//gtest属性“unused”声明类的实例或
//不必使用变量。
//gtest_disallow_assign_uuu disables operator=禁用。
//gtest_不允许复制并分配-禁用复制ctor和operator=。
//gtest必须使用结果声明必须使用函数的结果。
//在MSVC C4127所在的GTEST“有意创建条件”启动代码部分
//抑制（常量条件）。
//GTEST“有意创建条件”pop“完成代码”部分，其中MSVC C4127
//被抑制。
//
//C++ 11的特征包装：
//
//testing:：internal:：move-std:：move的可移植性包装。
//
//同步：
//mutex、mutexlock、threadlocal、getthreadcount（）。
//-同步原语。
//
//模板元编程：
//IS1指针，如Tr1；仅需要Symbian和IBM XL C/C++。
//iterator traits-std:：iterator_特性的部分实现，其中
//在用Sun C++编译时，在LBCSTD中不可用。
//
//智能指针：
//作用域指针-如tr2中所示。
//
//正则表达式：
//使用posix的简单正则表达式类
//类Unix上的扩展正则表达式语法
//平台或减少的常规异常语法
//其他平台，包括窗户。
//
//登录中：
//gtest_log_uu（）-以指定的严重性级别记录消息。
//log to stderr（）-将所有日志消息定向到stderr。
//flushinfolog（）-刷新信息日志消息。
//
//stdout和stderr捕获：
//capturestdout（）-开始捕获stdout。
//getcapturedsdout（）-停止捕获stdout并返回捕获的
//字符串。
//capturestderr（）-开始捕获stderr。
//getCapturedStrer（）-停止捕获stderr并返回捕获的
//字符串。
//
//整数类型：
//typewithsize-将整数映射到int类型。
//Int32、UInt32、Int64、UInt64、TimeInMillis
//-已知大小的整数。
//biggestant-最大的有符号整数类型。
//
//命令行实用程序：
//gtest_declare_*（）-声明标志。
//gtest_define_*（）-定义标志。
//getInjectableArgvs（）-返回命令行作为字符串的向量。
//
//环境变量实用程序：
//getenv（）-获取环境变量的值。
//boolfromgtestenv（）-分析bool环境变量。
//int32fromgtestenv（）-分析int32环境变量。
//stringfromgtestenv（）-分析字符串环境变量。

#include <ctype.h>   //用于空间等
#include <stddef.h>  //对于pTrdFifft
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32_WCE
# include <sys/types.h>
# include <sys/stat.h>
#endif  //！Wi32

#if defined __APPLE__
# include <AvailabilityMacros.h>
# include <TargetConditionals.h>
#endif

#include <algorithm>  //诺林
#include <iostream>  //诺林
#include <sstream>  //诺林
#include <string>  //诺林
#include <utility>

#define GTEST_DEV_EMAIL_ "googletestframework@@googlegroups.com"
#define GTEST_FLAG_PREFIX_ "gtest_"
#define GTEST_FLAG_PREFIX_DASH_ "gtest-"
#define GTEST_FLAG_PREFIX_UPPER_ "GTEST_"
#define GTEST_NAME_ "Google Test"
#define GTEST_PROJECT_URL_ "http://code.google.com/p/googletest/”

//确定用于编译此的gcc版本。
#ifdef __GNUC__
//40302表示版本4.3.2。
# define GTEST_GCC_VER_ \
    (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#endif  //第二节

//确定编译Google测试的平台。
#ifdef __CYGWIN__
# define GTEST_OS_CYGWIN 1
#elif defined __SYMBIAN32__
# define GTEST_OS_SYMBIAN 1
#elif defined _WIN32
# define GTEST_OS_WINDOWS 1
# ifdef _WIN32_WCE
#  define GTEST_OS_WINDOWS_MOBILE 1
# elif defined(__MINGW__) || defined(__MINGW32__)
#  define GTEST_OS_WINDOWS_MINGW 1
# elif defined(WINAPI_FAMILY)
#  include <winapifamily.h>
#  if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define GTEST_OS_WINDOWS_DESKTOP 1
#  elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#   define GTEST_OS_WINDOWS_PHONE 1
#  elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#   define GTEST_OS_WINDOWS_RT 1
#  else
//winapi_族已定义，但没有已知分区匹配。
//默认为桌面。
#   define GTEST_OS_WINDOWS_DESKTOP 1
#  endif
# else
#  define GTEST_OS_WINDOWS_DESKTOP 1
# endif  //Wi32
#elif defined __APPLE__
# define GTEST_OS_MAC 1
# if TARGET_OS_IPHONE
#  define GTEST_OS_IOS 1
# endif
#elif defined __FreeBSD__
# define GTEST_OS_FREEBSD 1
#elif defined __linux__
# define GTEST_OS_LINUX 1
# if defined __ANDROID__
#  define GTEST_OS_LINUX_ANDROID 1
# endif
#elif defined __MVS__
# define GTEST_OS_ZOS 1
#elif defined(__sun) && defined(__SVR4)
# define GTEST_OS_SOLARIS 1
#elif defined(_AIX)
# define GTEST_OS_AIX 1
#elif defined(__hpux)
# define GTEST_OS_HPUX 1
#elif defined __native_client__
# define GTEST_OS_NACL 1
#elif defined __OpenBSD__
# define GTEST_OS_OPENBSD 1
#elif defined __QNX__
# define GTEST_OS_QNX 1
#endif  //第七章

//用于禁用微软Visual C++警告的宏。
//
//gtest_disable_msc_warnings_push_u（4800 4385）
/*/*触发警告的代码C4800和C4385*/
//gtest_disable_msc_warnings_pop_（）。
如果msc ver>=1500
define gtest_disable_msc_warnings_push_u（warnings）
    _uupragma（警告（推送））\
    _uupragma（警告（禁用：警告））。
define gtest_disable_msc_warnings_pop_uu（）
    _uupragma（警告（pop））
否则
//旧版本的msvc没有\u pragma。
define gtest_disable_msc_warnings_push_u（警告）
定义gtest_禁用_msc_警告_pop_（））
第二节

ifndef gtest_lang_cxx11
//gcc和clang定义uuu gxx_实验性_cxx0x_uuuuu何时
//std=c，gnu++0x，11通过。C++ 11标准指定了一个
//_uu cplusplus的值，以及clang、gcc和的最新版本
//可能其他编译器也在C++ 11模式中设置了。
如果Gxx_实验性CxX0x uuU cplusplus>=201103L
//编译至少在C++ 11模式下。
定义gtest_lang_cx11 1
否则
定义gtest_lang_cx11 0
第二节
第二节

/不同于C++ 11语言支持，一些环境不提供
/正确的C++ 11库支持。值得注意的是，它可以内置
//c++ 11模式，当目标为Mac OS X 10.6时，它有一个旧的LIbSTDC++
//没有C++ 11的支持。
/ /
//LISBSTDC++具有足够的C++ 11支持，如GCC 4.6、
//20110325，但4.4和4.5系列中的维护版本如下
//这个日期，所以按日期戳检查这些版本。
//https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html abi.version
如果gtest_lang_cx11&&\
    （！定义（uu glibcxx_uuuu）（）
        _uuglibcxx_uuuux=20110325ul&/*gcc>=4.6.0*/ \

        /*老分支补丁发布黑名单：*/\
        我是个好朋友！=20110416ul&/*合同通用条款第4.4.6款*/ \

        /*LBCXXXX！=20120313ul&/*合同通用条款第4.4.7条
        我是个好朋友！=20110428ul&/*合同通用条款第4.5.3款*/ \

        /*LBCXXXX！=20120702ul））/*一般合同条款第4.5.4条*/
定义gtest_stdlib_cxx11 1
第二节

//只使用C++11库特征，如果库提供它们。
如果gtest_stdlib_cx11
定义gtest_有_std_begin_和_end_1
定义GTEST_具有_-std_-forward_列表_1
定义gtest_具有_std_功能_1
define gtest_has_std_initializer_list_1
定义gtest_有_std_move_1
定义GTEST_有_std_唯一_ptr_1
第二节

//c++ 11指定<tuple >提供STD::tuple。
//但是，有些平台可能还没有。
如果是GTEST诳U lang诳Cxx11
定义gtest_有标准tuple_1
如果定义了（clang_）
//灵感来源于http://clang.llvm.org/docs/languageextensions.html \35; uu has诳u include
如果定义了（包含了）&！_uuu has_u include（<tuple>）
Undef gtest_has覕std_tuple_
第二节
elif定义（35u msc35u ver）
//灵感来源于boost/config/stdlib/dinkumware.hpp
如果定义（cpplib_ver）&&cpplib_ver<520
Undef gtest_has覕std_tuple_
第二节
定义的elif
//灵感来自boost/config/stdlib/libstdcp3.hpp，
//http://gcc.gnu.org/gcc-4.2/changes.html和
//http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt01ch01.html manual.intro.status.standard.200x
如果Gnuc_uuuu4_（uu Gnuc_uuuu==4&&u Gnuc_u Minor_uuuuu2）
Undef gtest_has覕std_tuple_
第二节
第二节
第二节

//引入测试中使用的函数的定义：：internal:：posix
//命名空间（read、write、close、chdir、isatty、stat）。我们目前没有
//在Windows Mobile上使用它们。
如果GTEST操作系统窗口
如果有的话！GTEST操作系统Windows移动
include<direct.h>
包括<io.h>
第二节
//为了避免必须包含<windows.h>，请使用forward声明
//假设critical_节是rtl_critical_节的typedef。
//该假设由
//windowstypestest.critical_节是_rtl_critical_节。
结构关键部分；
否则
//这假设非Windows操作系统为以下操作系统提供unistd.h.。
//不是这样的，我们需要包含提供函数的头
//以上提到。
include<unistd.h>
include<strings.h>
endif//gtest_os_windows

如果GTEST操作系统Linux安卓
//用于定义与目标ndk api级别匹配的uu android_u api_uuu。
include<android/api level.h>//nolint
第二节

//将此定义为true iff google test可以使用posix正则表达式。
ifndef gtest_有位置
如果GTEST操作系统Linux安卓
//在Android上，<regex.h>仅从姜饼开始提供。
define gtest_has_posix_re（uuandroid_api_uuu>=9）
否则
定义GTEST_具有_posix_re（！GTEST操作系统窗口）
第二节
第二节

如果GTEST有位置

//在某些平台上，<regex.h>需要有人定义大小，以及
//否则不会编译。我们可以在这里包括它，因为我们已经
//包含<stdlib.h>，保证定义大小
//< STDDEF.H>。
include<regex.h>//nolint

定义GTEST_使用_posix_re 1

elif gtest_os_windows

//<regex.h>在Windows上不可用。使用我们自己的简单regex
//改为实现。
定义GTEST_使用诳Simple_re 1

否则

//<regex.h>可能在此平台上不可用。用我们自己的
//改为简单的regex实现。
定义GTEST_使用诳Simple_re 1

endif//gtest_具有\u posix_re

ifndef gtest_有\u例外
//用户没有告诉我们是否启用了异常，因此我们需要
//去弄清楚。
如果定义了（35u msc35u ver）定义了（35u borlandc_uuuuuu）
//MSVC和C++Builder对STL的实现使用了
//宏以启用异常，因此我们将执行相同的操作。
//假设默认情况下启用了异常。
ifndef诳u有诳u例外
define诳u has诳exceptions 1
endif//u有\u例外
定义gtest_有\u例外\u有\u例外
elif已定义
//clang defines uuu exceptions iff exceptions are enabled before clang 220714，
//但在此之后将启用iff清除。在obj-c++文件中，可以
/Objc异常清理，也需要清理，即使C++异常
//被禁用。CLANG具有检查C++的特性（CXXXX例外）
//从clang r206352开始的异常，但在
/那个。为了用CLAN可靠地检查C++异常可用性，请检查
//uuuu异常&&u具有_功能（cxx_异常）。
define gtest_has_exceptions（u exceptions&u has_feature（cxx_exceptions））。
elif已定义（35; gnuc_uuuu）&&uu例外
//gcc将异常定义为启用1个iff异常。
定义gtest_有_例外1
elif已定义（sunpro35u cc）
//Sun Pro CC支持异常。但是，没有编译时的方法
//检测是否启用。因此，我们假设
//除非用户另行通知，否则将启用它们。
定义gtest_有_例外1
定义的elif（uuu ibmcpp_uuu）&&uu例外
//xlc将异常定义为启用1个iff异常。
定义gtest_有_例外1
elif定义（35uu hp_acc）
//默认情况下，异常处理在HP Acc编译器中有效。它必须
//如果需要，由+noeh编译器选项打开。
定义gtest_有_例外1
否则
//对于其他编译器，我们假定已禁用异常
//保守。
定义gtest_有_例外0
endif//已定义（35u msc_ver）已定义（uu borlandc_uuuu）
endif//gtest_有_例外

如果有的话！定义（gtest_有_std_字符串）
//即使我们不再使用这个宏，我们还是保留它以防
//有些客户仍然依赖它。
定义gtest_具有_std_字符串1
艾利夫！gtest_有_std_字符串
//用户告诉我们：std:：string不可用。
错误“当：：std:：string不可用时，不能使用Google测试。”
我很喜欢你！定义（gtest_有_std_字符串）

ifndef gtest_具有_global_字符串
//用户没有告诉我们：：string是否可用，因此我们需要
//去弄清楚。

define gtest_has覕global_string 0

endif//gtest_具有_global_字符串

如果ndef gtest_具有_std_wstring
//用户没有告诉我们：：std:：wstring是否可用，因此我们需要
//去弄清楚。
//TODO（wan@google.com）：使用autoconf检测：：std:：wstring
//可用。

//cygwin 1.7及以下版本不支持：：std:：wstring。
//Solaris的libc++也不支持它。安卓
//不支持它，至少是最近的froyo（2.2）。
定义GTEST_具有标准字符串
    （！（gtest_OS_Linux_Android_gtest_OS_Cygwin_gtest_OS_Solaris）

endif//gtest_具有_-std_-wstring

如果ndef gtest_具有_globalwstring
//用户没有告诉我们：：wstring是否可用，因此我们需要
//去弄清楚。
定义GTEST_具有全局性
    （gtest_有标准字符串，&gtest_有全局字符串）
endif//gtest_具有全局性

//确定RTTI是否可用。
如果ndef gtest_u有_rtti
//用户没有告诉我们是否启用了rtti，所以我们需要
//弄清楚。

ifdef诳msc诳ver

ifdef _cpptti//msvc定义此宏iff rtti已启用。
定义GTEST_具有_rtti 1
否则
定义GTEST_具有_rtti 0
第二节

//从4.3.2版开始，gcc defines_uuxx_rtti iff rtti已启用。
elif已定义（uu gnuc_uuu）&&（gtest_gcc_ver_uu>=40302）

定义
//使用android ndk和
//frtti-fno异常，生成在链接时失败，未定义
//对uucxa_bad_typeid的引用。注意，如果stl或toolchain bug，
//因此检测到时禁用rtti。
如果gtest_os_linux_android&&defined（_stlport_major）&&&\
       ！已定义（uuu异常）
定义GTEST_具有_rtti 0
否则
定义GTEST_具有_rtti 1
endif//gtest_os_linux_android&&u stlport_major&！例外情况
否则
定义GTEST_具有_rtti 0
endif/诳uu gxx_rtti

//clang定义了从3.0版开始的uuugxx_rtti，但其手册建议
//使用具有\功能。自2.7以来是否支持_功能（cxx_rtti），则
/c++支持的第一个版本。
elif已定义

定义gtest_具有_rtti_uuu具有_功能（cxx_rtti）

//从9.0版开始，IBM Visual Age将“所有”定义为1，如果
//同时存在typeid和dynamic_cast功能。
定义的elif（uu ibmcpp_uuu）&&（uu ibmcpp_uuuuuuuuuuuux）=900）

ifdef_uu rtti_all_uu
定义GTEST_具有_rtti 1
否则
定义GTEST_具有_rtti 0
第二节

否则

//对于所有其他编译器，我们假定启用了rtti。
定义GTEST_具有_rtti 1

endif//_msc版本

endif//gtest_具有

//此标题负责在RTTI时包含<typeinfo>
//启用。
如果GTEST覕u有覕rtti
include<typeinfo>
第二节

//确定Google测试是否可以使用pthreads库。
如果ndef gtest有螺纹
//用户没有明确告诉我们，所以我们对
//哪些平台支持pthreads。
/ /
//若要在Google测试中禁用线程支持，请添加-dgtest_haspthread=0
//到编译器标志。
define gtest_haspthread（gtest_os_linux_gtest_os_mac_gtest_os_hpux
    gtest_os_qnx gtest_os_freebsd gtest_os_nacl）
endif//gtest_has诳pthread

如果GTEST有螺纹
//gtest port.h保证在gtest具有pthread时包含<pthread.h>
/真的。
include<pthread.h>//nolint

//对于timespec和nanosleep，在下面使用。
include<time.h>//nolint
第二节

//确定Google测试是否可以使用tr1/tuple。你可以定义
//此宏设置为0以防止Google测试使用tuple（any
//功能取决于在此模式下禁用的元组）。
如果ndef gtest_具有_tr1_元组
如果gtest_os_linux_android&&defined（_stlport_major）
//Android NDK附带的stlport既没有<tr1/tuple>也没有<tuple>。
定义gtest_有_tr1_tuple 0
否则
//用户没有告诉我们不要这样做，所以我们假设没问题。
定义gtest_有_tr1_tuple 1
第二节
endif//gtest_具有_tr1_元组

//确定Google测试是否自己的tr1元组实现
//应该使用。
ifndef gtest_使用_own_tr1_tuple
用户没有告诉我们，所以我们需要弄清楚。

//如果不确定用户是否有
//已经实现了。此时，libstdc++4.0.0+和
//MSVC 2010是唯一的主流标准库
//使用tr1元组实现。NVIDIA的CUDA NVCC编译器
//通过定义“gnuc”和“friends”假装为gcc，但不能
//编译gcc的tuple实现。MSVC 2008（9.0）提供TR1
//在323MB功能包下载中创建元组，我们不能假定
//用户有。QNX的QCC编译器是一个经过修改的gcc，但它没有
//支持tr1元组。LBC+++只提供STD：：元组，在C++ 11模式下，
//它可以与一些定义uu gnuc_uuuu的编译器一起使用。
如果（定义的（GNUC）&！定义（uu cudac_uuuu）&&（gtest_gcc_u ver_uuu>=40000）
      & &！测试操作系统已定义（_libcpp_version））_msc_ver>=1600
定义gtest_env_has_tr1_tuple_1
第二节

//c++ 11指定<tuple >提供STD::tuple。如果使用GTEST，请使用
//在C++ 11模式和LIbSTDC++中不是非常老的（二进制对象瞄准OS X 10.6）
//可以使用clang生成，但需要使用gcc4.2的libstdc++）。
如果是GTEST覕lang覕CXx11&（！定义（uu glibcxx_uuuu）u glibcxx_uuuuuu>20110325）
定义gtest_env_has_std_tuple_1
第二节

如果gtest_env_有_tr1_tuple_gtest_env_有_std_tuple_
定义gtest_使用_own_tr1_tuple 0
否则
定义gtest_使用_own_tr1_tuple 1
第二节

endif//gtest_使用_own_tr1_tuple

//为了避免在任何地方进行条件编译，我们使其
//gtest port.h负责包括头实现
/ /元组。
如果gtest_有标准tuple_
include<tuple>//iwyu pragma:export
定义gtest_tuple_namespace_u：：std
endif//gtest_具有_std_tuple_

//我们包括tr1:：tuple，即使std:：tuple可用于定义打印机
/他们。
如果gtest_有_tr1_元组
ifndef gtest_tuple_namespace_
定义gtest_tuple_namespace_:：std:：tr1
endif//gtest_tuple_namespace_

如果gtest_使用_own_tr1_tuple
//此文件由以下命令生成：
//泵.py gtest-tuple.h.泵
//不要手工编辑！！！！

//版权所有2009 Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan）

//实现google test和google mock所需的tr1元组的子集。

ifndef gtest_包括\u gtest_internal_gtest_tuple_h_
定义gtest_包括gtest_内部gtest_tuple_h_

include<utility>//for：：std：：pair.

//Symbian中使用的编译器有一个错误，阻止我们声明
//tuple模板作为朋友（它抱怨tuple被重新定义）。这个
//hack通过声明应该是的成员来绕过bug
//私有为公共。
//Sun Studio版本<12也有上述错误。
如果已定义（uu symbian32_uuu）（已定义（u sunpro_cc）&&u sunpro_cc<0x590）
将gtest_声明tuple_定义为_friend_uu public：
否则
定义gtest_声明tuple_为\u朋友\
    template<gtest_10_typenames_u（u）>friend class tuple；\
   私人：
第二节

//Visual Studio 2010、2012和2013在std:：tr1中定义冲突的符号
//有我们自己的定义。因此，使用我们自己的元组不起作用
//那些编译器。
如果定义了（_msc_ver）&&_msc_ver>=1600/*1600是Visual Studio 2010*/

# error "gtest's tuple doesn't compile on Visual Studio 2010 or later. \
GTEST_USE_OWN_TR1_TUPLE must be set to 0 on those compilers."
#endif

//gtest_n_tuple_（t）是n-tuple的类型。
#define GTEST_0_TUPLE_(T) tuple<>
#define GTEST_1_TUPLE_(T) tuple<T##0, void, void, void, void, void, void, \
    void, void, void>
#define GTEST_2_TUPLE_(T) tuple<T##0, T##1, void, void, void, void, void, \
    void, void, void>
#define GTEST_3_TUPLE_(T) tuple<T##0, T##1, T##2, void, void, void, void, \
    void, void, void>
#define GTEST_4_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, void, void, void, \
    void, void, void>
#define GTEST_5_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, void, void, \
    void, void, void>
#define GTEST_6_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, T##5, void, \
    void, void, void>
#define GTEST_7_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, T##5, T##6, \
    void, void, void>
#define GTEST_8_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, T##5, T##6, \
    T##7, void, void>
#define GTEST_9_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, T##5, T##6, \
    T##7, T##8, void>
#define GTEST_10_TUPLE_(T) tuple<T##0, T##1, T##2, T##3, T##4, T##5, T##6, \
    T##7, T##8, T##9>

//gtest_n_typenames_（t）声明n个类型名的列表。
#define GTEST_0_TYPENAMES_(T)
#define GTEST_1_TYPENAMES_(T) typename T##0
#define GTEST_2_TYPENAMES_(T) typename T##0, typename T##1
#define GTEST_3_TYPENAMES_(T) typename T##0, typename T##1, typename T##2
#define GTEST_4_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3
#define GTEST_5_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4
#define GTEST_6_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4, typename T##5
#define GTEST_7_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4, typename T##5, typename T##6
#define GTEST_8_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4, typename T##5, typename T##6, typename T##7
#define GTEST_9_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4, typename T##5, typename T##6, \
    typename T##7, typename T##8
#define GTEST_10_TYPENAMES_(T) typename T##0, typename T##1, typename T##2, \
    typename T##3, typename T##4, typename T##5, typename T##6, \
    typename T##7, typename T##8, typename T##9

//理论上，在：：std名称空间中定义内容是未定义的
//行为。我们可以这样做，因为我们正在扮演一个标准的角色
//图书馆供应商。
namespace std {
namespace tr1 {

template <typename T0 = void, typename T1 = void, typename T2 = void,
    typename T3 = void, typename T4 = void, typename T5 = void,
    typename T6 = void, typename T7 = void, typename T8 = void,
    typename T9 = void>
class tuple;

//命名空间gtest_internal中的任何内容都是Google测试的内部内容
//实现细节，不能直接在用户代码中使用。
namespace gtest_internal {

//ByRef:：如果t是引用，则类型为t；否则为const&。
template <typename T>
struct ByRef { typedef const T& type; };  //诺林
template <typename T>
struct ByRef<T&> { typedef T& type; };  //诺林

//一个方便的ByRef包装。
#define GTEST_BY_REF_(T) typename ::std::tr1::gtest_internal::ByRef<T>::type

//addref<t>：如果t是引用，则类型为t；否则为t&。这个
//与tr1:：add_reference<t>：：type相同。
template <typename T>
struct AddRef { typedef T& type; };  //诺林
template <typename T>
struct AddRef<T&> { typedef T& type; };  //诺林

//一个方便的addref包装器。
#define GTEST_ADD_REF_(T) typename ::std::tr1::gtest_internal::AddRef<T>::type

//用于实现get的帮助程序。
template <int k> class Get;

//用于实现tuple_element<k，t>的助手。KindexValid为真
//iff k<t类型元组中的字段数。
template <bool kIndexValid, int kIndex, class Tuple>
struct TupleElement;

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 0, GTEST_10_TUPLE_(T) > {
  typedef T0 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 1, GTEST_10_TUPLE_(T) > {
  typedef T1 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 2, GTEST_10_TUPLE_(T) > {
  typedef T2 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 3, GTEST_10_TUPLE_(T) > {
  typedef T3 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 4, GTEST_10_TUPLE_(T) > {
  typedef T4 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 5, GTEST_10_TUPLE_(T) > {
  typedef T5 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 6, GTEST_10_TUPLE_(T) > {
  typedef T6 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 7, GTEST_10_TUPLE_(T) > {
  typedef T7 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 8, GTEST_10_TUPLE_(T) > {
  typedef T8 type;
};

template <GTEST_10_TYPENAMES_(T)>
struct TupleElement<true, 9, GTEST_10_TUPLE_(T) > {
  typedef T9 type;
};

}  //命名空间gtest_内部

template <>
class tuple<> {
 public:
  tuple() {}
  /*le（const tuple&/*t*/）
  元组和运算符=（const tuple&/*t*/) { return *this; }

};

template <GTEST_1_TYPENAMES_(T)>
class GTEST_1_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0) : f0_(f0) {}

  tuple(const tuple& t) : f0_(t.f0_) {}

  template <GTEST_1_TYPENAMES_(U)>
  tuple(const GTEST_1_TUPLE_(U)& t) : f0_(t.f0_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_1_TYPENAMES_(U)>
  tuple& operator=(const GTEST_1_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_1_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_1_TUPLE_(U)& t) {
    f0_ = t.f0_;
    return *this;
  }

  T0 f0_;
};

template <GTEST_2_TYPENAMES_(T)>
class GTEST_2_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1) : f0_(f0),
      f1_(f1) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_) {}

  template <GTEST_2_TYPENAMES_(U)>
  tuple(const GTEST_2_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_) {}
  template <typename U0, typename U1>
  tuple(const ::std::pair<U0, U1>& p) : f0_(p.first), f1_(p.second) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_2_TYPENAMES_(U)>
  tuple& operator=(const GTEST_2_TUPLE_(U)& t) {
    return CopyFrom(t);
  }
  template <typename U0, typename U1>
  tuple& operator=(const ::std::pair<U0, U1>& p) {
    f0_ = p.first;
    f1_ = p.second;
    return *this;
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_2_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_2_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
};

template <GTEST_3_TYPENAMES_(T)>
class GTEST_3_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2) : f0_(f0), f1_(f1), f2_(f2) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_) {}

  template <GTEST_3_TYPENAMES_(U)>
  tuple(const GTEST_3_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_3_TYPENAMES_(U)>
  tuple& operator=(const GTEST_3_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_3_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_3_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
};

template <GTEST_4_TYPENAMES_(T)>
class GTEST_4_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3) : f0_(f0), f1_(f1), f2_(f2),
      f3_(f3) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_) {}

  template <GTEST_4_TYPENAMES_(U)>
  tuple(const GTEST_4_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_4_TYPENAMES_(U)>
  tuple& operator=(const GTEST_4_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_4_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_4_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
};

template <GTEST_5_TYPENAMES_(T)>
class GTEST_5_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3,
      GTEST_BY_REF_(T4) f4) : f0_(f0), f1_(f1), f2_(f2), f3_(f3), f4_(f4) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_) {}

  template <GTEST_5_TYPENAMES_(U)>
  tuple(const GTEST_5_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_5_TYPENAMES_(U)>
  tuple& operator=(const GTEST_5_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_5_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_5_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
};

template <GTEST_6_TYPENAMES_(T)>
class GTEST_6_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_(), f5_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3, GTEST_BY_REF_(T4) f4,
      GTEST_BY_REF_(T5) f5) : f0_(f0), f1_(f1), f2_(f2), f3_(f3), f4_(f4),
      f5_(f5) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_), f5_(t.f5_) {}

  template <GTEST_6_TYPENAMES_(U)>
  tuple(const GTEST_6_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_), f5_(t.f5_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_6_TYPENAMES_(U)>
  tuple& operator=(const GTEST_6_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_6_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_6_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    f5_ = t.f5_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
  T5 f5_;
};

template <GTEST_7_TYPENAMES_(T)>
class GTEST_7_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_(), f5_(), f6_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3, GTEST_BY_REF_(T4) f4,
      GTEST_BY_REF_(T5) f5, GTEST_BY_REF_(T6) f6) : f0_(f0), f1_(f1), f2_(f2),
      f3_(f3), f4_(f4), f5_(f5), f6_(f6) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_), f5_(t.f5_), f6_(t.f6_) {}

  template <GTEST_7_TYPENAMES_(U)>
  tuple(const GTEST_7_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_), f5_(t.f5_), f6_(t.f6_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_7_TYPENAMES_(U)>
  tuple& operator=(const GTEST_7_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_7_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_7_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    f5_ = t.f5_;
    f6_ = t.f6_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
  T5 f5_;
  T6 f6_;
};

template <GTEST_8_TYPENAMES_(T)>
class GTEST_8_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_(), f5_(), f6_(), f7_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3, GTEST_BY_REF_(T4) f4,
      GTEST_BY_REF_(T5) f5, GTEST_BY_REF_(T6) f6,
      GTEST_BY_REF_(T7) f7) : f0_(f0), f1_(f1), f2_(f2), f3_(f3), f4_(f4),
      f5_(f5), f6_(f6), f7_(f7) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_) {}

  template <GTEST_8_TYPENAMES_(U)>
  tuple(const GTEST_8_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_8_TYPENAMES_(U)>
  tuple& operator=(const GTEST_8_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_8_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_8_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    f5_ = t.f5_;
    f6_ = t.f6_;
    f7_ = t.f7_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
  T5 f5_;
  T6 f6_;
  T7 f7_;
};

template <GTEST_9_TYPENAMES_(T)>
class GTEST_9_TUPLE_(T) {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_(), f5_(), f6_(), f7_(), f8_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3, GTEST_BY_REF_(T4) f4,
      GTEST_BY_REF_(T5) f5, GTEST_BY_REF_(T6) f6, GTEST_BY_REF_(T7) f7,
      GTEST_BY_REF_(T8) f8) : f0_(f0), f1_(f1), f2_(f2), f3_(f3), f4_(f4),
      f5_(f5), f6_(f6), f7_(f7), f8_(f8) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_), f8_(t.f8_) {}

  template <GTEST_9_TYPENAMES_(U)>
  tuple(const GTEST_9_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_), f8_(t.f8_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_9_TYPENAMES_(U)>
  tuple& operator=(const GTEST_9_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_9_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_9_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    f5_ = t.f5_;
    f6_ = t.f6_;
    f7_ = t.f7_;
    f8_ = t.f8_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
  T5 f5_;
  T6 f6_;
  T7 f7_;
  T8 f8_;
};

template <GTEST_10_TYPENAMES_(T)>
class tuple {
 public:
  template <int k> friend class gtest_internal::Get;

  tuple() : f0_(), f1_(), f2_(), f3_(), f4_(), f5_(), f6_(), f7_(), f8_(),
      f9_() {}

  explicit tuple(GTEST_BY_REF_(T0) f0, GTEST_BY_REF_(T1) f1,
      GTEST_BY_REF_(T2) f2, GTEST_BY_REF_(T3) f3, GTEST_BY_REF_(T4) f4,
      GTEST_BY_REF_(T5) f5, GTEST_BY_REF_(T6) f6, GTEST_BY_REF_(T7) f7,
      GTEST_BY_REF_(T8) f8, GTEST_BY_REF_(T9) f9) : f0_(f0), f1_(f1), f2_(f2),
      f3_(f3), f4_(f4), f5_(f5), f6_(f6), f7_(f7), f8_(f8), f9_(f9) {}

  tuple(const tuple& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_), f3_(t.f3_),
      f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_), f8_(t.f8_), f9_(t.f9_) {}

  template <GTEST_10_TYPENAMES_(U)>
  tuple(const GTEST_10_TUPLE_(U)& t) : f0_(t.f0_), f1_(t.f1_), f2_(t.f2_),
      f3_(t.f3_), f4_(t.f4_), f5_(t.f5_), f6_(t.f6_), f7_(t.f7_), f8_(t.f8_),
      f9_(t.f9_) {}

  tuple& operator=(const tuple& t) { return CopyFrom(t); }

  template <GTEST_10_TYPENAMES_(U)>
  tuple& operator=(const GTEST_10_TUPLE_(U)& t) {
    return CopyFrom(t);
  }

  GTEST_DECLARE_TUPLE_AS_FRIEND_

  template <GTEST_10_TYPENAMES_(U)>
  tuple& CopyFrom(const GTEST_10_TUPLE_(U)& t) {
    f0_ = t.f0_;
    f1_ = t.f1_;
    f2_ = t.f2_;
    f3_ = t.f3_;
    f4_ = t.f4_;
    f5_ = t.f5_;
    f6_ = t.f6_;
    f7_ = t.f7_;
    f8_ = t.f8_;
    f9_ = t.f9_;
    return *this;
  }

  T0 f0_;
  T1 f1_;
  T2 f2_;
  T3 f3_;
  T4 f4_;
  T5 f5_;
  T6 f6_;
  T7 f7_;
  T8 f8_;
  T9 f9_;
};

//6.1.3.2元组创建功能。

//已知限制：我们不支持通过
//std:：tr1:：reference_wrapper<t>to make_tuple（）。我们没有
//机具系杆（）。

inline tuple<> make_tuple() { return tuple<>(); }

template <GTEST_1_TYPENAMES_(T)>
inline GTEST_1_TUPLE_(T) make_tuple(const T0& f0) {
  return GTEST_1_TUPLE_(T)(f0);
}

template <GTEST_2_TYPENAMES_(T)>
inline GTEST_2_TUPLE_(T) make_tuple(const T0& f0, const T1& f1) {
  return GTEST_2_TUPLE_(T)(f0, f1);
}

template <GTEST_3_TYPENAMES_(T)>
inline GTEST_3_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2) {
  return GTEST_3_TUPLE_(T)(f0, f1, f2);
}

template <GTEST_4_TYPENAMES_(T)>
inline GTEST_4_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3) {
  return GTEST_4_TUPLE_(T)(f0, f1, f2, f3);
}

template <GTEST_5_TYPENAMES_(T)>
inline GTEST_5_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4) {
  return GTEST_5_TUPLE_(T)(f0, f1, f2, f3, f4);
}

template <GTEST_6_TYPENAMES_(T)>
inline GTEST_6_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4, const T5& f5) {
  return GTEST_6_TUPLE_(T)(f0, f1, f2, f3, f4, f5);
}

template <GTEST_7_TYPENAMES_(T)>
inline GTEST_7_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4, const T5& f5, const T6& f6) {
  return GTEST_7_TUPLE_(T)(f0, f1, f2, f3, f4, f5, f6);
}

template <GTEST_8_TYPENAMES_(T)>
inline GTEST_8_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4, const T5& f5, const T6& f6, const T7& f7) {
  return GTEST_8_TUPLE_(T)(f0, f1, f2, f3, f4, f5, f6, f7);
}

template <GTEST_9_TYPENAMES_(T)>
inline GTEST_9_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4, const T5& f5, const T6& f6, const T7& f7,
    const T8& f8) {
  return GTEST_9_TUPLE_(T)(f0, f1, f2, f3, f4, f5, f6, f7, f8);
}

template <GTEST_10_TYPENAMES_(T)>
inline GTEST_10_TUPLE_(T) make_tuple(const T0& f0, const T1& f1, const T2& f2,
    const T3& f3, const T4& f4, const T5& f5, const T6& f6, const T7& f7,
    const T8& f8, const T9& f9) {
  return GTEST_10_TUPLE_(T)(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9);
}

//6.1.3.3元组助手类。

template <typename Tuple> struct tuple_size;

template <GTEST_0_TYPENAMES_(T)>
struct tuple_size<GTEST_0_TUPLE_(T) > {
  static const int value = 0;
};

template <GTEST_1_TYPENAMES_(T)>
struct tuple_size<GTEST_1_TUPLE_(T) > {
  static const int value = 1;
};

template <GTEST_2_TYPENAMES_(T)>
struct tuple_size<GTEST_2_TUPLE_(T) > {
  static const int value = 2;
};

template <GTEST_3_TYPENAMES_(T)>
struct tuple_size<GTEST_3_TUPLE_(T) > {
  static const int value = 3;
};

template <GTEST_4_TYPENAMES_(T)>
struct tuple_size<GTEST_4_TUPLE_(T) > {
  static const int value = 4;
};

template <GTEST_5_TYPENAMES_(T)>
struct tuple_size<GTEST_5_TUPLE_(T) > {
  static const int value = 5;
};

template <GTEST_6_TYPENAMES_(T)>
struct tuple_size<GTEST_6_TUPLE_(T) > {
  static const int value = 6;
};

template <GTEST_7_TYPENAMES_(T)>
struct tuple_size<GTEST_7_TUPLE_(T) > {
  static const int value = 7;
};

template <GTEST_8_TYPENAMES_(T)>
struct tuple_size<GTEST_8_TUPLE_(T) > {
  static const int value = 8;
};

template <GTEST_9_TYPENAMES_(T)>
struct tuple_size<GTEST_9_TUPLE_(T) > {
  static const int value = 9;
};

template <GTEST_10_TYPENAMES_(T)>
struct tuple_size<GTEST_10_TUPLE_(T) > {
  static const int value = 10;
};

template <int k, class Tuple>
struct tuple_element {
  typedef typename gtest_internal::TupleElement<
      k < (tuple_size<Tuple>::value), k, Tuple>::type type;
};

#define GTEST_TUPLE_ELEMENT_(k, Tuple) typename tuple_element<k, Tuple >::type

//6.1.3.4元件接入。

namespace gtest_internal {

template <>
class Get<0> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(0, Tuple))
Field(Tuple& t) { return t.f0_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(0, Tuple))
  ConstField(const Tuple& t) { return t.f0_; }
};

template <>
class Get<1> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(1, Tuple))
Field(Tuple& t) { return t.f1_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(1, Tuple))
  ConstField(const Tuple& t) { return t.f1_; }
};

template <>
class Get<2> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(2, Tuple))
Field(Tuple& t) { return t.f2_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(2, Tuple))
  ConstField(const Tuple& t) { return t.f2_; }
};

template <>
class Get<3> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(3, Tuple))
Field(Tuple& t) { return t.f3_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(3, Tuple))
  ConstField(const Tuple& t) { return t.f3_; }
};

template <>
class Get<4> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(4, Tuple))
Field(Tuple& t) { return t.f4_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(4, Tuple))
  ConstField(const Tuple& t) { return t.f4_; }
};

template <>
class Get<5> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(5, Tuple))
Field(Tuple& t) { return t.f5_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(5, Tuple))
  ConstField(const Tuple& t) { return t.f5_; }
};

template <>
class Get<6> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(6, Tuple))
Field(Tuple& t) { return t.f6_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(6, Tuple))
  ConstField(const Tuple& t) { return t.f6_; }
};

template <>
class Get<7> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(7, Tuple))
Field(Tuple& t) { return t.f7_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(7, Tuple))
  ConstField(const Tuple& t) { return t.f7_; }
};

template <>
class Get<8> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(8, Tuple))
Field(Tuple& t) { return t.f8_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(8, Tuple))
  ConstField(const Tuple& t) { return t.f8_; }
};

template <>
class Get<9> {
 public:
  template <class Tuple>
  static GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(9, Tuple))
Field(Tuple& t) { return t.f9_; }  //诺林

  template <class Tuple>
  static GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(9, Tuple))
  ConstField(const Tuple& t) { return t.f9_; }
};

}  //命名空间gtest_内部

template <int k, GTEST_10_TYPENAMES_(T)>
GTEST_ADD_REF_(GTEST_TUPLE_ELEMENT_(k, GTEST_10_TUPLE_(T)))
get(GTEST_10_TUPLE_(T)& t) {
  return gtest_internal::Get<k>::Field(t);
}

template <int k, GTEST_10_TYPENAMES_(T)>
GTEST_BY_REF_(GTEST_TUPLE_ELEMENT_(k,  GTEST_10_TUPLE_(T)))
get(const GTEST_10_TUPLE_(T)& t) {
  return gtest_internal::Get<k>::ConstField(t);
}

//6.1.3.5关系运算符

//我们只实现==和！=，因为我们还不需要其他的。

namespace gtest_internal {

//samesizetuplePrefixComparator<k，k>：：eq（t1，t2）返回true，如果
//T1的前k个字段等于T2的前k个字段。
//samesizetuplePrefixComparator（k1，k2）是一个编译器错误，如果
//K1！= K2。
template <int kSize1, int kSize2>
struct SameSizeTuplePrefixComparator;

template <>
struct SameSizeTuplePrefixComparator<0, 0> {
  template <class Tuple1, class Tuple2>
  /*tic bool eq（const tuple1&/*t1*/，const tuple2&/*t2*/）
    回归真实；
  }
}；

模板<int k>
结构samesizetuplePrefixComparator<k，k>
  template<class tuple1，class tuple2>
  静态bool eq（const tuple1&t1，const tuple2&t2）
    返回samesizetuplePrefixComparator<k-1，k-1>：：eq（t1，t2）&&
        ：：std:：tr1:：get<k-1>（t1）==：：std:：tr1:：get<k-1>（t2）；
  }
}；

//命名空间gtest_内部

template<gtest_10_typenames_u（t），gtest_10_typenames_u（u）>
inline bool operator==（const gtest_10_tuple_u（t）&t，
                       const gtest_10_tuple_u（u）&u）
  返回gtest_internal:：samesizetuplePrefixComparator<
      tuple_size<gtest_10_tuple_u（t）>：值，
      tuple_size<gtest_10_tuple_u（u）>：：value>：：eq（t，u）；
}

template<gtest_10_typenames_u（t），gtest_10_typenames_u（u）>
内联布尔运算符！=（const gtest_10_tuple_u（t）&t，，
                       const gtest_10_tuple_u（u）&u）返回！（t=＝u）；}

//6.1.4对。
//未实现。

//命名空间tr1
//命名空间标准

取消GTEST_0_tuple_
取消GTEST_1_tuple_
取消GTEST_2_tuple_
取消GTEST_3_tuple_
取消GTEST_4_tuple_
取消GTEST_5_tuple_
取消GTEST_6_tuple_
取消GTEST_7_tuple_
取消GTEST_8_tuple_
取消测试诳9诳tuple诳
未定义的gtest_10_tuple_

取消gtest_0_类型名_
取消gtest_1_类型名_
取消gtest_2_类型名_
取消gtest_3_类型名_
取消gtest_4_类型名_
取消gtest_5_类型名_
取消gtest_6_类型名_
取消gtest_7_类型名_
取消gtest_8_类型名_
取消gtest_9_类型名_
取消gtest_10_类型名_

Undef gtest_声明tuple_为\u friend_
取消测试由诳参考诳
取消测试添加参考
undef gtest_tuple_element_

endif//gtest_包括\u gtest_internal_gtest_tuple_h_
elif gtest_env_has_std_tuple_
include<tuple>
//c++ 11将其元组放入：：STD命名空间，而不是
//：：标准：：tr1。gtest期望tuple住在：：std:：tr1中，所以把它放在那里。
//这会导致未定义的行为，但受支持的编译器在
我们的计划。
命名空间STD{
命名空间Tr1 {
使用：：std：：get；
使用：：std：：生成元组；
使用：：std：：tuple；
使用：：std：：tuple_元素；
使用：：std：：tuple_大小；
}
}

伊利夫·格特斯特·奥斯·塞班

//在Symbian上，boost_has_tr1_tuple使boost的tr1 tuple库
//使用stlport的tuple实现，不幸的是，它没有
//与Symbian一起分发的stlport副本不完整。
//通过确保boost_has_tr1_tuple未定义，我们强制boost到
//使用自己的元组实现。
ifdef boost_具有_tr1_元组
亡灵提升诳有诳TR1诳
endif//Boost诳u has诳1诳tuple

//这将阻止<boost/tr1/detail/config.hpp>，它定义
//Boost_有_tr1_tuple，由Boost的<tuple>包含。
定义Boost_tr1_细节_配置_hpp_包括
include<tuple>//iwyu pragma:export//nolint

elif已定义（gnuc_uuu）&&（gtest_gcc_u ver_uuux=40000）
//gcc 4.0+在<tr1/tuple>头中实现tr1/tuple。这确实
//不符合tr1规范，该规范要求头为<tuple>。

如果有的话！gtest_has_rtti&&gtest_gcc_ver_40302
//在4.3.2版之前，gcc有一个bug，导致<tr1/functional>，
//包含在<tr1/tuple>中，当rtti为
/禁用。_tr1_功能是
//<tr1/功能>。因此，以下定义是防止
//<tr1/functional>包含在内。
定义功能1
include<tr1/tuple>
undef tr1功能//允许用户包括
                        //<tr1/functional>如果他选择。
否则
include<tr1/tuple>//nolint
我很喜欢你！gtest_has_rtti&&gtest_gcc_ver_40302

否则
//如果编译器不是gcc 4.0+，我们假设用户使用的是
//规范符合TR1实现。
include<tuple>//iwyu pragma:export//nolint
endif//gtest_使用_own_tr1_tuple

endif//gtest_具有_tr1_元组

//确定是否支持克隆（2）。
//通常只能在Linux上使用，不包括
//安腾体系结构上的Linux。
//另请参见http://linux.die.net/man/2/clone。
如果ndef gtest有克隆
用户没有告诉我们，所以我们需要弄清楚。

如果GTEST诳OS诳Linux&！已定义（u ia64_uuuu）
如果GTEST操作系统Linux安卓
//在Android上，clone（）仅在以姜饼为开头的ARM上可用。
如果定义了（uu arm_uuuuu）&uuu android_u api_uuuuuu>=9
定义gtest_有_克隆1
否则
定义gtest_有_克隆0
第二节
否则
定义gtest_有_克隆1
第二节
否则
定义gtest_有_克隆0
endif//gtest_os_linux&！已定义（u ia64_uuuu）

endif//gtest_有_克隆

//确定是否支持流重定向。这是用来测试的
//输出正确性并实现死亡测试。
ifndef gtest_具有_stream_重定向
//默认情况下，我们假定所有
//除了已知的移动平台。
如果gtest_os_windows_mobile_gtest_os_symbian_
    gtest_os_windows_phone_gtest_os_windows_rt
定义GTEST_具有_流_重定向0
否则
定义GTEST_具有_流_重定向1
我很喜欢你！测试Windows Mobile&！通用测试系统
endif//gtest_具有_stream_重定向

//确定是否支持死亡测试。
//Google测试不支持VC 7.1及以下版本的死亡测试：
//在调试配置中编译为GUI的VC 7.1应用程序中abort（）。
//弹出一个无法通过编程方式抑制的对话框窗口。
if（gtest_os_linux_gtest_os_cygwin_gtest_os_solaris_
     （GTEST操作系统&！GTEST操作系统）
     （gtest_os_windows_desktop&&_msc_ver>=1400）
     gtest_os_windows_mingw_gtest_os_u aix_gtest_os_hpux_
     gtest_os_openbsd gtest_os_qnx gtest_os_freebsd）
定义gtest_有_死亡_测试1
include<vector>//nolint
第二节

//我们不支持MSVC 7.1，现在禁用了异常。因此
//我们关心的所有编译器都足以支持
//值参数化测试。
定义gtest_有_参数_测试1

//确定是否支持类型驱动的测试。

//类型化测试需要<typeinfo>和variadic宏，gcc，vc++8.0，
//Sun Pro CC、IBM Visual Age和HP ACC支持。
如果定义（GNUC35UU）（_MSCU VER>=1400）定义（UUU SUNPROU CC）
    已定义（uu ibmcpp_uuu）已定义（uu hp_acc）
定义GTest_有_型_测试1
定义GTest_有_类型的_测试_p 1
第二节

//确定是否支持combine（）。只有当
//已启用值参数化测试。实现没有
//在Sun Studio上工作，因为它不理解模板化转换
/ /操作者。
如果gtest_有_参数_测试&&gtest_有_tr1_元组&！定义（uusunpro_cc）
定义GTEST诳U有诳U组合1
第二节

//确定系统编译器是否使用UTF-16编码宽字符串。
定义GTEST宽字符串使用UTF16
    （gtest_os_windows_gtest_os_cygwin_gtest_os_symbian_gtest_os_aix）

//确定测试结果是否可以流式传输到套接字。
如果gtest_os_linux
定义GTEST_can_stream_results_1
第二节

//定义一些实用程序宏。

//如果嵌套的“if”语句后跟
//不使用“else”语句和大括号显式消除
//“else”绑定。这会导致代码出现问题，如：
/ /
//（门）
//assert_*（condition）<“some message”；
/ /
//switch（0）case 0:“习语用于抑制这种情况。
ifdef_uu英特尔编译器
定义GTEST_模糊_其他_拦截器_
否则
define gtest_ambiguous_else_blocker_u switch（0）case 0:default://nolint
第二节

//在结构/类定义的末尾使用此注释
//阻止编译器优化不存在
/使用。当所有有趣的逻辑都发生在
//c'tor和/或d'tor。例子：
/ /
//结构foo
//foo（）…}
//gtest_属性_未使用u；
/ /
//还可以在变量或参数声明后使用它来告诉
//编译器不必使用变量/参数。
如果定义了（GNUC）&！定义（编译器）
定义gtest_u属性_unused_uuu属性_uuu（（unused））。
elif已定义
如果uu具有_属性（未使用）
定义gtest_u属性_unused_uuu属性_uuu（（unused））。
第二节
第二节
ifndef gtest_属性_未使用
定义gtest_属性_未使用
第二节

//不允许使用运算符的宏=
//这应该在类的私有声明中使用。
define gtest_disallow_assign_u（type）
  void运算符=（类型const&）

//不允许复制构造函数和运算符的宏=
//这应该在类的私有声明中使用。
define gtest_disallow_copy_and_assign_u（type）
  类型（类型const&）；\
  不允许分配类型

//告诉编译器警告声明函数的未使用返回值
//使用此宏。宏应用于函数声明
//在参数列表后面：
/ /
//sprocket*allocatesprocket（）gtest_必须使用_result_u；
如果定义了（gnuc_uuuu）&&（gtest_gcc_u ver_uux>=30400）&&！定义（编译器）
定义gtest_必须使用_result_uuuu属性_uuuu（（warn_unused_result））。
否则
定义gtest_必须使用结果
endif//uuu gnuc_uu&&（gtest_gcc_ver_u>=30400）&！编译程序

//MS+C++编译器在条件表达式为编译时发出警告。
/常数。在某些情况下，此警告为假阳性，需要
/抑制。在这种情况下，请使用以下两个宏：
/ /
//gtest_intentional_const_cond_push_u（）。
//while（真）
//gtest_intential_const_cond_pop_u（）有意
//}
定义GTEST_意向性_const_cond_push_u（）
    gtest禁用msc警告按钮（4127）
定义GTEST_意向性_const_cond_pop_uu（）
    gtest禁用msc警告

//确定编译器是否支持Microsoft的结构化异常
/处理。这是由几个Windows编译器支持的，但通常
//在任何其他系统上都不存在。
如果ndef gtest_has_seh
用户没有告诉我们，所以我们需要弄清楚。

如果定义了（35u msc35u ver）定义了（35u borlandc_uuuuuu）
//已知这两个编译器支持SEH。
定义gtest_has诳1
否则
//假设没有SEH。
定义GTEST_有\u seh 0
第二节

定义gtest_是_线程安全
    （0）
     （GTest操作系统窗口&！测试Windows电话&！GTEST操作系统
     gtest_has_pthread）

endif//gtest_有

γIFIFF

如果GTEST链接为共享库
定义gtest_api_uuu declspec（dllimport）
elif gtest_创建_共享_库
定义gtest_api_uuu declspec（dllexport）
第二节

endif//_msc版本

ifndef gtest_api_
定义GTEST_API_
第二节

γ-干扰素
//要求编译器从不内联给定函数。
定义gtest_no_inline_uuu attribute_uuu（（no inline））。
否则
定义gtest_no_inline_
第二节

//u libcpp_版本由llvm项目中的libc++库定义。
如果定义了（35; glibcxx定义了（35u libcpp35u版本）
定义GTEST_具有_cxxabi_h_1
否则
定义gtest_具有_cxxabi_h_0
第二节

//一个函数级属性，用于禁用检查是否使用未初始化的
//使用memorysanizer生成时的内存。
如果定义了（clang_）
如果有_U功能（记忆消毒剂）
定义gtest_属性_no_sanitize_memory_uuu
       _uuu attribute_uuu（（无清理内存）
否则
定义gtest_属性_no_sanitize_memory_
endif//u具有_功能（记忆消毒剂）
否则
定义gtest_属性_no_sanitize_memory_
endif/诳clang诳

//用于禁用AddressSanitizer检测的函数级属性。
如果定义了（clang_）
如果有功能（地址消毒剂）
定义gtest_属性_no_sanitize_address_uuu
       _uuu attribute_uuu（（no_sanitize_address））。
否则
定义gtest_属性_no_sanitize_address_
endif//u具有_功能（地址_消毒剂）
否则
定义gtest_属性_no_sanitize_address_
endif/诳clang诳

//用于禁用Threadsanitizer检测的函数级属性。
如果定义了（clang_）
如果有功能（螺纹消毒剂）
定义gtest_属性_no_sanitize_thread_uuu
       _uuu attribute_uuu（（no_sanitize_thread））。
否则
定义gtest_属性_no_sanitize_thread_
endif//uu具有_功能（线程_消毒剂）
否则
定义gtest_属性_no_sanitize_thread_
endif/诳clang诳

命名空间测试

类消息；

如果定义了（gtest_tuple_namespace_u）
//将tuple和friends导入：：testing命名空间。
//它是我们接口的一部分，让它们在：：testing中允许我们更改
//需要时的类型。
使用gtest-tuple-namespace-get；
使用gtest-tuple-namespace-tuple创建tuple；
使用gtest-tuple-name-tuple；
使用gtest-tuple-name-tuple-size；
使用gtest-tuple-namespace-tuple-element；
endif//已定义（gtest_tuple_namespace_uu）

命名空间内部

//谷歌测试用户不知道的秘密类型。它没有
//有目的地定义。因此不可能创造一个
//秘密对象，这就是我们想要的。
阶级秘密；

//gtest_compile_assert_u宏可用于验证编译时
//表达式为真。例如，您可以使用它来验证
//静态数组的大小：
/ /
//gtest_compile_assert_u（gtest_array_size_u（names）==num_name，
//名称_大小不正确）；
/ /
//或确保结构小于特定大小：
/ /
//gtest_compile_assert_u（sizeof（foo）<128，foo_too_large）；
/ /
//宏的第二个参数是变量名。如果
//表达式为false，大多数编译器都会发出警告/错误
//包含变量名。

如果是GTEST诳U lang诳Cxx11
define gtest_compile_assert_u（expr，msg）static_assert（expr，msg）
否则的话！GTEST语言CXx11
模板< BOOL >
  结构编译器
}；

define gtest_compile_assert_u（expr，msg）\
  typedef:：testing:：internal:：compileassert<（static_cast<bool>（expr））>\
      msg[static_cast<bool>（expr）？1:-1]gtest_属性\u未使用\u
我很喜欢你！GTEST语言CXx11

//gtest_compile_assert_的实现细节：
/ /
//（在C++ 11中，我们只使用STATICHYSTART而不是下面的）
/ /
//gtest_compile_assert_u通过定义具有-1的数组类型来工作
//表达式为false时的元素（因此是无效的）。
/ /
//-更简单的定义
/ /
//define gtest_compile_assert_uu（expr，msg）typedef char msg[（expr）？1：1
/ /
//不起作用，因为gcc支持可变长度数组，其大小为
//在运行时确定（这是GCC的扩展，而不是部分扩展）
//的C++标准）。因此，GCC未能拒绝
//遵循简单定义的代码：
/ /
//INFO；
//gtest_compile_assert_u（foo，msg）；//不应编译为foo为
////不是编译时常量。
/ /
//-通过使用compileassert<（bool（expr））>类型，我们确保
//expr是编译时常量。（模板参数必须为
//在编译时确定。）
/ /
//-compileasessert<（bool（expr））>中的外圆括号是必需的
//解决gcc 3.4.4和4.0.1中的错误。如果我们写了
/ /
//compileassert<bool（expr）>
/ /
//相反，这些编译器将拒绝编译
/ /
//gtest_compile_assert_u（5>0，一些_消息）；
/ /
//（他们似乎认为“5>0”中的“>”标志着
//模板参数列表。）
/ /
//-数组大小是（bool（expr）？1:-1），而不是简单的
/ /
//（（expr）？1：- 1）。
/ /
//这是为了避免在MS VC 7.1中遇到错误，因为
//原因（（0.0）？1:-1）不正确地评估为1。

//StaticAssertTypeEqHelper由在gtest.h中定义的StaticAssertTypeEq使用。
/ /
//此模板已声明，但故意未定义。
template<typename t1，typename t2>
结构静态断言类型eqhelper；

模板<typename t>
struct staticasserttypeeqhelper<t，t>
  枚举值=真
}；

//计算“array”中的元素数。
定义gtest_array_size_u（array）（sizeof（array）/sizeof（array[0]）。

如果gtest_具有_global_字符串
typedef：：字符串；
否则
typedef:：std:：string字符串；
endif//gtest_具有_global_字符串

如果GTEST诳U具有诳U Global诳String
typedef:：wstring wstring；
elif gtest_has覕std覕wstring
typedef:：std:：wstring wstring；
endif//gtest_具有全局性

//用于在常量条件下抑制警告的帮助程序。它只是
//返回“condition”。
gtest-api-bool-istree（bool条件）；

//定义作用域指针。

//此作用域指针的实现是部分的-它只包含
//足够的东西来满足谷歌测试的需要。
模板<typename t>
类范围的指针
 公众：
  typedef t元素类型；

  显式作用域指针（t*p=null）：指针
  ~scoped_ptr（）reset（）；

  t&operator*（）常量返回*指针
  t*运算符->（）常量返回指针
  t*get（）常量返回ptr_

  t*Relasee（）{
    t*const ptr=ptr_u；
    pTr=＝NULL；
    返回PTR；
  }

  无效重置（t*p=空）
    如果（P）！= pTr*）{
      if（istue（sizeof（t）>0））//确保t是完整的类型。
        删除PTRI；
      }
      pTr=＝P；
    }
  }

  朋友无效交换（作用域指针&A，作用域指针&B）
    使用std:：swap；
    交换（A.ptr_uuu，B.ptr_u）；
  }

 私人：
  T*PrTr.

  gtest不允许复制和分配（作用域指针）；
}；

/定义RE。

//ReXE.H>的一个简单的C++包装器。它使用POSIX扩展
//正则表达式语法。
等级GTEST-API-RE
 公众：
  //标准要求有一个复制构造函数来初始化对象
  //来自r值的引用。
  re（const re&other）init（other.pattern（））；

  //从字符串构造re。
  re（const:：std:：string&regex）init（regex.c_str（））；//nolint

如果gtest_具有_global_字符串

  re（const:：string&regex）init（regex.c_str（））；//nolint

endif//gtest_具有_global_字符串

  re（const char*regex）init（regex）；//nolint
  ~（）；

  //返回regex的字符串表示形式。
  const char*pattern（）const返回模式

  //fullmatch（str，re）返回真正的iff正则表达式re-matches
  //整个str.
  //partialMatch（str，re）返回true iff正则表达式re
  //匹配str的子字符串（包括str本身）。
  / /
  //todo（wan@google.com）：使fullmatch（）和partialMatch（）工作
  //当str包含nul字符时。
  静态bool fullmatch（const:：std:：string&str，const re&re）
    返回fullmatch（str.c_str（），re）；
  }
  静态bool partialMatch（const:：std:：string&str，const re&re）
    返回partialMatch（str.c_str（），re）；
  }

如果gtest_具有_global_字符串

  静态bool fullmatch（const:：string&str，const re&re）
    返回fullmatch（str.c_str（），re）；
  }
  静态bool partialMatch（const:：string&str，const re&re）
    返回partialMatch（str.c_str（），re）；
  }

endif//gtest_具有_global_字符串

  静态bool fullmatch（const char*str，const re&re）；
  静态bool partialMatch（const char*str，const re&re）；

 私人：
  void init（const char*regex）；

  //我们使用const char*而不是std:：string，就像google测试过去那样
  //当std:：string不可用时使用。todo（wan@google.com）：更改为
  //STD::String。
  const char*模式
  Boer-ISH-ValueS.

如果gtest使用了posix

  对于fullmatch（），regex_t full_regex_；/。
  regex_t partial_regex_；//用于partialMatch（）。

否则//GTEST使用简单的

  const char*full_pattern_；//for fullmatch（）；

第二节

  gtest不允许分配（re）；
}；

//格式化源文件路径和行号
//在用于编译此代码的编译器的错误消息中。
gtest_api_:：std:：string formatfilelocation（const char*文件，int行）；

//为与编译器无关的XML输出格式化文件位置。
//尽管此函数不依赖于平台，但我们将它放在
//格式化文件位置以对比这两个函数。
gtest_api_：：std:：string formatcompilerindependentfilelocation（const char*文件，
                                                               INT线）；

//定义日志记录实用程序：
//gtest_log_u（severity）-以指定的严重性级别记录消息。这个
//消息本身被流式处理到宏中。
//log to stderr（）-将所有日志消息定向到stderr。
//flushinfolog（）-刷新信息日志消息。

枚举gtestlogseverity
  GestSyfIn，
  警告，
  格氏错误
  致命死亡
}；

//格式化日志条目严重性，提供流对象以流式处理
//记录消息，退出时用换行符终止消息
/ /范围。
类gtest_api_ugtestlog_
 公众：
  gtestlog（gtestlog severity severity，const char*文件，int行）；

  //刷新缓冲区，如果严重性为gtest_fatal，则中止程序。
  ~GTestLogo（）；

  ：：std:：ostream&getstream（）返回：：std:：cerr；

 私人：
  const gtestlogseverity严重性

  不允许复制和分配（gtestlog）；
}；

定义gtest_log_uu（严重性）
    ：：测试：：内部：：GTestLog（：：测试：：内部：：GTest严重性，\
                                  _uuu file_uuuu，uuu line_uuu）.getstream（）。

inline void logtostderr（）
inline void flushinfolog（）fflush（空）；

//内部实现-不要使用。
/ /
//gtest_check_u是一个全模式断言。如果条件为
//不满意。
//Syopops:
//gtest_check_u（布尔_条件）；
/或
//gtest_check_u（boolean_condition）<“additional message”；
/ /
//检查条件，如果条件不满足
//它打印有关条件冲突的消息，包括
//条件本身，再加上流入它的附加消息（如果有），
//然后中止程序。它中止了程序，不管
//是否在调试模式下生成。
定义GTEST_检查_u（条件）
    Gtest_模棱两可\其他\阻滞剂\uuuuu \
    如果（：：测试：：内部：：IStree（条件））\
      \
    否则
      gtest_log_u（fatal）<“condition”condition“failed”.

//一个all-mode断言来验证给定的posix样式函数
//调用返回0（表示成功）。已知限制：此
//不扩展到平衡的“if”语句，因此将宏括起来
//在中，如果需要将其用作“if”中的唯一语句
//分支。
定义GTEST_检查_posix_成功_uu（posix_调用） \
  if（const int gtest_error=（posix_call））\
    gtest_log_u（fatal）<posix_call<“failed with error”\
                      <测试错误

如果gtest_有_std_move_
使用std：：move；
否则//gtest_有诳std_move_
模板<typename t>
施工和移动（施工和运输）
  返回T；
}
endif//gtest_有_std_move_

//内部实现-不要在用户代码中使用。
/ /
//将implicitcast用作安全版本的静态\ cast，以便在
//类型层次结构（例如，将foo*强制转换为superclasssoffoo*或
//常量foo*）。当使用implicitcast时，编译器会检查
//演员阵容是安全的。这种明确的暗示是必要的
//令人惊讶的是许多情况下C++需要精确的类型匹配。
//而不是可转换为目标类型的参数类型。
/ /
//使用implicitcast_u的语法与使用static_cast的语法相同：
/ /
//implicitcast_uu<totype>（expr）
/ /
//IpimCistCar将是C++标准库的一部分，
但提案提交得太晚了。它可能会
//它将成为未来的语言。
/ /
//这个相对丑陋的名字是故意的。它可以防止与
//用户可能具有类似的功能（例如隐式强制转换）。内部
//仅命名空间是不够的，因为ADL可以找到函数。
模板<typename to>
内联到implicitcast_u（to x）返回：：测试：：内部：：移动（x）；

//向上转换时（即，将指针从foo类型转换为type
//superclasssofoo），可以使用implicitcast，因为upcast
//总是成功的。当你向下投射（也就是说，从
//type foo to type subclassoffoo），static_cast<>不安全，因为
//你怎么知道指针是subclassoffoo类型的？它
//可以是裸foo，也可以是differentSubclassOffoo类型。因此，
//当您降级时，应该使用此宏。在调试模式下，我们
//使用dynamic_cast<>再次检查下行是否合法（我们死了
如果不是的话）。在正常模式下，我们进行有效的静态投射<>
代替。因此，在调试模式下测试以确保
//演员阵容合法！
//这是代码中我们应该使用的唯一位置动态_cast<>。
//特别是，为了
//执行rtti（例如这样的代码：
//if（dynamic_cast<subclass1>（foo））handleSubclass1对象（foo）；
//如果（dynamic_cast<subclass2>（foo））handleSubclass2对象（foo）；
//您应该以其他方式设计代码，而不需要这样做。
/ /
//这个相对丑陋的名字是故意的。它可以防止与
//用户可能具有类似的功能（例如，向下强制转换）。内部
//仅命名空间是不够的，因为ADL可以找到函数。
template<typename-to，typename-from>//如下使用：downcast_<t*>（foo）；
inline到downcast_uu（from*f）//因此我们只接受指针
  //确保to是from*的子类型。这个测试只在这里
  //用于编译时类型检查，并且在
  //在运行时优化构建，因为它将被优化掉
  完全。
  GTEST_意向性构建条件推送（
  如果（false）{
  故意制造条件
    const to to=空；
    ：：测试：：内部：：ImplicitCast_<From*>（To）；
  }

如果GTEST覕u有覕rtti
  //rtti:仅限调试模式！
  gtest_check_u（f==null dynamic_cast<to>（f）！= NULL）；
第二节
  返回static_cast<to>（f）；
}

//将类型base的指针向下强制转换为派生的。
//派生必须是基的子类。参数必须
//指向派生类型的类，而不是它的任何子类。
//当rtti可用时，函数执行运行时
//检查以强制执行此操作。
template<class derived，class base>
派生*检查到实际类型（基*基）
如果GTEST覕u有覕rtti
  gtest_check_u（typeid（*base）==typeid（derived））；
  返回dynamic_cast<derived*>（base）；//nolint
否则
  返回static_cast<derived*>（base）；//可怜人的downcast。
第二节
}

如果gtest_具有_stream_重定向

//定义stderr capturer:
//capturestdout-开始捕获stdout。
//getcapturedsdout-停止捕获stdout并返回捕获的字符串。
//capturestderr-开始捕获stderr。
//getcapturedsderr-停止捕获stderr并返回捕获的字符串。
/ /
gtest_api_uuvoid capturestdout（）；
gtest_api_u std:：string getCapturedDSdout（）；
gtest_api_uuvoid capturestderr（）；
gtest_api_u std:：string getcapturedsderr（）；

endif//gtest_具有_stream_重定向


如果GTEST覕U有覕死亡覕测试

const:：std:：vector<testing:：internal:：string>&getInjectableArgvs（）；
void setInjectableArgvs（const:：std:：vector<testing:：internal:：string>>
                             纽瓦尔加斯）；

//所有命令行参数的副本。由initGoogleTest（）设置。
extern:：std:：vector<testing:：internal:：string>g_argvs；

endif//gtest_具有_死亡_测试

//定义同步原语。
如果GTEST覕U是覕THREADSAFE
如果GTEST有螺纹
//休眠（大约）n毫秒。此功能仅用于测试
//Google测试自己的构造。也不要在用户测试中使用它
//直接或间接。
inline void sleepmillises（int n）
  const timespec time=
    0，//0秒。
    N*1000L*1000L，//和N ms。
  }；
  纳秒级（&time，空）；
}
endif//gtest_has诳pthread

如果0//OS检测
伊利夫·GTEST拥有螺纹
//允许控制器线程暂停执行新创建的
//线程，直到通知。必须创建此类的实例
//并在控制器线程中销毁。
/ /
//此类仅用于测试Google测试自己的构造。不
//直接或间接地在用户测试中使用它。
班级通知
 公众：
  notification（）：已通知_u（false）
    gtest_check_posix_success_u（pthread_mutex_init（&mutex_u，null））；
  }
  ~通知（）
    pthread_mutex_destroy（&mutex_u）；
  }

  //通知用此通知创建的所有线程启动。必须
  //从控制器线程调用。
  无效通知（）
    pthread_mutex_lock（&mutex_u）锁定；
    已通知=true；
    pthread_mutex_unlock（&mutex_u）解锁；
  }

  //在控制器线程通知之前阻止。必须从测试调用
  //线程。
  void waitForNotification（）
    为（；；）{
      pthread_mutex_lock（&mutex_u）锁定；
      const bool notified=已通知
      pthread_mutex_unlock（&mutex_u）解锁；
      如果（通知）
        断裂；
      睡眠毫秒（10）；
    }
  }

 私人：
  pthread_mutex_t mutex_；
  BoOL NoTiFidEd:

  不允许复制和分配通知；
}；

elif gtest_os_windows&！测试Windows电话&！测试操作系统窗口

gtest_api_uuvoid sleep毫秒（int n）；

//提供防漏Windows内核句柄所有权。
//用于死亡测试和线程支持。
类gtest_api_u autohandle_
 公众：
  //假设win32句柄类型等效于void*。这样做可以让我们
  //避免在此头文件中包含<windows.h>。包括<windows.h>is
  //不受欢迎，因为它定义了大量的符号和宏
  //与客户端代码冲突。这一假设由
  //windowstypestest.handleisvoidstar。
  typedef void*句柄；
  AutoHealHead（）；
  显式自动句柄（句柄句柄）；

  ~AutoHead（）；

  handle get（）常量；
  空复位（）；
  空位复位（手柄）；

 私人：
  //如果句柄是可以关闭的有效句柄对象，则返回true。
  bool iscloseable（）常量；

  手柄；

  gtest不允许复制和分配（autohandle）；
}；

//允许控制器线程暂停执行新创建的
//线程，直到通知。必须创建此类的实例
//并在控制器线程中销毁。
/ /
//此类仅用于测试Google测试自己的构造。不
//直接或间接地在用户测试中使用它。
GTEST类API通知
 公众：
  通知（）；
  无效通知（）；
  void waitForNotification（）；

 私人：
  自动处理事件

  不允许复制和分配通知；
}；
endif//操作系统检测

//在mingw上，我们可以同时使用gtest-os-windows和gtest-has-pthread
//已定义，但我们不想使用mingw的pthreads实现，
//与某些版本的POSIX标准存在一致性问题。
如果GTEST有螺纹&！测试窗口

//作为C函数，ThreadFuncWithClinkage本身不能模板化。
//因此，它无法选择正确的ThreadWithParam实例化
//以调用其run（）。将ThreadWithParamBase作为
//ThreadWithParam的非模板化基类允许我们绕过此
/问题。
类threadWithParamBase_
 公众：
  虚拟~threadWithParamBase（）
  虚拟void run（）=0；
}；

//pthread_create（）接受指向具有C链接的函数类型的指针。
//根据标准（7.5/1），不同链接的功能类型
//是不同的，即使它们在其他方面是相同的。一些编译器（用于
//例如，sunstudio）将它们视为不同的类型。自类方法
//不能用我们需要定义一个自由C函数的C链接来定义
//传递到pthread_create（）。
外部“C”内联void*threadfuncwithclinkage（void*thread）
  static_cast<threadWithParamBase*>（thread）->run（）；
  返回空；
}

//helper类，用于测试Google测试的多线程结构。
//要使用它，请写：
/ /
//void threadfunc（int param）/*使用param执行操作*/ }

//通知线程可以启动；
//…
////threadou canou start参数是可选的，可以提供空值。
//threadWithParam<int>thread（&threadFunc，5，&thread_can_start）；
//线程_can_start.notify（）；
//
//这些类只用于测试Google测试自己的构造。做
//不要直接或间接地在用户测试中使用它们。
template <typename T>
class ThreadWithParam : public ThreadWithParamBase {
 public:
  typedef void UserThreadFunc(T);

  ThreadWithParam(UserThreadFunc* func, T param, Notification* thread_can_start)
      : func_(func),
        param_(param),
        thread_can_start_(thread_can_start),
        finished_(false) {
    ThreadWithParamBase* const base = this;
//只能在所有字段之后创建线程，除了线程\
//已初始化。
    GTEST_CHECK_POSIX_SUCCESS_(
        pthread_create(&thread_, 0, &ThreadFuncWithCLinkage, base));
  }
  ~ThreadWithParam() { Join(); }

  void Join() {
    if (!finished_) {
      GTEST_CHECK_POSIX_SUCCESS_(pthread_join(thread_, 0));
      finished_ = true;
    }
  }

  virtual void Run() {
    if (thread_can_start_ != NULL)
      thread_can_start_->WaitForNotification();
    func_(param_);
  }

 private:
UserThreadFunc* const func_;  //用户提供的线程函数。
const T param_;  //用户为线程函数提供的参数。
//非空时，用于阻止执行，直到控制器线程
//通知。
  Notification* const thread_can_start_;
bool finished_;  //如果我们知道线程函数已经完成，则返回true。
pthread_t thread_;  //本机线程对象。

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ThreadWithParam);
};
# endif  //GTEST有线程&！测试窗口

# if 0  //操作系统检测
# elif GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_PHONE && !GTEST_OS_WINDOWS_RT

//mutex在Windows平台上实现mutex。它与
//使用类mutexlock:
//
//Mutex mutex；
//…
//mutex lock lock（&mutex）；//获取mutex并在
////当前作用域的结尾。
//
//静态互斥体*必须使用以下方法之一定义或声明
//宏指令：
//gtest定义静态互斥（g_一些互斥）；
//gtest-declare-static-mutex（g-some-mutex）；
//
//（非静态互斥体以通常的方式定义/声明）。
class GTEST_API_ Mutex {
 public:
  enum MutexType { kStatic = 0, kDynamic = 1 };
//我们依赖于kstaticmutex为0，因为它是链接器初始化的对象。
//在静态互斥体中键入_u。关键部分将被延迟初始化
//在threadSafeLazyInit（）中。
  enum StaticConstructorSelector { kStaticMutex = 0 };

//这个构造函数故意不做任何事情。它依赖于类型
//静态初始化为0（有效地将其设置为ksstatic）并启用
//threadSafeLazyInit（）以延迟初始化其余成员。
  /*LICIT互斥（staticConstructorSelector/*dummy*/）

  MutExter（）；
  ~ MutExter（）；

  空锁（）；

  空隙解锁（）；

  //如果当前线程持有互斥体，则不执行任何操作。否则，崩溃
  //很有可能。
  void asserthold（）；

 私人：
  //初始化静态互斥体中的所有者线程和关键部分。
  void threadSafeLazyInit（）；

  //根据http://blogs.msdn.com/b/oldnewthing/archive/2004/02/23/78395.aspx，
  //我们假设0是线程ID的无效值。
  unsigned int owner_thread_id_u；

  //对于静态互斥体，我们依赖于将这些成员初始化为零
  //通过链接器。
  mutextype类型；
  长关键段初始化阶段
  _rtl_critical_section*critical_section ；

  gtest不允许复制和分配（mutex）；
}；

define gtest_declare_static_mutex_u（mutex）声明\
    外部：：测试：：内部：：互斥体

define gtest_define_static_mutex_u（mutex）
    ：：测试：：内部：：mutex mutex（：：测试：：内部：：mutex:：kstaticmutex）

//我们不能将此类命名为mutexlock，因为ctor声明将
//与名为mutexlock的宏冲突，该宏在
/ /平台。这个宏被用作防御措施来防止
//无意中误用了mutexlock，如“mutexlock（&mu）”，而不是
//“mutexlock l（&mu）”。因此下面的typedef技巧。
类别GTestMutexLock
 公众：
  显式gtestmutexlock（mutex*mutex）
      ：mutex_u（mutex）mutex_u->lock（）；

  ~gtestmutexlock（）mutex_->unlock（）；

 私人：
  mutex*const mutex_u；

  gtest不允许复制和分配（gtestmutexlock）；
}；

typedef gtestmutexlock互斥锁；

//valueholder的基类<t>。允许调用方保留和删除值
//不知道它的类型。
类ThreadLocalValueHolderBase
 公众：
  虚拟~threadLocalValueHolderBase（）
}；

//提供线程向线程本地发送通知的方法
//不考虑其参数类型。
类ThreadLocalBase_
 公众：
  //创建一个新的valueholder对象，该对象保存传递给
  //此ThreadLocal的构造函数并返回它。是打电话的
  //当ThreadLocal<t>实例已经存在时，不调用此函数的责任
  //在当前线程上有一个值。
  虚拟线程localValueHolderBase*newValueForCurrentThread（）const=0；

 受保护的：
  threadLocalBase（）
  虚拟~threadLocalBase（）

 私人：
  gtest不允许复制和分配（threadlocalbase）；
}；

//将一个线程映射到一组线程局部变量，这些线程局部变量具有在该线程上实例化的值
//线程，并在线程退出时通知它们。线程本地实例是
//应一直保持，直到其上具有值的所有线程都已终止。
类gtest_api_uuthreadlocalregistry_
 公众：
  //在当前线程上将线程\本地\实例注册为具有值。
  //返回一个可用于从其他线程标识线程的值。
  静态线程LocalValueHolderBase*GetValueOnCurrentThread（
      const threadlocalbase*线程\u本地\u实例）；

  //在销毁ThreadLocal实例时调用。
  静态void OnThreadLocalDestroyed（
      const threadlocalbase*线程\u本地\u实例）；
}；

类gtest_api_uuThreadWithParamBase_
 公众：
  空连接（）；

 受保护的：
  类可运行
   公众：
    虚拟~runnable（）
    虚拟void run（）=0；
  }；

  threadwithparambase（runnable*runnable，notification*thread_can_start）；
  虚拟~threadWithParamBase（）；

 私人：
  自动手柄螺纹
}；

//helper类，用于测试Google测试的多线程结构。
模板<typename t>
类ThreadWithParam:公共ThreadWithParamBase_
 公众：
  typedef void用户线程函数（t）；

  threadwithparam（用户thread func*func，t参数，通知*线程\可以启动）
      ：threadWithParamBase（new runnableImpl（func，param），线程_can_start）
  }
  虚拟~threadwitparam（）

 私人：
  类runnableimpl:公共可运行
   公众：
    runnableimpl（用户线程函数*func，t参数）
        函数（FUNC），
          PARAMM（PARAM）{
    }
    虚拟~runnableimpl（）
    virtual void run（）
      函数（PARAMI）；
    }

   私人：
    用户线程功能*常量功能
    常数T；

    gtest不允许复制和分配（runnableimpl）；
  }；

  gtest不允许复制和分配（threadwithParam）；
}；

//在Windows系统上实现线程本地存储。
/ /
///线程1
//threadlocal<int>tl（100）；//100是每个线程的默认值。
/ /
///线程2
//tl.set（150）；//仅更改线程2的值。
//期望_eq（150，tl.get（））；
/ /
///线程1
//expect_eq（100，tl.get（））；//在线程1中，tl具有原始值。
//tl.set（200）；
//期望_eq（200，tl.get（））；
/ /
//模板类型参数t必须具有公共复制构造函数。
//另外，默认的threadlocal构造函数要求t
//公共默认构造函数。
/ /
//本地实例的用户必须确保
//使用该实例的线程（包括主线程）以前已退出
//摧毁它。否则，由
//线程本地实例不能保证在所有平台上都被销毁。
/ /
//Google测试只使用全局线程本地对象。这意味着他们
//将在main（）返回后死亡。因此，每个线程没有
//只要所有线程，google test管理的对象就会泄漏
//当main（）返回时，使用google测试已经退出。
模板<typename t>
类ThreadLocal:公共ThreadLocalBase_
 公众：
  threadlocal（）：默认值_（）
  显式threadlocal（const t&value）：默认值

  ~threadlocal（）threadlocallegistry:：onThreadLocalDestroyed（this）；

  t*pointer（）返回getorCreateValue（）；
  const t*pointer（）const返回getorCreateValue（）；
  const&get（）const return*pointer（）；
  void set（const t&value）*pointer（）=值；

 私人：
  //保持t的值。可以通过其基类删除，而不需要调用方
  //知道T的类型。
  类值持有者：公共线程本地值持有者数据库
   公众：
    显式值持有人（const&value）：值（value）

    t*pointer（）返回&value_

   私人：
    T值；
    GTEST不允许复制和分配（价值持有人）；
  }；


  t*getorCreateValue（）常量
    返回static_cast<valueholder*>（）
        threadLocalRegistry:：getValueOnCurrentThread（this））->pointer（）；
  }

  虚拟线程localValueHolderBase*newValueForCurrentThread（）const_
    返回新的值持有人（默认值）；
  }

  const t default_；//每个线程的默认值。

  gtest不允许复制和分配（threadlocal）；
}；

伊利夫·GTEST拥有螺纹

//mutexbase和mutex在基于pthreads的平台上实现mutex。
类mutexbase_
 公众：
  //获取该互斥体。
  空锁（）
    gtest_check_posix_success（pthread_mutex_lock（&mutex_））；
    所有者_u=pthread_self（）；
    拥有“所有者”=true；
  }

  //释放这个互斥体。
  空隙解锁（）
    //由于正在释放锁，因此所有者字段不应再为
    //被认为是有效的。我们不保护写信给这里的所有者
    //调用方确保当前线程拥有
    //调用此函数时互斥。
    拥有“所有者”=false；
    gtest_check_posix_success（pthread_mutex_unlock（&mutex_））；
  }

  //如果当前线程持有互斥体，则不执行任何操作。否则，崩溃
  //很有可能。
  void asserthold（）常量
    gtest_check_u（has_owner_u&&pthread_equal（owner_uu，pthread_self（）））
        <“当前线程没有保持互斥体@”<<this；
  }

  //在输入main（）之前，可以使用静态互斥体。甚至可以
  //在动态初始化阶段之前使用。因此我们
  //必须能够在链接时初始化静态互斥对象。
  //这意味着mutexbase必须是pod及其成员变量
  必须公开。
 公众：
  pthread_mutex_t mutex_；//底层pthread mutex。
  //has_owner_u指示下面的owner_u字段是否包含有效的线程
  //ID，因此可以安全地进行检查（例如，在pthread_equal（）中使用）。所有
  //应该通过检查该字段来保护对所有者字段的访问。
  //另一种选择可能是memset（）owner_u to all zero，但没有
  //确保零d pthread_t必然无效，甚至不同
  //来自pthread_self（）。
  布尔拥有所有者；
  pthread_t owner_u；//保存互斥体的线程。
}；

//Forward声明静态互斥体。
define gtest_declare_static_mutex_u（mutex）声明\
     外部：：测试：：内部：：mutexbase mutex

//定义静态（即在链接时）初始化静态互斥。
//此处的初始化列表没有显式初始化每个字段，
//而依赖于未指定字段的默认初始化。在
//特别是，所有者字段（pthread）未显式初始化。
//这允许初始化工作，无论pthread_t是标量还是结构。
//不能为此指定标志-wMissing字段初始值设定项以使其工作。
define gtest_define_static_mutex_u（mutex）
     ：：测试：：内部：：mutexbase mutex=pthread_mutex_初始值设定项，false_

//mutex类只能用于运行时创建的mutex。它
//否则与mutexbase共享其API。
类互斥体：公共互斥体数据库
 公众：
  互斥（{）
    gtest_check_posix_success_u（pthread_mutex_init（&mutex_u，null））；
    拥有“所有者”=false；
  }
  ~ MutExx（）{
    gtest_check_posix_success（pthread_mutex_destroy（&mutex_））；
  }

 私人：
  gtest不允许复制和分配（mutex）；
}；

//我们不能将此类命名为mutexlock，因为ctor声明将
//与名为mutexlock的宏冲突，该宏在
/ /平台。这个宏被用作防御措施来防止
//无意中误用了mutexlock，如“mutexlock（&mu）”，而不是
//“mutexlock l（&mu）”。因此下面的typedef技巧。
类别GTestMutexLock
 公众：
  显式gtestmutexlock（mutexbase*mutex）
      ：mutex_u（mutex）mutex_u->lock（）；

  ~gtestmutexlock（）mutex_->unlock（）；

 私人：
  mutexbase*const互斥

  gtest不允许复制和分配（gtestmutexlock）；
}；

typedef gtestmutexlock互斥锁；

//ThreadLocal的帮助程序。

//pthread_key_create（）要求deleteShreadLocalValue（）具有
/c连接。因此，不能将其模板化以访问
//threadlocal<t>。因此需要上课
//threadLocalValueHolderBase。
类ThreadLocalValueHolderBase
 公众：
  虚拟~threadLocalValueHolderBase（）
}；

//由pthread调用以删除存储的线程本地数据
//pthread_setspecific（）。
外部“C”内联void deletethreadlocalvalue（void*value_holder）
  删除static_cast<threadLocalValueHolderBase*>（value_holder）；
}

//在基于pthreads的系统上实现线程本地存储。
模板<typename t>
class threadlocal_
 公众：
  threadLocal（）：键_u（createKey（）），
                  Debug（）{}
  显式threadlocal（const t&value）：key_u（createkey（）），
                                         默认值

  ~threadlocal（）
    //销毁当前线程的托管对象（如果有）。
    删除线程本地值（pthread_getspecific（key_u））；

    //释放与键关联的资源。这不会*
    //删除其他线程的托管对象。
    gtest_check_posix_success_u（pthread_key_delete（key_u））；
  }

  t*pointer（）返回getorCreateValue（）；
  const t*pointer（）const返回getorCreateValue（）；
  const&get（）const return*pointer（）；
  void set（const t&value）*pointer（）=值；

 私人：
  //保存t类型的值。
  类值持有者：公共线程本地值持有者数据库
   公众：
    显式值持有人（const&value）：值（value）

    t*pointer（）返回&value_

   私人：
    T值；
    GTEST不允许复制和分配（价值持有人）；
  }；

  静态pthread_key_t createkey（）
    pthread_key_t key；
    //当线程退出时，将调用DeleteShreadLocalValue（）。
    //为该线程管理的对象。
    测试检查位置成功（
        pthread_key_create（&key，&deletethreadlocalvalue））；
    返回键；
  }

  t*getorCreateValue（）常量
    threadLocalValueHolderBase*常量Holder=
        static_cast<threadLocalValueHolderBase*>（pthread_GetSpecific（key_uu））；
    如果（持有者）！{NULL）{
      return checkeddowncasttoactualType<valueholder>（holder）->pointer（）；
    }

    valueholder*const new_holder=新的valueholder（默认值）；
    threadLocalValueHolderBase*const holder_base=新的_holder；
    gtest_check_posix_success_uu（pthread_setspecific（key_uu，holder_base））；
    返回新的_holder->pointer（）；
  }

  //键pthreads用于查找每个线程的值。
  const pthread_key_t key_u；
  const t default_；//每个线程的默认值。

  gtest不允许复制和分配（threadlocal）；
}；

endif//操作系统检测

否则//GTEST诳U是诳THREADSAFE

//同步原语（mutex、lock、
//和线程局部变量）。编译谷歌测试所必需的
//不支持互斥体-不支持在多个线程中使用Google测试
//在此类平台上支持。

类互斥
 公众：
  互斥（）{}
  空锁（）{}
  void unlock（）
  void asserthold（）常量
}；

define gtest_declare_static_mutex_u（mutex）声明\
  外部：：测试：：内部：：互斥体

define gtest_define_static_mutex（mutex）：：测试：：内部：：mutex mutex

//我们不能将此类命名为mutexlock，因为ctor声明将
//与名为mutexlock的宏冲突，该宏在
/ /平台。这个宏被用作防御措施来防止
//无意中误用了mutexlock，如“mutexlock（&mu）”，而不是
//“mutexlock l（&mu）”。因此下面的typedef技巧。
类别GTestMutexLock
 公众：
  显式gtestmutexlock（mutex*）//nolint
}；

typedef gtestmutexlock互斥锁；

模板<typename t>
class threadlocal_
 公众：
  threadlocal（）：值_（）
  显式threadlocal（const t&value）：值_u（value）
  t*pointer（）返回&value_
  const t*pointer（）const返回&value_
  const&get（）const返回值
  void set（const t&value）value_=value；
 私人：
  T值；
}；

endif//gtest_是诳threadsafe

//返回进程中运行的线程数，或0表示
//我们检测不到它。
gtest_api_u size_t getthreadcount（）；

//通过省略号（…）传递非pod类会导致ARM崩溃
//编译器并在Sun Studio中生成警告。诺基亚塞班
//IBM xl C/C++编译器尝试实例化复制构造函数。
//对于通过省略号（…）传递的对象，由于不可复制而失败
//对象。我们定义这一点是为了确保只通过POD
//这些系统上的省略号。
如果已定义（uu-Symbian32_uuu）已定义（u-Ibmcpp_uuuu）已定义（u-SunPro_-CC）
//如果编译器不喜欢空检测，我们将失去对空检测的支持
//通过省略号（…）传递非pod类。
定义gtest_省略号_需要_pod_1
否则
定义gtest_可以_比较_空1
第二节

//诺基亚Symbian和IBM XL C/C++编译器无法决定
//函数模板中的const和const。这些编译器
//u可以在t和t*的类模板专用化之间作出决定，
//因此，tr1：：type_traits-like is_pointer有效。
如果定义了（uu symbian32_uuuuu）定义了（uu ibmcpp_uuu）
定义GTEST_需求_为_指针_1
第二节

template<bool bool_value>
结构布尔常数
  typedef bool_constant<bool_value>type；
  静态常量bool value=bool_value；
}；
template<bool bool_value>const bool bool_constant<bool_value>：：value；

typedef bool_constant<false>false_type；
typedef bool_constant<true>true_type；

模板<typename t>
结构为_pointer:public false_type_

模板<typename t>
struct is_pointer<t*>：public true_type_

template<typename迭代器>
结构迭代器
  typedef typename迭代器：：value_type value_type；
}；

模板<typename t>
struct iteratortraits<t*>
  typedef t值\u类型；
}；

模板<typename t>
struct iteratortraits<const t*>
  typedef t值\u类型；
}；

如果GTEST操作系统窗口
定义GTEST路径
定义GTEST_有_alt_路径_sep_1
//编译器支持的最大有符号整数类型。
typedef？int64 biggestent；
否则
定义gtest_路径_sep_uu“/”
定义gtest_有_alt_路径_sep_0
typedef long long biggestent；//nolint
endif//gtest_os_windows

//用于char的实用程序。

//isspace（int ch）和friends接受无符号char或eof。烧焦
//可以签名，具体取决于编译器（或编译器标志）。
//因此，在调用
//isspace（）等。

内联bool isalpha（char ch）
  返回isalpha（static_cast<unsigned char>（ch））！＝0；
}
内联bool isalnum（char ch）
  返回isalnum（static_cast<unsigned char>（ch））！＝0；
}
内联bool isdigit（char ch）
  返回isdigit（static_cast<unsigned char>（ch））！＝0；
}
内联bool islower（char ch）
  返回islower（static_cast<unsigned char>（ch））！＝0；
}
内联bool isspace（char ch）
  返回isspace（static-cast<unsigned char>（ch））！＝0；
}
内联bool isupper（char ch）
  返回isupper（static_cast<unsigned char>（ch））！＝0；
}
内联bool isxdigit（char ch）
  返回isxdigit（static_cast<unsigned char>（ch））！＝0；
}
内联bool isxdigit（wchar_t ch）
  const unsigned char low_byte=static_cast<unsigned char>（ch）；
  返回ch==低字节&&isxdigit（低字节）！＝0；
}

内联char tolower（char ch）
  返回static_cast<char>（tolower（static_cast<unsigned char>（ch））；
}
内联char toupper（char ch）
  返回static_cast<char>（toupper（static_cast<unsigned char>（ch））；
}

inline std:：string striptrailingspaces（std:：string str）
  std:：string：：迭代器it=str.end（）；
  尽管如此！=str.begin（）&&isspace（*--it））
    它=str.erase（它）；
  返回STR；
}

//测试：：internal:：posix命名空间包含公用包装器
//posix函数。这些包装纸掩盖了
//windows/msvc和posix系统。因为有些编译器定义了这些
//标准函数作为宏，包装不能具有相同的名称
//作为包装函数。

命名空间posix

//在Windows上使用不同名称的函数。

如果GTEST操作系统窗口

typedef结构\u stat stat struct；

ifdef_uuu Borlandc_uu
inline int isatty（int fd）返回isatty（fd）；
inline int strcasecmp（const char*s1，const char*s2）
  返回stricmp（s1，s2）；
}
inline char*strdup（const char*src）返回strdup（src）；
否则的话！阿尔巴朗克
如果GTEST操作系统Windows移动
内联int isatty（int/*fd*/) { return 0; }

#  else
inline int IsATTY(int fd) { return _isatty(fd); }
#  endif  //GTEST操作系统Windows移动
inline int StrCaseCmp(const char* s1, const char* s2) {
  return _stricmp(s1, s2);
}
inline char* StrDup(const char* src) { return _strdup(src); }
# endif  //阿尔巴朗克

# if GTEST_OS_WINDOWS_MOBILE
inline int FileNo(FILE* file) { return reinterpret_cast<int>(_fileno(file)); }
//在Windows CE上不需要stat（）、rmdir（）和isdir（）。
//时间，因此这里没有定义。
# else
inline int FileNo(FILE* file) { return _fileno(file); }
inline int Stat(const char* path, StatStruct* buf) { return _stat(path, buf); }
inline int RmDir(const char* dir) { return _rmdir(dir); }
inline bool IsDir(const StatStruct& st) {
  return (_S_IFDIR & st.st_mode) != 0;
}
# endif  //GTEST操作系统Windows移动

#else

typedef struct stat StatStruct;

inline int FileNo(FILE* file) { return fileno(file); }
inline int IsATTY(int fd) { return isatty(fd); }
inline int Stat(const char* path, StatStruct* buf) { return stat(path, buf); }
inline int StrCaseCmp(const char* s1, const char* s2) {
  return strcasecmp(s1, s2);
}
inline char* StrDup(const char* src) { return strdup(src); }
inline int RmDir(const char* dir) { return rmdir(dir); }
inline bool IsDir(const StatStruct& st) { return S_ISDIR(st.st_mode); }

#endif  //GTEST操作系统窗口

//函数已被MSVC 8.0弃用。

/*st_disable_msc_warnings_push_u（4996/*已弃用函数*/）

inline const char*strncpy（char*dest，const char*src，size_）
  返回strncpy（dest，src，n）；
}

//chdir（）、freopen（）、fdopen（）、read（）、write（）、close（）和
//此时在Windows CE上不需要strError（），因此不需要
//在那里定义。

如果有的话！测试Windows Mobile&！测试Windows电话&！测试操作系统窗口
inline int chdir（const char*dir）返回chdir（dir）；
第二节
内联文件*fopen（const char*路径，const char*模式）
  返回fopen（路径、模式）；
}
如果有的话！GTEST操作系统Windows移动
内联文件*freopen（const char*路径，const char*模式，文件*stream）
  返回freopen（路径、模式、流）；
}
内联文件*fdopen（int fd，const char*模式）返回fdopen（fd，模式）；
第二节
inline int fclose（file*fp）返回fclose（fp）；
如果有的话！GTEST操作系统Windows移动
inline int read（int fd，void*buf，unsigned int count）
  返回static_cast<int>（read（fd，buf，count））；
}
inline int write（int fd，const void*buf，unsigned int count）
  返回static_cast<int>（write（fd，buf，count））；
}
内联int close（int fd）返回close（fd）；
inline const char*strerrror（int errnum）返回strerrror（errnum）；
第二节
inline const char*getenv（const char*name）
如果gtest_os_windows_mobile_gtest_os_windows_phone_gtest_os_windows_rt
  //我们在Windows CE上，它没有环境变量。
  static_cast<void>（name）；//以防止“unused argument”警告。
  返回空；
elif defined（uuo borlandc_uuu）defined（u sunos_5_8）defined（u sunos_5_9）
  //以编程方式清除的环境变量将设置为
  //空字符串而不是unset（空）。处理那个案子。
  const char*const env=getenv（名称）；
  返回！=空&&env[0]！=‘0’）？Env：NULL；
否则
  返回getenv（name）；
第二节
}

gtest禁用msc警告

如果GTEST操作系统Windows移动
//Windows CE没有C库。abort（）函数用于
//谷歌测试中的几个地方。这个实现提供了一个合理的
//模仿标准行为。
无效中止（）；
否则
inline void abort（）abort（）；
endif//gtest_os_windows_mobile

//命名空间posix

//msvc“deprecates”snprintf并在使用时发出警告。在
//为了避免这些警告，我们需要使用_snprintf或_snprintf_s on
//基于MSVC的平台。我们将gtest_snprintf_uu宏映射到适当的
//为了实现这一点。我们在这里使用宏定义是因为
//snprintf是一个变量函数。
如果诳msc诳ver>=1400&！GTEST操作系统Windows移动
//MSVC 2005及以上版本支持variadic宏。
定义gtest_snprintf_u（缓冲区、大小、格式等）
     _snprintf_s（缓冲区、大小、大小、格式、参数）
elif定义（35u msc35u ver）
//Windows CE没有定义_snprintf_s，2005年之前的msvc没有定义
//抱怨_snprintf。
定义gtest_snprintf_uu snprintf
否则
定义gtest_snprintf_u snprintf
第二节

//最大值。这个定义
//无论在补语中表示的是什么都有效，或者
//二的补码。
/ /
//我们不能依赖STL中的数值_限制，如_uu int64和long long
/不是标准C++的一部分，数字限制不需要
//为它们定义。
常量biggestnt kmaxbiggestent=
    ~（static_cast<biggestent>（1）<（8*sizeof（biggestent）-1））；

//此模板类用作从大小到
//类型。它将以字节为单位的大小映射为基元类型
/尺寸。例如
/ /
//typewithsize<4>：uint
/ /
//被typedef定义为无符号int（由4组成的无符号整数）
//字节）。
/ /
//这种功能应该属于STL，但我找不到它
/那里。
/ /
//Google测试在浮点的实现中使用这个类
比较。
/ /
//目前它只处理uint（unsigned int），因为这是所有的google测试
/需要。如果将来需要，可以很容易地添加其他类型
出现。
模板<大小>
class typewithsize_
 公众：
  //这样可以防止用户使用typewithsize<n>和不正确的
  // n的值。
  typedef void uint；
}；

//大小4的专门化。
模板< >
class typewithsize<4>
 公众：
  //unsigned int在gcc和msvc中的大小都为4。
  / /
  //作为base/basictypes.h不在Windows上编译，我们不能使用
  //uint32、uint64等。
  typedef int int；
  typedef无符号int uint；
}；

//大小8的专门化。
模板< >
class typewithsize<8>
 公众：
如果GTEST操作系统窗口
  typedef？int64 int；
  typedef unsigned_uu int64 uint；
否则
  typedef long long int；//nolint
  typedef unsigned long long uint；//nolint
endif//gtest_os_windows
}；

//已知大小的整数类型。
typedef typewithsize<4>：int int32；
typedef typewithsize<4>：uint uint32；
typedef typewithsize<8>：int int64；
typedef typewithsize<8>：uint uint64；
typedef typewithsize<8>：int timeinmillis；//表示以毫秒为单位的时间。

//用于命令行标志和环境变量的实用程序。

//用于引用标志的宏。
定义GTEST标志（名称）标志

//用于声明标志的宏。
define gtest_declare_bool_u（name）gtest_api_uextern bool gtest_flag（name）
定义gtest_声明_int32_u（名称）
    gtest_api_uuextern:：testing:：internal:：int32 gtest_标志（名称）
define gtest_declare_string_u（name）
    gtest-api-extern:：std:：string gtest-flag（名称）

//用于定义标志的宏。
定义gtest_定义_bool_u（名称，默认值，文档）
    gtest_api_bool gtest_flag（name）=（默认值）
定义gtest_定义_int32_u（名称，默认值，文档）
    gtest_api_:：testing:：internal:：int32 gtest_flag（name）=（默认值）
define gtest_define_string_u（名称，默认值，文档）
    gtest_api_:：std:：string gtest_flag（name）=（默认值）

//线程注释
定义gtest_exclusive_lock_required_u（锁）
定义gtest_lock_excluded_u（锁）

//分析32位有符号整数的“str”。如果成功，则写入结果
//to*value并返回true；否则保持*value不变并返回
/假。
//TODO（Chandlerc）：找到更好的重构标志和环境分析方法
//同时退出gtest-port.cc和gtest.cc以避免导出此实用程序
//函数。
bool parseint32（const message&src_text，const char*str，int32*value）；

//分析环境变量中的bool/int32/string
//对应于给定的Google测试标志。
bool boolfromgtestenv（const char*flag，bool default_val）；
gtest_api_uuInt32 int32fromgtestenv（const char*flag，int32 default_val）；
const char*stringfromgtestenv（const char*flag，const char*默认值）；

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_port_h_


如果gtest_os_linux
include<stdlib.h>
包括<sys/types.h>
include<sys/wait.h>
include<unistd.h>
endif//gtest_os_linux

如果GTEST诳U有诳U例外
include<stdExcept>
第二节

include<ctype.h>
include<float.h>
include<string.h>
包括<iomanip>
包括<limits>
包含“SET>”
include<string>
include<vector>

//版权所有2005，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan）
/ /
//谷歌C++测试框架（谷歌测试）
/ /
//此头文件定义消息类。
/ /
//重要提示：由于C++语言的限制，我们不得不
//在此头文件中保留一些内部实现详细信息。
//它们由如下注释清晰地标记：
/ /
///内部实现-不要在用户程序中使用。
/ /
//此类代码不是用户直接使用的，是使用者的主题
//更改而不通知。因此，不要在用户中依赖它
/程序！

ifndef gtest_包括gtest_gtest_message_h_
定义gtest_包括gtest_gtest_message_h_

包括<limits>


//确保全局命名空间中至少存在一个<<运算符。
//有关原因，请参见下面的消息和运算符<（…）。
void运算符<（const testing:：internal:：secret&，int）；

命名空间测试

//message类的工作方式与ostream中继器类似。
/ /
//典型用法：
/ /
/ / 1。将一组值流式传输到消息对象。
//它将记住字符串流中的文本。
/ / 2。然后您将消息对象传输到一个ostream。
//这将导致消息中的文本被流式处理
//到奥斯特林。
/ /
例如；
/ /
//测试：：消息foo；
//foo<<1<<“！=＜2；
//标准：：cout<<foo；
/ /
//将打印“1！= 2”。
/ /
//消息不打算从继承。尤其是
//析构函数不是虚拟的。
/ /
//请注意，stringstream在gcc和msvc中的行为不同。你
//在前者中不能将空字符指针流式传输到它，但在
//后者（这样做会导致访问冲突）。消息
//类通过将空字符指针视为
//（“null”）。
类gtest_api_uu message_
 私人：
  //的基本IO操纵器类型（endl、ends和flush）
  //窄流。
  typedef std:：ostream&（*basicnarrowiomanip）（std:：ostream&）；

 公众：
  //构造空消息。
  消息（）；

  //复制构造函数。
  消息（const message&msg）：ss_u（new:：std:：stringstream）//nolint
    *ss<<msg.getstring（）；
  }

  //从C字符串构造消息。
  显式消息（const char*str）：ss_uu（new:：std:：stringstream）
    ＊SS< <STR；
  }

如果是GTEST，则使用Symbian
  //将值（指针或非指针）流式传输到此对象。
  模板<typename t>
  内联消息和运算符<（const t&value）
    streamHelper（typename internal:：isou pointer<t>：：type（），value）；
    返回这个；
  }
否则
  //将非指针值流式传输到此对象。
  模板<typename t>
  内联消息和运算符<（const t&val）
    //某些库重载<<for stl containers。这些
    //重载是在全局命名空间中定义的，而不是在：：std中定义的。
    / /
    //c++的符号查找规则（即凯尼格查找）表示
    //重载在std命名空间或全局命名空间中可见
    //命名空间，但不是其他命名空间，包括测试
    //Google测试的消息类所在的命名空间。
    / /
    //允许STL容器（以及具有<<运算符的其他类型）
    //在全局命名空间中定义）用于Google测试
    //断言，测试：：消息必须访问自定义<<运算符
    //来自全局命名空间。有了这个使用声明，
    //在全局命名空间和那些
    //通过Koenig查找可见都在此函数中公开。
    使用：：operator<；
    ＊SS< <VAL；
    返回这个；
  }

  //将指针值流式传输到此对象。
  / /
  //此函数是前一个函数的重载。当你
  //流化指向消息的指针，此定义将用作
  //更专业。（C++标准，节）
  //[temp.func.order]。）如果流式处理非指针，则
  //将使用以前的定义。
  / /
  //此重载的原因是将空指针流式处理到
  //Ostream是未定义的行为。根据编译器的不同，
  //可能会得到“0”、“（nil）”、“（null）”或访问冲突。到
  //确保编译器之间的结果一致，我们始终处理空值
  //作为“（NULL）”。
  模板<typename t>
  内联消息和运算符<（t*const&pointer）//nolint
    if（指针==空）
      *ss<<“（空）”；
    }否则{
      *ss<<指针；
    }
    返回这个；
  }
endif//gtest诳os诳symbian

  //因为基本IO操纵器在两个窄
  //和宽流，我们必须提供这个专门的定义
  //of operator<<，即使其主体与
  //以上模板化版本。没有这个定义，流媒体
  //ENDL或其他基本IO操纵器到消息将混淆
  //编译器。
  消息和运算符<（basicnarrowiomanip val）
    ＊SS< <VAL；
    返回这个；
  }

  //对于bool值，我们希望看到true/false，而不是1/0。
  消息和运算符<（bool b）
    return*这个<（b？“true”：“false”）；
  }

  //这两个重载允许将宽C字符串流式处理到消息
  //使用UTF-8编码。
  消息和运算符<（const wchar_t*wide_c_str）；
  消息和运算符<（wchar_t*wide_c_str）；

如果GTEST_有_-std_-wstring
  //使用utf-8将给定的宽字符串转换为窄字符串
  //编码，并将结果流式传输到此消息对象。
  消息和运算符<（const:：std:：wstring&wstr）；
endif//gtest_具有_-std_-wstring

如果GTEST诳U具有诳U Global诳String
  //使用utf-8将给定的宽字符串转换为窄字符串
  //编码，并将结果流式传输到此消息对象。
  消息和运算符<（const:：wstring&wstr）；
endif//gtest_具有全局性

  //获取流式传输到此对象的文本到std：：string。
  //缓冲区中的每个\0'字符都被替换为“\\0”。
  / /
  //内部实现-不要在用户程序中使用。
  std:：string getstring（）常量；

 私人：

如果是GTEST，则使用Symbian
  //由于诺基亚Symbian编译器无法决定
  //函数模板中的const和const。诺基亚编译器
  //在t和t*的类模板专用化之间作出决定，因此a
  //tr1：：type_traits-like is_pointer有效，我们可以对此进行重载。
  模板<typename t>
  inline void streamhelper（内部：：true_type/*是_pointe*/, T* pointer) {

    if (pointer == NULL) {
      *ss_ << "(null)";
    } else {
      *ss_ << pointer;
    }
  }
  template <typename T>
  /*ine void streamhelper（内部：：false_type/*为_pointer*/，
                           常量和值）
    //请参阅上面message&operator<（const&）中的注释了解原因
    //我们需要这个using语句。
    使用：：operator<；
    ＊SS< <值；
  }
endif//gtest诳os诳symbian

  //我们将在此处保存流到此对象的文本。
  const internal:：scoped_ptr<：std:：stringstream>ss_uu；

  //我们声明（但不实现）这个来防止编译器
  //从实现赋值运算符开始。
  void运算符=（const message&）；
}；

//将消息流传送到Ostream。
inline std:：ostream&operator<（std:：ostream&os，const message&sb）
  返回os<<sb.getString（）；
}

命名空间内部

//将可流化值转换为std:：string。空指针是
//转换为“（空）”。当输入值为：：字符串时，
//：：std：：string、：：wstring或：：std：：wstring对象，每个nul
//其中的字符替换为“\\0”。
模板<typename t>
std:：string streamabletostring（const t&streamable）
  return（message（）<<streamable）.getString（）；
}

//命名空间内部
//命名空间测试

endif//gtest_包括gtest_gtest_message_h_
//版权所有2005，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan），eefacm@gmail.com（sean mcafee）
/ /
//谷歌C++测试框架（谷歌测试）
/ /
//此头文件声明字符串类和内部使用的函数
//谷歌测试。如有更改，恕不另行通知。它们不应该使用
//按Google测试外部的代码。
/ /
//此头文件由<gtest/internal/gtest internal.h>包含。
//它不应该被其他文件包含。

ifndef gtest_包括\u gtest_internal_gtest_string_h_
定义gtest_包括gtest_内部gtest_字符串_h_

ifdef_uuu Borlandc_uu
//String .h不保证在C++ Builder上提供StrcPy。
include<mem.h>
第二节

include<string.h>
include<string>


命名空间测试
命名空间内部

//string-包含静态字符串实用程序的抽象类。
类gtest_api_u string_
 公众：
  //静态实用工具方法

  //克隆以0结尾的C字符串，使用new分配内存。这个
  //调用方负责删除返回值，使用
  //删除[]。返回克隆的字符串，如果输入为
  //null。
  / /
  //这与string.h中的strdup（）不同，后者分配
  //使用malloc（）的内存。
  静态const char*clonecstring（const char*c_str）；

如果GTEST操作系统Windows移动
  //Windows CE没有win32 API的“ansi”版本。成为
  //能够将字符串传递到CE上的win32 api，我们需要转换它们
  //到'unicode'，utf-16。

  //从给定的ansi字符串创建一个utf-16宽字符串，并分配
  //内存使用new。调用方负责删除返回
  //使用delete[]的值。返回宽字符串，如果
  //输入为空。
  / /
  //使用ansi codepage（cp_acp）创建宽字符串
  //匹配win32调用的ansi版本和
  //c运行时。
  静态lpcwstr ansitoutf16（const char*c_str）；

  //从给定的宽字符串创建一个ansi字符串，并分配
  //内存使用new。调用方负责删除返回
  //使用delete[]的值。返回ansi字符串，如果
  //输入为空。
  / /
  //返回的字符串是使用ansi codepage（cp_acp）创建的
  //匹配win32调用的ansi版本和
  //c运行时。
  静态常量char*utf16toansi（lpcwstr utf16_str）；
第二节

  //比较两个C字符串。返回具有相同内容的真iff。
  / /
  //与strcmp（）不同，此函数可以处理空参数。一
  //认为空C字符串不同于任何非空C字符串，
  //包括空字符串。
  静态bool cstringequals（const char*lhs，const char*rhs）；

  //使用UTF-8编码将宽C字符串转换为字符串。
  //空值将转换为“（空）”。如果在
  //转换“”（未能从宽字符串转换）“是
  / /返回。
  static std：：string showwidecString（const wchar_t*wide_c_str）；

  //比较两个宽C字符串。返回真正的iff它们有相同的
  /内容。
  / /
  //与wcscmp（）不同，此函数可以处理空参数。一
  //认为空C字符串不同于任何非空C字符串，
  //包括空字符串。
  静态Bool widecStringEquals（const wchar_t*lhs，const wchar_t*rhs）；

  //比较两个C字符串，忽略大小写。返回真正的如果他们
  //内容相同。
  / /
  //与strcasecmp（）不同，此函数可以处理空参数。
  //认为空C字符串不同于任何非空C字符串，
  //包括空字符串。
  静态bool caseinsensitiveStringEquals（const char*lhs，
                                           常量char*rhs）；

  //比较两个宽C字符串，忽略大小写。返回真正的如果他们
  //内容相同。
  / /
  //与wcscasecmp（）不同，此函数可以处理空参数。
  //认为空C字符串不同于任何非空宽C字符串，
  //包括空字符串。
  //注意：不同平台上的实现略有不同。
  //在Windows上，此方法使用根据lc类型进行比较的wcsicmp
  //环境变量。在GNU平台上，此方法使用wcscasecmp
  //根据当前区域设置的lc类型类别进行比较。
  //在MacOS X上，它使用Towlower，它还使用
  //当前区域设置。
  静态bool case不敏感widecstingEquals（const wchar_t*lhs，
                                               const wchar_t*rhs）；

  //如果给定字符串以给定后缀结尾，则返回true，忽略
  //病例。任何字符串都被认为以空后缀结尾。
  静态bool endswithcase不敏感（
      const std:：string&str，const std:：string&suffix）；

  //将int值格式化为“%02d”。
  static std:：string formatintwidth2（int value）；/”%02d“用于width==2

  //将int值格式化为“%x”。
  static std:：string formathexint（int值）；

  //将字节格式化为“%02x”。
  static std：：字符串格式字节（无符号字符值）；

 私人：
  string（）；//不打算实例化。
；//类字符串

//以std:：string形式获取Stringstream缓冲区的内容。每一个‘0’
//缓冲区中的字符替换为“\\0”。
gtest_api_u std：：string stringstreamtostring（：：std：：string stream*stream）；

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_string_h_
//版权所有2008，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：keith.ray@gmail.com（keith ray）
/ /
//Google测试文件路径实用程序
/ /
//此头文件声明内部使用的类和函数
//谷歌测试。如有更改，恕不另行通知。
/ /
//此文件包含在<gtest/internal/gtest internal.h>中。
//不要单独包含这个头文件！

ifndef gtest_包括\u gtest_internal_gtest_filepath_h_
定义GTEST_包括GTEST_内部GTEST_文件路径_h_


命名空间测试
命名空间内部

//filepath-用于文件和目录路径名操作的类，其中
//处理平台特定的约定（如路径名分隔符）。
//用于帮助程序函数，用于命名XML输出目录中的文件。
//除了set方法外，所有方法都是const或static，这提供了
//“不变的价值对象”--对心灵的平静很有用。
//值以路径分隔符结尾的filepath（“like/this/”）表示
//一个目录，否则假定它代表一个文件。在任何一种情况下，
//它可能代表或不代表文件系统中的实际文件或目录。
//不检查名称的语法正确性--不检查是否非法
//字符、格式错误的路径等。

类gtest_api_u filepath_
 公众：
  filePath（）：路径名_（“”）
  文件路径（const filepath&rhs）：路径名_uu（rhs.pathname_123;_

  显式文件路径（const std:：string&pathname）：pathname_uu（pathname）
    正规化（）；
  }

  filepath&operator=（const filepath&rhs）
    集合（RHS）；
    返回这个；
  }

  void set（const filepath&rhs）
    路径名_uu=rhs.pathname_u；
  }

  const std:：string&string（）const返回路径名
  const char*c_str（）const返回路径名.c_str（）；

  //返回当前工作目录，如果不成功则返回“”。
  静态文件路径getcurrentdir（）；

  //给定directory=“dir”，base_name=“test”，number=0，
  //extension=“xml”，返回“dir/test.xml”。如果数字大于
  //大于零（如12），返回“dir/test_12.xml”。
  //在Windows平台上，使用\作为分隔符，而不是/。
  静态文件路径makefilename（const filepath&directory，
                               常量文件路径和基文件名，
                               整数，
                               const char*扩展名）；

  //给定directory=“dir”，relative_path=“test.xml”，
  //返回“dir/test.xml”。
  //在Windows上，使用\作为分隔符，而不是/。
  静态文件路径concatpaths（const filepath&directory，
                              const文件路径和相对路径）；

  //返回当前不存在的文件的路径名。路径名
  //将是directory/base_name.extension或
  //directory/base_name_<number>。如果directory/base_name.extension
  //已经存在。在找到路径名之前，编号将递增。
  //这还不存在。
  //示例：'dir/foo_test.xml'或'dir/foo_test_1.xml'。
  //如果有两个或多个进程调用此
  //同时执行函数--它们都可以选择相同的文件名。
  静态文件路径generateuniquefilename（const filepath&directory，
                                         常量文件路径和基文件名，
                                         const char*扩展名）；

  //如果路径为“”，则返回true。
  bool isEmpty（）const返回路径名.empty（）；

  //如果输入名有尾随分隔符，则将其移除并返回
  //名称，否则返回未修改的名称字符串。
  //在Windows平台上，使用\作为分隔符，其他平台使用/。
  filePath removeTrailingPathSeparator（）常量；

  //返回删除目录部分的文件路径的副本。
  //示例：file path（“path/to/file”）.removedirectoryname（）返回
  //文件路径（“file”）。如果没有目录部分（“just_a_file”），则返回
  //文件路径未修改。如果没有文件部分（“just_a_dir/”），则
  //返回空文件路径（“”）。
  //在Windows平台上，“\”是路径分隔符，否则是“/”。
  filePath removeDirectoryName（）常量；

  //removefilename返回删除文件名的目录路径。
  //示例：file path（“path/to/file”）.removefilename（）返回“path/to/”。
  //如果文件路径是“a \u文件”或/a \u文件，则removeFileName返回
  //filepath（“./”）或，在Windows上，filepath（“.\”）。如果文件路径
  //没有文件，如“just/a/dir/”，它返回未修改的文件路径。
  //在Windows平台上，“\”是路径分隔符，否则是“/”。
  filepath removefilename（）常量；

  //返回删除了不区分大小写扩展名的filepath的副本。
  //示例：filepath（“dir/file.exe”）.removeextension（“exe”）返回
  //文件路径（“dir/file”）。如果不区分大小写的扩展名不是
  //找到，返回原始文件路径的副本。
  文件路径removeextension（const char*extension）const；

  //创建目录以使路径存在。如果成功或如果
  //目录已经存在；如果无法创建，则返回false
  //目录。如果文件路径为
  //不表示目录（即不以路径分隔符结尾）。
  bool createDirectoriesrecusly（）常量；

  //创建目录以使路径存在。如果成功或
  //如果目录已经存在；如果无法创建
  //目录，无论什么原因，包括父目录
  /存在。未命名为“createddirectory”，因为这是Windows上的宏。
  bool createFolder（）常量；

  //如果filepath描述了文件系统中的某些内容，则返回true，
  //文件、目录或其他任何文件，并且存在某些内容。
  bool fileorddirectoryxists（）常量；

  //如果pathname描述文件系统中的目录，则返回true
  /存在。
  bool directoryxists（）常量；

  //如果filepath以路径分隔符结尾，则返回true，这表示
  //用于表示目录。否则返回false。
  //这不会检查目录（或文件）是否确实存在。
  bool isdirectory（）常量；

  //如果pathname描述根目录，则返回true。（窗户有一个
  //每个磁盘驱动器的根目录。）
  bool isrootdirectory（）const；

  //如果pathname描述绝对路径，则返回true。
  bool isabsolutepath（）const；

 私人：
  //用单个分隔符替换多个连续分隔符。
  //例如，“bar///foo”变为“bar/foo”。不排除其他
  //路径名中可能包含“.”或“..”的冗余。
  / /
  //具有多个连续分隔符的路径名可能出现在
  //用户错误，或者由于某些脚本或API生成路径名
  //带有尾随分隔符。在其他平台上使用相同的API或脚本
  //不能生成带有尾随“/”的路径名。然后在其他地方
  //pathname可能添加了另一个“/”和pathname组件，
  //不检查分隔符是否已存在。
  //脚本语言和操作系统可能允许类似“foo//bar”的路径
  //但是filepath中的一些函数不能正确处理这个问题。在
  //特别是，removeTrailingPathSeparator（）只删除一个分隔符，并且
  //在createDirectoriesrecursive（）中调用它，假定它将更改
  //从目录语法（尾随分隔符）到文件名语法的路径名。
  / /
  //在Windows上，此方法还将替代路径分隔符“/”替换为
  //主路径分隔符“\\”，因此，例如“bar\\/\\foo”将变为
  /“酒吧”。

  void normalize（）；

  //返回指向中最后出现的有效路径分隔符的指针
  //文件路径。例如，在Windows上，'/'和'\'都是有效路径
  /分离器。如果找不到路径分隔符，则返回空值。
  const char*findlastPathSeparator（）const；

  std：：字符串路径名\；
；//类文件路径

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_filepath_h_
//此文件由以下命令生成：
//pump.py gtest-type-util.h.泵
//不要手工编辑！！！！

//版权所有2008 Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan）

//实现类型化和类型参数化所需的类型实用程序
/测试。此文件由脚本生成。不要手工编辑！
/ /
//目前列表中最多支持50种类型，最多支持50种类型
//在一个类型参数化测试用例中键入参数化测试。
//如果需要，请联系googletestframework@googlegroups.com
/更多。

ifndef gtest_包括\u gtest_internal_gtest_type_utili_h_
定义GTEST_包括GTEST_内部GTEST_类型


/ifdef uu gnuc_uuu在这里太笼统了。可以不使用GCC
//libstdc++（cxxabi.h的来源）。
如果gtest_有_cxxabi_h_
include<cxxabi.h>
elif定义（35uu hp_acc）
include<acxx_demangle.h>
endif//gtest_hash_cxxabi_h_

命名空间测试
命名空间内部

//gettypename<t>（）返回类型t的人类可读名称。
//nb:这个函数也在google mock中使用，所以不要将它移到
//下面的“仅类型化测试”部分。
模板<typename t>
std:：string gettypename（）
如果GTEST覕u有覕rtti

  const char*const name=typeid（t）.name（）；
如果gtest_定义了_xxabi_h_（_uu hp_acc）
  INT状态＝0；
  //gcc对typeid（t）.name（）的实现会管理类型名，
  //所以我们必须把它弄乱。
如果gtest_有_cxxabi_h_
  使用abi:：uucxa_demangle；
endif//gtest_具有_cxxabi_h_
  char*const readable_name=uucxa_demangle（name，0，0，&status）；
  const std:：string name_str（status==0？可读的名称：名称）；
  自由（可读的名称）；
  返回名称\str；
否则
  返回名称；
endif//gtest_具有_cxxabi_h__uuu hp_acc

否则

  return“<type>”；

endif//gtest_具有
}

如果GTEST_有_型_测试_GTEST_有_型_测试_P

//assertytypeeq<t1，t2>：：定义了类型iff t1和t2相同
//类型。这可以用作编译时断言，以确保
//两种类型相等。

template<typename t1，typename t2>
结构断言类型eq；

模板<typename t>
结构断言类型eq<t，t>
  typedef bool类型；
}；

//用作类参数的默认值的唯一类型
//模板类型。这允许我们模拟变量模板
//（例如类型< int >、类型< int、双>等），其中C++不
//直接支持。
结构无{}；

//以下结构和结构模板系列用于
//表示类型列表。特别是，类型sn<t1，t2，…，tn>
//表示一个类型列表，其中包含n个类型（T1、T2、…、和TN）。
//除了类型S0，族中的每个结构都有两个成员类型：
//头指向列表中的第一个类型，尾指向
//列表。

//空类型列表。
结构类型0

//键入长度为1、2、3等的列表。

模板<typename t1>
结构类型1{
  typedef T1头；
  typedef类型0尾；
}；
template<typename t1，typename t2>
结构类型2{
  typedef T1头；
  typedef types1<t2>tail；
}；

template<typename t1，typename t2，typename t3>
结构类型3{
  typedef T1头；
  typedef types2<t2，t3>tail；
}；

template<typename t1，typename t2，typename t3，typename t4>
结构类型4{
  typedef T1头；
  typedef types3<t2，t3，t4>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5>
结构类型5{
  typedef T1头；
  typedef types4<t2，t3，t4，t5>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名称T6>
结构类型6{
  typedef T1头；
  typedef types5<t2，t3，t4，t5，t6>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7>
结构类型7{
  typedef T1头；
  typedef types6<t2，t3，t4，t5，t6，t7>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8>
结构类型8{
  typedef T1头；
  typedef types7<t2，t3，t4，t5，t6，t7，t8>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8，typename t9>
结构类型9{
  typedef T1头；
  typedef types8<t2，t3，t4，t5，t6，t7，t8，t9>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8，typename t9，typename t10>
结构类型10
  typedef T1头；
  typedef types9<t2，t3，t4，t5，t6，t7，t8，t9，t10>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名称T11>
结构类型11
  typedef T1头；
  typedef types10<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12>
结构类型12
  typedef T1头；
  typedef types11<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13>
结构类型13
  typedef T1头；
  typedef types12<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11、类型名t12、类型名t13、类型名t14>
结构类型14
  typedef T1头；
  typedef types13<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11、类型名t12、类型名t13、类型名t14、类型名t15>
结构类型15
  typedef T1头；
  typedef types14<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      T15>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    T16>
结构类型16
  typedef T1头；
  typedef types15<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      T16>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17>
结构类型17
  typedef T1头；
  typedef types16<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      T16、T17>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18>
结构类型18
  typedef T1头；
  typedef types17<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16、类型名t17、类型名t18、类型名t19>
结构类型19
  typedef T1头；
  typedef types18<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16、类型名t17、类型名t18、类型名t19、类型名t20>
结构类型20
  typedef T1头；
  typedef types19<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称T21>
结构类型21
  typedef T1头；
  typedef types20<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21，类型名t22>
结构类型22
  typedef T1头；
  typedef types21<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21，类型名t22，类型名t23>
结构类型23
  typedef T1头；
  typedef types22<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21、类型名t22、类型名t23、类型名t24>
结构类型24
  typedef T1头；
  typedef types23<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25>
结构类型25
  typedef T1头；
  typedef types24<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名称T26>
结构类型26
  typedef T1头；
  typedef types25<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27>
结构类型27
  typedef T1头；
  typedef types26<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28>
结构类型28
  typedef T1头；
  typedef types27<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26、类型名t27、类型名t28、类型名t29>
结构类型29
  typedef T1头；
  typedef types28<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      T29 >尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26、类型名t27、类型名t28、类型名t29、类型名t30>
结构类型30
  typedef T1头；
  typedef types29<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      T30>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型命名T31
结构类型31
  typedef T1头；
  typedef types30<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      T30、T31 >尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32>
结构类型32
  typedef T1头；
  typedef types31<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33>
结构类型33
  typedef T1头；
  typedef types32<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33、类型名t34>
结构类型34
  typedef T1头；
  typedef types33<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33、类型名t34、类型名t35>
结构类型35
  typedef T1头；
  typedef types34<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型命名T36>
结构类型36
  typedef T1头；
  typedef types35<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37>
结构类型37
  typedef T1头；
  typedef types36<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38>
结构类型38
  typedef T1头；
  typedef types37<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38、类型名t39>
结构类型39
  typedef T1头；
  typedef types38<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38、类型名t39、类型名t40>
结构类型40
  typedef T1头；
  typedef types39<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型命名T41
结构类型41
  typedef T1头；
  typedef types40<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42>
结构类型42
  typedef T1头；
  typedef types41<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43>
结构类型43
  typedef T1头；
  typedef types42<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      T4>尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44>
结构类型44
  typedef T1头；
  typedef types43<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      T44 >尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    typename t41，typename t42，typename t43，typename t44，typename t45>
结构类型45
  typedef T1头；
  typedef types44<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      T44、T45 >尾；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型46
结构类型46
  typedef T1头；
  typedef types45<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      t44，t45，t46>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47>
结构类型47
  typedef T1头；
  typedef types46<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      t44，t45，t46，t47>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47，类型名t48>
结构类型48
  typedef T1头；
  typedef types47<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      t44、t45、t46、t47、t48>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47，类型名t48，类型名t49>
结构类型49
  typedef T1头；
  typedef types48<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      t44、t45、t46、t47、t48、t49>尾部；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46、类型名t47、类型名t48、类型名t49、类型名t50>
结构类型50
  typedef T1头；
  typedef types49<t2、t3、t4、t5、t6、t7、t8、t9、t10、t11、t12、t13、t14、t15，
      t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
      t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
      t44、t45、t46、t47、t48、t49、t50>尾部；
}；


//命名空间内部

//我们不想要求用户直接写typesn<…>
//因为这需要他们计算长度。类型<…>很多
//更容易编写，但在
//编译器错误，因为gcc坚持打印出每个模板
//参数，即使它有默认值（这意味着类型<int>
//将在编译器中显示为类型<int，none，none，…，none>
/ /错误）。
/ /
//我们的解决方案是将这两种方法中最好的部分结合起来：a
//用户将写入类型<t1，…，tn>，google test将转换
//在内部键入sn<t1，…，tn>以生成错误消息
/可读。翻译是由
//类型模板。
template<typename t1=internal:：none，typename t2=internal:：none，
    typename t3=内部：：无，typename t4=内部：：无，
    typename t5=内部：：无，typename t6=内部：：无，
    typename t7=内部：：无，typename t8=内部：：无，
    typename t9=内部：：无，typename t10=内部：：无，
    typename t11=内部：：无，typename t12=内部：：无，
    typename t13=内部：：无，typename t14=内部：：无，
    typename t15=内部：：无，typename t16=内部：：无，
    typename t17=内部：：无，typename t18=内部：：无，
    typename t19=内部：：无，typename t20=内部：：无，
    类型名t21=内部：：无，类型名t22=内部：：无，
    类型名t23=内部：：无，类型名t24=内部：：无，
    类型名t25=内部：：无，类型名t26=内部：：无，
    类型名t27=内部：：无，类型名t28=内部：：无，
    类型名t29=内部：：无，类型名t30=内部：：无，
    typename t31=内部：：无，typename t32=内部：：无，
    typename t33=内部：：无，typename t34=内部：：无，
    typename t35=内部：：无，typename t36=内部：：无，
    typename t37=内部：：无，typename t38=内部：：无，
    typename t39=内部：：无，typename t40=内部：：无，
    typename t41=内部：：无，typename t42=内部：：无，
    typename t43=内部：：无，typename t44=内部：：无，
    typename t45=内部：：无，typename t46=内部：：无，
    typename t47=内部：：无，typename t48=内部：：无，
    typename t49=内部：：无，typename t50=内部：：无>
结构类型{
  typedef内部：：types50<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46、t47、t48、t49、t50>型；
}；

模板< >
结构类型<内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types0类型；
}；
模板<typename t1>
结构类型<T1，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types1<t1>类型；
}；
template<typename t1，typename t2>
结构类型<T1，T2，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types2<t1，t2>类型；
}；
template<typename t1，typename t2，typename t3>
结构类型<T1，T2，T3，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef internal:：types3<t1，t2，t3>类型；
}；
template<typename t1，typename t2，typename t3，typename t4>
结构类型<T1、T2、T3、T4、内部：：无、内部：：无、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef internal:：types4<t1，t2，t3，t4>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5>
结构类型<T1、T2、T3、T4、T5、内部：：无、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef internal:：types5<t1，t2，t3，t4，t5>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名称T6>
结构类型<T1、T2、T3、T4、T5、T6、内部：：无、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef internal:：types6<t1，t2，t3，t4，t5，t6>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7>
结构类型<T1、T2、T3、T4、T5、T6、T7、内部：：无、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types7<t1，t2，t3，t4，t5，t6，t7>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、内部：：无、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef internal:：types8<t1，t2，t3，t4，t5，t6，t7，t8>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8，typename t9>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types9<t1，t2，t3，t4，t5，t6，t7，t8，t9>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    typename t6，typename t7，typename t8，typename t9，typename t10>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef internal:：types10<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名称T11>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef internal:：types11<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types12<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，
      T12>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types13<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      T13>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11、类型名t12、类型名t13、类型名t14>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types14<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      T13、T14>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11、类型名t12、类型名t13、类型名t14、类型名t15>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types15<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    T16>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types16<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types17<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types18<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16、类型名t17、类型名t18、类型名t19>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types19<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15、t16、t17、t18、t19>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16、类型名t17、类型名t18、类型名t19、类型名t20>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types20<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15、t16、t17、t18、t19、t20>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称T21>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types21<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15、t16、t17、t18、t19、t20、t21>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21，类型名t22>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16、t17、t18、t19、t20、t21、t22，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types22<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15、t16、t17、t18、t19、t20、t21、t22>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21，类型名t22，类型名t23>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16、t17、t18、t19、t20、t21、t22、t23，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types23<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名t21、类型名t22、类型名t23、类型名t24>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types24<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types25<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13、t14、t15、t16、t17、t18、t19、t20、t21、t22、t23、t24、t25>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名称T26>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types26<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，
      T26>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types27<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      T27＞型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types28<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      T27、T28＞型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26、类型名t27、类型名t28、类型名t29>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types29<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26、类型名t27、类型名t28、类型名t29、类型名t30>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types30<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型命名T31
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types31<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types32<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types33<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27、t28、t29、t30、t31、t32、t33>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33、类型名t34>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types34<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27、t28、t29、t30、t31、t32、t33、t34>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31、类型名t32、类型名t33、类型名t34、类型名t35>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types35<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      


    
    
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    
    类型命名T36>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types36<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27、t28、t29、t30、t31、t32、t33、t34、t35、t36>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31、t32、t33、t34、t35、t36、t37，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types37<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types38<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27、t28、t29、t30、t31、t32、t33、t34、t35、t36、t37、t38>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38、类型名t39>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types39<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27、t28、t29、t30、t31、t32、t33、t34、t35、t36、t37、t38、t39>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36、类型名t37、类型名t38、类型名t39、类型名t40>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types40<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，
      T40>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型命名T41
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无>
  typedef内部：：types41<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      T41＞型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，内部：：无，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types42<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      T41、T42＞型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无，内部：：无>
  typedef内部：：types43<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无，内部：：无>
  typedef内部：：types44<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44>类型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    typename t41，typename t42，typename t43，typename t44，typename t45>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，t45，
    内部：：无，内部：：无，内部：：无，内部：：无，
    内部：：无>
  typedef内部：：types45<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    
    
    
    

    
    
    
  
      
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，t45，
    t46，t47，内部：：无，内部：：无，内部：：无>
  typedef内部：：types47<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46、t47>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47，类型名t48>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，t45，
    t46，t47，t48，内部：：无，内部：：无>
  typedef内部：：types48<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46、t47、t48>型；
}；
template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46，类型名t47，类型名t48，类型名t49>
结构类型<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14、T15，
    t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，t30，
    t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，t45，
    t46，t47，t48，t49，内部：：无>
  typedef内部：：types49<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46、t47、t48、t49>型；
}；

命名空间内部

define gtest_template_utemplate<typename t>class

//模板“selector”struct templatesel<tmpl>用于
//表示tmpl，它必须是具有一种类型的类模板
//参数，作为类型。模板：定义了类型
//作为类型tmpl<t>。这允许我们实际实例化
//模板“selected”by templatesel<tmpl>。
/ /
//此技巧对于为类模板模拟typedef是必需的，
/c++不直接支持。
template<gtest_template_u tmpl>
结构模板
  
  
    
  



  






无结构

//以下结构和结构模板系列用于
//表示模板列表。尤其是模板n<t1，t2，…，
//tn>表示n个模板（T1、T2、…、和tn）的列表。除了
//对于templates0，族中的每个结构都有两个成员类型：
//指向列表中第一个模板的选择器，并返回tail
//对于列表的其余部分。

//空模板列表。
结构模板0

//长度为1、2、3等的模板列表。

模板<gtest_template_u T1>
结构模板1
  typedef templatesel<t1>head；
  typedef模板0尾；
}；
template<gtest_template_1，gtest_template_2>
结构模板2
  typedef templatesel<t1>head；
  typedef templates1<t2>tail；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3>
结构模板3
  typedef templatesel<t1>head；
  typedef templates2<t2，t3>tail；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    gtest_模板t4>
结构模板4
  typedef templatesel<t1>head；
  typedef templates3<t2，t3，t4>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    gtest_template_4，gtest_template_5>
结构模板5
  typedef templatesel<t1>head；
  typedef templates4<t2，t3，t4，t5>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6>
结构模板6
  typedef templatesel<t1>head；
  typedef templates5<t2，t3，t4，t5，t6>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板
结构模板7
  typedef templatesel<t1>head；
  typedef templates6<t2，t3，t4，t5，t6，t7>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    GTEST_u template_u7，GTEST_template_8>
结构模板8
  typedef templatesel<t1>head；
  typedef templates7<t2，t3，t4，t5，t6，t7，t8>tail；
}；


    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    
结构模板9
  typedef templatesel<t1>head；
  typedef templates8<t2，t3，t4，t5，t6，t7，t8，t9>tail；


template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    
结构模板10
  
  


template<gtest_template_1，gtest_template_2，gtest_template_3，
    
    测试模板，测试模板，测试模板，
    gtest_template_ut10，gtest_template_t11>
结构模板11
  typedef templatesel<t1>head；
  typedef templates10<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11>tail；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    gtest_模板_t10，gtest_模板_t11，gtest_模板_t12>
结构模板12
  typedef templatesel<t1>head；
  typedef templates11<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    gtest_模板t13>
结构模板13
  typedef templatesel<t1>head；
  typedef templates12<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    gtest_template_t13，gtest_template_t14>
结构模板14
  typedef templatesel<t1>head；
  typedef模板13<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      T14>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板Ut13、GTESTUU模板Ut14、GTESTU模板Ut15>
结构模板15
  typedef templatesel<t1>head；
  typedef模板14<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      T15>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    gtest_模板t16>
结构模板16
  typedef templatesel<t1>head；
  typedef模板15<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      T15、T16>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    gtest_template_t16，gtest_template_t17>
结构模板17
  typedef templatesel<t1>head；
  typedef模板16<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTU模板Ut17、GTESTU模板Ut18>
结构模板18
  typedef templatesel<t1>head；
  typedef模板17<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_模板t19>
结构模板19
  typedef templatesel<t1>head；
  typedef模板18<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_template_t19，gtest_template_t20>
结构模板20
  typedef templatesel<t1>head；
  typedef模板19<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_template_t19、gtest_template_t20、gtest_template_t21>
结构模板21
  typedef templatesel<t1>head；
  typedef模板20<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    gtest_模板t22>
结构模板22
  typedef templatesel<t1>head；
  typedef模板21<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    gtest_模板t22，gtest_模板t23>
结构模板23
  typedef templatesel<t1>head；
  typedef模板22<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24>
结构模板24
  typedef templatesel<t1>head；
  typedef模板23<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_模板
结构模板25
  typedef templatesel<t1>head；
  typedef模板24<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_template_t25，gtest_template_t26>
结构模板26
  typedef templatesel<t1>head；
  typedef模板25<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_模板_t25、gtest_模板_t26、gtest_模板_t27>
结构模板27
  typedef templatesel<t1>head；
  typedef模板26<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    gtest_模板
结构模板28
  typedef templatesel<t1>head；
  typedef模板27<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      T28 >尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    gtest_template_t28，gtest_template_t29>
结构模板29
  typedef templatesel<t1>head；
  typedef模板28<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      T29 >尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30>
结构模板30
  typedef templatesel<t1>head；
  typedef模板29<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      T29、T30>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    gtest_模板
结构模板31
  typedef templatesel<t1>head；
  typedef模板30<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    gtest_template_ut31，gtest_template_ut32>
结构模板32
  typedef templatesel<t1>head；
  typedef模板31<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33>
结构模板33
  typedef templatesel<t1>head；
  typedef模板32<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板
结构模板34
  typedef templatesel<t1>head；
  typedef模板33<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34，gtest_模板_t35>
结构模板35
  typedef templatesel<t1>head；
  typedef模板34<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36>
结构模板36
  typedef templatesel<t1>head；
  typedef模板35<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_模板
结构模板37
  typedef templatesel<t1>head；
  typedef模板36<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_template_t37，gtest_template_t38>
结构模板38
  typedef templatesel<t1>head；
  typedef模板37<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_模板_t37、gtest_模板_t38、gtest_模板_t39>
结构模板39
  typedef templatesel<t1>head；
  typedef模板38<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板t40>
结构模板40
  typedef templatesel<t1>head；
  typedef模板39<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_template_ut40，gtest_template_t41>
结构模板41
  typedef templatesel<t1>head；
  typedef模板40<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_template_ut40，gtest_template_t41，gtest_template_t42>
结构模板42
  typedef templatesel<t1>head；
  typedef模板41<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      T42 >尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_模板t43>
结构模板43
  typedef templatesel<t1>head；
  typedef模板42<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      T4>尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_template_ut43，gtest_template_t44>
结构模板44
  typedef templatesel<t1>head；
  typedef模板43<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      T43、T44 >尾；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_template_ut43、gtest_template_t44、gtest_template_t45>
结构模板45
  typedef templatesel<t1>head；
  typedef模板44<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43，t44，t45>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板t46>
结构模板46
  typedef templatesel<t1>head；
  typedef模板45<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43、t44、t45、t46>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_template_ut46，gtest_template_t47>
结构模板47
  typedef templatesel<t1>head；
  typedef模板46<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43、t44、t45、t46、t47>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板_t46、gtest_模板_t47、gtest_模板_t48>
结构模板48
  typedef templatesel<t1>head；
  typedef模板47<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43、t44、t45、t46、t47、t48>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板_t46，gtest_模板_t47，gtest_模板_t48，
    gtest_模板t49>
结构模板49
  typedef templatesel<t1>head；
  typedef模板48<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43、t44、t45、t46、t47、t48、t49>尾部；
}；

template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板_t46，gtest_模板_t47，gtest_模板_t48，
    gtest_模板t49，gtest_模板t50>
结构模板50
  typedef templatesel<t1>head；
  typedef模板49<t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，t14，
      t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
      t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，
      t43、t44、t45、t46、t47、t48、t49、t50>尾部；
}；


//我们不想要求用户直接编写模板n<…>
//因为这需要他们计算长度。模板<…>很多
//更容易编写，但在
//编译器错误，因为gcc坚持打印出每个模板
//参数，即使它有默认值（这意味着templates<list>
//将在编译器中显示为templates<list，nonet，nonet，…，nonet>
/ /错误）。
/ /
//我们的解决方案是将这两种方法中最好的部分结合起来：a
//用户将编写模板<t1，…，tn>，google test将翻译
//内部模板n<t1，…，tn>生成错误消息
/可读。翻译是由
//模板模板。
template<gtest_template_t1=nonet，gtest_template_t2=nonet，
    gtest_template_t3=nonet，gtest_template_ut4=nonet，
    gtest_template_t5=无，gtest_template_t6=无，
    gtest_template_u7=nonet，gtest_template_8=nonet，
    gtest_template_9=nonet，gtest_template_10=nonet，
    gtest_template_t11=nonet，gtest_template_t12=nonet，
    gtest_template_t13=nonet，gtest_template_t14=nonet，
    gtest_template_t15=nonet，gtest_template_t16=nonet，
    gtest_template_t17=nonet，gtest_template_t18=nonet，
    gtest_template_t19=nonet，gtest_template_t20=nonet，
    gtest_template_t21=nonet，gtest_template_t22=nonet，
    gtest_template_t23=nonet，gtest_template_t24=nonet，
    gtest_template_t25=nonet，gtest_template_t26=nonet，
    gtest_template_t27=nonet，gtest_template_t28=nonet，
    gtest_template_t29=nonet，gtest_template_t30=nonet，
    gtest_template_t31=nonet，gtest_template_t32=nonet，
    gtest_template_t33=nonet，gtest_template_t34=nonet，
    gtest_template_t35=nonet，gtest_template_t36=nonet，
    gtest_template_t37=nonet，gtest_template_t38=nonet，
    gtest_template_t39=nonet，gtest_template_t40=nonet，
    gtest_template_t41=无，gtest_template_t42=无，
    gtest_template_t43=无，gtest_template_t44=无，
    gtest_template_t45=nonet，gtest_template_t46=nonet，
    gtest_template_t47=nonet，gtest_template_t48=nonet，
    gtest_template_t49=nonet，gtest_template_t50=nonet>
结构模板
  typedef模板50<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44、t45、t46、t47、t48、t49、t50>型；
}；

模板< >
结构模板<nonet，nonet，nonet，nonet，nonet，nonet，nonet，nonet，nonet，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET> {
  typedef模板0类型；
}；
模板<gtest_template_u T1>
结构模板<T1，nonet，nonet，nonet，nonet，nonet，nonet，nonet，nonet，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET> {
  typedef templates1<t1>类型；
}；
template<gtest_template_1，gtest_template_2>
结构模板<t1，t2，nonet，nonet，nonet，nonet，nonet，nonet，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET> {
  typedef templates2<t1，t2>type；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3>
结构模板<T1，T2，T3，NONET，NONET，NONET，NONET，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不
  typedef templates3<t1，t2，t3>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    gtest_模板t4>
结构模板<T1，T2，T3，T4，nonet，nonet，nonet，nonet，nonet，nonet，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不
  typedef templates4<t1，t2，t3，t4>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    gtest_template_4，gtest_template_5>
结构模板<T1，T2，T3，T4，T5，NONET，NONET，NONET，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无，无，无
  typedef templates5<t1，t2，t3，t4，t5>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6>
结构模板<T1，T2，T3，T4，T5，T6，NONET，NONET，NONET，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无，无，无
  typedef templates6<t1，t2，t3，t4，t5，t6>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板
结构模板<T1，T2，T3，T4，T5，T6，T7，NONET，NONET，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无，无
  typedef templates7<t1，t2，t3，t4，t5，t6，t7>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    GTEST_u template_u7，GTEST_template_8>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，NONET，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无，无
  typedef templates8<t1，t2，t3，t4，t5，t6，t7，t8>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    GTESTUU模板U7、GTESTU模板U8、GTESTU模板U9>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，T9，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无，无
  typedef templates9<t1，t2，t3，t4，t5，t6，t7，t8，t9>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    gtest_模板t10>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，T9，T10，NONET，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无
  typedef templates10<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    gtest_template_ut10，gtest_template_t11>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，T9，T10，T11，NONET，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无
  typedef templates11<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    gtest_模板_t10，gtest_模板_t11，gtest_模板_t12>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，T9，T10，T11，T12，NONET，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无，无，无，无
  typedef templates12<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    gtest_模板t13>
结构模板<T1，T2，T3，T4，T5，T6，T7，T8，T9，T10，T11，T12，T13，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不
  typedef模板13<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      T13>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    gtest_template_t13，gtest_template_t14>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不
  typedef模板14<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      T14>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板Ut13、GTESTUU模板Ut14、GTESTU模板Ut15>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，无极，无极，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无
  typedef模板15<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      T14、T15>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    gtest_模板t16>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，无极，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无
  typedef模板16<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    gtest_template_t16，gtest_template_t17>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无，无
  typedef模板17<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTU模板Ut17、GTESTU模板Ut18>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无
  typedef模板18<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_模板t19>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、nonet、nonet、nonet、nonet、nonet、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无
  typedef模板19<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_template_t19，gtest_template_t20>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、nonet、nonet、nonet、nonet、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无
  typedef模板20<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14、t15、t16、t17、t18、t19、t20>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    gtest_template_t19、gtest_template_t20、gtest_template_t21>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、t21、nonet、nonet、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无，无
  typedef模板21<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    gtest_模板t22>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、t21、t22、nonet、nonet、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无
  typedef模板22<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    gtest_模板t22，gtest_模板t23>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、t21、t22、t23、nonet、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无
  typedef模板23<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、t21、t22、t23、t24、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    无，无，无
  typedef模板24<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_模板
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15、t16、t17、t18、t19、t20、t21、t22、t23、t24、t25、nonet、nonet、nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET，NONET> {
  typedef模板25<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_template_t25，gtest_template_t26>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，nonet，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET，NONET> {
  typedef模板26<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14、t15、t16、t17、t18、t19、t20、t21、t22、t23、t24、t25、t26>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    gtest_模板_t25、gtest_模板_t26、gtest_模板_t27>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，nonet，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET，NONET> {
  typedef模板27<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      T27＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    gtest_模板
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET，NONET> {
  typedef模板28<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      T28＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    gtest_template_t28，gtest_template_t29>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    不，不，不，不，不，不，不，不，不，不，不，不，不，
    NONET> {
  typedef模板29<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      T28、T29＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    T30，无极，无极，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不
  typedef模板30<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    gtest_模板
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    T30，T31，无极，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不
  typedef模板31<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    gtest_template_ut31，gtest_template_ut32>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，无极，无极，无极，无极，无极，无极，无极，无极，
    不，不，不，不，不，不，不，不，不，不，不，不
  typedef模板32<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、nonet、nonet、nonet、nonet、nonet、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无，无
  typedef模板33<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、nonet、nonet、nonet、nonet、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无，无
  typedef模板34<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34，gtest_模板_t35>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、nonet、nonet、nonet、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无，无
  typedef模板35<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28、t29、t30、t31、t32、t33、t34、t35>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、t36、nonet、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无，无
  typedef模板36<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_模板
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、t36、t37、nonet、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无
  typedef模板37<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_template_t37，gtest_template_t38>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、t36、t37、t38、nonet、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无
  typedef模板38<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    gtest_模板_t37、gtest_模板_t38、gtest_模板_t39>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、t36、t37、t38、t39、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无，无
  typedef模板39<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板t40>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30、t31、t32、t33、t34、t35、t36、t37、t38、t39、t40、nonet、nonet、nonet，
    无，无，无，无，无，无，无，无
  typedef模板40<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28、t29、t30、t31、t32、t33、t34、t35、t36、t37、t38、t39、t40>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_template_ut40，gtest_template_t41>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，nonet，nonet，
    无，无，无，无，无，无，无，无
  typedef模板41<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      T41＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_template_ut40，gtest_template_t41，gtest_template_t42>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，nonet，
    无，无，无，无，无，无，无，无
  typedef模板42<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      T42＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_模板t43>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
    无，无，无，无，无，无，无，无
  typedef模板43<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      T42、T43＞型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_template_ut43，gtest_template_t44>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    不，不，不，不，不，不，不
  typedef模板44<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    gtest_template_ut43、gtest_template_t44、gtest_template_t45>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    t45，无，无，无，无，无
  typedef模板45<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42，t43，t44，t45>类型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板t46>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    t45，t46，无，无，无，无
  typedef模板46<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44、t45、t46>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_template_ut46，gtest_template_t47>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    t45，t46，t47，无，无，无
  typedef模板47<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44、t45、t46、t47>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板_t46、gtest_模板_t47、gtest_模板_t48>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    t45、t46、t47、t48、nonet、nonet>>
  typedef模板48<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44、t45、t46、t47、t48>型；
}；
template<gtest_template_1，gtest_template_2，gtest_template_3，
    GTESTUU模板Ut4、GTESTU模板Ut5、GTESTU模板Ut6，
    测试模板，测试模板，测试模板，
    GTESTUU模板UT10、GTESTUU模板UT11、GTESTUU模板UT12，
    GTESTUU模板UT13、GTESTUU模板UT14、GTESTUU模板UT15，
    GTESTUU模板Ut16、GTESTUU模板Ut17、GTESTUU模板Ut18，
    GTESTUU模板Ut19、GTESTU模板Ut20、GTESTU模板Ut21，
    GTESTUU模板Ut22、GTESTU模板Ut23、GTESTU模板Ut24，
    GTESTUU模板Ut25，GTESTU模板Ut26，GTESTU模板Ut27，
    GTESTUU模板Ut28、GTESTU模板Ut29、GTESTU模板Ut30，
    GTESTUU模板Ut31、GTESTU模板Ut32、GTESTU模板Ut33，
    gtest_模板_t34、gtest_模板_t35、gtest_模板_t36，
    GTESTUU模板UT37、GTESTU模板UT38、GTESTU模板UT39，
    gtest_模板_t40，gtest_模板_t41，gtest_模板_t42，
    GTESTUU模板Ut43、GTESTUU模板Ut44、GTESTUU模板Ut45，
    gtest_模板_t46，gtest_模板_t47，gtest_模板_t48，
    gtest_模板t49>
结构模板<T1、T2、T3、T4、T5、T6、T7、T8、T9、T10、T11、T12、T13、T14，
    t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，t29，
    t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，t44，
    t45，t46，t47，t48，t49，无
  typedef模板49<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
      t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，
      t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，
      t42、t43、t44、t45、t46、t47、t48、t49>型；
}；

//类型列表模板可以使用单个类型
//或类型“test”case（）中的类型<…>列表，以及
//实例化_-typed_-test_-case_p（）。

模板<typename t>
结构类型列表
  typedef types1<t>类型；
}；

template<typename t1，typename t2，typename t3，typename t4，typename t5，
    类型名t6，类型名t7，类型名t8，类型名t9，类型名t10，
    类型名t11，类型名t12，类型名t13，类型名t14，类型名t15，
    类型名t16，类型名t17，类型名t18，类型名t19，类型名t20，
    类型名称t21，类型名称t22，类型名称t23，类型名称t24，类型名称t25，
    类型名t26，类型名t27，类型名t28，类型名t29，类型名t30，
    类型名t31，类型名t32，类型名t33，类型名t34，类型名t35，
    类型名t36，类型名t37，类型名t38，类型名t39，类型名t40，
    类型名t41，类型名t42，类型名t43，类型名t44，类型名t45，
    类型名t46、类型名t47、类型名t48、类型名t49、类型名t50>
struct typelist<types<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，t13，
    t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，t27，t28，
    t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，t41，t42，t43，
    t44、t45、t46、t47、t48、t49、t50>
  typedef typename types<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10，t11，t12，
      t13，t14，t15，t16，t17，t18，t19，t20，t21，t22，t23，t24，t25，t26，
      t27，t28，t29，t30，t31，t32，t33，t34，t35，t36，t37，t38，t39，t40，
      t41、t42、t43、t44、t45、t46、t47、t48、t49、t50>：类型类型；
}；

endif//gtest_有_类型_测试_gtest_有_类型_测试_P

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_type_util_h_

//由于C++预处理器的怪异，我们需要双重间接。
//当两个令牌中的一个是uu line_uuu时连接两个令牌。写作
/ /
//foo___盄line_uuu
/ /
//将导致标记foo_uuu line_uuu，而不是foo后面跟
//当前行号。有关详细信息，请参阅
//http://www. PARASIFIF.COM/C++-FAQLIT/MISC技术问题。
定义gtest_concat_token（foo，bar）gtest_concat_token_impl_uu（foo，bar）
定义gtest_concat_token_impl_uu（foo，bar）foo_bar

类协议消息；
命名空间协议2类消息；

命名空间测试

//转发声明。

class assertion result；//断言的结果。
class message；//表示失败消息。
class test；//表示一个测试。
类testinfo；//有关测试的信息。
类test part result；//测试部件的结果。
class unittest；//一组测试用例。

模板<typename t>
：：std：：string printToString（const&value）；

命名空间内部

struct traceinfo；//跟踪点信息。
class scoped trace；//实现范围跟踪。
类testinfoimpl；//testinfo的不透明实现
类UnitTestImpl；//UnitTest的不透明实现

//调用了多少次initGoogleTest（）。
gtest_api_uuextern int g_init_gtest_count；

//失败消息中用于指示
//堆栈跟踪。
gtest_api_uuextern const char kstacktracemarker[]；

//两个重载助手，用于在编译时检查
//表达式是空指针文本（即空或任何0值
//编译时积分常量）。他们的回报值有
//大小不同，因此我们可以使用sizeof（）来测试哪个版本是
//由编译器选择。这些帮助器没有实现，例如
我们只需要他们的签名。
/ /
//给定isNullLiteralHelper（x），编译器将选择第一个
//version if x可以隐式转换为secret*，并选择
//否则是第二个版本。因为秘密是秘密，不完整
//类型，用户可以写入的唯一具有类型secret*的表达式是
//空指针文本。因此，我们知道x是一个空值
//如果且仅当第一个版本被
//编译器。
char isNullLiteralHelper（秘密*p）；
char（&isNullLiteralHelper（…））[2]；/nolint

//编译时bool常量，当且仅当x是
//空指针文本（即空或任何0值编译时
//积分常数）。
ifdef gtest_ellipsis_needs_pod_
//如果编译器不喜欢空检测，我们将失去对空检测的支持
//通过省略号（…）传递非pod类。
define gtest_is_null_literal_ux）false
否则
define gtest_is覕null覕literal_x覕
    （sizeof（：：testing:：internal:：isNullLiteralHelper（x））==1）
endif//gtest_ellipsis_needs_pod_

//将用户提供的消息附加到Google测试生成的消息。
gtest_api_u std:：string appendusermessage（
    const std:：string&gtest_msg，const message&user_msg）；

如果GTEST诳U有诳U例外

//此异常由（且仅由）失败的Google测试引发
//当gtest_标志（throw_on_failure）为真时断言（如果异常
//已启用）。我们从std：：runtime_error派生它，它用于
//可能仅在运行时检测到错误。自从
//std:：runtime_错误继承自std:：exception，许多测试
//框架知道如何提取和打印其中的消息。
类gtest_api_ugletestfailureexception:public:：std:：runtime_error_
 公众：
  显式googletestfailureexception（const testpartresult&failure）；
}；

endif//gtest_有_例外

//用于在用户程序中创建作用域跟踪的帮助程序类。
类gtest_api_uuScopedTrace_
 公众：
  //选择器将给定的源文件位置和消息推送到
  //由google test维护的跟踪堆栈。
  ScopedTrace（const char*文件、int行、const消息和消息）；

  //司机弹出司机推送的信息。
  / /
  //请注意，为了提高效率，d'tor不是虚拟的。
  //不要从ScopedTrace继承！
  ~范围内跟踪（）；

 私人：
  gtest不允许复制和分配（scopedtrace）；
gtest_attribute_unused_u；//ScopedTrace对象在其
                            //c'tor和d'tor。所以它没有
                            //否则需要使用。

名称空间编辑距离
//返回从“左”到“右”的最佳编辑。
//所有编辑的成本相同，替换的优先级低于
//添加/删除。
//Wagner Fischer算法的简单实现。
//参见http://en.wikipedia.org/wiki/wagner-fischer_算法
枚举编辑类型kmatch，kadd，kremove，kreplace
gtest_api_uustd:：vector<editType>calculateOptimaledits（
    const std:：vector<size_t>左，const std:：vector<size_t>右；

//与上面相同，但输入被表示为字符串。
gtest_api_uustd:：vector<editType>calculateOptimaledits（
    const std:：vector<std:：string>左（&L）
    const std:：vector<std:：string>&right）；

//以统一的diff格式创建输入字符串的diff。
gtest_api_uuStd:：string createUnifiedDiff（const std:：vector<std:：string>&left，
                                         const std:：vector<std:：string>右（&R）
                                         大小上下文=2）；

//命名空间编辑_距离

//计算“左”和“右”之间的差异，并以统一的差异返回
//格式。
//如果不为空，则将找到的行总数存储在“total \u line \u count”中
//左+右。
gtest_api_uustd:：string diffstrings（const std:：string&left，
                                   常量标准：：字符串和右键，
                                   尺寸_t*总_行_计数）；

//构造并返回相等断言的消息
//（例如断言eq、预期streq等）失败。
/ /
//前四个参数是断言中使用的表达式
//及其值，作为字符串。例如，对于断言eq（foo，bar）
//其中foo为5，bar为6，我们有：
/ /
//应输入_表达式：“foo”
//实际_表达式：“bar”
//应为_值：“5”
//实际值：“6”
/ /
//如果断言为
//*_strcaseq*。如果为真，字符串“（忽略大小写）”将
//插入到消息中。
gtest-api-assertionresult eqfailure（const char*应为表达式，
                                     const char*实际表达式，
                                     const std：：字符串和预期的_值，
                                     const std：：字符串和实际值，
                                     bool忽略了\u case）；

//为布尔断言（如expect_true）构造失败消息。
gtest_api_uu std:：string getBooleassertionFailureMessage（）。
    const断言结果&断言结果，
    const char*表达式文本，
    const char*实际谓词值，
    const char*预期的谓词值）；

//此模板类表示一个IEEE浮点数
//（单精度或双精度，取决于
//模板参数）。
/ /
//此类的目的是做更复杂的数字
//比较。（由于四舍五入误差等原因，很可能
//两个浮点完全相等。因此天真
//通过==操作进行比较通常不起作用。）
/ /
//IEEE浮点格式：
/ /
//最有意义的位是最左边的，一个IEEE
//浮点看起来像
/ /
//符号\位指数\位分数\位
/ /
//这里，符号位是一个指定
/数字。
/ /
//对于float，有8个指数位和23个小数位。
/ /
//对于double，有11个指数位和52个小数位。
/ /
//有关详细信息，请访问
//http://en.wikipedia.org/wiki/ieee_浮点_标准。
/ /
//模板参数：
/ /
//raw type:原始浮点类型（float或double）
模板<typename rawtype>
类浮动点
 公众：
  //定义的无符号整数类型的大小与
  //浮点数。
  typedef typename typewithsize<sizeof（rawtype）>：uint位；

  / /常量。

  一个数的位的//。
  静态常量大小_t kbitcount=8*sizeof（rawtype）；

  一个数中分数位的//。
  静态常量大小_t kFractionBitcount=
    std:：numeric_limits<rawtype>：：数字-1；

  一个数的指数位的//。
  静态常量大小“kexponentbitcount=kbitcount-1-kfractionbitcount”；

  //符号位的掩码。
  static const bits ksignbitmask=static_cast<bits>（1）<（kbitcount-1）；

  //分数位的掩码。
  静态常量位kFractionBitmask=
    ~static_cast<bits>（0）>>（kexponentbitcount+1）；

  //指数位的掩码。
  静态常量位kexponentbitmask=~（ksignbitmask kfractionbitmask）；

  //我们要容忍多少ULP（最后一个单位），当
  //比较两个数字。值越大，我们的错误就越多
  /允许。0值意味着两个数字必须完全相同
  //被视为相等。
  / /
  //单个浮点运算的最大误差为0.5
  //单位位于最后一个位置。在Intel CPU上，所有浮点
  //计算的精度为80位，而double的精度为64位
  //位。因此，4应该足够普通使用。
  / /
  //有关ulp的详细信息，请参阅以下文章：
  //http://randomascii.wordpress.com/2012/02/25/comparising-floating-point-numbers-2012-edition/
  静态常量大小_t kmaxulps=4；

  //从原始浮点数构造floating point。
  / /
  //在Intel CPU上，传递非规范化NaN（不是数字）
  //around可能会改变其位，尽管新值是有保证的
  //同时也是一个NAN。因此，不要期望此构造函数
  //当x是NaN时，保留x中的位。
  显式浮点（const rawtype&x）u_u.value_x；

  //静态方法

  //将位模式重新解释为浮点数。
  / /
  //需要此函数来测试almostEquals（）方法。
  静态rawtype reinterpretbits（const bits位）
    浮点数fp（0）；
    fp.u_u.bits_u u=位；
    返回fp.u_u.value_u u；
  }

  //返回表示正无穷大的浮点数。
  静态rawtype infinity（）
    返回reinterpretBits（kexponentBitmask）；
  }

  //返回可表示的最大有限浮点数。
  静态rawtype max（）；

  //非静态方法

  //返回表示此数字的位。
  const bits&bits（）const返回u _u.bits

  //返回该数字的指数位。
  bits exponent_bits（）const返回kexponentbitmask&u_u.bits_

  //返回该数字的小数位。
  bits fraction_bits（）const返回kFractionBitmask&u_.bits_

  //返回该数字的符号位。
  bits sign_bit（）const返回ksignbitmask&u_u.bits_

  //返回true iff这是NaN（不是数字）。
  bool is_nan（）const_
    //如果指数位都是1和分数，则为NaN
    //位不完全是零。
    返回（指数_位（）==kexponentbitmask）&&（分数_位（）！＝0）；
  }

  //返回真iff此数字最多不超过kmaxulps ulp
  //RHS。特别是，此功能：
  / /
  如果其中一个数字是（或两者都是）NaN，则//-返回false。
  //-将真正大的数字视为几乎等于无穷大。
  //-认为+0.0和-0.0相距0 dlp。
  bool almostquals（const floatingpoint&rhs）const
    //IEEE标准规定，任何涉及
    //NaN必须返回false。
    如果（is_nan（）rhs.is_nan（））返回false；

    返回gnimagnitudenumbers（u位、rhs.u位）之间的距离
        <kxululp；
  }

 私人：
  //用于存储实际浮点数的数据类型。
  活接头浮动点活接头
    rawtype value_；//原始浮点数。
    bits位；//表示数字的位。
  }；

  //将整数从符号和大小表示转换为
  //有偏差的表示。更准确地说，让n为2
  //的幂（kbitcount-1），整数x由
  //无符号数字x+n。
  / /
  //例如，
  / /
  //-n+1（使用
  //符号和大小）用1表示；
  //0用n表示；并且
  //n-1（使用
  //符号和大小）用2n-1表示。
  / /
  //阅读http://en.wikipedia.org/wiki/signed_number_representations
  //有关有符号数字表示的详细信息。
  静态位签名和诊断（const bits&sam）
    如果（ksignbitmask&sam）
      //Sam表示负数。
      返回~sam+1；
    }否则{
      //Sam表示正数。
      返回ksignbitmask sam；
    }
  }

  //在符号和大小表示中给定两个数字，
  //以无符号数返回它们之间的距离。
  静态位之间的距离（常量位和sam1，
                                                     常量位和sam2）
    const bits biased1=要校准的签名和诊断（sam1）；
    const bits biased2=签名和诊断（sam2）；
    返回（biased1>=biased2）？（biased1-biased2）：（biased2-biased1）；
  }

  浮点数联合
}；

//我们不能使用std:：numeric_limits，因为它与max（）冲突。
//宏由<windows.h>定义。
模板< >
inline float floatingpoint<float>：：max（）return flt_max；
模板< >
inline double floatingpoint<double>：：max（）return dbl_max；

//typedefs我们的floatingpoint模板类的实例
/小心使用。
typedef floatingpoint<float>float；
typedef floatingpoint<double>double；

//为了找出使用不同的
//在同一个测试用例中测试fixture类，我们需要分配
//fixture类的唯一ID并进行比较。typeid类型为
//用于保存此类ID。用户应将typeid视为不透明的
//类型：对typeid值唯一允许的操作是比较
//使用==运算符表示相等。
typedef const void*typeid；

模板<typename t>
类类型IDHelper
 公众：
  //虚拟_u不能有const类型。否则一个过分渴望
  //编译器（例如MSVC 7.1和8.0）可能会尝试合并
  //typeidHelper<t>：将不同的ts作为“优化”进行虚拟。
  静态布尔假人
}；

模板<typename t>
bool typeidhelper<t>：：dummy_u=false；

//gettypeid<t>（）返回t类型的ID。不同的值将
//为不同类型返回。使用调用函数两次
//相同类型的参数保证返回相同的ID。
模板<typename t>
typeid gettypeid（）
  //需要编译器来分配
  //typeidHelper<t>：用于实例化的每个t的虚拟变量
  //模板。因此，虚拟地址保证
  独一无二。
  返回&（typeidhelper<t>：：dummy_u）；
}

//返回：：testing:：test的类型ID。总是叫这个来代替
//的get type id<：测试：：测试>（）以获取的类型ID
//：：测试：：测试，因为后者可能由于
//将Google测试编译为Mac OS X时怀疑存在链接器错误
//框架。
gtest_api_utypeid gettestypeid（）；

//定义创建实例的抽象工厂接口
//测试对象的。
类TestFactoryBase
 公众：
  虚拟~TestFactoryBase（）

  //创建要运行的测试实例。创建和销毁实例
  //在testinfoimpl:：run（）中
  虚拟测试*createTest（）=0；

 受保护的：
  testfactoryBase（）

 私人：
  gtest不允许复制和分配（testfactoryBase）；
}；

//此类提供了Teatfactorybase接口的实现。
//用于测试和测试宏。
template<class testclass>
类testfactoryImpl:公共testfactoryBase
 公众：
  virtual test*createtest（）返回新的testclass；
}；

如果GTEST操作系统窗口

//用于实现hresult检查宏的谓词格式化程序
/断言预期结果成功失败
//我们传递long而不是hresult，以避免导致
//包含对hresult类型的依赖项。
gtest_API_uuAssertionResult ishResultSuccess（const char*expr，
                                            long hr）；//nolint
gtest_API_uuAssertionResult ishResultFailure（const char*expr，
                                            long hr）；//nolint

endif//gtest_os_windows

//setuptestcase（）和teardowntestcase（）函数的类型。
typedef void（*setuptestcasefunc）（）；
typedef void（*teardowntestcasefunc）（）；

//创建一个新的testinfo对象并将其注册到google test；
//返回创建的对象。
/ /
//参数：
/ /
//测试用例名称：测试用例的名称
//name:测试名称
//type_param测试类型参数的名称，如果
//这不是类型化测试或类型参数化测试。
//测试值参数的value_param文本表示，
//如果不是类型参数化测试，则为空。
//fixture_class_id:测试fixture类的ID
//设置\u tc:指向设置测试用例的函数的指针
//拆下\u tc：指向拆下测试用例的函数的指针
//工厂：指向创建测试对象的工厂的指针。
//新创建的testinfo实例将假定
//工厂对象的所有权。
gtest_api_uuestinfo*生成和注册testinfo（
    const char*测试用例名称，
    常量字符*名称，
    const char*类型\u参数，
    常量char*值\u参数，
    typeid夹具\u类\u id，
    设置测试案例功能设置
    拆卸下外壳
    测试工厂基础*工厂）；

//如果*pstr以给定前缀开头，则将*pstr修改为正确的
//超过前缀并返回true；否则保持*pstr不变
//并返回false。pstr、*pstr和prefix都不能为空。
gtest-api-bool skiprefix（const char*前缀，const char**pstr）；

如果GTEST_有_型_测试_GTEST_有_型_测试_P

//类型参数化测试用例定义的状态。
类gtest_api_uutypedtestcasepstate_
 公众：
  typedtestcasepstate（）：已注册_u（false）

  //将给定的测试名称添加到定义的测试名称中，并返回true
  //如果测试用例尚未注册，则中止
  /程序。
  bool addtestname（const char*文件，int行，const char*大小写名称，
                   const char*测试名称）
    如果（注册）
      fprintf（stderr，“%s测试%s必须在前面定义”
              “注册\u类型的\u测试\u案例\p（%s，…）.\n”，
              formatfilelocation（file，line）.c_str（），test_name，case_name）；
      FFLUH（STDRR）；
      POSIX：：中止（）；
    }
    定义的测试名称插入（测试名称）；
    回归真实；
  }

  //验证已注册的测试是否与中的测试名称匹配
  //defined_test_names_uu；如果成功，返回已注册的_测试，或者
  //否则中止程序。
  const char*verifyregisteredtestnames（
      const char*文件，int行，const char*注册的_测试）；

 私人：
  布尔已注册
  ：：std：：set<const char*>defined_test_names_uuu；
}；

//跳到“str”中第一个逗号后的第一个非空格字符；
//如果在“str”中找不到逗号，则返回空值。
inline const char*skipcomma（const char*str）
  const char*逗号=strchr（str，，'）；
  if（逗号=空）
    返回空；
  }
  while（isspace（（++逗号））
  返回逗号；
}

//返回“str”在其第一个逗号之前的前缀；返回
//如果不包含逗号，则为整个字符串。
inline std：：string getprefixuntilcomma（const char*str）
  const char*逗号=strchr（str，，'）；
  返回逗号==空？str:std:：string（str，逗号）；
}

//typeParameterizedTest<fixture，testsel，types>：：register（）
//用google test注册类型参数化测试的列表。这个
//返回值不重要-我们只需要返回一些
//这样我们就可以在命名空间范围内调用此函数。
/ /
//实现说明：gtest_template_u macro声明一个模板
//模板参数。它在gtest type util.h中定义。
template<gtest_template_uufixture，class testsel，typename types>
类类型参数化测试
 公众：
  //“index”是类型列表“types”中测试的索引
  //在实例化_类型的_test_case_p（prefix，test case，
  /类型。“index”的有效值为[0，n-1]，其中n是
  //类型长度。
  静态布尔寄存器（const char*前缀，const char*大小写名称，
                       const char*测试名称，int index）
    typedef typename types:：head type；
    typedef fixture<type>fixtureClass；
    typedef typename gtest_bind_u（testsel，type）测试类；

    //首先，注册类型中的第一个类型参数化测试
    //列表。
    生成和注册目标信息（
        （std：：string（prefix）+（prefix[0]='\0'？“）：“/”）+案例名称+“/”
         +streamableToString（index））.c_str（），
        striptrailingspaces（getPrefixUntilComma（test_name））.c_str（），
        gettypename<type>（）.c_str（），
        空，//无值参数。
        gettypeid<fixtureClass>（），
        测试类：：设置测试用例，
        测试类：：teardowntestcase，
        new testfactoryImpl<testclass>）；

    //接下来，使用类型列表的尾部递归（在编译时）。
    返回typeParameterizedTest<fixture，testsel，typename types:：tail>
        ：：寄存器（前缀，大小写名称，测试名称，索引+1）；
  }
}；

//编译时递归的基本情况。
template<gtest_template_uufixture，class testsel>
class typeparameterizedtest<fixture，testsel，types0>
 公众：
  静态布尔寄存器（const char*/*prefi*/, const char* /*case_name*/,

                       /*st char*/*测试_名称*/，int/*索引*/）
    回归真实；
  }
}；

//typeParameteredTestCase<fixture，tests，types>：：register（）
//在Google中注册“tests”和“types”的所有组合
/测试。回报值是微不足道的-我们只需要回报
//这样我们就可以在命名空间范围内调用此函数。
template<gtest_template_uufixture，typename tests，typename types>
类类型参数化测试用例
 公众：
  静态布尔寄存器（const char*前缀，const char*大小写名称，
                       const char*测试名称）
    typedef typename测试：：head head；

    //首先，在“test”中为“types”中的每个类型注册第一个测试。
    类型参数化测试<fixture，head，types>：：register（
        前缀，大小写名称，测试名称，0）；

    //接下来，用测试列表的尾部递归（在编译时）。
    返回typeParameterizedTestCase<fixture，typename tests:：tail，types>
        ：：寄存器（前缀，case_name，skipcomma（test_name））；
  }
}；

//编译时递归的基本情况。
template<gtest_template_uufixture，typename types>
class typeparameterizedtestcase<fixture，templates0，types>
 公众：
  静态布尔寄存器（const char*/*prefi*/, const char* /*case_name*/,

                       /*st char*/*测试_名称*/）
    回归真实；
  }
}；

endif//gtest_有_类型_测试_gtest_有_类型_测试_P

//以std:：string形式返回当前操作系统堆栈跟踪。
/ /
//要包含的最大堆栈帧数由
//gtest_stack_trace_depth标志。skip_count参数
//指定要跳过的顶层帧的数目，而不是
//根据要包含的帧数计数。
/ /
//例如，如果foo（）调用bar（），而bar（）又调用
//getcurrentOsstackTraceExceptTop（…，1），foo（）将包含在
//除了bar（）和getcurrentostackTraceExceptTop（）之外的跟踪不会。
gtest_api_uustd:：string getcurrentostacktraceexcepttop（
    单元测试*单元测试，int skip_count）；

//用于禁止对无法访问的代码或常量发出警告的帮助程序
/条件。

//始终返回true。
gtest_api_ubool alwaystrue（）；

//始终返回false。
inline bool alwaysfase（）返回！AlWistRuee（）；

//用于抑制常量char上clang的错误警告的帮助程序*
//条件表达式中声明的变量在中始终为空
//else分支。
结构gtest_api_uuConstCharptr_
  constcharptr（const char*str）：值（str）
  operator bool（）const返回true；
  常量char*值；
}；

//一个简单的线性同余生成器，用于生成随机
//具有均匀分布的数字。与rand（）和srand（）不同，它
//不使用全局状态（因此不能干扰用户
/ /代码）。与rand_r（）不同，它是可移植的。LCG不是很随机的，
//但这对我们来说已经足够好了。
类别GTest_API_uuRandom_
 公众：
  静态常量uint32 kmaxrange=1u<<31；

  显式随机（uint32 seed）：状态（seed）

  空重设（uint32 seed）状态=seed；

  //从[0，range]生成一个随机数。如果“range”为
  //0或大于kmaxrange。
  uint32生成（uint32范围）；

 私人：
  UIT32 32；
  不允许复制和分配（随机）；
}；

//定义compileasserttypesequal<t1，t2>类型的变量将导致
//编译器错误iff t1和t2是不同的类型。
template<typename t1，typename t2>
结构编译软件类型Sequal；

模板<typename t>
struct compileasserttypesequal<t，t>
}；

//如果引用类型是引用类型，则从类型中移除引用，
//否则保持不变。这和
//tr1：：删除\u引用，目前还没有广泛使用。
模板<typename t>
struct removereference typedef t type；；//nolint
模板<typename t>
struct removereference<t&>typedef t type；；//nolint

//一个方便的包装，用于在参数
//t取决于模板参数。
定义GTEST_删除参考_u（t）
    typename:：testing:：internal:：removereference<t>：：type

//如果类型是const类型，则从该类型中移除const，否则将离开
//不变。这与tr1相同：：remove_const，而不是
//广泛可用。
模板<typename t>
struct removeconst typedef t type；；//nolint
模板<typename t>
struct removeconst<const>typedef t type；；//nolint

//MSVC 8、Sun C++和IBM XL C++有一个导致上述问题的bug
//定义未能删除“const int[3]”和“const”中的const
//字符[3][4]'。下面的专门化解决了这个bug。
模板<typename t，大小\u t n>
结构移除
  typedef typename removeconst<t>：：type type[n]；
}；

如果定义（35u-msc-ver）&&35u-msc-ver<1400
//这是唯一允许VC++7.1在
//'const int[3]和'const int[3][4]'。但是，它会给GCC带来麻烦
//因此需要有条件地编译。
模板<typename t，大小\u t n>
结构移除
  typedef typename removeconst<t>：：type type[n]；
}；
第二节

//一个在参数
//t取决于模板参数。
定义gtest_删除常数（t）
    类型名：：测试：：内部：：删除内容t<t>：：类型

//将const u&、u&、const u和u全部转换为u。
定义GTEST_删除_引用_和_常量_u（t）
    删除常数（删除参考）

//如果类型不是引用类型，则添加对该类型的引用，
//否则保持不变。这和
//tr1：：添加_引用，目前还没有广泛使用。
模板<typename t>
struct addreference typedef t&type；；//nolint
模板<typename t>
struct addreference<t&>typedef t&type；；//nolint

//在参数t
//取决于模板参数。
定义gtest_添加参考_u（t）
    类型名：：测试：：内部：：AddReference<t>：：类型

//根据需要添加对t顶部const的引用。例如，
//它转换
/ /
//char==>常量char&
//常量char==>常量char&
//char&==>常量char&
//常量char&==>常量char&
/ /
//参数t必须依赖于某些模板参数。
定义GTESTU参考到常数（T）58396;
    gtest_add_reference_uu（const gtest_remove_reference_u（t））。

//implicitlyconvertable<from，to>：value是编译时bool
//不能隐式转换为
//类型为。
template<typename-from，typename-to>
类implicitlyconvertible
 私人：
  //我们只需要为其类型提供以下助手函数。
  //它们没有实现。

  //makeFrom（）是一个类型为From的表达式。我们不能简单地
  //使用From（），因为From类型可能没有公共默认值
  //构造函数。
  static typename addreference<from>：：type makefrom（）；

  //这两个函数被重载。给出一个表达式
  //helper（x），如果x可以
  //隐式转换为类型to；否则它将选择
  //第二个版本。
  / /
  //第一个版本返回大小为1的值，第二个版本返回
  //version返回大小为2的值。因此，通过检查
  //helper（x）的大小，可以在编译时完成，我们可以知道
  //使用了哪个版本的helper（），因此x是否可以
  //隐式转换为类型to。
  静态字符助手（to）；
  静态字符（&helper（…）[2]；/nolint

  //我们必须将“public”部分放在“private”部分之后，
  //或msvc拒绝编译代码。
 公众：
如果定义了（Borlandc_uuuu）
  //C++Builder不能在模板中使用成员过载解决方案
  //实例化。最简单的解决方法是使用它的C++ 0x类型特征。
  //函数（C++Builder 2009及以上）。
  static const bool value=uu可转换（从、到）；
否则
  //msvc警告从double隐式转换为int
  //可能丢失数据，因此我们需要暂时禁用
  /警告。
  gtest_disable_msc_warnings_push_u（4244）禁用_msc_警告
  静态常量bool值=
      sizeof（helper（implicitlyConvertible:：makeFrom（））=1；
  gtest禁用msc警告
endif/Uuuu Borlandc_uu
}；
template<typename-from，typename-to>
const bool implicitlyconvertable<from，to>：value；

//isaprotocolmessage:：value是编译时bool常量，它是
//true iff t是protocolmessage、protoc2:：message或子类类型
/那些。
模板<typename t>
结构IsaprotocolMessage
    ：public bool_constant<
  implicitlyConvertible<const t*，const:：protocolMessage*>：：value
  implicitlyconvertible<const t*，const:：proto2:：message*>：：value>
}；

//当编译器看到表达式isContainerTest（0）时，如果c是
//stl-style container类，iscontainerTest的第一个重载
//是可行的（因为C:：Iterator*和C:：Const_Iterator*都是
//有效类型和空值可以隐式转换为它们）。它将
//在第二个重载上选取，因为“int”与
//参数0的类型。如果C:：Iterator或C:：Const_Iterator不是
//有效类型，第一个重载不可行，第二个重载
//将选择重载。因此，我们可以确定c是否
//通过检查isContainerTest的类型来创建容器类。
//表达式的值不重要。
/ /
//请注意，我们同时查找C:：Iterator和C:：Const_Iterator。这个
/原因是C++将类的名称作为
//类本身（例如，可以将类迭代器作为
//“迭代器”或“迭代器：：迭代器”）。如果我们查找C:：Iterator
//只有，例如，我们会错误地认为一个名为
//迭代器是一个STL容器。
/ /
//还要注意更简单的重载方法
//isContainerTest（typename c:：const_迭代器*）和
//ISBuleCestTo（…）不适用于Visual Ac++和Sun C++。
typedef int iscontainer；
模板<class c>
iscontainer iscontainertest（int/*虚拟*/,

                            /*名称C：：迭代器*/*it*/=null，
                            类型名C：：const_迭代器*/*const_it*/ = NULL) {

  return 0;
}

typedef char IsNotContainer;
template <class C>
/*otcontainer iscontainertest（long/*dummy*/）返回\0'；

//enableif<condition>：：当“cond”为true时，类型为void，并且
//当'cond'为false时未定义。使用sfinae生成函数
//仅当特定表达式为true时才应用重载，添加
//“typename enableif<expression>：：type*=0”作为最后一个参数。
template<bool>struct-enableif；
template<>struct enableif<true>typedef void type；；//nolint

//本机数组的实用程序。

//arrayeq（）使用
//elements'operator==，其中k可以是任何大于等于0的整数。当K是
//0，arrayeq（）退化为比较一对值。

模板<typename t，typename u>
bool arrayeq（const t*lhs，大小，const u*rhs）；

//当k为0时使用此通用版本。
模板<typename t，typename u>
inline bool arrayeq（const t&lhs，const u&rhs）返回lhs==rhs；

//当k大于等于1时使用此重载。
template<typename t，typename u，size_t n>
inline bool arrayeq（const t（&lhs）[n]，const u（&rhs）[n]）；
  返回内部：：arrayeq（lhs，n，rhs）；
}

//此帮助程序可减少代码膨胀。如果我们把它的逻辑放进去
//上一个arrayeq（）函数，不同大小的数组将
//导致模板代码的不同副本。
模板<typename t，typename u>
bool arrayeq（const t*lhs，大小，const u*rhs）
  对于（尺寸t i=0；i！=大小；I++）{
    如果（！）内部：：arrayeq（lhs[i]，rhs[i]））
      返回错误；
  }
  回归真实；
}

//查找迭代器范围[开始，结束]中
//等于elem。元素本身可以是本机数组类型。
template<typename iter，typename element>
iter arrayawarefind（iter begin，iter end，const element&elem）
  它=开始；它！=结束；++it）{
    if（内部：：arrayeq（*it，elem））
      归还它；
  }
  返回端；
}

//copyArray（）使用元素复制k维本机数组
//operator=，其中k可以是大于等于0的任何整数。当K为0时，
//copyArray（）退化为复制单个值。

模板<typename t，typename u>
void copyarray（const*from，size_t size，u*to）；

//当k为0时使用此通用版本。
模板<typename t，typename u>
inline void copyarray（const t&from，u*to）*to=from；

//当k大于等于1时使用此重载。
template<typename t，typename u，size_t n>
inline void copyarray（const t（&from）[N]，u（*to）[N]）
  内部：：CopyArray（从，n，*到）；
}

//此帮助程序可减少代码膨胀。如果我们把它的逻辑放进去
//上一个copyArray（）函数，不同大小的数组
//将导致模板代码的不同副本。
模板<typename t，typename u>
void copyarray（const*from，size_t size，u*to）
  对于（尺寸t i=0；i！=大小；I++）{
    内部：：CopyArray（从[i]到+i）；
  }
}

//NativeArray对象（见下文）与
//它表示的本机数组。
//我们使用两个不同的结构来允许使用不可复制的类型，只要
//传递RelationToSourceReference（）。
结构相对源引用；
结构RelationToSourceCopy；

//使本机数组适应只读STL样式容器。相反
//对于完整的STL容器概念，此适配器仅实现
//对Google Mock的容器匹配器有用的成员。新成员
//应根据需要添加。为了简化实现，我们只
//支持元素是原始类型（即没有顶级常量或
//引用修饰符）。客户有责任满足
//此要求。元素本身可以是数组类型（因此
//支持多维数组）。
模板<typename element>
类NativeArray_
 公众：
  //stl样式容器typedef。
  typedef元素值_type；
  typedef元素*迭代器；
  typedef const element*const_迭代器；

  //从本机数组构造。引用源。
  NativeArray（const element*array，size_t count，relationSourceReference）
    initref（数组，计数）；
  }

  //从本机数组构造。复制源。
  NativeArray（const element*array，size_t count，relationSourceCopy）
    initcopy（数组，计数）；
  }

  //复制构造函数。
  nativearray（const nativearray&rhs）
    （此->*rhs.clone_uu）（rhs.array_u，rhs.size_u）；
  }

  ~nativeArray（）
    如果（克洛尼！=本机数组（&N）：initref）
      删除[]数组
  }

  //stl样式的容器方法。
  size_t size（）常量返回大小
  const_iterator begin（）const返回数组_
  const_iterator end（）const返回数组_+size_
  bool operator==（const nativearray&rhs）const_
    返回大小（）==rhs.size（）&&
        arrayeq（begin（），size（），rhs.begin（））；
  }

 私人：
  枚举{
    kchecktypeisnotconstorareference=静态断言类型eqhelper<
        元素，gtest_remove_reference_and_const_u（element）>：值，
  }；

  //用输入的副本初始化此对象。
  void initcopy（const element*array，size_t a_size）
    element*const copy=new element[a_size]；
    CopyArray（数组，A_大小，copy）；
    复制；
    SiZe＝Ay尺寸；
    克隆&nativeArray:：initcopy；
  }

  //用输入的引用初始化此对象。
  void initref（const element*array，size_t a_size）
    阵列；
    SiZe＝Ay尺寸；
    克隆&nativeArray:：initref；
  }

  常量元素*数组
  SIEZHETT SIZEZ；
  void（nativearray:：*克隆）（const元素*，大小为t）；

  gtest不允许分配（nativearray）；
}；

//命名空间内部
//命名空间测试

定义gtest_message_at_u（文件、行、消息、结果类型）
  ：：测试：：内部：：断言帮助程序（结果类型、文件、行、消息）
    =：：测试：：消息（）

定义gtest_message_u（message，result_type）
  gtest_message_at_uuu（uu file_uuuu，uuu line_uuuu，message，result_type）（测试信息\u在\处（uu文件\行\信息，结果\类型）

定义gtest_致命_故障_uu（消息）
  返回gtest_message_u（消息，：：testing:：testpartresult:：kfatalfailure）

定义gtest_非致命性_故障_uu（消息）
  gtest_message_u（消息，：：testing:：testpartresult:：knonfatalfailure）

定义gtest_成功_uuu（消息）
  gtest_message_u（消息，：：testing:：testpartresult:：ksuccess）

//取消以下代码的MSVC警告4072（无法访问的代码）
//如果返回或引发（或不返回或引发某些
/情况。
定义gtest_抑制_不可到达_代码_警告_uu（语句）
  if（：：testing:：internal:：alwaystrue（））语句；

定义gtest_test_throw（语句，预期_exception，fail）
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  if（：：测试：：内部：：constchaptr gtest_msg=）
    bool gtest捕获了\u expected=false；\
    尝试{
      gtest_禁止在（语句）下面出现无法访问的代码警告
    }
    catch（应为\u exception const&）\
      gtest_catched_expected=true；\
    }
    渔获量（…）{
      gtest_msg.value=\
          “应为：”语句“引发类型为”的异常
          预期的_异常。\n实际：它抛出了一个不同的类型。
      goto gtest_concat_token_u（gtest_label_testhrow_uu，uu line_uuuu）；\
    }
    如果（！）GTEST_捕捉到了_
      gtest_msg.value=\
          “应为：”语句“引发类型为”的异常
          预期的_异常。\n实际：它什么也不抛出。“\35;
      goto gtest_concat_token_u（gtest_label_testhrow_uu，uu line_uuuu）；\
    }
  }否则
    gtest_concat_token_uu（gtest_label_testhrow_uu，uu line_uuuu）：\
      失败（gtest-msg.value）

定义gtest_test_no_throw_uu（语句，失败）
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  如果（：：测试：：内部：：alwaystrue（））
    尝试{
      gtest_禁止在（语句）下面出现无法访问的代码警告
    }
    渔获量（…）{
      转到gtest_concat_token_uu（gtest_label_testnothrow_uu，uu line_uuuu）；\
    }
  }否则
    gtest_concat_token_uu（gtest_label_testnothrow_uu，uu line_uuu）：\
      fail（“expected：”statement“不会引发异常。\n \
           “实际：它抛出。”）

定义gtest_test_any_throw_uu（语句，失败）
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  如果（：：测试：：内部：：alwaystrue（））
    bool gtest捕获了\u any=false；\
    尝试{
      gtest_禁止在（语句）下面出现无法访问的代码警告
    }
    渔获量（…）{
      gtest_捕获了\u any=true；\
    }
    如果（！）GTEST_抓到了
      goto gtest_concat_token_u（gtest_label_testanythrow_uu，uu line_uuuu）；\
    }
  }否则
    gtest_concat_token_uu（gtest_label_testanythrow_uu，uu line_uuu）：\
      fail（“expected：”statement“引发异常。\n”
           “实际：没有。”）


//实现布尔测试断言，如expect_true。表达式可以是
//布尔表达式或断言结果。文本是文本
//将表达式重新呈现为expect_true。
定义gtest_test_boolean_u（表达式，文本，实际，预期，失败）
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  if（const:：testing:：assertionresult gtest_ar_uu=\
      ：：测试：：断言结果（表达式））\
    \
  否则
    失败（：：测试：：内部：：GetBooleassertionFailureMessage（\
        gtest_ar_u，text，actual，expected.c_str（））

定义gtest_test_no_fatal_failure_uu（语句，失败） \
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  如果（：：测试：：内部：：alwaystrue（））
    ：：测试：：内部：：hasNewFatalFailureHelper GTest_致命故障_检查器；\
    gtest_禁止在（语句）下面出现无法访问的代码警告
    if（gtest_fatal_failure_checker.has_new_fatal_failure（））
      转到gtest_concat_token_u（gtest_label_testno fatal_uuu，uuu line_uuuuu）；\
    }
  }否则
    gtest_concat_token_uu（gtest_label_testno fatal_uu，uu line_uuuu）：\
      fail（“预期：”语句“不会产生新的致命性”
           “当前线程失败。\n”\
           “实际：确实如此。”）

//扩展到实现给定测试的类的名称。
定义gtest_test_class_name_u（test_case_name，test_name） \
  测试用例测试

//用于定义测试的helper宏。
定义gtest_test_u（test_case_name，test_name，parent_class，parent_id） \
class gtest_test_class_name_u（test_case_name，test_name）：公共父级\
 公众：
  gtest_test_class_name_u（test_case_name，test_name）（）
 私人：
  虚拟void testbody（）；\
  static:：testing:：test info*const test_info_ugtest_attribute_unused_uuu；\
  不允许复制和分配
      gtest_test_class_name_u（test_case_name，test_name））；\
}；
\
：：testing：：testinfo*const gtest_test_class_name（test_case_name，test_name）测试\
  ：：TestyInfo=
    ：：测试：：内部：：生成和注册目标信息（\
        测试用例名称，测试名称，空，空
        （帕伦特）
        父级\u class:：SetupTestCase，\
        父级\u class:：teardowntestcase，\
        新：：测试：：内部：：TestFactoryImpl<\
            gtest_test_class_name_u（test_case_name，test_name）>）；\
void gtest_test_class_name_u（test_case_name，test_name）：：testbody（））

endif//gtest_包括\u gtest_internal_gtest_internal_h_

//版权所有2005，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan）
/ /
//谷歌C++测试框架（谷歌测试）
/ /
//此头文件定义了用于死亡测试的公共API。它是
//包含在gtest.h中，因此用户不需要包含此内容
/直接。

ifndef gtest_包括gtest_gtest_death_测试_h_
定义gtest_包括gtest_gtest_death_test_h_

//版权所有2005，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan），eefacm@gmail.com（sean mcafee）
/ /
//谷歌C++测试框架（谷歌测试）
/ /
//此头文件定义实现所需的内部实用程序
//死亡测试。如有更改，恕不另行通知。

ifndef gtest_包括\u gtest_internal_gtest_death_test_internal_h_
定义gtest_包括gtest_内部gtest_死亡测试_内部


include<stdio.h>

命名空间测试
命名空间内部

gtest-declare-string（内部运行死亡测试）；

//标志的名称（分析Google测试标志时需要）。
const char kdeathteststyleflag[]=“死亡测试样式”；
const char kdeathtestusefork[]=“死亡测试使用\u fork”；
const char kinternalundeathtestflag[]=“内部运行\死亡\测试”；

如果GTEST覕U有覕死亡覕测试

//DeathTest是一个类，它隐藏了
//gtest_death_test_u宏。它是抽象的；它的静态创建方法
//返回依赖于当前死亡测试的具体类
//样式，由--gtest_death_test_样式和/或定义
//-gtest_内部_运行_死亡_测试标志。

//在描述死亡测试结果时，这些术语用于
//对应定义：
/ /
//退出状态：指定格式的整数退出信息
//等待（2）
//退出代码：传递给exit（3）、exit（2）或
//从main（）返回
等级：GTESTUAPIUUUdeathtest_
 公众：
  //如果在确定
  //当前死亡测试要采取的适当操作；例如，
  //如果gtest_death_test_样式标志设置为无效值。
  //lastmessage方法将返回一个更详细的消息，
  //病例。否则，由“测试”指向的死亡测试指针
  //参数已设置。如果应该跳过死亡测试，则指针
  //设置为空；否则设置为新混凝土的地址
  //控制当前测试执行的DeathTest对象。
  静态bool create（const char*语句，const re*regex，
                     const char*文件，int行，deathtest**测试）；
  死亡测试（）；
  虚拟~deathtest（）

  //在删除死亡测试时中止该测试的帮助程序类。
  类返回Sentinel
   公众：
    显式返回sentinel（deathtest*test）：测试（test）
    ~returnsentinel（）test_->abort（test_遇到_return_语句）；
   私人：
    死亡测试*const测试
    gtest不允许拷贝和分配；
  gtest_属性_未使用

  //死亡时可能承担的角色的枚举
  //遇到测试。执行意味着死亡测试逻辑应该
  //立即执行。监督意味着项目应该准备
  //子进程执行死亡的适当环境
  //测试，然后等待它完成。
  枚举testrole监督测试，执行测试

  //可能中止测试的三个原因的枚举。
  枚举Aborteason
    测试遇到了返回语句，
    测试抛出了异常，
    测试没有死
  }；

  //担任上述角色之一。
  虚拟测试角色assumerole（）=0；

  //等待死亡测试完成并返回其状态。
  virtual int wait（）=0；

  //如果死亡测试通过，则返回true；也就是说，测试过程
  //在测试期间退出，其退出状态与用户提供的
  //谓词及其stderr输出与用户提供的正则
  //表达。
  //用户提供的谓词可能是宏表达式，而不是
  //不是函数指针或函数，或者是wait和passed
  联合起来。
  通过虚拟bool（bool exit_status_ok）=0；

  //表示死亡测试没有按预期死亡。
  虚拟无效中止（中止原因）=0；

  //返回有关
  //最后一次死亡测试。
  静态const char*lastmessage（）；

  静态void set_last_death_test_message（const std:：string&message）；

 私人：
  //包含上次死亡测试结果说明的字符串。
  static std:：string last_death_test_message_u；

  gtest不允许复制和分配（deathtest）；
}；

//死亡测试的工厂接口。可以模拟出来进行测试。
死亡测试工厂
 公众：
  虚拟~DeathTestFactory（）
  虚拟bool create（const char*语句，const re*regex，
                      const char*文件，int行，deathtest**测试）=0；
}；

//用于正常使用的具体DeathTestFactory实现。
类DefaultDeathTestFactory:公共DeathTestFactory
 公众：
  虚拟bool create（const char*语句，const re*regex，
                      const char*文件，int行，deathtest**测试）；
}；

//如果exit_status描述终止的进程，则返回true
//通过一个信号，或以非零退出代码正常退出。
gtest-api-bool退出失败（int-exit-boo状态）；

//陷阱C++异常逃避语句并作为测试报告它们
/失败。请注意，这里不实现捕获SEH异常。
如果GTEST诳U有诳U例外
define gtest_execute_death_test_statement_u（statement，death_test）
  尝试{
    gtest_禁止在（语句）下面出现无法访问的代码警告
  catch（const:：std:：exception&gtest_exception）
    FPrTNF（\
        STDRR
        \n%s:捕获了std:：exception派生的异常，该异常转义了\
        “死亡测试声明。异常消息：%s\n“，
        ：：测试：：内部：：格式文件位置（uu file_uuu，uu line_uuuu）.c_str（），\
        gtest_exception.what（））；\
    fflush（stderr）；\
    死亡测试->中止（：：测试：：内部：：死亡测试：：测试引发了异常）；\
  捕获（…）
    死亡测试->中止（：：测试：：内部：：死亡测试：：测试引发了异常）；\
  }

否则
define gtest_execute_death_test_statement_u（statement，death_test）
  GTEST_抑制_Unreachable_code_warning_below_u（语句）

第二节

//此宏用于实现assert_death*，expect_death*，
//断言退出*，并期望退出*。
定义gtest_death_test_u（语句、谓词、regex、fail） \
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  如果（：：测试：：内部：：alwaystrue（））
    const:：testing:：internal:：re&gtest_regex=（regex）；\
    ：：测试：：内部：：死亡测试*测试\
    如果（！）：：测试：：内部：：DeathTest：：创建（语句，&gtest_regex，\
        _uuu file_uuu，uu line_uuu，&gtest_dt））
      转到gtest concat_token_uu（gtest_label_uu，uu line_uuuuu）；\
    }
    如果（GestydT）！{NULL）{
      ：：测试：：内部：：作用域\ptr<：测试：：内部：：DeathTest>\
          gtest_dt_ptr（gtest_dt）；\
      开关（gtest_dt->assumerole（））\
        案例：：测试：：内部：：死亡测试：：监督测试：
          如果（！）gtest_dt->passed（谓词（gtest_dt->wait（））
            转到gtest concat_token_uu（gtest_label_uu，uu line_uuuuu）；\
          }
          打破；
        案例：：测试：：内部：：死亡测试：：执行_测试：
          ：：测试：：内部：：死亡测试：：返回Sentinel \
              gtest_哨兵（gtest_dt）；\
          gtest_execute_death_test_statement_u（语句，gtest_dt）；\
          gtest_dt->abort（：：testing:：internal:：deathtest:：test_did_did_die）；\
          打破；
        }
        默认值：
          打破；
      }
    }
  }否则
    gtest_concat_token_uuu（gtest_label_uuu，uuu line_uuuu）：\
      失败（：：测试：：内部：：DeathTest:：LastMessage（））
//此处的符号“fail”扩展为消息
//可以流式处理。

//此宏用于在中编译时实现assert/expect调试死亡
//ndebug模式。在这种情况下，我们需要执行语句，regex是
//忽略，并且宏必须接受流式消息，即使消息
//从不打印。
define gtest_execute_statement_u（语句，regex）
  Gtest_模棱两可\其他\阻滞剂\uuuuu \
  如果（：：测试：：内部：：alwaystrue（））
     gtest_禁止在（语句）下面出现无法访问的代码警告
  }否则
    ：：测试：：消息（）

//表示
/--gtest_内部运行_死亡_测试标志，因为它存在于
//调用了运行所有测试。
类InternalRundeathTestFlag_
 公众：
  InternalRundeathTestFlag（const std:：string&a_文件，
                           int a线，
                           国际指数
                           int ax写的fd
      ：文件u文件、行u行、索引u索引，
        写

  ~InternalRundeathTestFlag（）
    if（写fd_u>=0）
      posix:：close（写入fd）；
  }

  const std:：string&file（）const返回文件
  int line（）常量返回行
  int index（）常量返回索引
  int write_fd（）const返回write_fd_

 私人：
  std：：字符串文件\；
  INTINLY；
  int索引；
  In写；

  gtest不允许复制和分配（internalrundeathtestflag）；
}；

//返回新创建的带有字段的InternalRundeathTestFlag对象
//从gtest_标志（内部_运行_死亡_测试）标志初始化，如果
//已指定标志；否则返回空值。
InternalRundeathTestFlag*ParseInternalRundeathTestFlag（）；

否则//GTEST诳有诳死亡诳测试

//此宏用于实现宏，例如
//如果支持，则预期“死亡”，如果在系统上支持，则断言“死亡”
//不支持死亡测试。这些宏必须在这样的系统上编译
//iff expectou death和assertou death使用相同的参数编译
//支持死亡测试的系统。这样就可以编写这样的宏
//在不支持死亡测试的系统上，确保
//在死亡测试支持系统上编译。
/ /
/ /参数：
//语句-宏（如expect\death）将测试的语句
//用于程序终止。这个宏必须确保
//已编译但未执行语句，以确保
//如果支持编译
//参数iff expect_death与它一起编译。
//regex-宏（如expect \death）将用于测试的regex
//语句的输出。此参数必须是
//已编译但未由该宏计算，以确保
//此宏只接受宏
//希望死亡可以接受。
//terminator-如果支持，则必须是expect \u death \的空语句
//支持ActhTyDeAthiFiFor的返回语句。
//这样可以确保断言“如果支持”不会导致“死亡”
//编译断言死亡不存在的内部函数
//编译。
/ /
//使用始终为false的分支来确保
//语句和regex已编译（因而语法正确），但
//从未执行。无法访问的代码宏保护终止符
//语句生成“unreachable code”警告
//语句无条件返回或引发。位于的消息构造函数
//END允许将附加消息流式处理到
//宏，用于编译与Expect_Death/Assert_Death的兼容性。
定义gtest_unsupported_death_test_u（statement，regex，terminator）\
    Gtest_模棱两可\其他\阻滞剂\uuuuu \
    如果（：：测试：：内部：：alwaystrue（））
      测试日志（警告）
          <“此平台不支持死亡测试。\n”\
          <“statement'”statement“'cannot be verified.”；（无法验证）
    else if（：：testing:：internal:：AlwaysFase（））
      ：：测试：：内部：：re:：PartialMatch（“.*”，（regex））；\
      gtest_禁止在（语句）下面出现无法访问的代码警告
      终止符；
    }否则
      ：：测试：：消息（）

endif//gtest_具有_死亡_测试

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_death_test_internal_h_

命名空间测试

//此标志控制死亡测试的样式。有效值为“threadsafe”，
//表示死亡测试子进程将重新执行测试二进制文件
//从一开始，只运行一个死亡测试，或“fast”，
//表示子进程将立即执行测试逻辑
//分叉后。
gtest-declare-string（死亡测试风格）；

如果GTEST覕U有覕死亡覕测试

命名空间内部

//返回一个布尔值，指示调用方当前是否
//在死亡测试子进程的上下文中执行。工具如
//Valgrind堆检查程序可能需要这样做来修改其死亡行为
/测试。重要提示：这是一个内部实用程序。使用它可能会破坏
//执行死亡测试。用户代码不能使用它。
gtest_api_ubool indeathtestchild（）；

//命名空间内部

//以下宏可用于编写死亡测试。

//下面是当断言“死亡”或“预期死亡”是
/ /执行：
/ /
/ / 1。如果有多个活动项，它将生成一个警告
//线程。这是因为只有fork（）或clone（）是安全的
//当只有一个线程时。
/ /
/ / 2。父进程clone（）是一个子进程并运行死亡
//在其中进行测试；子进程在
//死亡测试，如果尚未退出。
/ /
/ / 3。父进程等待子进程终止。
/ /
/ / 4。父进程检查的退出代码和错误消息
//子进程。
/ /
/实例：
/ /
//断言“死亡”（server.sendmessage（56，“hello”），“无效端口号”）；
//对于（int i=0；i<5；i++）
//预期死亡（server.processrequest（i），
//“无效请求。*在processRequest（）”中）
//<“未能按要求死亡”<<i；
//}
/ /
//断言退出（server.exitnow（），：：testing:：exitedwitcode（0），“exiting”）；
/ /
//bool killedbysigup（int exit_code）
//返回wifsignaled（exit_code）&&wtermsig（exit_code）==叹气；
//}
/ /
//断言退出（client.hangupserver（），killedBySighup，“挂断！”；
/ /
//在死亡测试中使用的正则表达式上：
/ /
//在符合POSIX的系统（*nix）上，我们使用<regex.h>库，
//使用posix扩展regex语法。
/ /
//在其他平台（如Windows）上，我们只支持简单的regex
//作为google测试的一部分实现的语法。这有限
//编写时，大多数情况下实现应该足够
//死亡测试；虽然它缺少许多可以在PCRE中找到的功能
//或POSIX扩展regex语法。例如，我们不支持
//union（“x_y”）、grouping（“（x y）”）、方括号（“[x y]”）和
//重复计数（“x 5,7”），等等。
/ /
//下面是我们支持的语法。我们选择它是一个
//pcre和posix扩展regex的子集，因此很容易
//无论你来自哪里，都要学习。在以下内容中：“a”表示
//文字字符、句点（.）或单个\\转义序列；
//“x”和“y”表示正则表达式；“m”和“n”表示
//自然数。
/ /
//c匹配任何文本字符c
//\\d与任何十进制数字匹配
//\\d匹配任何不是十进制数字的字符
//\\f匹配\f
//\\n匹配\n
//\\r匹配\r
//\\s匹配任何ASCII空白，包括\n
//\\s匹配任何非空白字符
//\\t匹配\t
//\\v匹配\v
//\\w与任何字母、数字或十进制数字匹配
//\\w匹配\\w不匹配的任何字符
//\\c与任何文字字符c匹配，该字符必须是标点符号
/匹配任何单个字符，除了\n
/a？匹配0或1次
//a*匹配0次或多次出现的
//A+匹配1个或多个
//^匹配字符串的开头（不是每行的开头）
//$匹配字符串的结尾（不是每行的结尾）
//x y匹配x，后跟y
/ /
//如果意外使用PCRE或POSIX扩展regex功能
//我们没有实现，会出现运行时故障。在那
//大小写，请尝试在
//以上语法。
/ /
//此实现*不是*意味着高度调优或健壮
//作为已编译的regex库，但其性能应足以满足
//死亡测试，通过启动
//子进程。
/ /
//已知警告：
/ /
//一个“threasafe”类型的死亡测试获取测试的路径
//argv[0]中的程序，并在子进程中重新执行它。为了
//简单，当前实现不搜索路径
//启动子进程时。这意味着用户必须
//通过至少包含一个的路径调用测试程序
//路径分隔符（例如，path/to/foo_test和
///absolute/path/to/bar_测试可以，但foo_测试不行）。这个
//很少出现问题，因为人们通常不将测试二进制
//路径中的目录。
/ /
//todo（wan@google.com）：让线程安全死亡测试搜索路径。

//断言给定的语句导致程序退出，并且
//满足谓词并发出错误输出的整数退出状态
//与regex匹配。
define assert_exit（语句、谓词、regex）
    gtest_death_test_u（语句、谓词、regex、gtest_致命_失败）

//类似于assert_exit，但继续执行
//测试用例，如果有的话：
define expect_exit（语句、谓词、regex）
    gtest_death_test_u（语句、谓词、regex、gtest_非致命性_失败）

//断言给定语句导致程序退出，或者
//使用非零退出代码显式退出或被
//信号，并发出与regex匹配的错误输出。
定义断言死亡（语句，regex）
    断言退出（语句，：：testing：：internal：：exitedUnsuccessfully，regex）

//就像断言死亡，但继续进行
//测试用例，如果有的话：
定义预期死亡（声明，regex）
    预期退出（语句，：：testing：：internal：：exitedUnsuccessfully，regex）

//可以在断言中使用的两个谓词类，Expect _exit*：

//测试退出代码用给定的退出代码描述正常退出。
类gtest_api_uuedtwithcode_
 公众：
  显式exitedWithCode（int exit_code）；
  bool operator（）（int exit_status）const；
 私人：
  //没有实现-不支持赋值。
  void operator=（const exited with code&other）；

  const int退出代码
}；

如果有的话！GTEST操作系统窗口
//测试退出代码是否描述由
//给定信号。
等级GTESTUAPIUUUKILLEDBYSIGNAL_
 公众：
  显式KilledBySignal（int signum）；
  bool operator（）（int exit_status）const；
 私人：
  施工图
}；
我很喜欢你！GTEST操作系统窗口

//Expect调试死亡声明给定语句在调试模式下死亡。
//死亡测试框架使其具有有趣的语义，
//因为调用的副作用只在opt模式下可见，而不是
//在调试模式下。
/ /
//在实践中，这可用于测试使用
//使用以下样式记录（dfatal）宏：
/ /
//int dieindBugor12（int*副作用）
//如果（副作用）
//*副作用=12；
//}
//log（dfatal）<“死亡”；
/返回12；
//}
/ /
//测试（testcase，testdieor12worksindgbandopt）
//int sideffect=0；
////仅在dbg中断言。
//预期“debug”死亡（dieindBugor12（&sideffect），“死亡”）；
/ /
//ifdef ndebug
///opt模式具有可见的副作用。
//期望_eq（12，副作用）；
/或其他
///DBG模式没有可见的副作用。
//期望_eq（0，副作用）；
//π介子
//}
/ /
//这将断言dieindBugReturn12InOpt（）在调试时崩溃
//模式，通常是由于dcheck或log（dfatal），但返回
//在opt模式下适当的回退值（在本例中为12）。如果你
//需要测试函数在opt中是否有适当的副作用
//模式，包括针对副作用的断言。将军
//此模式为：
/ /
//期望_debug_death（
////此处的副作用将在中的此语句之后生效
///opt模式，但在调试模式下没有。
//期望_eq（12，dieindBugor12（&sideffect））；
//}，“死亡”；
/ /
nIFUF调试

定义expect诳debug诳death（statement，regex）诳
  gtest_execute_语句（语句，regex）

define assert_debug_death（statement，regex）\
  gtest_execute_语句（语句，regex）

否则

定义expect诳debug诳death（statement，regex）诳
  预期死亡（声明，regex）

define assert_debug_death（statement，regex）\
  断言死亡（声明，regex）

endif//预期调试死亡的ndebug
endif//gtest_具有_死亡_测试

//如果支持（statement，regex）和
//断言“如果支持则死亡”（statement，regex）扩展到实际死亡测试，如果
//支持死亡测试，否则只发出警告。这是
//将死亡测试断言与正常测试结合使用时很有用
//一个测试中的断言。
如果GTEST覕U有覕死亡覕测试
定义预期死亡，如果支持的话（语句，regex）
    预期死亡（声明，regex）
define assert_death_if_supported（statement，regex）
    断言死亡（声明，regex）
否则
定义预期死亡，如果支持的话（语句，regex）
    gtest_unsupported_death_test_u（声明，regex，）
define assert_death_if_supported（statement，regex）
    gtest_unsupported_death_test_u（语句，regex，返回）
第二节

//命名空间测试

endif//gtest_包括gtest_gtest_death_测试_h_
//此文件由以下命令生成：
//pump.py gtest-param-test.h.泵
//不要手工编辑！！！！

//版权所有2008，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：vladl@google.com（vlad losev）
/ /
//用于实现参数化测试的宏和函数
//谷歌C++测试框架（谷歌测试）
/ /
//此文件由脚本生成。不要手工编辑！
/ /
ifndef gtest_包括gtest_gtest_param_test_h_
定义gtest_包括gtest_gtest_param_test_h_


//值参数化测试允许您使用不同的
//不写入同一测试的多个副本的参数。
/ /
//以下是如何使用值参数化测试：

若0

//要编写值参数化测试，首先应该定义fixture
/类。它通常来自testing:：testwithParam（请参见下面的
//另一个继承方案有时在更复杂的情况下很有用
//类层次结构），其中是参数值的类型。
//testwithParam<t>本身是从testing:：test派生的。T可以是任何
//可复制类型。如果是原始指针，则负责管理
//指定值的寿命。

class footerst:public:：testing:：testwithParam<const char*>
  //您可以在这里实现所有常见的类fixture成员。
}；

//然后，使用test_p宏定义尽可能多的参数化测试
//根据您的需要。_p后缀用于“参数化”
//或者“模式”，随便你想什么。

测试_P（footerst，doesblah）
  //在测试内部，使用getParam（）方法访问测试参数
  //的testWithParam<t>类：
  期望_true（foo.blah（getparam（））；
  …
}

测试_P（footerst，hasblahbrah）
  …
}

//最后，可以使用实例化测试用例来实例化测试
//使用所需的任何一组参数。谷歌测试定义了一个数字
//用于生成测试参数的函数。他们归还我们所说的
（/惊讶！）参数生成器。下面是它们的摘要，其中
//都在测试命名空间中：
/ /
/ /
//range（begin，end[，step]）-生成值begin，begin+step，
//开始+步骤+步骤，…。价值观不是
//包含结束。步骤默认为1。
//值（v1，v2，…，vn）-生成值v1，v2，…，vn。
//valuesin（container）-从C样式数组、STL生成值
//valuesin（begin，end）容器或迭代器范围[begin，end]。
//bool（）-生成sequence false，true。
//combine（g1，g2，…，gn）-生成所有组合（笛卡尔积
//对于数学知识）生成的值
//由n个生成器生成。
/ /
//有关详细信息，请参见下面这些函数定义中的注释
//在这个文件中。
/ /
//以下语句将从最底部的测试用例实例化测试
//每个参数值为“meeny”、“miny”和“moe”。

实例化测试用例
                        测试器，
                        值（“meeny”、“miny”、“moe”）；

//为了区分模式的不同实例，（是的，您
//可以多次实例化）第一个参数
//实例化_test_case_p宏是将添加到
//实际测试用例名称。记住为不同的
//实例化。以上实例中的测试将
/这些名字：
/ /
//*instantiationname/footerst.doesblah/0表示“meeny”
//*instantiationname/footerst.doesblah/1表示“miny”
//*instantiationname/footerst.doesblah/2表示“moe”
//*instantiationname/footerst.hasblahblah/0用于“meeny”
//*instantiationname/footerst.hasblahblah/1表示“miny”
//*instantiationname/footerst.hasblahblah/2用于“moe”
/ /
//您可以在--gtest_过滤器中使用这些名称。
/ /
//此语句将再次从footerst实例化所有测试，每个测试
//参数值为“cat”和“dog”：

const char*pets[]=“cat”，“dog”
实例化_test_case_p（anotherInstantiationName，footst，valuesin（pets））；

//以上实例化的测试将具有以下名称：
/ /
//*对于“cat”，另一个instantiationname/footerst.doesblah/0
//*另一个“dog”的instantiationname/footerst.doesblah/1
//*另一个“cat”的instantiationname/footerst.hasblahblah/0
//*另一个“dog”的instantiationname/footerst.hasblahblah/1
/ /
//请注意，实例化_test_case_p将实例化所有测试
//在给定的测试用例中，它们的定义是否在
//在实例化_test_case_p语句之后。
/ /
//请注意，生成器表达式（包括
//generators）在启动main（）后在initGoogleTest（）中进行计算。
//这允许用户一方面按顺序调整生成器参数
//要动态确定要运行的一组测试，另一方面，
//让用户有机会使用Google测试检查生成的测试
//reflection api before run_all_tests（）is executed.
/ /
//您可以看到samples/sample7_unittest.cc和samples/sample8_unittest.cc
//更多示例。
/ /
//以后，我们计划发布用于定义新参数的API
/发电机。但是现在这个接口仍然是内部接口的一部分
//实现并可能更改。
/ /
/ /
//参数化测试夹具必须从testing:：test和from派生
//测试：：withParamInterface<t>，其中t是参数的类型
/ /值。从testwithParam继承满足该要求，因为
//testwithParam<t>继承自test和withParamInterface。在更多
//但是，继承复杂的层次结构有时很有用
//与test和withParamInterface分开。例如：

Class BaseTest:公共：：测试：：测试
  //可以继承非参数化测试的所有常规成员
  //这里是fixture。
}；

类派生测试：public basetest，public:：testing:：withParamInterface<int>>
  //通常的测试夹具成员也在这里。
}；

测试_F（basetest，hasfoo）
  //这是一个普通的非参数化测试。
}

测试（衍生测试，doesblah）
  //getparam在这里的工作方式与从testwithparam继承时的工作方式相同。
  期望_true（foo.blah（getparam（））；
}

NeNEFF／/ 0


如果有的话！通用测试系统
包括<utility>
第二节

//scripts/fusegtest.py取决于包含gtest自己的头
//*无条件地*。因此，这些包括不能移动
//如果gtest_有_参数_测试，则在内部。
//版权所有2008 Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：vladl@google.com（vlad losev）

//用于实现参数化测试的类型和函数实用程序。

ifndef gtest_包括\u gtest_internal_gtest_param_util_h_
定义GTEST_包括GTEST_内部GTEST_参数_实用程序_h_

include<iterator>
包括<utility>
include<vector>

//scripts/fusegtest.py取决于包含gtest自己的头
//*无条件地*。因此，这些包括不能移动
//如果gtest_有_参数_测试，则在内部。
//版权所有2003 Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：dan egnor（egnor@google.com）
/ /
//带有引用跟踪的“smart”指针类型。每个指向
//特定对象保存在循环链接列表中。当最后一个指针
//对象被销毁或重新分配，对象被删除。
/ /
//如果使用得当，则当最后一个引用消失时，将删除该对象。
//有几个注意事项：
与所有参考计数方案一样，周期会导致泄漏。
//-每个智能指针实际上是两个指针（8字节而不是4字节）。
//-每次分配指针时，指向该指针的整个指针列表
//对象被遍历。因此，该类不适用于
//通常是指向特定对象的两个或三个以上指针。
//-只要复制链接的对象，就只跟踪引用。
//如果链接指针<>转换为原始指针并返回，则会发生错误
//将发生（双重删除）。
/ /
//此类的一个好用途是在STL容器中存储对象引用。
//可以将链接指针<>安全地放在向量<>中。
//其他用途可能没有那么好。
/ /
//注意：如果使用的类型不完整且链接了ptr<>，则类
//*包含*链接的指针<>必须具有构造函数和析构函数（偶数
//如果他们什么都不做！）.
/ /
//比尔·吉本斯建议我们用这样的东西。
/ /
//线程安全：
//与其他链接指针实现不同，在此实现中
//在以下意义上，链接的指针对象是线程安全的：
//-同时复制链接的指针对象是安全的，
//-从*中复制*并读取其底层是安全的。
//同时使用原始指针（例如via get（））和
//-写两个指向相同的链接指针是安全的
//同时共享对象。
//todo（wan@google.com）：将此重命名为安全链接指针以避免
//与普通链接指针混淆。

ifndef gtest_包括\u gtest_internal_gtest_linked_ptr_h_
定义gtest_包括gtest_内部gtest_链接的ptr_h_

include<stdlib.h>
include<assert.h>


命名空间测试
命名空间内部

//保护所有链接的指针对象的复制。
gtest-api-gtest-declare-static-mutex（g-linked-ptr-mutex）；

//链接指针的所有实例都在内部使用。它需要
//非模板类，因为不同类型的链接指针可以引用
//同一对象（链接指针<superclass>（obj）vs链接指针<subclass>（obj））。
//因此，需要不同类型的链接指针参与
//在同一个循环链接列表中，因此这里需要一个类类型。
/ /
//不要直接使用这个类。使用链接的指针。
类链接_ptr_internal_
 公众：
  //创建只包含此实例的新圆。
  void join_new（）
    nExt=这个；
  }

  //许多链接指针操作可能会更改某些链接指针的p.link指针
  //变量p与此对象位于同一个圆中。因此我们需要
  //防止两个这样的操作同时发生。
  / /
  //请注意，不同类型的链接指针对象可以共存于
  //圆（例如链接指针<base>，链接指针<derived1>，和
  //链接指针<derived2>）。因此，我们必须使用一个互斥体来
  //保护所有链接的指针对象。这会造成严重后果
  //生产代码争用，但在测试中可以接受
  //框架。

  //加入一个已有的圆。
  void join（链接的指针内部常量*指针）
      gtest_lock_excluded_u（g_linked_ptr_mutex）
    

    链接的指针内部常量*p=指针；
    同时（P->下一步uuu！{PTR）{
      断言（P->Next uu！=这个& &
             “正在尝试连接（）一个已经存在的链接环。”
             “是否启用了gmock线程安全？”）；
      P= P-＞NEXTXY；
    }
    p->next_uu=this；
    NExt=＝PTR；
  }

  //离开我们所属的任何圈子。如果我们是
  //圆的最后一个成员。完成后，可以连接（）另一个。
  布尔部（）
      gtest_lock_excluded_u（g_linked_ptr_mutex）
    互斥锁（&g_-linked_-ptr_-mutex）；

    如果（next_==this）返回true；
    链接的_ptr_internal const*p=下一个_uu；
    同时（P->下一步uuu！=这个）{
      断言（P->Next uu！= NEXTY＆&
             “尝试离开（）我们不在的链接环。”
             “是否启用了gmock线程安全？”）；
      P= P-＞NEXTXY；
    }
    p->next_uu=next_uuu；
    返回错误；
  }

 私人：
  可变链接指针内部常量*下一个常量；
}；

模板<typename t>
类链接指针
 公众：
  typedef t元素类型；

  //接管原始指针的所有权。这应该尽快发生
  //创建对象后可能。
  显式链接指针（t*ptr=null）捕获（ptr）；
  ~链接的_ptr（）device（）；

  //复制现有的链接指针<>，将自己添加到引用列表中。
  template<typename u>linked&ptr（linked&ptr<u>const&ptr）copy（&ptr）；
  链接指针（链接指针常量和指针）//nolint
    断言（& PTR！=这个）；
    复制（和PTR）；
  }

  //赋值释放旧值并获取新值。
  template<typename u>linked&operator=（linked&ptr<u>const&ptr）
    部门；
    复制（和PTR）；
    返回这个；
  }

  链接的指针和运算符=（链接的指针常量和指针）
    如果（和PTR）！=这个）{
      部门；
      复制（和PTR）；
    }
    返回这个；
  }

  //智能指针成员。
  无效重置（t*ptr=null）
    部门；
    捕获（PTR）；
  }
  t*get（）常量返回值
  t*运算符->（）常量返回值
  t&operator*（）常量返回*值

  bool operator==（t*p）const返回值==p；
  布尔操作员！=（t*p）常量返回值u！= p；}
  模板<typename u>
  bool operator==（linked_ptr<u>const&ptr）const_
    返回值_==ptr.get（）；
  }
  模板<typename u>
  布尔操作员！=（linked_ptr<u>const&ptr）const_
    返回值！= pTr.GET（）；
  }

 私人：
  模板<typename u>
  朋友类链接；

  T*ValueSe；
  链接的指针内部链接

  虚部（）
    if（link_.device（））删除值_u；
  }

  无效捕获（t*ptr）
    Valuez＝PTR；
    链接join_new（）；
  }

  template<typename u>void copy（链接的指针<u>const*ptr）
    value_u=ptr->get（）；
    如果（值）
      链接连接（&ptr->link）；
    其他的
      链接join_new（）；
  }
}；

模板<typename t>inline
bool operator==（t*ptr，const linked_ptr<t>>&x）
  返回ptr==x.get（）；
}

模板<typename t>inline
布尔操作员！=（t*ptr，const-linked_ptr<t>&x）
  返回PTR！= x.GET（）；
}

//一个将t*转换为链接指针的函数<t>
//例如，Make_-Linked_-ptr（new foobarbaz<type>（arg））是一个较短的表示法
//对于链接的指针<foobarbaz<type>>（new foobarbaz<type>（arg））。
模板<typename t>
链接指针<t>生成指针（t*ptr）
  返回链接指针（ptr）；
}

//命名空间内部
//命名空间测试

endif//gtest_包括\u gtest_internal_gtest_linked_ptr_h_
//版权所有2007，Google Inc.
//保留所有权利。
/ /
//重新分布并以源和二进制形式使用，有或没有
//如果满足以下条件，则允许修改
//MET：
/ /
//*源代码的重新分发必须保留上述版权
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
在文件和/或其他材料中，
//分布。
//*谷歌公司的名称或其名称
//贡献者可用于支持或推广源于
//此软件没有特定的事先书面许可。
/ /
//本软件由版权所有者和贡献者提供
“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//放弃特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、偶然的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何造成的
//责任理论，无论是合同责任、严格责任还是侵权责任
//（包括疏忽或其他）因使用不当而引起的
//本软件，即使已告知此类损坏的可能性。
/ /
//作者：wan@google.com（zhanyong wan）

//谷歌测试-谷歌C++测试框架
/ /
//此文件实现了一个通用值打印机，可以打印
//任何类型t的值：
/ /
//void:：testing:：internal:：universalprinter<t>：：print（value，ostream_ptr）；
/ /
//用户可以通过
//在命名空间中定义operator<<（）或printto（），该命名空间
//定义t。更具体地说，是
//将使用以下列表（假定t在命名空间中定义）
//FO）：
/ /
/ / 1。foo:：printto（const&，ostream*）
/ / 2。在foo或
//全局命名空间。
/ /
//如果上面没有定义，它将打印
//如果是协议缓冲区，则返回该值，或者在
//否则为value。
/ /
//为了帮助调试：当t是引用类型时，
//同时打印值；当t是（const）char指针时，两个
//指针值及其指向的以nul结尾的字符串是
/印刷。
/ /
//我们还提供一些方便的包装：
/ /
///将值打印到字符串。对于（const或not）字符
///指针，以nul结尾的字符串（但不是指针）是
///打印。
//标准：：字符串：：测试：：printtoString（const&value）；
/ /
///简洁地打印一个值：对于引用类型，引用的
////打印值（但不打印地址）；对于（const或not）char
///指针，以nul结尾的字符串（但不是指针）是
///打印。
//void:：testing:：internal:：universalterseprint（const&value，ostream*）；
/ /
///使用编译器推断的类型打印值。差异
////从UniversalTersePrint（）中，此函数打印
///指针和（const或not）char指针的以nul结尾的字符串。
//void:：testing:：internal:：universalprint（const t&value，ostream*）；
/ /
///将元组的字段简洁地打印到字符串向量，1
///每个字段的元素。必须在中启用元组支持
///gtest端口.h.
//std:：vector<string>UniversalTersePrintTupleFieldsToStrings（）。
//常量tuple&value）；
/ /
//已知限制：
/ /
//打印原语打印stl样式容器的元素
//使用编译器推断的类型*iter，其中iter是
//容器的常量迭代器。当常量迭代器是输入时
//迭代器，但不是前向迭代器，此推断类型不能
//匹配值\类型，打印输出可能不正确。在
//练习，对于大多数容器来说，这很少是一个问题
//const_迭代器是正向迭代器。如果有
//实际需要。请注意，此修复程序不能依赖值类型
//被定义为许多用户定义的容器类型没有
//ValueSype类型。

ifndef gtest_包括gtest_gtest_打印机_h_
定义GTEST_包括GTEST_GTEST_打印机_H_

include<ostream>//nolint
include<sstream>
include<string>
包括<utility>
include<vector>

如果gtest_有标准tuple_
include<tuple>
第二节

命名空间测试

//在“internal”和“internal2”名称空间中的定义是
//如有更改，恕不另行通知。不要在用户代码中使用它们！
命名空间内部2

//将给定对象中给定的字节数打印到给定的
/ /流。
gtest_api_uuvoid printBytesInObjectto（const unsigned char*obj_bytes，无符号字符，字节）
                                     尺寸计数，
                                     ：：std：：ostream*os）；

//用于选择给定类型既没有<<
//nor printto（）。
EnUM TypeKind {
  kprotobuf，//原型buf类型
  kconvertibletointeger，//可隐式转换为biggestent的类型
                          //（例如，命名或未命名的枚举类型）
  Kothertype//还有别的吗？
}；

//typewithoutformatter<t，ktypeKind>：调用了printValue（value，os）
//由通用打印机打印t类型的值，如果两者都不打印
//为t定义了运算符<<nor printto（），其中ktypeKind是
//t的“kind”，由枚举类型kind定义。
模板<typename t，typenkind ktypekind>
带outformatter的类类型
 公众：
  //当ktypeKind为KotherType时调用此默认版本。
  静态void printValue（const&value，：：std:：ostream*os）
    printBytesInObjectto（reinterpret_cast<const unsigned char*>（&value），
                         sizeof（值），os）；
  }
}；

//当字符串
//不超过这个数量的字符；否则我们使用
//debugString（）以提高可读性。
常量大小Kprotobufonnelinermaxlength=50；

模板<typename t>
class typewithoutformatter<t，kprotobuf>
 公众：
  静态void printValue（const&value，：：std:：ostream*os）
    const:：testing:：internal:：string short_str=value.shortDebugString（）；
    const:：testing:：internal:：string pretty_str=
        short-str.length（）<=kProtoBufonLinerMax长度？
        short_str：（“\n”+value.debugString（））；
    *操作系统<（“<”+pretty_str+“>”）；
  }
}；

模板<typename t>
class typewithoutformatter<t，kconvertibletointeger>
 公众：
  //因为t没有<<operator或printto（），但可以隐式
  //转换为biggestant，我们将其打印为biggestent。
  / /
  //很可能t是一个枚举类型（命名或未命名），其中
  //将其打印为整数是所需的行为。万一
  //t不是枚举，最好将其打印为整数
  //假设它没有用户定义的打印机。
  静态void printValue（const&value，：：std:：ostream*os）
    const internal:：biggestent kbigitht=值；
    ＊OS< <
  }
}；

//将给定值打印到给定的Ostream。如果值是
//协议消息，打印其调试字符串；如果是枚举或
//属于隐式可转换为biggestant的类型，它被打印为
//整数；否则将打印值中的字节。这是
//当UniversalPrinter不知道时，它会做什么？
//T和T类型既没有<<operator也没有printto（）。
/ /
//用户可以通过定义
//定义foo的命名空间中的<<运算符。
/ /
//我们将此运算符放在命名空间“internal2”中，而不是“internal”
//为了简化实现，“内部”中的代码需要
//在stl中使用<<会与我们自己的<<相冲突
//在“内部”中。
/ /
//请注意，此运算符<<采用通用std:：basic_ostream<char，
//chartraits>type，而不是更受限制的std:：ostream。如果
//我们将其定义为采用std:：ostream，而不是
//“二义性重载”编译器在尝试打印类型时出错
//支持流式处理到std:：basic_ostream<char的foo，
//chartraits>，因为编译器无法判断
//运算符<（std:：ostream&，const&）或
//operator<（std:：basic_stream<char，chartraits>，const foo&）is more
/具体。
template<typename char，typename chartraits，typename t>
：：std：：basic_ostream<char，chartraits>和operator<<（
    ：：std:：basic_ostream<char，chartraits>&os，const t&x）
  带输出格式的类型<t，
      （内部：：IsProtocolMessage<t>：：Value？克托布夫：
       内部：：implicitlyconvertible<const&，内部：：biggestent>：：value？
       kconvertibletointeger:kothertype>：：打印值（x，&os）；
  返回操作系统；
}

//命名空间内部2
//命名空间测试

//此命名空间不能嵌套在：：testing或名称查找中
//实现通用打印机所需的魔力无法发挥作用。
名称空间测试_

//用于打印不是STL样式容器的值，
//用户没有为其定义printto（）。
模板<typename t>
void defaultprintinconcontainerto（const&value，：：std:：ostream*os）
  //使用以下语句，在非限定名称查找期间，
  //testing：：internal2：：operator<<显示时就像在中声明它一样
  //包含这两个名称的最近的封闭命名空间
  //：：测试_内部和：：测试：：内部2，即全局
  //命名空间。有关详细信息，请参阅C++标准部分。
  //7.3.4-1[命名空间.udir]。这让我们回到
  //测试：：Internal2:：Operator<<以防T不带<<
  /操作员。
  / /
  //无法写入“using:：testing:：internal2:：operator<<；”，这是
  //由于编译器错误，GCC 3.3无法编译。
  使用命名空间：：testing:：internal2；//nolint

  //假设t在命名空间foo中定义，在下一个语句中，
  //编译器将考虑所有：
  / /
  / / 1。foo：：operator<<（感谢Koenig查找）
  / / 2。：：operator<<（当前命名空间包含在：：）中，
  / / 3。testing:：internal2:：operator<<（感谢上面的using语句）。
  / /
  //将选择类型与t最匹配的运算符<。
  / /
  我们故意让2成为候选人，有时
  //无法定义1（例如，当foo为：：std时，定义
  //它中的任何内容都是未定义的行为，除非您是编译器
  / /供应商。
  ＊OS<值；
}

//命名空间测试_内部

命名空间测试
命名空间内部

//UniversalPrinter<t>：：print（value，ostream_ptr）打印给定的
//给定Ostream的值。呼叫方必须确保
//“ostream_ptr”不是空值，或者行为未定义。
/ /
//我们将UniversalPrinter定义为类模板（而不是
//函数模板），因为我们需要将其部分专用于
//引用类型，不能使用函数模板。
模板<typename t>
类通用打印机；

模板<typename t>
void universalprint（const&value，：：std:：ostream*os）；

//用于在用户未定义STL样式容器时打印
//它的printto（）。
模板<typename c>
void defaultprintto（iscontainer/*虚拟*/,

                    /*se_type/*不是指针*/，
                    常量容器（&C）：：std:：ostream*os）
  const size_t kmaxcount=32；//要打印的最大元素数。
  ＊OS < < {】；
  尺寸t计数=0；
  for（typename c:：const_迭代器it=container.begin（）；
       它！=container.end（）；++it，++count）
    如果（计数>0）
      ＊OS< <，>；
      如果（count==kmaxcount）//已打印足够。
        ＊OS<…
        断裂；
      }
    }
    ＊OS< <
    //我们不能在此处调用printto（*it，os），因为printto（）没有
    //句柄*它是本机数组。
    内部：：通用打印（*it，os）；
  }

  如果（计数>0）
    ＊OS< <
  }
  ＊OS< < }；
}

//用于打印既不是char指针也不是成员的指针
//指针，当用户没有为其定义printto（）时。（成员）
//变量指针或成员函数指针没有真正指向
//地址空间中的位置。他们的代表是
//定义了实现。因此，它们将被打印为原始
//字节。
模板<typename t>
void defaultprintto（isnotcontainer/*虚拟*/,

                    /*e_type/*是指针*/，
                    T*P，：：std:：ostream*os）
  如果（p=空）
    ＊OS＜“空”；
  }否则{
    //c++不允许从函数指针转换到任何对象
    //指针。
    / /
    //isrue（）使警告静音：“条件始终为真”，
    //“无法访问的代码”。
    if（isrue（implicitlyconvertable<t*，const void*>：：value））
      //t不是函数类型。我们只要打<<来打印P，
      //依靠ADL获取用户定义的<<指针
      //类型（如果有）。
      ＊OS＜P；
    }否则{
      //t是一个函数类型，因此“*os<<p”不能满足我们的需要
      //（它只是把p打印成bool）。我们想把P印成警察
      /无效。但是，我们不能直接将其强制转换为const void*，
      //甚至使用reinterpret_cast，作为gcc的早期版本
      //（例如3.4.5）当p是函数时，无法编译强制转换
      //指针。铸造到uint64首先解决了这个问题。
      *OS<<reinterpret_cast<const void**（
          reinterpret_cast<internal:：uint64>（p））；
    }
  }
}

//用于在用户
//没有为它定义printto（）。
模板<typename t>
void defaultprintto（isnotcontainer/*虚拟*/,

                    /*se_type/*不是指针*/，
                    常量值（&V）：：std:：ostream*os）
  ：：测试_internal:：defaultprintinconcontainerto（value，os）；
}

//使用<<运算符（如果有）打印给定值；
//否则打印其中的字节。这是什么
//UniversalPrinter<t>：：print（）在printto（）未专门化时执行
//或为T类型重载。
/ /
//用户可以通过定义
//定义foo的命名空间中printto（）的重载。我们
//为用户提供此选项，因为有时定义<<运算符
//不希望使用foo（例如，编码样式可能会阻止这样做，
//或者已经有<<运算符，但它不执行用户的操作
/想要。
模板<typename t>
void printto（const&value，：：std:：ostream*os）
  //defaultprintto（）被重载。前两种的类型
  //参数决定将选择哪个版本。如果t是
  //stl-style容器，将调用容器的版本；如果
  //t是指针，将调用指针版本；否则
  //将调用通用版本。
  / /
  //请注意，在检查之前，我们在这里检查容器类型
  //对于我们的运算符中的协议消息类型<。理由是：
  / /
  //对于协议消息，我们希望给人们一个机会
  //通过定义printto（）或
  //运算符<。对于STL容器，其他格式可以是
  //与容器的google mock格式不兼容
  //元素；因此，我们在这里检查容器类型以确保
  //使用了我们的格式。
  / /
  //需要defaultprintto（）的第二个参数来绕过错误
  //在Symbian的C++编译器中防止它从右边取过来
  //重载范围：
  / /
  //打印到（const t&x，…）；
  //打印到（t*x，…）；
  defaultprintto（iscontainertest<t>（0），is_pointer<t>（），value，os）；
}

//下面的printto（）重载列表说明
//通用打印机：print（）如何打印标准类型（内置
//类型、字符串、普通数组和指针）。

//各种字符类型的重载。
gtest_api_uuvoid printto（无符号char c，：：std:：ostream*os）；
gtest_api_uuvoid printto（签名字符c，：：std:：ostream*os）；
inline void printto（char c，：：std:：ostream*os）
  //打印普通字符时，我们总是将其视为无符号字符。这个
  //这样，输出不会受到编译器是否认为
  //字符是否有符号。
  printto（static_cast<unsigned char>（c），操作系统）；
}

//其他简单内置类型的重载。
inline void printto（bool x，：：std:：ostream*os）
  ＊OS< <（x？）true”：“false”）；
}

//wchar_t类型的重载。
//如果wchar_t可打印或作为其内部符号打印
//否则编码，也作为其十进制代码（L''0'除外）。
//L'\0'字符打印为“L'\0'”。打印十进制代码
//在编译器实现wchar_t时作为有符号整数
//作为有符号类型，并在wchar\u时作为无符号整数打印
//作为无符号类型实现。
gtest_api_uuvoid printto（wchar_t wc，：：std:：ostream*os）；

//C字符串的重载。
gtest_api_uuvoid printto（const char*s，：：std:：ostream*os）；
inline void printto（char*s，：：std:：ostream*os）
  printto（implicitcast_u<const char*>（s），操作系统）；
}

//有符号/无符号字符通常用于表示二进制数据，因此
//为了安全，我们将指向它的指针打印为void*。
inline void printto（const signed char*s，：：std:：ostream*os）
  printto（implicitcast_u<const void*>（s），os）；
}
inline void printto（带符号的char*s，：：std:：ostream*os）
  printto（implicitcast_u<const void*>（s），os）；
}
inline void printto（const unsigned char*s，：：std:：ostream*os）
  printto（implicitcast_u<const void*>（s），os）；
}
inline void printto（无符号char*s，：：std:：ostream*os）
  printto（implicitcast_u<const void*>（s），os）；
}

//可以将msvc配置为将wchar_t定义为无符号typedef
/短。它定义了当wchar_t是本地的时定义的\u native_wchar_t_
//类型。当wchar_t是typedef时，定义const的重载
//wchar_t*将导致无符号short*作为宽字符串打印，
//可能导致无效的内存访问。
如果有的话！已定义（_msc_ver）已定义（_native_wchar_t_defined）
//宽C字符串的重载
gtest-api-void printto（const-wchar-t*s，：：std:：ostream*os）；
inline void printto（wchar_t*s，：：std:：ostream*os）
  printto（implicitcast_uu<const wchar_t*>（s），操作系统）；
}
第二节

//C数组的重载。打印多维数组
/正确地。

//打印数组中给定数目的元素，而不打印
//花括号。
模板<typename t>
void printrawarrayto（const t a[]，size_t count，：：std:：ostream*os）
  通用打印（A[0]，操作系统）；
  对于（尺寸t i=1；i！=计数；I++）{
    ＊OS< <，>；
    通用打印（A[I]，OS）；
  }
}

//重载：：string和：：std：：string。
如果gtest_具有_global_字符串
gtest_api_uuvoid printstringto（const:：string&s，：：std:：ostream*os）；
inline void printto（const:：string&s，：：std:：ostream*os）
  打印字符串到（s，os）；
}
endif//gtest_具有_global_字符串

gtest_api_uuvoid printstringto（const:：std:：string&s，：：std:：ostream*os）；
inline void printto（const:：std:：string&s，：：std:：ostream*os）
  打印字符串到（s，os）；
}

//重载：：wstring和：：std：：wstring。
如果GTEST诳U具有诳U Global诳String
gtest_api_uuvoid printwidestingto（const:：wstring&s，：：std:：ostream*os）；
inline void printto（const:：wstring&s，：：std:：ostream*os）
  打印宽度字符串到（s，os）；
}
endif//gtest_具有全局性

如果GTEST_有_-std_-wstring
gtest_api_uuvoid printwidestingto（const:：std:：wstring&s，：：std:：ostream*os）；
inline void printto（const:：std:：wstring&s，：：std:：ostream*os）
  打印宽度字符串到（s，os）；
}
endif//gtest_具有_-std_-wstring

如果gtest_具有_tr1_tuple gtest_具有_std_tuple_
//用于打印元组的helper函数。t必须用实例化
//元组类型。
模板<typename t>
void printupleto（const t&t，：：std:：ostream*os）；
endif//gtest_有_tr1_tuple_gtest_有_std_tuple_

如果gtest_有_tr1_元组
//重载：：std:：tr1:：tuple。打印函数参数时需要，
//打包为元组。

//为各种算术的元组重载printto（）。我们支持
//最多10个字段的元组。以下实施工作
//无论是否使用
//是否为非标准变量模板功能。

inline void printto（const:：std:：tr1:：tuple<>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}

模板<typename t1>
void printto（const:：std:：tr1:：tuple<t1>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2>
void printto（const:：std:：tr1:：tuple<t1，t2>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3>
void printto（const:：std:：tr1:：tuple<t1，t2，t3>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4，t5>&t，
             ：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5，
          类型名称T6>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4，t5，t6>&t，
             ：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5，
          类型名t6，类型名t7>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4，t5，t6，t7>&t，
             ：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5，
          typename t6，typename t7，typename t8>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4，t5，t6，t7，t8>&t，
             ：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5，
          typename t6，typename t7，typename t8，typename t9>
void printto（const:：std:：tr1:：tuple<t1，t2，t3，t4，t5，t6，t7，t8，t9>&t，
             ：：std:：ostream*os）
  打印目录（T，OS）；
}

template<typename t1，typename t2，typename t3，typename t4，typename t5，
          typename t6，typename t7，typename t8，typename t9，typename t10>
无效PrintTo
    常量：：std:：tr1:：tuple<t1，t2，t3，t4，t5，t6，t7，t8，t9，t10>&t，
    ：：std:：ostream*os）
  打印目录（T，OS）；
}
endif//gtest_具有_tr1_元组

如果gtest_有标准tuple_
模板<类型名…类型>
void printto（const:：std:：tuple<types…>&t，：：std:：ostream*os）
  打印目录（T，OS）；
}
endif//gtest_具有_std_tuple_

//std:：pair的重载。
template<typename t1，typename t2>
void printto（const:：std:：pair<t1，t2>&value，：：std:：ostream*os）
  ＊OS< <（）；
  //此处不能使用UniversalPrint（value.first，os），因为T1可能是
  //引用类型。打印值相同。秒。
  通用打印机：打印（value.first，os）；
  ＊OS< <，>；
  通用打印机：打印（value.second，os）；
  ＊OS<<”；
}

//通过让编译器
//为t选择printto（）的右重载。
模板<typename t>
类通用打印机
 公众：
  //msvc警告向函数类型添加const，因此我们希望
  //禁用警告。
  gtest禁用msc警告按钮（4180）

  //注意：我们故意不将此printto（）作为该名称调用。
  //与的主体中的：：testing：：internal：：printto冲突
  //函数。
  静态无效打印（常量和值，：：std:：ostream*os）
    //默认情况下，：：testing:：internal:：printto（）用于打印
    //值。
    / /
    //感谢Koenig查找，如果t是一个类并且有自己的类
    //printto（）函数在其命名空间中定义，该函数将
    //在这里可见。因为它比一般的更具体
    //在：：testing:：internal中，编译器将在
    //以下语句-正是我们想要的。
    printto（值，os）；
  }

  gtest禁用msc警告
}；

//UniversalPrintArray（begin，len，os）打印一个“len”数组
//元素，从地址“begin”开始。
模板<typename t>
void universalprintary（const t*begin，size_t len，：：std：：ostream*os）
  如果（Le=＝0）{
    ＊OS＜“{}”；
  }否则{
    ＊OS＜“{”；
    常量大小k阈值=18；
    常量大小\u t kchunksize=8；
    //如果数组中的元素多于kthreshold，则必须
    //只打印第一个和最后一个
    //kChunkSize元素。
    //todo（wan@google.com）：让用户使用标志控制阈值。
    如果（len<=kthreshold）
      printrawarrayto（开始，len，os）；
    }否则{
      printrawarrayto（开始，kChunkSize，OS）；
      *操作系统<“，…，”
      printrawarrayto（begin+len-kchunksize，kchunksize，os）；
    }
    ＊OS＜“}”；
  }
}
//此重载紧凑地打印（const）char数组。
gtest_api_uuvoid通用打印阵列（
    const char*begin，大小\u t len，：：std：：ostream*os）；

//此重载紧凑地打印（const）wchar_t数组。
gtest_api_uuvoid通用打印阵列（
    const wchar_t*begin，size_t len，：：std：：ostream*os）；

//实现打印数组类型t[n]。
模板<typename t，大小\u t n>
class universalprinter<t[n]>
 公众：
  //打印给定的数组，同时省略某些元素
  很多。
  静态无效打印（const t（&a）[N]，：std:：ostream*os）
    通用打印阵列（A、N、OS）；
  }
}；

//实现打印引用类型t&。
模板<typename t>
类通用打印机
 公众：
  //msvc警告向函数类型添加const，因此我们希望
  //禁用警告。
  gtest禁用msc警告按钮（4180）

  静态无效打印（常量和值，：：std:：ostream*os）
    //打印值的地址。我们在这里用重新解释
    //因为当t是函数类型时，static_cast不会编译。
    *OS<“@”<<reinterpret_cast<const void*>（&value）<“”；

    //然后打印值本身。
    通用打印（值，OS）；
  }

  gtest禁用msc警告
}；

//简洁地打印一个值：对于引用类型，引用的值
//（但不是地址）已打印；对于（const）char指针，
//打印以nul结尾的字符串（但不打印指针）。

模板<typename t>
类通用打印机
 公众：
  静态无效打印（常量和值，：：std:：ostream*os）
    通用打印（值，OS）；
  }
}；
模板<typename t>
类通用打印机
 公众：
  静态无效打印（常量和值，：：std:：ostream*os）
    通用打印（值，OS）；
  }
}；
模板<typename t，大小\u t n>
class universalterseprinter<t[n]>
 公众：
  静态无效打印（const t（&value）[N]，：std:：ostream*os）
    通用打印机：<t[n]>：打印（value，os）；
  }
}；
模板< >
class universalterseprinter<const char*>
 公众：
  静态无效打印（const char*str，：：std:：ostream*os）
    if（str==null）
      ＊OS＜“空”；
    }否则{
      通用打印（字符串（str），os）；
    }
  }
}；
模板< >
class universalterseprinter<char*>
 公众：
  静态无效打印（char*str，：：std:：ostream*os）
    UniversalTersePrinter<const char/>：打印（str，os）；
  }
}；

如果GTEST_有_-std_-wstring
模板< >
class universalterseprinter<const wchar_t*>
 公众：
  静态无效打印（const wchar_t*str，：：std:：ostream*os）
    if（str==null）
      ＊OS＜“空”；
    }否则{
      通用打印（：：std:：wstring（str），os）；
    }
  }
}；
第二节

模板< >
class universalterseprinter<wchar_t*>
 公众：
  静态无效打印（wchar_t*str，：：std:：ostream*os）
    UniversalTersePrinter<const wchar_t/>：打印（str，os）；
  }
}；

模板<typename t>
void universalterseprint（const&value，：：std:：ostream*os）
  UniversalTersePrinter<t>：打印（值，OS）；
}

//使用编译器推断的类型打印值。这个
//此函数与UniversalTersePrint（）的区别在于
//（const）char指针，它同时打印指针和
//以nul结尾的字符串。
模板<typename t>
void universalprint（const&value，：：std:：ostream*os）
  //VC++7.1中阻止我们实例化的错误的WorkarOnd
  //直接使用t的通用打印机。
  TyWIFT T1；
  通用打印机：打印（value，os）；
}

typedef:：std:：vector<string>strings；

//tuplepolicy<tuplet>必须提供：
// tuple大小
//元组元组的大小。
//get<size_t i>（const tuplet&t）
//静态函数提取元组元组的元素i。
//-tuple_element<size_t i>：类型
//元组元组的元素i的类型。
template<typename tuplet>
结构策略；

如果gtest_有_tr1_元组
template<typename tuplet>
结构策略
  typedef元组元组；
  静态常量大小tuple_size=：：std:：tr1:：tuple_size<tuple>：：value；

  模板
  结构tuple_element:：：std:：tr1:：tuple_element<i，tuple>

  模板
  静态类型名addreference<
      const typename:：std:：tr1:：tuple_element<i，tuple>：：type>：：type get（
      const tuple和tuple）
    返回：：std:：tr1:：get<i>（tuple）；
  }
}；
template<typename tuplet>
const size_t tuplepolicy<tuple t>：：tuple_size；
endif//gtest_具有_tr1_元组

如果gtest_有标准tuple_
模板<类型名…类型>
结构tuplepolicy<：std:：tuple<types…>>
  typedef:：std:：tuple<types…>tuple；
  静态常量大小tuple-size=：：std:：tuple-size<tuple>：：value；

  模板
  结构tuple_element:：：std:：tuple_element<i，tuple>；

  模板
  静态const typename:：std:：tuple_element<i，tuple>：：type&get（
      const tuple和tuple）
    返回：：std：：get<i>（tuple）；
  }
}；
模板<类型名…类型>
const-size_t tuplepolicy<：std:：tuple<types…>>：tuple_-size；
endif//gtest_具有_std_tuple_

如果gtest_具有_tr1_tuple gtest_具有_std_tuple_
//此帮助器模板允许printto（）用于元组和
//要由定义的UniversalTersePrintTupleFieldsToStrings（）。
//对元组字段数的归纳。想法是
//tuplePrefixPrinter<n>：：printprifixto（t，os）打印第一个n
//元组t中的字段，并且可以根据
//tuplePrefixprinter<n-1>。
/ /
//感应情况。
模板
结构TuplePrefixprinter
  //打印元组的前n个字段。
  template<typename tuple>
  静态void printprefixto（const tuple&t，：：std:：ostream*os）
    tuplePrefixPrinter<n-1>：：printprefixTo（t，os）；
    GTEST_意向性构建条件推送（
    如果（n＞1）{
    故意制造条件
      ＊OS< <，>；
    }
    通用打印机
        typename tuplepolicy<tuple>：：template tuple_element<n-1>：：type>
        ：：print（tuplepolicy<tuple>：：template get<n-1>（t），os）；
  }

  //简洁地将元组的前n个字段打印到字符串向量，
  //每个字段一个元素。
  template<typename tuple>
  静态void terseprintprixtoStrings（const tuple&t，strings*strings）
    tuplePrefixPrinter<n-1>：：tersePrintPrefixToStrings（t，strings）；
    ：：std：：stringstream ss；
    通用terseprint（tuplepolicy<tuple>：：template get<n-1>（t），&ss）；
    字符串->后推（ss.str（））；
  }
}；

//基本情况。
模板< >
结构tuplePrefixprinter<0>
  template<typename tuple>
  静态void printprefixto（const tuple&，：：std:：ostream*）

  template<typename tuple>
  静态void terseprintprixtoStrings（const tuple&，strings*）
}；

//用于打印元组的helper函数。
//元组必须是std:：tr1:：tuple或std:：tuple类型。
template<typename tuple>
void printupleto（const tuple&t，：：std:：ostream*os）
  ＊OS< <（）；
  tuplePrefixprinter<tuplepolicy<tuple>：：tuple_-size>：：printprifixto（t，os）；
  ＊OS< <“”；
}

//将元组的字段简洁地打印到字符串向量1
//每个字段的元素。查看之前的评论
//UniversalTersePrint（）了解如何定义“简洁”。
template<typename tuple>
字符串UniversalTersePrintTupleFieldsToStrings（const tuple&value）
  字符串结果；
  tuplePrefixprinter<tuplepolicy<tuple>：：tuple_size>：：
      TersePrintPrefixToStrings（值和结果）；
  返回结果；
}
endif//gtest_有_tr1_tuple_gtest_有_std_tuple_

//命名空间内部

模板<typename t>
：：std：：string printToString（const t&value）
  ：：std：：stringstream ss；
  内部：：UniversalTersePrinter<t>：：打印（值，&ss）；
  返回ss.str（）；
}

//命名空间测试

endif//gtest_包括\u gtest_gtest_打印机\u h_

如果gtest_有_参数_测试

命名空间测试
命名空间内部

//内部实现-不要在用户代码中使用。
/ /
//输出一条消息，解释不同
//同一测试用例的fixture类。这可能发生在
//test_p宏用于定义两个同名的测试
//但在不同的命名空间中。
gtest_api_uuvoid reportInvalidTestCaseType（const char*测试用例名称，
                                          const char*文件，int行）；

template<typename>class paramgeneratorinterface；
template<typename>class paramgenerator；

//用于迭代实现提供的元素的接口
//参数GeneratorInterface<t>。
模板<typename t>
类ParamIteratorInterface_
 公众：
  虚拟~ParamIteratorInterface（）
  //指向基生成器实例的指针。
  //仅用于迭代器比较
  //确保两个迭代器属于同一个生成器。
  virtual const paramgeneratorinterface<t>*basegenerator（）const=0；
  //前进迭代器指向下一个元素
  //由发电机提供。呼叫方负责
  //不在迭代器上调用advance（）等于
  //baseGenerator（）->end（）。
  虚空推进（）=0；
  //克隆迭代器对象。用于实现复制语义
  //的paramiterator<t>。
  虚拟参数迭代器interface*clone（）const=0；
  //取消引用当前迭代器并提供（只读）访问
  //指向的值。打电话的人有责任不打电话
  //迭代器上的current（）等于baseGenerator（）->end（）。
  //用于实现paramgenerator。
  虚拟常量t*current（）常量=0；
  //确定给定的迭代器和其他点是否相同
  //生成器生成的序列中的元素。
  //用于实现paramgenerator。
  虚拟bool等于（const-paramiteratorinterface&other）const=0；
}；

//类对由实现提供的元素进行迭代
//ParamGeneratorInterface<t>。它包装ParamIteratorInterface<t>
//并实现const forward迭代器概念。
模板<typename t>
类参数迭代器
 公众：
  typedef t值\u类型；
  typedef const t&reference；
  typedef ptrdiff差异类型；

  //ParamIterator假定拥有impl_u指针的所有权。
  paramiterator（const paramiterator&other）：impl_u（other.impl_->clone（））
  paramiterator&operator=（const paramiterator&other）
    如果（这个）！=和其他）
      impl_u.reset（其他.impl_uu->clone（））；
    返回这个；
  }

  const&operator*（）const return*impl->current（）；
  const*operator->（）const return impl_->current（）；
  //运算符+的前缀版本。
  参数迭代器和运算符++（）
    impl_->advance（）；
    返回这个；
  }
  //operator++的后缀版本。
  paramiterator运算符++（int/*未使用*/) {

    ParamIteratorInterface<T>* clone = impl_->Clone();
    impl_->Advance();
    return ParamIterator(clone);
  }
  bool operator==(const ParamIterator& other) const {
    return impl_.get() == other.impl_.get() || impl_->Equals(*other.impl_);
  }
  bool operator!=(const ParamIterator& other) const {
    return !(*this == other);
  }

 private:
  friend class ParamGenerator<T>;
  explicit ParamIterator(ParamIteratorInterface<T>* impl) : impl_(impl) {}
  scoped_ptr<ParamIteratorInterface<T> > impl_;
};

//paramGeneratorInterface<t>是访问生成器的二进制接口
//以其他翻译单位定义。
template <typename T>
class ParamGeneratorInterface {
 public:
  typedef T ParamType;

  virtual ~ParamGeneratorInterface() {}

//发电机接口定义
  virtual ParamIteratorInterface<T>* Begin() const = 0;
  virtual ParamIteratorInterface<T>* End() const = 0;
};

//包装ParamGeneratorInterface<t>并提供常规生成器语法
//与STL容器概念兼容。
//这个类实现了复制初始化语义和包含的
//paramGeneratorInterface<t>实例在所有副本之间共享
//原始对象的。这是可能的，因为该实例是不可变的。
template<typename T>
class ParamGenerator {
 public:
  typedef ParamIterator<T> iterator;

  explicit ParamGenerator(ParamGeneratorInterface<T>* impl) : impl_(impl) {}
  ParamGenerator(const ParamGenerator& other) : impl_(other.impl_) {}

  ParamGenerator& operator=(const ParamGenerator& other) {
    impl_ = other.impl_;
    return *this;
  }

  iterator begin() const { return iterator(impl_->Begin()); }
  iterator end() const { return iterator(impl_->End()); }

 private:
  linked_ptr<const ParamGeneratorInterface<T> > impl_;
};

//从两个可比较的值范围生成值。可以用来
//生成实现运算符+（）和
//操作符<（）。
//这个类在range（）函数中使用。
template <typename T, typename IncrementT>
class RangeGenerator : public ParamGeneratorInterface<T> {
 public:
  RangeGenerator(T begin, T end, IncrementT step)
      : begin_(begin), end_(end),
        step_(step), end_index_(CalculateEndIndex(begin, end, step)) {}
  virtual ~RangeGenerator() {}

  virtual ParamIteratorInterface<T>* Begin() const {
    return new Iterator(this, begin_, 0, step_);
  }
  virtual ParamIteratorInterface<T>* End() const {
    return new Iterator(this, end_, end_index_, step_);
  }

 private:
  class Iterator : public ParamIteratorInterface<T> {
   public:
    Iterator(const ParamGeneratorInterface<T>* base, T value, int index,
             IncrementT step)
        : base_(base), value_(value), index_(index), step_(step) {}
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<T>* BaseGenerator() const {
      return base_;
    }
    virtual void Advance() {
      value_ = value_ + step_;
      index_++;
    }
    virtual ParamIteratorInterface<T>* Clone() const {
      return new Iterator(*this);
    }
    virtual const T* Current() const { return &value_; }
    virtual bool Equals(const ParamIteratorInterface<T>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const int other_index =
          CheckedDowncastToActualType<const Iterator>(&other)->index_;
      return index_ == other_index;
    }

   private:
    Iterator(const Iterator& other)
        : ParamIteratorInterface<T>(),
          base_(other.base_), value_(other.value_), index_(other.index_),
          step_(other.step_) {}

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<T>* const base_;
    T value_;
    int index_;
    const IncrementT step_;
};  //类RangeGenerator:：迭代器

  static int CalculateEndIndex(const T& begin,
                               const T& end,
                               const IncrementT& step) {
    int end_index = 0;
    for (T i = begin; i < end; i = i + step)
      end_index++;
    return end_index;
  }

//没有实现-不支持分配。
  void operator=(const RangeGenerator& other);

  const T begin_;
  const T end_;
  const IncrementT step_;
//end（）迭代器的索引。生成的
//序列被索引（基于0），以帮助迭代器比较。
  const int end_index_;
};  //等级范围生成器


//从一对STL样式的迭代器生成值。用于
//valuesin（）函数。元素从源范围复制
//因为源可以位于堆栈和生成器上
//可能会持续超过堆栈帧。
template <typename T>
class ValuesInIteratorRangeGenerator : public ParamGeneratorInterface<T> {
 public:
  template <typename ForwardIterator>
  ValuesInIteratorRangeGenerator(ForwardIterator begin, ForwardIterator end)
      : container_(begin, end) {}
  virtual ~ValuesInIteratorRangeGenerator() {}

  virtual ParamIteratorInterface<T>* Begin() const {
    return new Iterator(this, container_.begin());
  }
  virtual ParamIteratorInterface<T>* End() const {
    return new Iterator(this, container_.end());
  }

 private:
  typedef typename ::std::vector<T> ContainerType;

  class Iterator : public ParamIteratorInterface<T> {
   public:
    Iterator(const ParamGeneratorInterface<T>* base,
             typename ContainerType::const_iterator iterator)
        : base_(base), iterator_(iterator) {}
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<T>* BaseGenerator() const {
      return base_;
    }
    virtual void Advance() {
      ++iterator_;
      value_.reset();
    }
    virtual ParamIteratorInterface<T>* Clone() const {
      return new Iterator(*this);
    }
//我们需要使用迭代器引用的缓存值，因为*迭代器\u
//可以返回一个临时对象（类型不是t），所以
//“返回&*迭代器”不起作用。
//值_u在此处更新，而不是在advance（）中更新，因为advance（）。
//可以将迭代器推进到超出范围的末尾，而不能
//发现那个事实。另一方面，客户端代码是
//负责不在超出范围的迭代器上调用current（）。
    virtual const T* Current() const {
      if (value_.get() == NULL)
        value_.reset(new T(*iterator_));
      return value_.get();
    }
    virtual bool Equals(const ParamIteratorInterface<T>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      return iterator_ ==
          CheckedDowncastToActualType<const Iterator>(&other)->iterator_;
    }

   private:
    Iterator(const Iterator& other)
//显式构造函数调用禁止错误警告
//在提供-wextra选项时由GCC发出。
        : ParamIteratorInterface<T>(),
          base_(other.base_),
          iterator_(other.iterator_) {}

    const ParamGeneratorInterface<T>* const base_;
    typename ContainerType::const_iterator iterator_;
//*迭代器的缓存值。我们把它放在这里，以便通过
//包装迭代器运算符中的指针->（）。
//值_u必须是可变的才能在当前（）中访问。
//使用作用域指针有助于管理缓存值的生存期，
//它受迭代器本身的寿命限制。
    mutable scoped_ptr<const T> value_;
};  //类值InIteratorRangeGenerator:：Iterator

//没有实现-不支持分配。
  void operator=(const ValuesInIteratorRangeGenerator& other);

  const ContainerType container_;
};  //类值InIteratorRangeGenerator

//内部实现-不要在用户代码中使用。
//
//存储参数值，然后创建用该值参数化的测试
//价值。
template <class TestClass>
class ParameterizedTestFactory : public TestFactoryBase {
 public:
  typedef typename TestClass::ParamType ParamType;
  explicit ParameterizedTestFactory(ParamType parameter) :
      parameter_(parameter) {}
  virtual Test* CreateTest() {
    TestClass::SetParam(&parameter_);
    return new TestClass();
  }

 private:
  const ParamType parameter_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestFactory);
};

//内部实现-不要在用户代码中使用。
//
//TestMetaFactoryBase是创建
//用于传递到makeandregistertestinfo函数的测试工厂。
template <class ParamType>
class TestMetaFactoryBase {
 public:
  virtual ~TestMetaFactoryBase() {}

  virtual TestFactoryBase* CreateTestFactory(ParamType parameter) = 0;
};

//内部实现-不要在用户代码中使用。
//
//TestMetaFactory创建用于传递到
//生成和注册测试信息函数。因为makeandregistertestinfo收到
//测试工厂指针的所有权，无法传递同一工厂对象
//用那种方法两次。但参数化的testcaseinfo将调用
//用于每个测试/参数值组合。因此需要元工厂
//创建者类。
template <class TestCase>
class TestMetaFactory
    : public TestMetaFactoryBase<typename TestCase::ParamType> {
 public:
  typedef typename TestCase::ParamType ParamType;

  TestMetaFactory() {}

  virtual TestFactoryBase* CreateTestFactory(ParamType parameter) {
    return new ParameterizedTestFactory<TestCase>(parameter);
  }

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestMetaFactory);
};

//内部实现-不要在用户代码中使用。
//
//参数化的TestCaseInfoBase是通用接口
//参数化的TestCaseInfo类。参数化测试用例信息库
//累积测试宏调用提供的测试信息
//以及由实例化测试用例宏调用提供的生成器
//并使用该信息注册所有生成的测试实例
//在注册测试方法中。ParameterTestCaseRegistry类保留
//指向参数化的TestCaseInfo对象的指针集合
//并在被请求时对每个函数调用registertests（）。
class ParameterizedTestCaseInfoBase {
 public:
  virtual ~ParameterizedTestCaseInfoBase() {}

//用于显示的测试用例名称的基础部分。
  virtual const string& GetTestCaseName() const = 0;
//用于验证身份的测试用例ID。
  virtual TypeId GetTestCaseTypeId() const = 0;
//UnitTest类调用此方法以在此
//在运行所有测试宏之前先测试用例。
//此方法不应在任何单个方法上多次调用
//参数化的TestCaseInfoBase派生类的实例。
  virtual void RegisterTests() = 0;

 protected:
  ParameterizedTestCaseInfoBase() {}

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseInfoBase);
};

//内部实现-不要在用户代码中使用。
//
//参数化测试用例信息汇总从测试获取的测试
//特定测试用例和生成器的宏调用
//从实例测试用例宏调用中获取
//测试用例。它用所有
//发电机。
template <class TestCase>
class ParameterizedTestCaseInfo : public ParameterizedTestCaseInfoBase {
 public:
//paramtype和generatorcreationfunc是私有类型，但是必需的
//对于公共方法addTestPattern（）和的声明
//addTestCaseInstantation（）。
  typedef typename TestCase::ParamType ParamType;
//返回适当生成器类型的实例的函数。
  typedef ParamGenerator<ParamType>(GeneratorCreationFunc)();

  explicit ParameterizedTestCaseInfo(const char* name)
      : test_case_name_(name) {}

//用于显示的测试用例基名称。
  virtual const string& GetTestCaseName() const { return test_case_name_; }
//用于验证身份的测试用例ID。
  virtual TypeId GetTestCaseTypeId() const { return GetTypeId<TestCase>(); }
//test_p宏使用addtestpattern（）来记录信息
//关于localtestinfo结构中的单个测试。
//测试用例名称是测试用例的基名称（不调用
//前缀）。test_base_name是没有
//参数索引。对于测试序列a/footerst.dobar/1 footerst is
//测试用例基名称，dobar是测试基名称。
  void AddTestPattern(const char* test_case_name,
                      const char* test_base_name,
                      TestMetaFactoryBase<ParamType>* meta_factory) {
    tests_.push_back(linked_ptr<TestInfo>(new TestInfo(test_case_name,
                                                       test_base_name,
                                                       meta_factory)));
  }
//实例化_test_case_p宏使用addGenerator（）来记录信息
//关于发电机。
  int AddTestCaseInstantiation(const string& instantiation_name,
                               GeneratorCreationFunc* func,
                               /*st char*/*文件*/，
                               INT/*线*/) {

    instantiations_.push_back(::std::make_pair(instantiation_name, func));
return 0;  //返回仅用于在命名空间范围内运行此方法的值。
  }
//UnitTest类调用此方法在此测试用例中注册测试
//在run_all_tests宏中运行测试之前的测试用例。
//此方法不应在任何单个方法上多次调用
//参数化的TestCaseInfoBase派生类的实例。
//UnitTest有一个防止多次调用此方法的保护。
  virtual void RegisterTests() {
    for (typename TestInfoContainer::iterator test_it = tests_.begin();
         test_it != tests_.end(); ++test_it) {
      linked_ptr<TestInfo> test_info = *test_it;
      for (typename InstantiationContainer::iterator gen_it =
               instantiations_.begin(); gen_it != instantiations_.end();
               ++gen_it) {
        const string& instantiation_name = gen_it->first;
        ParamGenerator<ParamType> generator((*gen_it->second)());

        string test_case_name;
        if ( !instantiation_name.empty() )
          test_case_name = instantiation_name + "/";
        test_case_name += test_info->test_case_base_name;

        int i = 0;
        for (typename ParamGenerator<ParamType>::iterator param_it =
                 generator.begin();
             param_it != generator.end(); ++param_it, ++i) {
          Message test_name_stream;
          test_name_stream << test_info->test_base_name << "/" << i;
          MakeAndRegisterTestInfo(
              test_case_name.c_str(),
              test_name_stream.GetString().c_str(),
NULL,  //没有类型参数。
              PrintToString(*param_it).c_str(),
              GetTestCaseTypeId(),
              TestCase::SetUpTestCase,
              TestCase::TearDownTestCase,
              test_info->test_meta_factory->CreateTestFactory(*param_it));
}  //对于帕拉姆
}  //为GunuIT
}  //对于TestIt
}  //注册测试

 private:
//localtestinfo结构保留注册的单个测试的信息
//使用测试宏。
  struct TestInfo {
    TestInfo(const char* a_test_case_base_name,
             const char* a_test_base_name,
             TestMetaFactoryBase<ParamType>* a_test_meta_factory) :
        test_case_base_name(a_test_case_base_name),
        test_base_name(a_test_base_name),
        test_meta_factory(a_test_meta_factory) {}

    const string test_case_base_name;
    const string test_base_name;
    const scoped_ptr<TestMetaFactoryBase<ParamType> > test_meta_factory;
  };
  typedef ::std::vector<linked_ptr<TestInfo> > TestInfoContainer;
//保留<instantiation name，sequence generator creation function>
//从实例化测试用例宏接收。
  typedef ::std::vector<std::pair<string, GeneratorCreationFunc*> >
      InstantiationContainer;

  const string test_case_name_;
  TestInfoContainer tests_;
  InstantiationContainer instantiations_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseInfo);
};  //类参数化测试用例信息

//内部实现-不要在用户代码中使用。
//
//参数化测试用例注册表包含参数化测试用例信息库的映射
//通过测试用例名称访问的类。测试并实例化测试用例
//宏使用它来定位其相应的参数化测试用例信息
//描述符。
class ParameterizedTestCaseRegistry {
 public:
  ParameterizedTestCaseRegistry() {}
  ~ParameterizedTestCaseRegistry() {
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      delete *it;
    }
  }

//查找或创建并返回包含有关
//特定测试用例的测试和实例化。
  template <class TestCase>
  ParameterizedTestCaseInfo<TestCase>* GetTestCasePatternHolder(
      const char* test_case_name,
      const char* file,
      int line) {
    ParameterizedTestCaseInfo<TestCase>* typed_test_info = NULL;
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      if ((*it)->GetTestCaseName() == test_case_name) {
        if ((*it)->GetTestCaseTypeId() != GetTypeId<TestCase>()) {
//抱怨谷歌测试设备使用不当
//因为我们不能保证正确，所以终止程序。
//本例中的测试用例设置和分解。
          ReportInvalidTestCaseType(test_case_name,  file, line);
          posix::Abort();
        } else {
//此时，我们确信我们找到的物体是相同的。
//我们要找的类型，所以我们把它降为那个类型
//无需进一步检查。
          typed_test_info = CheckedDowncastToActualType<
              ParameterizedTestCaseInfo<TestCase> >(*it);
        }
        break;
      }
    }
    if (typed_test_info == NULL) {
      typed_test_info = new ParameterizedTestCaseInfo<TestCase>(test_case_name);
      test_case_infos_.push_back(typed_test_info);
    }
    return typed_test_info;
  }
  void RegisterTests() {
    for (TestCaseInfoContainer::iterator it = test_case_infos_.begin();
         it != test_case_infos_.end(); ++it) {
      (*it)->RegisterTests();
    }
  }

 private:
  typedef ::std::vector<ParameterizedTestCaseInfoBase*> TestCaseInfoContainer;

  TestCaseInfoContainer test_case_infos_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ParameterizedTestCaseRegistry);
};

}  //命名空间内部
}  //命名空间测试

#endif  //gtest_有参数测试

#endif  //gtest_包括gtest_internal_gtest_param_util_h_
//此文件由以下命令生成：
//pump.py gtest-param-util-generated.h.泵
//不要手工编辑！！！！

//版权所有2008 Google Inc.
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：vladl@google.com（vlad losev）

//用于实现参数化测试的类型和函数实用程序。
//此文件由脚本生成。不要手工编辑！
//
//目前google测试最多支持50个参数值，
//联合起来最多10个论点。请联系
//googletestframework@googlegroups.com，如果您需要更多信息。
//请注意，要合并的参数数量是有限的
//通过元组实现的最大数量
//当前设置为10。

#ifndef GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PARAM_UTIL_GENERATED_H_
#define GTEST_INCLUDE_GTEST_INTERNAL_GTEST_PARAM_UTIL_GENERATED_H_

//scripts/fuse-gtest.py取决于包含GTest自己的标题
//*无条件地*。因此，这些包括不能移动
//内部如果gtest_有_参数_测试。

#if GTEST_HAS_PARAM_TEST

namespace testing {

//valuesin（）的前向声明，该声明在
//包括/gtest/gtest参数test.h。
template <typename ForwardIterator>
internal::ParamGenerator<
  typename ::testing::internal::IteratorTraits<ForwardIterator>::value_type>
ValuesIn(ForwardIterator begin, ForwardIterator end);

template <typename T, size_t N>
internal::ParamGenerator<T> ValuesIn(const T (&array)[N]);

template <class Container>
internal::ParamGenerator<typename Container::value_type> ValuesIn(
    const Container& container);

namespace internal {

//在values（）函数中用于提供多态功能。
template <typename T1>
class ValueArray1 {
 public:
  explicit ValueArray1(T1 v1) : v1_(v1) {}

  template <typename T>
  operator ParamGenerator<T>() const { return ValuesIn(&v1_, &v1_ + 1); }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray1& other);

  const T1 v1_;
};

template <typename T1, typename T2>
class ValueArray2 {
 public:
  ValueArray2(T1 v1, T2 v2) : v1_(v1), v2_(v2) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray2& other);

  const T1 v1_;
  const T2 v2_;
};

template <typename T1, typename T2, typename T3>
class ValueArray3 {
 public:
  ValueArray3(T1 v1, T2 v2, T3 v3) : v1_(v1), v2_(v2), v3_(v3) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray3& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
};

template <typename T1, typename T2, typename T3, typename T4>
class ValueArray4 {
 public:
  ValueArray4(T1 v1, T2 v2, T3 v3, T4 v4) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray4& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
class ValueArray5 {
 public:
  ValueArray5(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray5& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
class ValueArray6 {
 public:
  ValueArray6(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray6& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
class ValueArray7 {
 public:
  ValueArray7(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray7& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
class ValueArray8 {
 public:
  ValueArray8(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
      T8 v8) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray8& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
class ValueArray9 {
 public:
  ValueArray9(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8,
      T9 v9) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray9& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
class ValueArray10 {
 public:
  ValueArray10(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray10& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11>
class ValueArray11 {
 public:
  ValueArray11(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6),
      v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray11& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12>
class ValueArray12 {
 public:
  ValueArray12(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5),
      v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray12& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13>
class ValueArray13 {
 public:
  ValueArray13(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13) : v1_(v1), v2_(v2), v3_(v3), v4_(v4),
      v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11),
      v12_(v12), v13_(v13) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray13& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14>
class ValueArray14 {
 public:
  ValueArray14(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray14& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15>
class ValueArray15 {
 public:
  ValueArray15(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray15& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16>
class ValueArray16 {
 public:
  ValueArray16(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9),
      v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15),
      v16_(v16) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray16& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17>
class ValueArray17 {
 public:
  ValueArray17(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16,
      T17 v17) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray17& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18>
class ValueArray18 {
 public:
  ValueArray18(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray18& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19>
class ValueArray19 {
 public:
  ValueArray19(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6),
      v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13),
      v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray19& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20>
class ValueArray20 {
 public:
  ValueArray20(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5),
      v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12),
      v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18),
      v19_(v19), v20_(v20) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray20& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21>
class ValueArray21 {
 public:
  ValueArray21(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21) : v1_(v1), v2_(v2), v3_(v3), v4_(v4),
      v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11),
      v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17),
      v18_(v18), v19_(v19), v20_(v20), v21_(v21) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray21& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22>
class ValueArray22 {
 public:
  ValueArray22(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray22& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23>
class ValueArray23 {
 public:
  ValueArray23(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray23& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24>
class ValueArray24 {
 public:
  ValueArray24(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9),
      v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15),
      v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21),
      v22_(v22), v23_(v23), v24_(v24) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray24& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25>
class ValueArray25 {
 public:
  ValueArray25(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24,
      T25 v25) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray25& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26>
class ValueArray26 {
 public:
  ValueArray26(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray26& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27>
class ValueArray27 {
 public:
  ValueArray27(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6),
      v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13),
      v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19),
      v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25),
      v26_(v26), v27_(v27) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray27& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28>
class ValueArray28 {
 public:
  ValueArray28(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5),
      v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12),
      v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18),
      v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24),
      v25_(v25), v26_(v26), v27_(v27), v28_(v28) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray28& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29>
class ValueArray29 {
 public:
  ValueArray29(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29) : v1_(v1), v2_(v2), v3_(v3), v4_(v4),
      v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11),
      v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17),
      v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23),
      v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28), v29_(v29) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray29& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30>
class ValueArray30 {
 public:
  ValueArray30(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray30& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31>
class ValueArray31 {
 public:
  ValueArray31(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30), v31_(v31) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray31& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32>
class ValueArray32 {
 public:
  ValueArray32(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9),
      v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15),
      v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21),
      v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27),
      v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray32& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33>
class ValueArray33 {
 public:
  ValueArray33(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32,
      T33 v33) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray33& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34>
class ValueArray34 {
 public:
  ValueArray34(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33), v34_(v34) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray34& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35>
class ValueArray35 {
 public:
  ValueArray35(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6),
      v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13),
      v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19),
      v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25),
      v26_(v26), v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31),
      v32_(v32), v33_(v33), v34_(v34), v35_(v35) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray35& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36>
class ValueArray36 {
 public:
  ValueArray36(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5),
      v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12),
      v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18),
      v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24),
      v25_(v25), v26_(v26), v27_(v27), v28_(v28), v29_(v29), v30_(v30),
      v31_(v31), v32_(v32), v33_(v33), v34_(v34), v35_(v35), v36_(v36) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray36& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37>
class ValueArray37 {
 public:
  ValueArray37(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37) : v1_(v1), v2_(v2), v3_(v3), v4_(v4),
      v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11),
      v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17),
      v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23),
      v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28), v29_(v29),
      v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34), v35_(v35),
      v36_(v36), v37_(v37) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray37& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38>
class ValueArray38 {
 public:
  ValueArray38(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34),
      v35_(v35), v36_(v36), v37_(v37), v38_(v38) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray38& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39>
class ValueArray39 {
 public:
  ValueArray39(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34),
      v35_(v35), v36_(v36), v37_(v37), v38_(v38), v39_(v39) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray39& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40>
class ValueArray40 {
 public:
  ValueArray40(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9),
      v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15),
      v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21),
      v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27),
      v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33),
      v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38), v39_(v39),
      v40_(v40) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray40& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41>
class ValueArray41 {
 public:
  ValueArray41(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40,
      T41 v41) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33), v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38),
      v39_(v39), v40_(v40), v41_(v41) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray41& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42>
class ValueArray42 {
 public:
  ValueArray42(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33), v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38),
      v39_(v39), v40_(v40), v41_(v41), v42_(v42) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray42& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43>
class ValueArray43 {
 public:
  ValueArray43(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6),
      v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13),
      v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19),
      v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25),
      v26_(v26), v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31),
      v32_(v32), v33_(v33), v34_(v34), v35_(v35), v36_(v36), v37_(v37),
      v38_(v38), v39_(v39), v40_(v40), v41_(v41), v42_(v42), v43_(v43) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray43& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44>
class ValueArray44 {
 public:
  ValueArray44(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5),
      v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12),
      v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17), v18_(v18),
      v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23), v24_(v24),
      v25_(v25), v26_(v26), v27_(v27), v28_(v28), v29_(v29), v30_(v30),
      v31_(v31), v32_(v32), v33_(v33), v34_(v34), v35_(v35), v36_(v36),
      v37_(v37), v38_(v38), v39_(v39), v40_(v40), v41_(v41), v42_(v42),
      v43_(v43), v44_(v44) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray44& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45>
class ValueArray45 {
 public:
  ValueArray45(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45) : v1_(v1), v2_(v2), v3_(v3), v4_(v4),
      v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10), v11_(v11),
      v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16), v17_(v17),
      v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22), v23_(v23),
      v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28), v29_(v29),
      v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34), v35_(v35),
      v36_(v36), v37_(v37), v38_(v38), v39_(v39), v40_(v40), v41_(v41),
      v42_(v42), v43_(v43), v44_(v44), v45_(v45) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray45& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46>
class ValueArray46 {
 public:
  ValueArray46(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45, T46 v46) : v1_(v1), v2_(v2), v3_(v3),
      v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34),
      v35_(v35), v36_(v36), v37_(v37), v38_(v38), v39_(v39), v40_(v40),
      v41_(v41), v42_(v42), v43_(v43), v44_(v44), v45_(v45), v46_(v46) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_), static_cast<T>(v46_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray46& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
  const T46 v46_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47>
class ValueArray47 {
 public:
  ValueArray47(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47) : v1_(v1), v2_(v2),
      v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9), v10_(v10),
      v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15), v16_(v16),
      v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21), v22_(v22),
      v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27), v28_(v28),
      v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33), v34_(v34),
      v35_(v35), v36_(v36), v37_(v37), v38_(v38), v39_(v39), v40_(v40),
      v41_(v41), v42_(v42), v43_(v43), v44_(v44), v45_(v45), v46_(v46),
      v47_(v47) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_), static_cast<T>(v46_), static_cast<T>(v47_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray47& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
  const T46 v46_;
  const T47 v47_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48>
class ValueArray48 {
 public:
  ValueArray48(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47, T48 v48) : v1_(v1),
      v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7), v8_(v8), v9_(v9),
      v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14), v15_(v15),
      v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20), v21_(v21),
      v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26), v27_(v27),
      v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32), v33_(v33),
      v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38), v39_(v39),
      v40_(v40), v41_(v41), v42_(v42), v43_(v43), v44_(v44), v45_(v45),
      v46_(v46), v47_(v47), v48_(v48) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_), static_cast<T>(v46_), static_cast<T>(v47_),
        static_cast<T>(v48_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray48& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
  const T46 v46_;
  const T47 v47_;
  const T48 v48_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48, typename T49>
class ValueArray49 {
 public:
  ValueArray49(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47, T48 v48,
      T49 v49) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33), v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38),
      v39_(v39), v40_(v40), v41_(v41), v42_(v42), v43_(v43), v44_(v44),
      v45_(v45), v46_(v46), v47_(v47), v48_(v48), v49_(v49) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_), static_cast<T>(v46_), static_cast<T>(v47_),
        static_cast<T>(v48_), static_cast<T>(v49_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray49& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
  const T46 v46_;
  const T47 v47_;
  const T48 v48_;
  const T49 v49_;
};

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48, typename T49, typename T50>
class ValueArray50 {
 public:
  ValueArray50(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
      T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
      T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
      T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
      T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
      T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47, T48 v48, T49 v49,
      T50 v50) : v1_(v1), v2_(v2), v3_(v3), v4_(v4), v5_(v5), v6_(v6), v7_(v7),
      v8_(v8), v9_(v9), v10_(v10), v11_(v11), v12_(v12), v13_(v13), v14_(v14),
      v15_(v15), v16_(v16), v17_(v17), v18_(v18), v19_(v19), v20_(v20),
      v21_(v21), v22_(v22), v23_(v23), v24_(v24), v25_(v25), v26_(v26),
      v27_(v27), v28_(v28), v29_(v29), v30_(v30), v31_(v31), v32_(v32),
      v33_(v33), v34_(v34), v35_(v35), v36_(v36), v37_(v37), v38_(v38),
      v39_(v39), v40_(v40), v41_(v41), v42_(v42), v43_(v43), v44_(v44),
      v45_(v45), v46_(v46), v47_(v47), v48_(v48), v49_(v49), v50_(v50) {}

  template <typename T>
  operator ParamGenerator<T>() const {
    const T array[] = {static_cast<T>(v1_), static_cast<T>(v2_),
        static_cast<T>(v3_), static_cast<T>(v4_), static_cast<T>(v5_),
        static_cast<T>(v6_), static_cast<T>(v7_), static_cast<T>(v8_),
        static_cast<T>(v9_), static_cast<T>(v10_), static_cast<T>(v11_),
        static_cast<T>(v12_), static_cast<T>(v13_), static_cast<T>(v14_),
        static_cast<T>(v15_), static_cast<T>(v16_), static_cast<T>(v17_),
        static_cast<T>(v18_), static_cast<T>(v19_), static_cast<T>(v20_),
        static_cast<T>(v21_), static_cast<T>(v22_), static_cast<T>(v23_),
        static_cast<T>(v24_), static_cast<T>(v25_), static_cast<T>(v26_),
        static_cast<T>(v27_), static_cast<T>(v28_), static_cast<T>(v29_),
        static_cast<T>(v30_), static_cast<T>(v31_), static_cast<T>(v32_),
        static_cast<T>(v33_), static_cast<T>(v34_), static_cast<T>(v35_),
        static_cast<T>(v36_), static_cast<T>(v37_), static_cast<T>(v38_),
        static_cast<T>(v39_), static_cast<T>(v40_), static_cast<T>(v41_),
        static_cast<T>(v42_), static_cast<T>(v43_), static_cast<T>(v44_),
        static_cast<T>(v45_), static_cast<T>(v46_), static_cast<T>(v47_),
        static_cast<T>(v48_), static_cast<T>(v49_), static_cast<T>(v50_)};
    return ValuesIn(array);
  }

 private:
//没有实现-不支持分配。
  void operator=(const ValueArray50& other);

  const T1 v1_;
  const T2 v2_;
  const T3 v3_;
  const T4 v4_;
  const T5 v5_;
  const T6 v6_;
  const T7 v7_;
  const T8 v8_;
  const T9 v9_;
  const T10 v10_;
  const T11 v11_;
  const T12 v12_;
  const T13 v13_;
  const T14 v14_;
  const T15 v15_;
  const T16 v16_;
  const T17 v17_;
  const T18 v18_;
  const T19 v19_;
  const T20 v20_;
  const T21 v21_;
  const T22 v22_;
  const T23 v23_;
  const T24 v24_;
  const T25 v25_;
  const T26 v26_;
  const T27 v27_;
  const T28 v28_;
  const T29 v29_;
  const T30 v30_;
  const T31 v31_;
  const T32 v32_;
  const T33 v33_;
  const T34 v34_;
  const T35 v35_;
  const T36 v36_;
  const T37 v37_;
  const T38 v38_;
  const T39 v39_;
  const T40 v40_;
  const T41 v41_;
  const T42 v42_;
  const T43 v43_;
  const T44 v44_;
  const T45 v45_;
  const T46 v46_;
  const T47 v47_;
  const T48 v48_;
  const T49 v49_;
  const T50 v50_;
};

# if GTEST_HAS_COMBINE
//内部实现-不要在用户代码中使用。
//
//从产生的值的笛卡尔积生成值
//通过参数生成器。
//
template <typename T1, typename T2>
class CartesianProductGenerator2
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2> > {
 public:
  typedef ::testing::tuple<T1, T2> ParamType;

  CartesianProductGenerator2(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2)
      : g1_(g1), g2_(g2) {}
  virtual ~CartesianProductGenerator2() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current2_;
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    ParamType current_value_;
};  //类cartesianprogramGenerator2:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator2& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
};  //CartesianProductGenerator2类


template <typename T1, typename T2, typename T3>
class CartesianProductGenerator3
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3> > {
 public:
  typedef ::testing::tuple<T1, T2, T3> ParamType;

  CartesianProductGenerator3(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3)
      : g1_(g1), g2_(g2), g3_(g3) {}
  virtual ~CartesianProductGenerator3() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current3_;
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    ParamType current_value_;
};  //类cartesianprogramGenerator3:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator3& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
};  //CartesianProtobGenerator3类


template <typename T1, typename T2, typename T3, typename T4>
class CartesianProductGenerator4
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4> ParamType;

  CartesianProductGenerator4(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4) {}
  virtual ~CartesianProductGenerator4() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current4_;
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    ParamType current_value_;
};  //类cartesianproductGenerator4:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator4& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
};  //CartesianProductGenerator4类


template <typename T1, typename T2, typename T3, typename T4, typename T5>
class CartesianProductGenerator5
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5> ParamType;

  CartesianProductGenerator5(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5) {}
  virtual ~CartesianProductGenerator5() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current5_;
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    ParamType current_value_;
};  //类cartesianprogramGenerator5:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator5& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
};  //CartesianProductGenerator5类


template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
class CartesianProductGenerator6
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5,
        T6> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5, T6> ParamType;

  CartesianProductGenerator6(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5,
      const ParamGenerator<T6>& g6)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6) {}
  virtual ~CartesianProductGenerator6() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin(), g6_, g6_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end(), g6_, g6_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5,
      const ParamGenerator<T6>& g6,
      const typename ParamGenerator<T6>::iterator& current6)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5),
          begin6_(g6.begin()), end6_(g6.end()), current6_(current6)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current6_;
      if (current6_ == end6_) {
        current6_ = begin6_;
        ++current5_;
      }
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_ &&
          current6_ == typed_other->current6_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_),
        begin6_(other.begin6_),
        end6_(other.end6_),
        current6_(other.current6_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_, *current6_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_ ||
          current6_ == end6_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    const typename ParamGenerator<T6>::iterator begin6_;
    const typename ParamGenerator<T6>::iterator end6_;
    typename ParamGenerator<T6>::iterator current6_;
    ParamType current_value_;
};  //类cartesianprogramGenerator6:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator6& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
  const ParamGenerator<T6> g6_;
};  //CartesianProductGenerator6类


template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
class CartesianProductGenerator7
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5, T6,
        T7> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5, T6, T7> ParamType;

  CartesianProductGenerator7(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5,
      const ParamGenerator<T6>& g6, const ParamGenerator<T7>& g7)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7) {}
  virtual ~CartesianProductGenerator7() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin(), g6_, g6_.begin(), g7_,
        g7_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end(), g6_, g6_.end(), g7_, g7_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5,
      const ParamGenerator<T6>& g6,
      const typename ParamGenerator<T6>::iterator& current6,
      const ParamGenerator<T7>& g7,
      const typename ParamGenerator<T7>::iterator& current7)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5),
          begin6_(g6.begin()), end6_(g6.end()), current6_(current6),
          begin7_(g7.begin()), end7_(g7.end()), current7_(current7)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current7_;
      if (current7_ == end7_) {
        current7_ = begin7_;
        ++current6_;
      }
      if (current6_ == end6_) {
        current6_ = begin6_;
        ++current5_;
      }
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_ &&
          current6_ == typed_other->current6_ &&
          current7_ == typed_other->current7_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_),
        begin6_(other.begin6_),
        end6_(other.end6_),
        current6_(other.current6_),
        begin7_(other.begin7_),
        end7_(other.end7_),
        current7_(other.current7_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_, *current6_, *current7_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_ ||
          current6_ == end6_ ||
          current7_ == end7_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    const typename ParamGenerator<T6>::iterator begin6_;
    const typename ParamGenerator<T6>::iterator end6_;
    typename ParamGenerator<T6>::iterator current6_;
    const typename ParamGenerator<T7>::iterator begin7_;
    const typename ParamGenerator<T7>::iterator end7_;
    typename ParamGenerator<T7>::iterator current7_;
    ParamType current_value_;
};  //类cartesianproductGenerator7:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator7& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
  const ParamGenerator<T6> g6_;
  const ParamGenerator<T7> g7_;
};  //CartesianProductGenerator7类


template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
class CartesianProductGenerator8
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5, T6,
        T7, T8> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8> ParamType;

  CartesianProductGenerator8(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5,
      const ParamGenerator<T6>& g6, const ParamGenerator<T7>& g7,
      const ParamGenerator<T8>& g8)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7),
          g8_(g8) {}
  virtual ~CartesianProductGenerator8() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin(), g6_, g6_.begin(), g7_,
        g7_.begin(), g8_, g8_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end(), g6_, g6_.end(), g7_, g7_.end(), g8_,
        g8_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5,
      const ParamGenerator<T6>& g6,
      const typename ParamGenerator<T6>::iterator& current6,
      const ParamGenerator<T7>& g7,
      const typename ParamGenerator<T7>::iterator& current7,
      const ParamGenerator<T8>& g8,
      const typename ParamGenerator<T8>::iterator& current8)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5),
          begin6_(g6.begin()), end6_(g6.end()), current6_(current6),
          begin7_(g7.begin()), end7_(g7.end()), current7_(current7),
          begin8_(g8.begin()), end8_(g8.end()), current8_(current8)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current8_;
      if (current8_ == end8_) {
        current8_ = begin8_;
        ++current7_;
      }
      if (current7_ == end7_) {
        current7_ = begin7_;
        ++current6_;
      }
      if (current6_ == end6_) {
        current6_ = begin6_;
        ++current5_;
      }
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_ &&
          current6_ == typed_other->current6_ &&
          current7_ == typed_other->current7_ &&
          current8_ == typed_other->current8_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_),
        begin6_(other.begin6_),
        end6_(other.end6_),
        current6_(other.current6_),
        begin7_(other.begin7_),
        end7_(other.end7_),
        current7_(other.current7_),
        begin8_(other.begin8_),
        end8_(other.end8_),
        current8_(other.current8_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_, *current6_, *current7_, *current8_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_ ||
          current6_ == end6_ ||
          current7_ == end7_ ||
          current8_ == end8_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    const typename ParamGenerator<T6>::iterator begin6_;
    const typename ParamGenerator<T6>::iterator end6_;
    typename ParamGenerator<T6>::iterator current6_;
    const typename ParamGenerator<T7>::iterator begin7_;
    const typename ParamGenerator<T7>::iterator end7_;
    typename ParamGenerator<T7>::iterator current7_;
    const typename ParamGenerator<T8>::iterator begin8_;
    const typename ParamGenerator<T8>::iterator end8_;
    typename ParamGenerator<T8>::iterator current8_;
    ParamType current_value_;
};  //cartesianprogramGenerator8类：：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator8& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
  const ParamGenerator<T6> g6_;
  const ParamGenerator<T7> g7_;
  const ParamGenerator<T8> g8_;
};  //CartesianProductGenerator8类


template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
class CartesianProductGenerator9
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5, T6,
        T7, T8, T9> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9> ParamType;

  CartesianProductGenerator9(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5,
      const ParamGenerator<T6>& g6, const ParamGenerator<T7>& g7,
      const ParamGenerator<T8>& g8, const ParamGenerator<T9>& g9)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7), g8_(g8),
          g9_(g9) {}
  virtual ~CartesianProductGenerator9() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin(), g6_, g6_.begin(), g7_,
        g7_.begin(), g8_, g8_.begin(), g9_, g9_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end(), g6_, g6_.end(), g7_, g7_.end(), g8_,
        g8_.end(), g9_, g9_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5,
      const ParamGenerator<T6>& g6,
      const typename ParamGenerator<T6>::iterator& current6,
      const ParamGenerator<T7>& g7,
      const typename ParamGenerator<T7>::iterator& current7,
      const ParamGenerator<T8>& g8,
      const typename ParamGenerator<T8>::iterator& current8,
      const ParamGenerator<T9>& g9,
      const typename ParamGenerator<T9>::iterator& current9)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5),
          begin6_(g6.begin()), end6_(g6.end()), current6_(current6),
          begin7_(g7.begin()), end7_(g7.end()), current7_(current7),
          begin8_(g8.begin()), end8_(g8.end()), current8_(current8),
          begin9_(g9.begin()), end9_(g9.end()), current9_(current9)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current9_;
      if (current9_ == end9_) {
        current9_ = begin9_;
        ++current8_;
      }
      if (current8_ == end8_) {
        current8_ = begin8_;
        ++current7_;
      }
      if (current7_ == end7_) {
        current7_ = begin7_;
        ++current6_;
      }
      if (current6_ == end6_) {
        current6_ = begin6_;
        ++current5_;
      }
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_ &&
          current6_ == typed_other->current6_ &&
          current7_ == typed_other->current7_ &&
          current8_ == typed_other->current8_ &&
          current9_ == typed_other->current9_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_),
        begin6_(other.begin6_),
        end6_(other.end6_),
        current6_(other.current6_),
        begin7_(other.begin7_),
        end7_(other.end7_),
        current7_(other.current7_),
        begin8_(other.begin8_),
        end8_(other.end8_),
        current8_(other.current8_),
        begin9_(other.begin9_),
        end9_(other.end9_),
        current9_(other.current9_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_, *current6_, *current7_, *current8_,
            *current9_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_ ||
          current6_ == end6_ ||
          current7_ == end7_ ||
          current8_ == end8_ ||
          current9_ == end9_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    const typename ParamGenerator<T6>::iterator begin6_;
    const typename ParamGenerator<T6>::iterator end6_;
    typename ParamGenerator<T6>::iterator current6_;
    const typename ParamGenerator<T7>::iterator begin7_;
    const typename ParamGenerator<T7>::iterator end7_;
    typename ParamGenerator<T7>::iterator current7_;
    const typename ParamGenerator<T8>::iterator begin8_;
    const typename ParamGenerator<T8>::iterator end8_;
    typename ParamGenerator<T8>::iterator current8_;
    const typename ParamGenerator<T9>::iterator begin9_;
    const typename ParamGenerator<T9>::iterator end9_;
    typename ParamGenerator<T9>::iterator current9_;
    ParamType current_value_;
};  //类cartesianprogramGenerator9:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator9& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
  const ParamGenerator<T6> g6_;
  const ParamGenerator<T7> g7_;
  const ParamGenerator<T8> g8_;
  const ParamGenerator<T9> g9_;
};  //CartesianProductGenerator9类


template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
class CartesianProductGenerator10
    : public ParamGeneratorInterface< ::testing::tuple<T1, T2, T3, T4, T5, T6,
        T7, T8, T9, T10> > {
 public:
  typedef ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> ParamType;

  CartesianProductGenerator10(const ParamGenerator<T1>& g1,
      const ParamGenerator<T2>& g2, const ParamGenerator<T3>& g3,
      const ParamGenerator<T4>& g4, const ParamGenerator<T5>& g5,
      const ParamGenerator<T6>& g6, const ParamGenerator<T7>& g7,
      const ParamGenerator<T8>& g8, const ParamGenerator<T9>& g9,
      const ParamGenerator<T10>& g10)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7), g8_(g8),
          g9_(g9), g10_(g10) {}
  virtual ~CartesianProductGenerator10() {}

  virtual ParamIteratorInterface<ParamType>* Begin() const {
    return new Iterator(this, g1_, g1_.begin(), g2_, g2_.begin(), g3_,
        g3_.begin(), g4_, g4_.begin(), g5_, g5_.begin(), g6_, g6_.begin(), g7_,
        g7_.begin(), g8_, g8_.begin(), g9_, g9_.begin(), g10_, g10_.begin());
  }
  virtual ParamIteratorInterface<ParamType>* End() const {
    return new Iterator(this, g1_, g1_.end(), g2_, g2_.end(), g3_, g3_.end(),
        g4_, g4_.end(), g5_, g5_.end(), g6_, g6_.end(), g7_, g7_.end(), g8_,
        g8_.end(), g9_, g9_.end(), g10_, g10_.end());
  }

 private:
  class Iterator : public ParamIteratorInterface<ParamType> {
   public:
    Iterator(const ParamGeneratorInterface<ParamType>* base,
      const ParamGenerator<T1>& g1,
      const typename ParamGenerator<T1>::iterator& current1,
      const ParamGenerator<T2>& g2,
      const typename ParamGenerator<T2>::iterator& current2,
      const ParamGenerator<T3>& g3,
      const typename ParamGenerator<T3>::iterator& current3,
      const ParamGenerator<T4>& g4,
      const typename ParamGenerator<T4>::iterator& current4,
      const ParamGenerator<T5>& g5,
      const typename ParamGenerator<T5>::iterator& current5,
      const ParamGenerator<T6>& g6,
      const typename ParamGenerator<T6>::iterator& current6,
      const ParamGenerator<T7>& g7,
      const typename ParamGenerator<T7>::iterator& current7,
      const ParamGenerator<T8>& g8,
      const typename ParamGenerator<T8>::iterator& current8,
      const ParamGenerator<T9>& g9,
      const typename ParamGenerator<T9>::iterator& current9,
      const ParamGenerator<T10>& g10,
      const typename ParamGenerator<T10>::iterator& current10)
        : base_(base),
          begin1_(g1.begin()), end1_(g1.end()), current1_(current1),
          begin2_(g2.begin()), end2_(g2.end()), current2_(current2),
          begin3_(g3.begin()), end3_(g3.end()), current3_(current3),
          begin4_(g4.begin()), end4_(g4.end()), current4_(current4),
          begin5_(g5.begin()), end5_(g5.end()), current5_(current5),
          begin6_(g6.begin()), end6_(g6.end()), current6_(current6),
          begin7_(g7.begin()), end7_(g7.end()), current7_(current7),
          begin8_(g8.begin()), end8_(g8.end()), current8_(current8),
          begin9_(g9.begin()), end9_(g9.end()), current9_(current9),
          begin10_(g10.begin()), end10_(g10.end()), current10_(current10)    {
      ComputeCurrentValue();
    }
    virtual ~Iterator() {}

    virtual const ParamGeneratorInterface<ParamType>* BaseGenerator() const {
      return base_;
    }
//不应对超出范围的迭代器调用advance
//因此，任何组件迭代器都不能超出范围的末尾。
    virtual void Advance() {
      assert(!AtEnd());
      ++current10_;
      if (current10_ == end10_) {
        current10_ = begin10_;
        ++current9_;
      }
      if (current9_ == end9_) {
        current9_ = begin9_;
        ++current8_;
      }
      if (current8_ == end8_) {
        current8_ = begin8_;
        ++current7_;
      }
      if (current7_ == end7_) {
        current7_ = begin7_;
        ++current6_;
      }
      if (current6_ == end6_) {
        current6_ = begin6_;
        ++current5_;
      }
      if (current5_ == end5_) {
        current5_ = begin5_;
        ++current4_;
      }
      if (current4_ == end4_) {
        current4_ = begin4_;
        ++current3_;
      }
      if (current3_ == end3_) {
        current3_ = begin3_;
        ++current2_;
      }
      if (current2_ == end2_) {
        current2_ = begin2_;
        ++current1_;
      }
      ComputeCurrentValue();
    }
    virtual ParamIteratorInterface<ParamType>* Clone() const {
      return new Iterator(*this);
    }
    virtual const ParamType* Current() const { return &current_value_; }
    virtual bool Equals(const ParamIteratorInterface<ParamType>& other) const {
//拥有相同的基极发生器可以保证另一个基极
//迭代器是相同的类型，我们可以向下转换。
      GTEST_CHECK_(BaseGenerator() == other.BaseGenerator())
          << "The program attempted to compare iterators "
          << "from different generators." << std::endl;
      const Iterator* typed_other =
          CheckedDowncastToActualType<const Iterator>(&other);
//如果迭代器都指向
//各自的范围。这可以发生在各种各样的时尚，
//所以我们必须咨询Atend（）。
      return (AtEnd() && typed_other->AtEnd()) ||
         (
          current1_ == typed_other->current1_ &&
          current2_ == typed_other->current2_ &&
          current3_ == typed_other->current3_ &&
          current4_ == typed_other->current4_ &&
          current5_ == typed_other->current5_ &&
          current6_ == typed_other->current6_ &&
          current7_ == typed_other->current7_ &&
          current8_ == typed_other->current8_ &&
          current9_ == typed_other->current9_ &&
          current10_ == typed_other->current10_);
    }

   private:
    Iterator(const Iterator& other)
        : base_(other.base_),
        begin1_(other.begin1_),
        end1_(other.end1_),
        current1_(other.current1_),
        begin2_(other.begin2_),
        end2_(other.end2_),
        current2_(other.current2_),
        begin3_(other.begin3_),
        end3_(other.end3_),
        current3_(other.current3_),
        begin4_(other.begin4_),
        end4_(other.end4_),
        current4_(other.current4_),
        begin5_(other.begin5_),
        end5_(other.end5_),
        current5_(other.current5_),
        begin6_(other.begin6_),
        end6_(other.end6_),
        current6_(other.current6_),
        begin7_(other.begin7_),
        end7_(other.end7_),
        current7_(other.current7_),
        begin8_(other.begin8_),
        end8_(other.end8_),
        current8_(other.current8_),
        begin9_(other.begin9_),
        end9_(other.end9_),
        current9_(other.current9_),
        begin10_(other.begin10_),
        end10_(other.end10_),
        current10_(other.current10_) {
      ComputeCurrentValue();
    }

    void ComputeCurrentValue() {
      if (!AtEnd())
        current_value_ = ParamType(*current1_, *current2_, *current3_,
            *current4_, *current5_, *current6_, *current7_, *current8_,
            *current9_, *current10_);
    }
    bool AtEnd() const {
//当
//组件迭代器已到达其范围的结尾。
      return
          current1_ == end1_ ||
          current2_ == end2_ ||
          current3_ == end3_ ||
          current4_ == end4_ ||
          current5_ == end5_ ||
          current6_ == end6_ ||
          current7_ == end7_ ||
          current8_ == end8_ ||
          current9_ == end9_ ||
          current10_ == end10_;
    }

//没有实现-不支持分配。
    void operator=(const Iterator& other);

    const ParamGeneratorInterface<ParamType>* const base_;
//开始[i]和结束[i]定义迭代器遍历的第i个范围。
//当前[i]是实际的遍历迭代器。
    const typename ParamGenerator<T1>::iterator begin1_;
    const typename ParamGenerator<T1>::iterator end1_;
    typename ParamGenerator<T1>::iterator current1_;
    const typename ParamGenerator<T2>::iterator begin2_;
    const typename ParamGenerator<T2>::iterator end2_;
    typename ParamGenerator<T2>::iterator current2_;
    const typename ParamGenerator<T3>::iterator begin3_;
    const typename ParamGenerator<T3>::iterator end3_;
    typename ParamGenerator<T3>::iterator current3_;
    const typename ParamGenerator<T4>::iterator begin4_;
    const typename ParamGenerator<T4>::iterator end4_;
    typename ParamGenerator<T4>::iterator current4_;
    const typename ParamGenerator<T5>::iterator begin5_;
    const typename ParamGenerator<T5>::iterator end5_;
    typename ParamGenerator<T5>::iterator current5_;
    const typename ParamGenerator<T6>::iterator begin6_;
    const typename ParamGenerator<T6>::iterator end6_;
    typename ParamGenerator<T6>::iterator current6_;
    const typename ParamGenerator<T7>::iterator begin7_;
    const typename ParamGenerator<T7>::iterator end7_;
    typename ParamGenerator<T7>::iterator current7_;
    const typename ParamGenerator<T8>::iterator begin8_;
    const typename ParamGenerator<T8>::iterator end8_;
    typename ParamGenerator<T8>::iterator current8_;
    const typename ParamGenerator<T9>::iterator begin9_;
    const typename ParamGenerator<T9>::iterator end9_;
    typename ParamGenerator<T9>::iterator current9_;
    const typename ParamGenerator<T10>::iterator begin10_;
    const typename ParamGenerator<T10>::iterator end10_;
    typename ParamGenerator<T10>::iterator current10_;
    ParamType current_value_;
};  //类cartesianproductGenerator10:：迭代器

//没有实现-不支持分配。
  void operator=(const CartesianProductGenerator10& other);

  const ParamGenerator<T1> g1_;
  const ParamGenerator<T2> g2_;
  const ParamGenerator<T3> g3_;
  const ParamGenerator<T4> g4_;
  const ParamGenerator<T5> g5_;
  const ParamGenerator<T6> g6_;
  const ParamGenerator<T7> g7_;
  const ParamGenerator<T8> g8_;
  const ParamGenerator<T9> g9_;
  const ParamGenerator<T10> g10_;
};  //CartesianProductGenerator10类


//内部实现-不要在用户代码中使用。
//
//提供带有多态特性的combine（）的助手类。他们允许
//如果t为，则将cartesiaprobuntgeneratorn<t>转换为paramgenerator<u>
//可转换为U。
//
template <class Generator1, class Generator2>
class CartesianProductHolder2 {
 public:
CartesianProductHolder2(const Generator1& g1, const Generator2& g2)
      : g1_(g1), g2_(g2) {}
  template <typename T1, typename T2>
  operator ParamGenerator< ::testing::tuple<T1, T2> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2> >(
        new CartesianProductGenerator2<T1, T2>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder2& other);

  const Generator1 g1_;
  const Generator2 g2_;
};  //CartesianProductHolder2类

template <class Generator1, class Generator2, class Generator3>
class CartesianProductHolder3 {
 public:
CartesianProductHolder3(const Generator1& g1, const Generator2& g2,
    const Generator3& g3)
      : g1_(g1), g2_(g2), g3_(g3) {}
  template <typename T1, typename T2, typename T3>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3> >(
        new CartesianProductGenerator3<T1, T2, T3>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder3& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
};  //CartesianProductHolder3类

template <class Generator1, class Generator2, class Generator3,
    class Generator4>
class CartesianProductHolder4 {
 public:
CartesianProductHolder4(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4) {}
  template <typename T1, typename T2, typename T3, typename T4>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4> >(
        new CartesianProductGenerator4<T1, T2, T3, T4>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder4& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
};  //CartesianProductHolder4类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5>
class CartesianProductHolder5 {
 public:
CartesianProductHolder5(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5> >(
        new CartesianProductGenerator5<T1, T2, T3, T4, T5>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder5& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
};  //CartesianProductHolder5类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5, class Generator6>
class CartesianProductHolder6 {
 public:
CartesianProductHolder6(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5,
    const Generator6& g6)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5,
      typename T6>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6> >(
        new CartesianProductGenerator6<T1, T2, T3, T4, T5, T6>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_),
        static_cast<ParamGenerator<T6> >(g6_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder6& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
  const Generator6 g6_;
};  //CartesianProductHolder6类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5, class Generator6, class Generator7>
class CartesianProductHolder7 {
 public:
CartesianProductHolder7(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5,
    const Generator6& g6, const Generator7& g7)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5,
      typename T6, typename T7>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6,
      T7> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7> >(
        new CartesianProductGenerator7<T1, T2, T3, T4, T5, T6, T7>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_),
        static_cast<ParamGenerator<T6> >(g6_),
        static_cast<ParamGenerator<T7> >(g7_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder7& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
  const Generator6 g6_;
  const Generator7 g7_;
};  //CartesianProductHolder7类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5, class Generator6, class Generator7,
    class Generator8>
class CartesianProductHolder8 {
 public:
CartesianProductHolder8(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5,
    const Generator6& g6, const Generator7& g7, const Generator8& g8)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7),
          g8_(g8) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5,
      typename T6, typename T7, typename T8>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7,
      T8> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8> >(
        new CartesianProductGenerator8<T1, T2, T3, T4, T5, T6, T7, T8>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_),
        static_cast<ParamGenerator<T6> >(g6_),
        static_cast<ParamGenerator<T7> >(g7_),
        static_cast<ParamGenerator<T8> >(g8_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder8& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
  const Generator6 g6_;
  const Generator7 g7_;
  const Generator8 g8_;
};  //CartesianProductHolder8类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5, class Generator6, class Generator7,
    class Generator8, class Generator9>
class CartesianProductHolder9 {
 public:
CartesianProductHolder9(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5,
    const Generator6& g6, const Generator7& g7, const Generator8& g8,
    const Generator9& g9)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7), g8_(g8),
          g9_(g9) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5,
      typename T6, typename T7, typename T8, typename T9>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8,
      T9> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8,
        T9> >(
        new CartesianProductGenerator9<T1, T2, T3, T4, T5, T6, T7, T8, T9>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_),
        static_cast<ParamGenerator<T6> >(g6_),
        static_cast<ParamGenerator<T7> >(g7_),
        static_cast<ParamGenerator<T8> >(g8_),
        static_cast<ParamGenerator<T9> >(g9_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder9& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
  const Generator6 g6_;
  const Generator7 g7_;
  const Generator8 g8_;
  const Generator9 g9_;
};  //CartesianProductHolder9类

template <class Generator1, class Generator2, class Generator3,
    class Generator4, class Generator5, class Generator6, class Generator7,
    class Generator8, class Generator9, class Generator10>
class CartesianProductHolder10 {
 public:
CartesianProductHolder10(const Generator1& g1, const Generator2& g2,
    const Generator3& g3, const Generator4& g4, const Generator5& g5,
    const Generator6& g6, const Generator7& g7, const Generator8& g8,
    const Generator9& g9, const Generator10& g10)
      : g1_(g1), g2_(g2), g3_(g3), g4_(g4), g5_(g5), g6_(g6), g7_(g7), g8_(g8),
          g9_(g9), g10_(g10) {}
  template <typename T1, typename T2, typename T3, typename T4, typename T5,
      typename T6, typename T7, typename T8, typename T9, typename T10>
  operator ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9,
      T10> >() const {
    return ParamGenerator< ::testing::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9,
        T10> >(
        new CartesianProductGenerator10<T1, T2, T3, T4, T5, T6, T7, T8, T9,
            T10>(
        static_cast<ParamGenerator<T1> >(g1_),
        static_cast<ParamGenerator<T2> >(g2_),
        static_cast<ParamGenerator<T3> >(g3_),
        static_cast<ParamGenerator<T4> >(g4_),
        static_cast<ParamGenerator<T5> >(g5_),
        static_cast<ParamGenerator<T6> >(g6_),
        static_cast<ParamGenerator<T7> >(g7_),
        static_cast<ParamGenerator<T8> >(g8_),
        static_cast<ParamGenerator<T9> >(g9_),
        static_cast<ParamGenerator<T10> >(g10_)));
  }

 private:
//没有实现-不支持分配。
  void operator=(const CartesianProductHolder10& other);

  const Generator1 g1_;
  const Generator2 g2_;
  const Generator3 g3_;
  const Generator4 g4_;
  const Generator5 g5_;
  const Generator6 g6_;
  const Generator7 g7_;
  const Generator8 g8_;
  const Generator9 g9_;
  const Generator10 g10_;
};  //CartesianProductHolder10类

# endif  //GTEST？有联合收割机

}  //命名空间内部
}  //命名空间测试

#endif  //gtest_有参数测试

#endif  //gtest_包括gtest_internal_gtest_param_util_generated_h_

#if GTEST_HAS_PARAM_TEST

namespace testing {

//生成参数生成器的函数。
//
//google test使用这些生成器生成值参数-
//参数化测试。当参数化测试用例被实例化时
//使用特定的生成器，Google测试创建并运行测试
//对于发电机产生的序列中的每个元件。
//
//在下面的示例中，将实例化来自测试用例footerst的测试
//每三次，参数值为3、5和8：
//
//footerst类：public testwithparam<int>…}；
//
//测试_p（footerst，测试this）
//}
//测试_p（footerst，testhat）
//}
//实例化_test_case_p（testsequence，footerst，values（3，5，8））；
//

//range（）返回提供范围内值序列的生成器。
//
//简介：
//范围（开始、结束）
//-返回生成一系列值的生成器start，start+1，
//开始+2，…，。
//范围（开始、结束、步骤）
//-返回生成值序列start、start+step的生成器，
//开始+步骤+步骤，。
//笔记：
//*生成的序列从不包含结束。例如，范围（1，5）
//返回生成序列1、2、3、4的生成器。范围（1, 9, 2）
//返回生成1、3、5、7的生成器。
//*开始和结束必须具有相同的类型。该类型可以是任何整数或
//满足这些条件的浮点类型或用户定义的类型：
//*它必须是可分配的（定义了运算符=）。
//*它必须具有运算符+（）（运算符+（int兼容类型），用于
//双操作数版本）。
//*必须定义运算符<（）。
//结果序列中的元素也将具有该类型。
//*必须满足条件start<end才能得到结果序列
//包含任何元素。
//
template <typename T, typename IncrementT>
internal::ParamGenerator<T> Range(T start, T end, IncrementT step) {
  return internal::ParamGenerator<T>(
      new internal::RangeGenerator<T, IncrementT>(start, end, step));
}

template <typename T>
internal::ParamGenerator<T> Range(T start, T end) {
  return Range(start, end, 1);
}

//函数的作用是：生成参数来自
//容器。
//
//简介：
//值in（const t（&array）[n]）
//-返回生成序列的生成器，其中元素来自
//C型数组。
//值in（常量容器和容器）
//-返回生成序列的生成器，其中元素来自
//STL样式的容器。
//值sin（迭代器开始、迭代器结束）
//-返回生成序列的生成器，其中元素来自
//由一对STL样式迭代器定义的范围[开始，结束]。这些
//迭代器也可以是普通的C指针。
//
//请注意，valuesin复制容器中的值
//传入并保持它们在run_all_tests（）中生成测试。
//
//实例：
//
//这将从测试用例StringTest实例化测试
//每个C字符串值为“foo”、“bar”和“baz”：
//
//const char*字符串[]=“foo”，“bar”，“baz”
//实例化_test_case_p（stringsequence，srtingtest，valuesin（strings））；
//
//这将从测试用例stlstringtest实例化测试
//每个字符串的值分别为“a”和“b”：
//
//：：std：：vector<：std：：string>getParameterStrings（）
//：：std：：vector<：std：：string>v；
//v.推回（“a”）；
//v.推回（“b”）；
//返回V；
//}
//
//实例化测试用例（charsequence，
//斯塔斯特林试验
//valuesin（getParameterStrings（））；
//
//
//这还将从Chartest实例化测试
//每个参数值为“a”和“b”：
//
//：：std:：list<char>getParameterChars（）
//：：std:：list<char>list；
//list.push_back（'a'）；
//list.push_back（'b'）；
//返回列表；
//}
//：：std:：list<char>l=getParameterChars（）；
//实例化测试用例（CharSequence2，
//宪章
//值sin（l.begin（），l.end（））；
//
template <typename ForwardIterator>
internal::ParamGenerator<
  typename ::testing::internal::IteratorTraits<ForwardIterator>::value_type>
ValuesIn(ForwardIterator begin, ForwardIterator end) {
  typedef typename ::testing::internal::IteratorTraits<ForwardIterator>
      ::value_type ParamType;
  return internal::ParamGenerator<ParamType>(
      new internal::ValuesInIteratorRangeGenerator<ParamType>(begin, end));
}

template <typename T, size_t N>
internal::ParamGenerator<T> ValuesIn(const T (&array)[N]) {
  return ValuesIn(array, array + N);
}

template <class Container>
internal::ParamGenerator<typename Container::value_type> ValuesIn(
    const Container& container) {
  return ValuesIn(container.begin(), container.end());
}

//values（）允许从显式指定的
//参数。
//
//简介：
//值（t v1，t v2，…，t vn）
//-返回生成元素v1、v2、…、vn序列的生成器。
//
//例如，它从测试用例bartest中实例化每个测试
//值为“1”、“2”和“3”：
//
//实例化_test_case_p（numsequence，bartest，values（“one”，“two”，“three”））；
//
//这将用值1、2、3.5实例化测试用例baztest中的每个测试。
//值的确切类型将取决于baztest中参数的类型。
//
//实例化_test_case_p（floatingnumbers，baztest，values（1，2，3.5））；
//
//目前，values（）支持1到50个参数。
//
template <typename T1>
internal::ValueArray1<T1> Values(T1 v1) {
  return internal::ValueArray1<T1>(v1);
}

template <typename T1, typename T2>
internal::ValueArray2<T1, T2> Values(T1 v1, T2 v2) {
  return internal::ValueArray2<T1, T2>(v1, v2);
}

template <typename T1, typename T2, typename T3>
internal::ValueArray3<T1, T2, T3> Values(T1 v1, T2 v2, T3 v3) {
  return internal::ValueArray3<T1, T2, T3>(v1, v2, v3);
}

template <typename T1, typename T2, typename T3, typename T4>
internal::ValueArray4<T1, T2, T3, T4> Values(T1 v1, T2 v2, T3 v3, T4 v4) {
  return internal::ValueArray4<T1, T2, T3, T4>(v1, v2, v3, v4);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
internal::ValueArray5<T1, T2, T3, T4, T5> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5) {
  return internal::ValueArray5<T1, T2, T3, T4, T5>(v1, v2, v3, v4, v5);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6>
internal::ValueArray6<T1, T2, T3, T4, T5, T6> Values(T1 v1, T2 v2, T3 v3,
    T4 v4, T5 v5, T6 v6) {
  return internal::ValueArray6<T1, T2, T3, T4, T5, T6>(v1, v2, v3, v4, v5, v6);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7>
internal::ValueArray7<T1, T2, T3, T4, T5, T6, T7> Values(T1 v1, T2 v2, T3 v3,
    T4 v4, T5 v5, T6 v6, T7 v7) {
  return internal::ValueArray7<T1, T2, T3, T4, T5, T6, T7>(v1, v2, v3, v4, v5,
      v6, v7);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8>
internal::ValueArray8<T1, T2, T3, T4, T5, T6, T7, T8> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8) {
  return internal::ValueArray8<T1, T2, T3, T4, T5, T6, T7, T8>(v1, v2, v3, v4,
      v5, v6, v7, v8);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9>
internal::ValueArray9<T1, T2, T3, T4, T5, T6, T7, T8, T9> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9) {
  return internal::ValueArray9<T1, T2, T3, T4, T5, T6, T7, T8, T9>(v1, v2, v3,
      v4, v5, v6, v7, v8, v9);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10>
internal::ValueArray10<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> Values(T1 v1,
    T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10) {
  return internal::ValueArray10<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>(v1,
      v2, v3, v4, v5, v6, v7, v8, v9, v10);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11>
internal::ValueArray11<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10,
    T11> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11) {
  return internal::ValueArray11<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10,
      T11>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12>
internal::ValueArray12<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
    T12> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12) {
  return internal::ValueArray12<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13>
internal::ValueArray13<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12,
    T13> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13) {
  return internal::ValueArray13<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14>
internal::ValueArray14<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14) {
  return internal::ValueArray14<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
      v14);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15>
internal::ValueArray15<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8,
    T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15) {
  return internal::ValueArray15<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
      v13, v14, v15);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16>
internal::ValueArray16<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16) {
  return internal::ValueArray16<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,
      v12, v13, v14, v15, v16);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17>
internal::ValueArray17<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17) {
  return internal::ValueArray17<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
      v11, v12, v13, v14, v15, v16, v17);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18>
internal::ValueArray18<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6,
    T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18) {
  return internal::ValueArray18<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18>(v1, v2, v3, v4, v5, v6, v7, v8, v9,
      v10, v11, v12, v13, v14, v15, v16, v17, v18);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19>
internal::ValueArray19<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5,
    T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14,
    T15 v15, T16 v16, T17 v17, T18 v18, T19 v19) {
  return internal::ValueArray19<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19>(v1, v2, v3, v4, v5, v6, v7, v8,
      v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20>
internal::ValueArray20<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13,
    T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20) {
  return internal::ValueArray20<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20>(v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21>
internal::ValueArray21<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13,
    T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21) {
  return internal::ValueArray21<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21>(v1, v2, v3, v4, v5, v6,
      v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22>
internal::ValueArray22<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22> Values(T1 v1, T2 v2, T3 v3,
    T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22) {
  return internal::ValueArray22<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22>(v1, v2, v3, v4,
      v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
      v20, v21, v22);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23>
internal::ValueArray23<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22, T23 v23) {
  return internal::ValueArray23<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23>(v1, v2, v3,
      v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
      v20, v21, v22, v23);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24>
internal::ValueArray24<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22, T23 v23, T24 v24) {
  return internal::ValueArray24<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24>(v1, v2,
      v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,
      v19, v20, v21, v22, v23, v24);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25>
internal::ValueArray25<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25> Values(T1 v1,
    T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11,
    T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19,
    T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25) {
  return internal::ValueArray25<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25>(v1,
      v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
      v18, v19, v20, v21, v22, v23, v24, v25);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26>
internal::ValueArray26<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
    T26> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26) {
  return internal::ValueArray26<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,
      v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27>
internal::ValueArray27<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26,
    T27> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27) {
  return internal::ValueArray27<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14,
      v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28>
internal::ValueArray28<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27,
    T28> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28) {
  return internal::ValueArray28<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
      v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27,
      v28);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29>
internal::ValueArray29<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28, T29 v29) {
  return internal::ValueArray29<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
      v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26,
      v27, v28, v29);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30>
internal::ValueArray30<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8,
    T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16,
    T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24,
    T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30) {
  return internal::ValueArray30<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,
      v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25,
      v26, v27, v28, v29, v30);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31>
internal::ValueArray31<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31) {
  return internal::ValueArray31<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
      v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24,
      v25, v26, v27, v28, v29, v30, v31);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32>
internal::ValueArray32<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31,
    T32 v32) {
  return internal::ValueArray32<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32>(v1, v2, v3, v4, v5, v6, v7, v8, v9,
      v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
      v24, v25, v26, v27, v28, v29, v30, v31, v32);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33>
internal::ValueArray33<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6,
    T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31,
    T32 v32, T33 v33) {
  return internal::ValueArray33<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33>(v1, v2, v3, v4, v5, v6, v7, v8,
      v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
      v24, v25, v26, v27, v28, v29, v30, v31, v32, v33);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34>
internal::ValueArray34<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5,
    T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14,
    T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22,
    T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30,
    T31 v31, T32 v32, T33 v33, T34 v34) {
  return internal::ValueArray34<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34>(v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
      v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35>
internal::ValueArray35<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13,
    T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21,
    T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29,
    T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35) {
  return internal::ValueArray35<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35>(v1, v2, v3, v4, v5, v6,
      v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21,
      v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36>
internal::ValueArray36<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13,
    T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21,
    T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29,
    T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36) {
  return internal::ValueArray36<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36>(v1, v2, v3, v4,
      v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
      v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33,
      v34, v35, v36);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37>
internal::ValueArray37<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37> Values(T1 v1, T2 v2, T3 v3,
    T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28,
    T29 v29, T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36,
    T37 v37) {
  return internal::ValueArray37<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37>(v1, v2, v3,
      v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
      v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33,
      v34, v35, v36, v37);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38>
internal::ValueArray38<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28,
    T29 v29, T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36,
    T37 v37, T38 v38) {
  return internal::ValueArray38<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38>(v1, v2,
      v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18,
      v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32,
      v33, v34, v35, v36, v37, v38);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39>
internal::ValueArray39<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39> Values(T1 v1, T2 v2,
    T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12,
    T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20,
    T21 v21, T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28,
    T29 v29, T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36,
    T37 v37, T38 v38, T39 v39) {
  return internal::ValueArray39<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39>(v1,
      v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,
      v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31,
      v32, v33, v34, v35, v36, v37, v38, v39);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40>
internal::ValueArray40<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40> Values(T1 v1,
    T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11,
    T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19,
    T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27,
    T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35,
    T36 v36, T37 v37, T38 v38, T39 v39, T40 v40) {
  return internal::ValueArray40<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,
      v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29,
      v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41>
internal::ValueArray41<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40,
    T41> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
    T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41) {
  return internal::ValueArray41<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14,
      v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28,
      v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42>
internal::ValueArray42<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41,
    T42> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
    T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
    T42 v42) {
  return internal::ValueArray42<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
      v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27,
      v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41,
      v42);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43>
internal::ValueArray43<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42,
    T43> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
    T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
    T42 v42, T43 v43) {
  return internal::ValueArray43<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
      v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26,
      v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40,
      v41, v42, v43);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44>
internal::ValueArray44<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8, T9 v9,
    T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16, T17 v17,
    T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24, T25 v25,
    T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32, T33 v33,
    T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40, T41 v41,
    T42 v42, T43 v43, T44 v44) {
  return internal::ValueArray44<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11,
      v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25,
      v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39,
      v40, v41, v42, v43, v44);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45>
internal::ValueArray45<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7, T8 v8,
    T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15, T16 v16,
    T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23, T24 v24,
    T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31, T32 v32,
    T33 v33, T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39, T40 v40,
    T41 v41, T42 v42, T43 v43, T44 v44, T45 v45) {
  return internal::ValueArray45<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45>(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10,
      v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24,
      v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38,
      v39, v40, v41, v42, v43, v44, v45);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46>
internal::ValueArray46<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45, T46> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31,
    T32 v32, T33 v33, T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39,
    T40 v40, T41 v41, T42 v42, T43 v43, T44 v44, T45 v45, T46 v46) {
  return internal::ValueArray46<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45, T46>(v1, v2, v3, v4, v5, v6, v7, v8, v9,
      v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
      v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,
      v38, v39, v40, v41, v42, v43, v44, v45, v46);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47>
internal::ValueArray47<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45, T46, T47> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6, T7 v7,
    T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31,
    T32 v32, T33 v33, T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39,
    T40 v40, T41 v41, T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47) {
  return internal::ValueArray47<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45, T46, T47>(v1, v2, v3, v4, v5, v6, v7, v8,
      v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23,
      v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37,
      v38, v39, v40, v41, v42, v43, v44, v45, v46, v47);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48>
internal::ValueArray48<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45, T46, T47, T48> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5, T6 v6,
    T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14, T15 v15,
    T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22, T23 v23,
    T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30, T31 v31,
    T32 v32, T33 v33, T34 v34, T35 v35, T36 v36, T37 v37, T38 v38, T39 v39,
    T40 v40, T41 v41, T42 v42, T43 v43, T44 v44, T45 v45, T46 v46, T47 v47,
    T48 v48) {
  return internal::ValueArray48<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45, T46, T47, T48>(v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22,
      v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36,
      v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48, typename T49>
internal::ValueArray49<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45, T46, T47, T48, T49> Values(T1 v1, T2 v2, T3 v3, T4 v4, T5 v5,
    T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13, T14 v14,
    T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21, T22 v22,
    T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29, T30 v30,
    T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36, T37 v37, T38 v38,
    T39 v39, T40 v40, T41 v41, T42 v42, T43 v43, T44 v44, T45 v45, T46 v46,
    T47 v47, T48 v48, T49 v49) {
  return internal::ValueArray49<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45, T46, T47, T48, T49>(v1, v2, v3, v4, v5, v6,
      v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21,
      v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35,
      v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9, typename T10,
    typename T11, typename T12, typename T13, typename T14, typename T15,
    typename T16, typename T17, typename T18, typename T19, typename T20,
    typename T21, typename T22, typename T23, typename T24, typename T25,
    typename T26, typename T27, typename T28, typename T29, typename T30,
    typename T31, typename T32, typename T33, typename T34, typename T35,
    typename T36, typename T37, typename T38, typename T39, typename T40,
    typename T41, typename T42, typename T43, typename T44, typename T45,
    typename T46, typename T47, typename T48, typename T49, typename T50>
internal::ValueArray50<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13,
    T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25, T26, T27, T28,
    T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39, T40, T41, T42, T43,
    T44, T45, T46, T47, T48, T49, T50> Values(T1 v1, T2 v2, T3 v3, T4 v4,
    T5 v5, T6 v6, T7 v7, T8 v8, T9 v9, T10 v10, T11 v11, T12 v12, T13 v13,
    T14 v14, T15 v15, T16 v16, T17 v17, T18 v18, T19 v19, T20 v20, T21 v21,
    T22 v22, T23 v23, T24 v24, T25 v25, T26 v26, T27 v27, T28 v28, T29 v29,
    T30 v30, T31 v31, T32 v32, T33 v33, T34 v34, T35 v35, T36 v36, T37 v37,
    T38 v38, T39 v39, T40 v40, T41 v41, T42 v42, T43 v43, T44 v44, T45 v45,
    T46 v46, T47 v47, T48 v48, T49 v49, T50 v50) {
  return internal::ValueArray50<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11,
      T12, T13, T14, T15, T16, T17, T18, T19, T20, T21, T22, T23, T24, T25,
      T26, T27, T28, T29, T30, T31, T32, T33, T34, T35, T36, T37, T38, T39,
      T40, T41, T42, T43, T44, T45, T46, T47, T48, T49, T50>(v1, v2, v3, v4,
      v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19,
      v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33,
      v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47,
      v48, v49, v50);
}

//bool（）允许使用一组（false，true）中的参数生成测试。
//
//简介：
//布尔（）
//-返回生成元素为假、真的序列的生成器。
//
//它在测试依赖于布尔标记的代码时很有用。组合
//当使用
//combine（）函数。
//
//在下面的示例中，测试用例flagDependentTest中的所有测试
//将用参数false和true实例化两次。
//
//class flagDependentTest:公共测试：：testWithParam<bool>
//virtual void setup（）
//外部_flag=getParam（）；
//}
//}
//实例化_test_case_p（boolsequence，flagDependentTest，bool（））；
//
inline internal::ParamGenerator<bool> Bool() {
  return Values(false, true);
}

# if GTEST_HAS_COMBINE
//combine（）允许用户组合两个或多个序列以生成
//这些序列元素的笛卡尔积的值。
//
//简介：
//合并（gen1，gen2，…，genn）
//-返回生成序列的生成器，其中元素来自
//序列中元素的笛卡尔积
//Gen1，Gen2，…，Genn.序列元素的类型为
//tuple<t1，t2，…，tn>其中t1，t2，…，tn是类型
//由gen1，gen2，…，genn生成的序列中的元素。
//
//Combine最多可以有10个参数。这个号码目前有限
//按Google使用的元组实现中的最大元素数
//测试。
//
//例子：
//
//这将实例化测试用例中的测试，并用
//参数值tuple（“cat”，black），tuple（“cat”，white），
//tuple（“狗”，黑色）和tuple（“狗”，白色）：
//
//枚举颜色黑色、灰色、白色
//类动物学家
//：public testing:：testwitparam<tuple<const char*，color>>…
//
//测试…
//
//实例化_test_case_p（动画变量，动画测试，
//组合（值（“cat”、“dog”），
//值（黑色、白色））；
//
//这将在flagDependentTest中实例化所有变量为2的测试
//布尔标志：
//
//类标志DependentTest
//：公共测试：：testWithParam<tuple<bool，bool>>
//virtual void setup（）
////从元组中分配外部_标志_1和外部_标志_2值。
//TIE（外部_标志_1，外部_标志_2）=getParam（）；
//}
//}；
//
//测试_p（flagDependentTest，测试功能1）
////在这里使用external_flag_1和external_flag_2测试代码。
//}
//实例化测试用例（twobolsequence，flagDependentTest，
//合并（bool（），bool（））；
//
template <typename Generator1, typename Generator2>
internal::CartesianProductHolder2<Generator1, Generator2> Combine(
    const Generator1& g1, const Generator2& g2) {
  return internal::CartesianProductHolder2<Generator1, Generator2>(
      g1, g2);
}

template <typename Generator1, typename Generator2, typename Generator3>
internal::CartesianProductHolder3<Generator1, Generator2, Generator3> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3) {
  return internal::CartesianProductHolder3<Generator1, Generator2, Generator3>(
      g1, g2, g3);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4>
internal::CartesianProductHolder4<Generator1, Generator2, Generator3,
    Generator4> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4) {
  return internal::CartesianProductHolder4<Generator1, Generator2, Generator3,
      Generator4>(
      g1, g2, g3, g4);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5>
internal::CartesianProductHolder5<Generator1, Generator2, Generator3,
    Generator4, Generator5> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5) {
  return internal::CartesianProductHolder5<Generator1, Generator2, Generator3,
      Generator4, Generator5>(
      g1, g2, g3, g4, g5);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5, typename Generator6>
internal::CartesianProductHolder6<Generator1, Generator2, Generator3,
    Generator4, Generator5, Generator6> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5, const Generator6& g6) {
  return internal::CartesianProductHolder6<Generator1, Generator2, Generator3,
      Generator4, Generator5, Generator6>(
      g1, g2, g3, g4, g5, g6);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5, typename Generator6,
    typename Generator7>
internal::CartesianProductHolder7<Generator1, Generator2, Generator3,
    Generator4, Generator5, Generator6, Generator7> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5, const Generator6& g6,
        const Generator7& g7) {
  return internal::CartesianProductHolder7<Generator1, Generator2, Generator3,
      Generator4, Generator5, Generator6, Generator7>(
      g1, g2, g3, g4, g5, g6, g7);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5, typename Generator6,
    typename Generator7, typename Generator8>
internal::CartesianProductHolder8<Generator1, Generator2, Generator3,
    Generator4, Generator5, Generator6, Generator7, Generator8> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5, const Generator6& g6,
        const Generator7& g7, const Generator8& g8) {
  return internal::CartesianProductHolder8<Generator1, Generator2, Generator3,
      Generator4, Generator5, Generator6, Generator7, Generator8>(
      g1, g2, g3, g4, g5, g6, g7, g8);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5, typename Generator6,
    typename Generator7, typename Generator8, typename Generator9>
internal::CartesianProductHolder9<Generator1, Generator2, Generator3,
    Generator4, Generator5, Generator6, Generator7, Generator8,
    Generator9> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5, const Generator6& g6,
        const Generator7& g7, const Generator8& g8, const Generator9& g9) {
  return internal::CartesianProductHolder9<Generator1, Generator2, Generator3,
      Generator4, Generator5, Generator6, Generator7, Generator8, Generator9>(
      g1, g2, g3, g4, g5, g6, g7, g8, g9);
}

template <typename Generator1, typename Generator2, typename Generator3,
    typename Generator4, typename Generator5, typename Generator6,
    typename Generator7, typename Generator8, typename Generator9,
    typename Generator10>
internal::CartesianProductHolder10<Generator1, Generator2, Generator3,
    Generator4, Generator5, Generator6, Generator7, Generator8, Generator9,
    Generator10> Combine(
    const Generator1& g1, const Generator2& g2, const Generator3& g3,
        const Generator4& g4, const Generator5& g5, const Generator6& g6,
        const Generator7& g7, const Generator8& g8, const Generator9& g9,
        const Generator10& g10) {
  return internal::CartesianProductHolder10<Generator1, Generator2, Generator3,
      Generator4, Generator5, Generator6, Generator7, Generator8, Generator9,
      Generator10>(
      g1, g2, g3, g4, g5, g6, g7, g8, g9, g10);
}
# endif  //GTEST？有联合收割机



# define TEST_P(test_case_name, test_name) \
  class GTEST_TEST_CLASS_NAME_(test_case_name, test_name) \
      : public test_case_name { \
   public: \
    GTEST_TEST_CLASS_NAME_(test_case_name, test_name)() {} \
    virtual void TestBody(); \
   private: \
    static int AddToRegistry() { \
      ::testing::UnitTest::GetInstance()->parameterized_test_registry(). \
          GetTestCasePatternHolder<test_case_name>(\
              #test_case_name, __FILE__, __LINE__)->AddTestPattern(\
                  #test_case_name, \
                  #test_name, \
                  new ::testing::internal::TestMetaFactory< \
                      GTEST_TEST_CLASS_NAME_(test_case_name, test_name)>()); \
      return 0; \
    } \
    static int gtest_registering_dummy_ GTEST_ATTRIBUTE_UNUSED_; \
    GTEST_DISALLOW_COPY_AND_ASSIGN_(\
        GTEST_TEST_CLASS_NAME_(test_case_name, test_name)); \
  }; \
  int GTEST_TEST_CLASS_NAME_(test_case_name, \
                             test_name)::gtest_registering_dummy_ = \
      GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::AddToRegistry(); \
  void GTEST_TEST_CLASS_NAME_(test_case_name, test_name)::TestBody()

# define INSTANTIATE_TEST_CASE_P(prefix, test_case_name, generator) \
  ::testing::internal::ParamGenerator<test_case_name::ParamType> \
      gtest_##prefix##test_case_name##_EvalGenerator_() { return generator; } \
  int gtest_##prefix##test_case_name##_dummy_ = \
      ::testing::UnitTest::GetInstance()->parameterized_test_registry(). \
          GetTestCasePatternHolder<test_case_name>(\
              #test_case_name, __FILE__, __LINE__)->AddTestCaseInstantiation(\
                  #prefix, \
                  &gtest_##prefix##test_case_name##_EvalGenerator_, \
                  __FILE__, __LINE__)

}  //命名空间测试

#endif  //gtest_有参数测试

#endif  //gtest_包括gtest_gtest_param_test_h_
//版权所有2006，Google Inc.
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：wan@google.com（zhanyong wan）
//
//谷歌C++测试框架定义在生产代码中很有用。

#ifndef GTEST_INCLUDE_GTEST_GTEST_PROD_H_
#define GTEST_INCLUDE_GTEST_GTEST_PROD_H_

//当需要测试类的私有成员或受保护成员时，
//使用friend_test宏将测试声明为
//班级。例如：
//
//类Myclass {
//私人：
//void mymethod（）；
//朋友测试（myclasstest，mymethod）；
//}；
//
//类MyClassTest:公共测试：：测试
///…
//}；
//
//测试F（MyClassTest，MyMethod）
////在此可以调用MyClass:：MyMethod（）。
//}

#define FRIEND_TEST(test_case_name, test_name)\
friend class test_case_name##_##test_name##_Test

#endif  //gtest_包括gtest_gtest_prod_h_
//版权所有2008，Google Inc.
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：mhule@google.com（markus heule）
//

#ifndef GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_
#define GTEST_INCLUDE_GTEST_GTEST_TEST_PART_H_

#include <iosfwd>
#include <vector>

namespace testing {

//表示测试部件结果的可复制对象（即
//断言或显式fail（）、add_failure（）或success（））。
//
//不要从testpartresult继承，因为它的析构函数不是虚拟的。
class GTEST_API_ TestPartResult {
 public:
//测试部分的可能结果（即断言或
//显式success（）、fail（）或add_failure（））。
  enum Type {
kSuccess,          //成功。
kNonFatalFailure,  //失败，但测试可以继续。
kFatalFailure      //失败，应终止测试。
  };

//Cortestpartresult没有默认构造函数。
//始终使用此构造函数（带参数）创建
//TestPartResult对象。
  TestPartResult(Type a_type,
                 const char* a_file_name,
                 int a_line_number,
                 const char* a_message)
      : type_(a_type),
        file_name_(a_file_name == NULL ? "" : a_file_name),
        line_number_(a_line_number),
        summary_(ExtractSummary(a_message)),
        message_(a_message) {
  }

//获取测试部件的结果。
  Type type() const { return type_; }

//获取测试部件所在的源文件的名称，或者
//如果未知则为空。
  const char* file_name() const {
    return file_name_.empty() ? NULL : file_name_.c_str();
  }

//获取源文件中发生测试部件的行，
//或者-1，如果不知道的话。
  int line_number() const { return line_number_; }

//获取失败消息的摘要。
  const char* summary() const { return summary_.c_str(); }

//获取与测试部件关联的消息。
  const char* message() const { return message_.c_str(); }

//如果测试部分通过，则返回true。
  bool passed() const { return type_ == kSuccess; }

//如果测试部件失败，则返回true。
  bool failed() const { return type_ != kSuccess; }

//如果测试部件非致命失败，则返回true。
  bool nonfatally_failed() const { return type_ == kNonFatalFailure; }

//如果测试部分最终失败，则返回true。
  bool fatally_failed() const { return type_ == kFatalFailure; }

 private:
  Type type_;

//通过省略堆栈获取失败消息的摘要
//追踪。
  static std::string ExtractSummary(const char* message);

//测试部件所在的源文件的名称，或
//如果源文件未知。
  std::string file_name_;
//源文件中测试部件所在的行，或-1
//如果行号未知。
  int line_number_;
std::string summary_;  //测试失败摘要。
std::string message_;  //测试失败消息。
};

//打印testpartresult对象。
std::ostream& operator<<(std::ostream& os, const TestPartResult& result);

//testpartresult对象的数组。
//
//不要从TestPartResultArray继承，因为其析构函数不是
//事实上的。
class GTEST_API_ TestPartResultArray {
 public:
  TestPartResultArray() {}

//将给定的testpartresult追加到数组。
  void Append(const TestPartResult& result);

//返回给定索引处的testpartresult（从0开始）。
  const TestPartResult& GetTestPartResult(int index) const;

//返回数组中的testpartresult对象数。
  int size() const;

 private:
  std::vector<TestPartResult> array_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestPartResultArray);
};

//此接口知道如何报告测试部件结果。
class TestPartResultReporterInterface {
 public:
  virtual ~TestPartResultReporterInterface() {}

  virtual void ReportTestPartResult(const TestPartResult& result) = 0;
};

namespace internal {

//此助手类由断言预期_否_致命_失败使用，无法检查
//语句生成新的致命错误。为此，它将自己注册为
//当前测试部件结果报告器。除了检查致命故障
//报告中，它只将报告委托给前结果报告人。
//原始结果报告器在析构函数中恢复。
//内部实现-不要在用户程序中使用。
class GTEST_API_ HasNewFatalFailureHelper
    : public TestPartResultReporterInterface {
 public:
  HasNewFatalFailureHelper();
  virtual ~HasNewFatalFailureHelper();
  virtual void ReportTestPartResult(const TestPartResult& result);
  bool has_new_fatal_failure() const { return has_new_fatal_failure_; }
 private:
  bool has_new_fatal_failure_;
  TestPartResultReporterInterface* original_reporter_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(HasNewFatalFailureHelper);
};

}  //命名空间内部

}  //命名空间测试

#endif  //gtest_包括gtest_gtest_test_part_h_
//版权所有2008 Google Inc.
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。
//
//作者：wan@google.com（zhanyong wan）

#ifndef GTEST_INCLUDE_GTEST_GTEST_TYPED_TEST_H_
#define GTEST_INCLUDE_GTEST_GTEST_TYPED_TEST_H_

//此头实现类型化测试和类型参数化测试。

//类型化（亦称类型驱动）测试对中的类型重复相同的测试
//名单。您必须知道在编写时要测试哪些类型
//类型化测试。您可以这样做：

#if 0

//首先，定义一个fixture类模板。它应该参数化
//一种类型。记住要从testing:：test派生它。
template <typename T>
class FooTest : public testing::Test {
 public:
  ...
  typedef std::list<T> List;
  static T shared_;
  T value_;
};

//接下来，将类型列表与测试用例关联，这将是
//对列表中的每种类型重复。typedef对于
//要正确分析的宏。
typedef testing::Types<char, int, unsigned int> MyTypes;
TYPED_TEST_CASE(FooTest, MyTypes);

//如果类型列表只包含一个类型，则可以编写该类型
//直接不带类型<…>：
//类型化测试用例（footerst，int）；

//然后，使用类型化的_test（）而不是test_f（）来定义尽可能多的类型化
//根据需要对此测试用例进行测试。
TYPED_TEST(FooTest, DoesBlah) {
//在测试中，请参阅typeparam以获取类型参数。
//由于我们在派生类模板中，所以C++需要使用
//通过“this”访问footerst的成员。
  TypeParam n = this->value_;

//要访问fixture的静态成员，请添加测试fixture:：
//前缀。
  n += TestFixture::shared_;

//要引用fixture中的typedef，请添加“typename”
//测试夹具：：“前缀。
  typename TestFixture::List values;
  values.push_back(n);
  ...
}

TYPED_TEST(FooTest, HasPropertyA) { ... }

#endif  //零

//类型参数化测试是参数化的抽象测试模式
//一种类型。与类型化测试相比，类型参数化测试
//允许您在不知道类型的情况下定义测试模式
//参数为。定义的模式可以用
//不同类型，任意次数，任意数量的翻译
//单位。
//
//如果要设计接口或概念，可以定义
//一组类型参数化测试，用于验证
//接口/概念的有效实现应该有。然后，
//每个实现都可以很容易地实例化测试套件来验证
//它符合要求，不必写
//重复类似的测试。下面是一个例子：

#if 0

//首先，定义一个fixture类模板。它应该参数化
//一种类型。记住要从testing:：test派生它。
template <typename T>
class FooTest : public testing::Test {
  ...
};

//接下来，声明您将定义一个类型参数化测试用例
//（后缀为“参数化”或“模式”，无论您是谁
//喜欢）：
TYPED_TEST_CASE_P(FooTest);

//然后，使用类型化的_test_p（）定义尽可能多的类型参数化测试
//对于此类型的参数化测试用例。
TYPED_TEST_P(FooTest, DoesBlah) {
//在测试中，请参阅typeparam以获取类型参数。
  TypeParam n = 0;
  ...
}

TYPED_TEST_P(FooTest, HasPropertyA) { ... }

//现在棘手的部分：您需要在注册之前注册所有测试模式
//您可以实例化它们。宏的第一个参数是
//测试用例名称；其余是此测试中测试的名称
//案例。
REGISTER_TYPED_TEST_CASE_P(FooTest,
                           DoesBlah, HasPropertyA);

//最后，您可以用您的类型自由地实例化模式。
//想要。如果将上述代码放在头文件中，则可以包括
//它在多个C++源文件中并多次实例化。
//
//为了区分模式的不同实例，首先
//实例化宏的参数是将要添加的前缀
//实际测试用例名称。记住要为选择唯一的前缀
//不同的实例。
typedef testing::Types<char, int, unsigned int> MyTypes;
INSTANTIATE_TYPED_TEST_CASE_P(My, FooTest, MyTypes);

//如果类型列表只包含一个类型，则可以编写该类型
//直接不带类型<…>：
//实例化_-typed_-test_-case_p（my，footst，int）；

#endif  //零


//实现类型化测试。

#if GTEST_HAS_TYPED_TEST

//内部实现-不要在用户代码中使用。
//
//扩展到的类型参数的typedef的名称
//给定测试用例。
# define GTEST_TYPE_PARAMS_(TestCaseName) gtest_type_params_##TestCaseName##_

//下面的“types”模板参数周围必须有空格
//因为有些编译器在传递模板时可能会阻塞“>>”
//实例（例如，types<int>）
# define TYPED_TEST_CASE(CaseName, Types) \
  typedef ::testing::internal::TypeList< Types >::type \
      GTEST_TYPE_PARAMS_(CaseName)

# define TYPED_TEST(CaseName, TestName) \
  template <typename gtest_TypeParam_> \
  class GTEST_TEST_CLASS_NAME_(CaseName, TestName) \
      : public CaseName<gtest_TypeParam_> { \
   private: \
    typedef CaseName<gtest_TypeParam_> TestFixture; \
    typedef gtest_TypeParam_ TypeParam; \
    virtual void TestBody(); \
  }; \
  bool gtest_##CaseName##_##TestName##_registered_ GTEST_ATTRIBUTE_UNUSED_ = \
      ::testing::internal::TypeParameterizedTest< \
          CaseName, \
          ::testing::internal::TemplateSel< \
              GTEST_TEST_CLASS_NAME_(CaseName, TestName)>, \
          GTEST_TYPE_PARAMS_(CaseName)>::Register(\
              "", #CaseName, #TestName, 0); \
  template <typename gtest_TypeParam_> \
  void GTEST_TEST_CLASS_NAME_(CaseName, TestName)<gtest_TypeParam_>::TestBody()

#endif  //GTest_有_型_测试

//实现类型参数化测试。

#if GTEST_HAS_TYPED_TEST_P

//内部实现-不要在用户代码中使用。
//
//扩展到类型参数化测试的命名空间名称
//在中定义了给定的类型参数化测试用例。确切的
//名称空间的名称可能随时更改，恕不另行通知。
# define GTEST_CASE_NAMESPACE_(TestCaseName) \
  gtest_case_##TestCaseName##_

//内部实现-不要在用户代码中使用。
//
//扩展到用于记住
//给定测试用例中定义的测试。
# define GTEST_TYPED_TEST_CASE_P_STATE_(TestCaseName) \
  gtest_typed_test_case_p_state_##TestCaseName##_

//内部实现-不要直接在用户代码中使用。
//
//扩展到用于记住
//在给定测试用例中注册的测试。
# define GTEST_REGISTERED_TEST_NAMES_(TestCaseName) \
  gtest_registered_test_names_##TestCaseName##_

//类型参数化测试宏中定义的变量是
//与通常一样，这些宏在.h文件中使用，可以
//包含在连接在一起的多个翻译单元中。
# define TYPED_TEST_CASE_P(CaseName) \
  static ::testing::internal::TypedTestCasePState \
      GTEST_TYPED_TEST_CASE_P_STATE_(CaseName)

# define TYPED_TEST_P(CaseName, TestName) \
  namespace GTEST_CASE_NAMESPACE_(CaseName) { \
  template <typename gtest_TypeParam_> \
  class TestName : public CaseName<gtest_TypeParam_> { \
   private: \
    typedef CaseName<gtest_TypeParam_> TestFixture; \
    typedef gtest_TypeParam_ TypeParam; \
    virtual void TestBody(); \
  }; \
  static bool gtest_##TestName##_defined_ GTEST_ATTRIBUTE_UNUSED_ = \
      GTEST_TYPED_TEST_CASE_P_STATE_(CaseName).AddTestName(\
          __FILE__, __LINE__, #CaseName, #TestName); \
  } \
  template <typename gtest_TypeParam_> \
  void GTEST_CASE_NAMESPACE_(CaseName)::TestName<gtest_TypeParam_>::TestBody()

# define REGISTER_TYPED_TEST_CASE_P(CaseName, ...) \
  namespace GTEST_CASE_NAMESPACE_(CaseName) { \
  typedef ::testing::internal::Templates<__VA_ARGS__>::type gtest_AllTests_; \
  } \
  static const char* const GTEST_REGISTERED_TEST_NAMES_(CaseName) = \
      GTEST_TYPED_TEST_CASE_P_STATE_(CaseName).VerifyRegisteredTestNames(\
          __FILE__, __LINE__, #__VA_ARGS__)

//下面的“types”模板参数周围必须有空格
//因为有些编译器在传递模板时可能会阻塞“>>”
//实例（例如，types<int>）
# define INSTANTIATE_TYPED_TEST_CASE_P(Prefix, CaseName, Types) \
  bool gtest_##Prefix##_##CaseName GTEST_ATTRIBUTE_UNUSED_ = \
      ::testing::internal::TypeParameterizedTestCase<CaseName, \
          GTEST_CASE_NAMESPACE_(CaseName)::gtest_AllTests_, \
          ::testing::internal::TypeList< Types >::type>::Register(\
              #Prefix, #CaseName, GTEST_REGISTERED_TEST_NAMES_(CaseName))

#endif  //GTEST_有_型_测试_p

#endif  //gtest_包括gtest_gtest_type_test_h_

//根据平台的不同，可以使用不同的字符串类。
//在Linux上，除了：：std:：string之外，Google还利用
//class:：string，它与：：std:：string具有相同的接口，但是
//具有不同的实现。
//
//您可以将gtest-has-global-string定义为1以指示
//：：string可用，并且是：：std：：string的不同类型，或者
//将其定义为0表示其他情况。
//
//如果：：std：：string和：：string是平台上的同一类
//由于存在别名，您应该将gtest_has_global_string定义为0。
//
//如果不定义gtest_has_global_string，则定义为
//启发式地

namespace testing {

//声明标志。

//此标志临时启用禁用的测试。
GTEST_DECLARE_bool_(also_run_disabled_tests);

//此标志使调试器出现断言失败。
GTEST_DECLARE_bool_(break_on_failure);

//此标志控制Google测试是否捕获所有测试引发的异常
//并记录为失败。
GTEST_DECLARE_bool_(catch_exceptions);

//此标志允许在终端输出中使用颜色。可用值为
//“是”启用颜色，“否”（禁用颜色）或“自动”（默认）
//让谷歌测试决定。
GTEST_DECLARE_string_(color);

//此标志使用全局模式设置要按名称选择的筛选器
//要运行的测试。如果没有给出过滤器，则执行所有测试。
GTEST_DECLARE_string_(filter);

//此标志使Google测试列出测试。未列出任何测试
//如果提供了标志，则实际运行。
GTEST_DECLARE_bool_(list_tests);

//此标志控制Google测试是否向文件发出详细的XML报告
//除了正常的文本输出。
GTEST_DECLARE_string_(output);

//此标志控制Google测试是否打印每个
//测试。
GTEST_DECLARE_bool_(print_time);

//此标志指定随机数种子。
GTEST_DECLARE_int32_(random_seed);

//此标志设置重复测试的次数。默认值
//是1。如果值为-1，则测试将永远重复。
GTEST_DECLARE_int32_(repeat);

//此标志控制Google测试是否包括Google内部测试
//在故障堆栈跟踪中堆栈帧。
GTEST_DECLARE_bool_(show_internal_stack_frames);

//当指定此标志时，测试的顺序将在每次迭代中随机化。
GTEST_DECLARE_bool_(shuffle);

//此标志指定要
//在故障消息中打印。
GTEST_DECLARE_int32_(stack_trace_depth);

//当指定此标志时，失败的断言将引发
//如果启用了异常，或使用
//否则为非零代码。
GTEST_DECLARE_bool_(throw_on_failure);

//当此标志用“host:port”字符串设置时，支持
//平台测试结果将流式传输到上的指定端口
//指定的主机。
GTEST_DECLARE_string_(stream_result_to);

//有效堆栈跟踪深度的上限。
const int kMaxStackTraceDepth = 100;

namespace internal {

class AssertHelper;
class DefaultGlobalTestPartResultReporter;
class ExecDeathTest;
class NoExecDeathTest;
class FinalSuccessChecker;
class GTestFlagSaver;
class StreamingListenerTest;
class TestResultAccessor;
class TestEventListenersAccessor;
class TestEventRepeater;
class UnitTestRecordPropertyTestHelper;
class WindowsDeathTest;
class UnitTestImpl* GetUnitTestImpl();
void ReportFailureInUnknownLocation(TestPartResult::Type result_type,
                                    const std::string& message);

}  //命名空间内部

//其中一些类的友元关系是循环的。
//如果不转发声明，编译器可能会混淆类
//在作用域上具有相同命名类的友谊子句中。
class Test;
class TestCase;
class TestInfo;
class UnitTest;

//用于指示断言是否成功的类。什么时候？
//断言未成功，断言结果对象
//记住一条描述失败方式的非空消息。
//
//要创建此类的实例，请使用工厂函数之一
//（AssessionSuccess（）和AssessionFailure（））。
//
//此类有两个用途：
//1。定义要与布尔测试断言一起使用的谓词函数
//期望真/期望假及其断言对应物
//2。定义谓词格式函数
//用于谓词断言（assert_pred_format*等）。
//
//例如，如果定义iseven谓词：
//
//测试：：断言结果ISeven（int n）
//如果（（n%2）==0）
//返回testing:：assertionsuccess（）；
//其他的
//返回testing:：assertionfailure（）<<n<“is odd”；
//}
//
//那么失败的期望值期望为真（iseven（fib（5）））
//将打印消息
//
//值：iseven（fib（5））
//实际：假（5为奇数）
//期望：真
//
//而不是更不透明
//
//值：iseven（fib（5））
//实际：假
//期望：真
//
//如果iseven是一个简单的布尔谓词。
//
//如果您希望重用谓词并希望支持信息性的
//Expect_false和Assert_false中的消息（出现否定断言
//在我们的测试中，大约是阳性的一半），为
//成功和失败案例：
//
//测试：：断言结果ISeven（int n）
//如果（（n%2）==0）
//返回testing:：assertionsuccess（）<<n<“is even”；
//其他的
//返回testing:：assertionfailure（）<<n<“is odd”；
//}
//
//然后将打印一个expect_false（iseven（fib（6））语句
//
//值：iseven（fib（6））
//实际值：真（8为偶数）
//预期：假
//
//注意：支持负布尔断言的谓词减少了
//积极的表现，所以小心不要在测试中使用它们。
//有很多（数万）的正布尔断言。
//
//要将该类与Expect_Pred_格式断言一起使用，例如：
//
////验证foo（）是否返回偶数。
//预期格式1（iseven，foo（））；
//
//您需要定义：
//
//测试：：断言结果ISeven（const char*expr，int n）
//如果（（n%2）==0）
//返回testing:：assertionsuccess（）；
//其他的
//返回测试：：断言失败（）。
//<“expected：”<<expr<“is even \n actual:it's“<<n；
//}
//
//如果foo（）返回5，您将看到以下消息：
//
//应输入：foo（）为偶数
//实际：5
//
class GTEST_API_ AssertionResult {
 public:
//复制构造函数。
//在Expect_true/false（断言_结果）中使用。
  AssertionResult(const AssertionResult& other);

  /*st_disable_msc_warnings_push_u（4800/*强制值为bool*/）

  //在expect_true/false（bool_表达式）中使用。
  / /
  //t必须在上下文中可转换为bool。
  / /
  //第二个参数防止在以下情况下考虑此重载：
  //参数可以隐式转换为断言结果。在那种情况下
  //我们希望使用AssertionResult的复制构造函数。
  模板<typename t>
  显式断言结果（
      成功与成功，
      类型名内部：：enableif<
          ！内部：：implicitlyconvertible<t，断言结果>：：值>：：类型*
          /*使能*/ = NULL)

      : success_(success) {}

  GTEST_DISABLE_MSC_WARNINGS_POP_()

//分配运算符。
  AssertionResult& operator=(AssertionResult other) {
    swap(other);
    return *this;
  }

//如果断言成功，则返回true。
operator bool() const { return success_; }  //诺林

//返回断言的否定。与expect/assert_false一起使用。
  AssertionResult operator!() const;

//返回流到此断言结果中的文本。测试断言
//当它们失败时使用它（即谓词的结果与
//断言的期望）。当没有任何东西流入
//对象，返回空字符串。
  const char* message() const {
    return message_.get() != NULL ?  message_->c_str() : "";
  }
//TODO（vladl@google.com）：在确保没有客户机使用它之后删除它。
//已弃用；请改用message（）。
  const char* failure_message() const { return message(); }

//将自定义失败消息流到此对象中。
  template <typename T> AssertionResult& operator<<(const T& value) {
    AppendMessage(Message() << value);
    return *this;
  }

//允许流式处理基本输出操作器，如endl或flush-into
//这个对象。
  AssertionResult& operator<<(
      ::std::ostream& (*basic_manipulator)(::std::ostream& stream)) {
    AppendMessage(Message() << basic_manipulator);
    return *this;
  }

 private:
//将消息的内容附加到消息\。
  void AppendMessage(const Message& a_message) {
    if (message_.get() == NULL)
      message_.reset(new ::std::string);
    message_->append(a_message.GetString().c_str());
  }

//将此断言结果的内容与其他内容交换。
  void swap(AssertionResult& other);

//存储断言谓词的结果。
  bool success_;
//存储描述条件的消息，以防期望
//构造不满足谓词的结果。
//通过指针引用以避免占用太多堆栈帧空间
//测试断言。
  internal::scoped_ptr< ::std::string> message_;
};

//生成成功的断言结果。
GTEST_API_ AssertionResult AssertionSuccess();

//生成失败的断言结果。
GTEST_API_ AssertionResult AssertionFailure();

//使用给定的失败消息生成失败的断言结果。
//已弃用；请使用assertionfailure（）<<msg。
GTEST_API_ AssertionResult AssertionFailure(const Message& msg);

//所有测试都继承自的抽象类。
//
//在Google测试中，一个单元测试程序包含一个或多个测试用例，并且
//每个测试用例包含一个或多个测试。
//
//使用测试宏定义测试时，不需要
//显式地从测试派生-测试宏自动执行
//这是给你的。
//
//从测试派生的唯一时间是定义测试夹具时
//用于测试。例如：
//
//类footerst:公共测试：：测试
//受保护的：
//void setup（）重写…}
//void TearDown（）重写…}
//…
//}；
//
//测试_f（最底部，条形）…}
//测试（footerst，baz）……}
//
//测试不可复制。
class GTEST_API_ Test {
 public:
  friend class TestInfo;

//定义指向设置和分解函数的指针的类型
//一个测试用例。
  typedef internal::SetUpTestCaseFunc SetUpTestCaseFunc;
  typedef internal::TearDownTestCaseFunc TearDownTestCaseFunc;

//委托人是虚拟的，因为我们打算从测试中继承。
  virtual ~Test();

//设置此测试用例中所有测试共享的内容。
//
//在运行第一个
//在测试用例foo中测试。因此，子类可以定义自己的
//SETUPTESTCASE（）方法来隐藏在super中定义的
//班级。
  static void SetUpTestCase() {}

//撕下这个测试用例中所有测试共享的东西。
//
//在运行最后一个测试之后，Google测试将调用foo:：teardowntestcase（）。
//在测试用例foo中测试。因此，子类可以定义自己的
//teardowntestcase（）方法来隐藏在super中定义的
//班级。
  static void TearDownTestCase() {}

//如果当前测试有致命错误，则返回true。
  static bool HasFatalFailure();

//返回true如果当前测试有非致命错误。
  static bool HasNonfatalFailure();

//如果当前测试具有（致命或
//非致命）故障。
  static bool HasFailure() { return HasFatalFailure() || HasNonfatalFailure(); }

//记录当前测试、测试用例或整个测试的属性
//当在
//测试用例。只记住给定键的最后一个值。这些
//是公共静态的，因此可以从
//不是测试夹具的成员。在
//测试的寿命（从其构造函数开始到
//当其析构函数完成时）将以XML形式输出为
//<testcase>元素。从夹具记录的属性
//SETUPTESTCASE或TEARDOWNTESTCASE作为
//对应的<testsuite>元素。在中调用recordproperty
//全局上下文（在调用run-all-u测试之前或之后
//在Google注册的环境对象的设置/拆卸方法
//test）将作为<testsuites>元素的属性输出。
  static void RecordProperty(const std::string& key, const std::string& value);
  static void RecordProperty(const std::string& key, int value);

 protected:
//创建测试对象。
  Test();

//设置测试夹具。
  virtual void SetUp();

//拆下测试夹具。
  virtual void TearDown();

 private:
//返回true iff当前测试的fixture类与
//当前测试用例中的第一个测试。
  static bool HasSameFixtureClass();

//在设置测试夹具后运行测试。
//
//子类必须实现这个来定义测试逻辑。
//
//不要在用户程序中直接重写此函数。
//相反，使用test或test_f宏。
  virtual void TestBody() = 0;

//设置、执行和分解测试。
  void Run();

//删除自我。我们故意为此取了一个不寻常的名字
//内部方法，以避免与用户测试中使用的名称冲突。
  void DeleteSelf_() { delete this; }

//使用gtestflagsaver保存和恢复所有Google测试标志。
  const internal::GTestFlagSaver* const gtest_flag_saver_;

//通常用户将setup（）拼写为setup（）并花费很长时间
//想知道为什么谷歌测试从未调用它。声明
//以下方法仅用于在
//编译时间：
//
//-返回类型被故意选择为非空，因此
//如果在用户的
//测试夹具。
//
//-此方法是私有的，因此它将是另一个编译器错误
//如果方法是从用户的测试夹具调用的。
//
//不要覆盖此功能。
//
//如果看到有关重写以下函数的错误，或者
//关于它是私有的，您将SETUP（）拼写错误为SETUP（）。
  struct Setup_should_be_spelled_SetUp {};
  virtual Setup_should_be_spelled_SetUp* Setup() { return NULL; }

//我们不允许复制测试。
  GTEST_DISALLOW_COPY_AND_ASSIGN_(Test);
};

typedef internal::TimeInMillis TimeInMillis;

//表示用户指定的测试属性的可复制对象，可以
//作为键/值字符串对输出。
//
//不要从testproperty继承，因为它的析构函数不是虚拟的。
class TestProperty {
 public:
//Cortestproperty没有默认构造函数。
//始终使用此构造函数（带参数）创建
//TestProperty对象。
  TestProperty(const std::string& a_key, const std::string& a_value) :
    key_(a_key), value_(a_value) {
  }

//获取用户提供的密钥。
  const char* key() const {
    return key_.c_str();
  }

//获取用户提供的值。
  const char* value() const {
    return value_.c_str();
  }

//设置新值，覆盖构造函数中提供的值。
  void SetValue(const std::string& new_value) {
    value_ = new_value;
  }

 private:
//用户提供的密钥。
  std::string key_;
//用户提供的值。
  std::string value_;
};

//单个测试的结果。这包括一个列表
//testpartresults，一个testproperties列表，一个多少的计数
//测试中有死亡测试，运行需要多少时间
//测试。
//
//测试结果不可复制。
class GTEST_API_ TestResult {
 public:
//创建空的测试结果。
  TestResult();

//德托。不要从TestResult继承。
  ~TestResult();

//获取所有测试部件的编号。这是数字的和
//成功的测试部件和失败的测试部件的数量。
  int total_part_count() const;

//返回测试属性的数目。
  int test_property_count() const;

//如果测试通过，则返回true（即没有测试部件失败）。
  bool Passed() const { return !Failed(); }

//如果测试失败，则返回true。
  bool Failed() const;

//如果测试最终失败，则返回true。
  bool HasFatalFailure() const;

//返回true如果测试有非致命错误。
  bool HasNonfatalFailure() const;

//返回已用时间（毫秒）。
  TimeInMillis elapsed_time() const { return elapsed_time_; }

//返回所有结果中的第i个测试部分结果。我可以测距
//从0到测试_属性_count（）-1。如果我不在这个范围内，中止
//程序。
  const TestPartResult& GetTestPartResult(int i) const;

//返回第i个测试属性。我可以从0到
//测试_属性_count（）-1。如果我不在这个范围内，中止
//程序。
  const TestProperty& GetTestProperty(int i) const;

 private:
  friend class TestInfo;
  friend class TestCase;
  friend class UnitTest;
  friend class internal::DefaultGlobalTestPartResultReporter;
  friend class internal::ExecDeathTest;
  friend class internal::TestResultAccessor;
  friend class internal::UnitTestImpl;
  friend class internal::WindowsDeathTest;

//获取testpartresults的矢量。
  const std::vector<TestPartResult>& test_part_results() const {
    return test_part_results_;
  }

//获取testproperties的向量。
  const std::vector<TestProperty>& test_properties() const {
    return test_properties_;
  }

//设置经过的时间。
  void set_elapsed_time(TimeInMillis elapsed) { elapsed_time_ = elapsed; }

//向列表中添加测试属性。属性已验证，可以添加
//无效时的非致命故障（例如，如果它与保留的冲突
//关键名称）。如果已经为同一个键记录了属性，则
//将更新值，而不是为同一个值存储多个值
//关键。xml_元素指定属性所针对的元素
//记录并用于验证。
  void RecordProperty(const std::string& xml_element,
                      const TestProperty& test_property);

//如果密钥是google test的保留属性，则添加失败
//测试用例标记。如果属性有效，则返回true。
//TODO（RUSSR）：验证属性名是合法的，并且是人类可读的。
  static bool ValidateTestProperty(const std::string& xml_element,
                                   const TestProperty& test_property);

//将测试部件结果添加到列表中。
  void AddTestPartResult(const TestPartResult& test_part_result);

//返回死亡测试计数。
  int death_test_count() const { return death_test_count_; }

//增加死亡测试计数，返回新计数。
  int increment_death_test_count() { return ++death_test_count_; }

//清除测试部件结果。
  void ClearTestPartResults();

//清除对象。
  void Clear();

//保护财产载体和所有权的可变状态
//属性，其值可以更新。
  internal::Mutex test_properites_mutex_;

//测试部分结果的向量
  std::vector<TestPartResult> test_part_results_;
//测试属性的向量
  std::vector<TestProperty> test_properties_;
//运行死亡测试计数。
  int death_test_count_;
//已用时间（毫秒）。
  TimeInMillis elapsed_time_;

//我们不允许复制测试结果。
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestResult);
};  //类测试结果

//testinfo对象存储有关测试的以下信息：
//
//测试用例名
//测试名称
//是否应运行测试
//在调用时创建测试对象的函数指针
//试验结果
//
//testinfo的构造函数向unittest注册自己。
//使run_All_tests（）宏知道要执行的测试
//跑。
class GTEST_API_ TestInfo {
 public:
//销毁testinfo对象。这个函数不是虚拟的，所以
//不要从testinfo继承。
  ~TestInfo();

//返回测试用例名称。
  const char* test_case_name() const { return test_case_name_.c_str(); }

//返回测试名称。
  const char* name() const { return name_.c_str(); }

//返回参数类型的名称，如果不是类型化的，则返回空值
//或类型参数化测试。
  const char* type_param() const {
    if (type_param_.get() != NULL)
      return type_param_->c_str();
    return NULL;
  }

//返回值参数的文本表示形式，如果是
//不是值参数化测试。
  const char* value_param() const {
    if (value_param_.get() != NULL)
      return value_param_->c_str();
    return NULL;
  }

//如果此测试应该运行，即如果测试不是
//已禁用（或已禁用，但同时运行已禁用的测试标志
//和它的全名与用户指定的筛选器匹配。
//
//谷歌测试允许用户用全名过滤测试。
//测试用例foo中测试条的全名定义为
//“Fo.bar”。仅运行与筛选器匹配的测试。
//
//过滤器是以冒号分隔的glob（非regex）模式列表，
//可选后跟一个“-”和一个冒号分隔的列表
//负模式（要排除的测试）。如果测试
//匹配其中一个正模式，但不匹配
//消极的模式。
//
//例如，*a*：foo.*是一个与任何字符串匹配的过滤器
//包含字符“a”或以“foo”开头。
  bool should_run() const { return should_run_; }

//返回true iff此测试将出现在XML报告中。
  bool is_reportable() const {
//目前，XML报告包括所有与过滤器匹配的测试。
//将来，我们可能会修正由于
//削刃。
    return matches_filter_;
  }

//返回测试结果。
  const TestResult* result() const { return &result_; }

 private:
#if GTEST_HAS_DEATH_TEST
  friend class internal::DefaultDeathTestFactory;
#endif  //GTEST有死亡测试
  friend class Test;
  friend class TestCase;
  friend class internal::UnitTestImpl;
  friend class internal::StreamingListenerTest;
  friend TestInfo* internal::MakeAndRegisterTestInfo(
      const char* test_case_name,
      const char* name,
      const char* type_param,
      const char* value_param,
      internal::TypeId fixture_class_id,
      Test::SetUpTestCaseFunc set_up_tc,
      Test::TearDownTestCaseFunc tear_down_tc,
      internal::TestFactoryBase* factory);

//构造一个testinfo对象。新构造的实例假定
//工厂对象的所有权。
  TestInfo(const std::string& test_case_name,
           const std::string& name,
const char* a_type_param,   //如果不是类型参数化测试，则为空
const char* a_value_param,  //如果不是值参数化测试，则为空
           internal::TypeId fixture_class_id,
           internal::TestFactoryBase* factory);

//增加此测试中遇到的死亡测试的数量，以便
//远。
  int increment_death_test_count() {
    return result_.increment_death_test_count();
  }

//创建测试对象，运行它，记录它的结果，然后
//删除它。
  void Run();

  static void ClearTestResult(TestInfo* test_info) {
    test_info->result_.Clear();
  }

//这些字段是测试的不变属性。
const std::string test_case_name_;     //测试用例名
const std::string name_;               //测试名称
//参数类型的名称，如果不是类型或
//类型参数化测试。
  const internal::scoped_ptr<const ::std::string> type_param_;
//值参数的文本表示形式，如果不是
//值参数化测试。
  const internal::scoped_ptr<const ::std::string> value_param_;
const internal::TypeId fixture_class_id_;   //测试夹具类的ID
bool should_run_;                 //真的如果这个测试应该运行
bool is_disabled_;                //如果此测试被禁用，则为真
bool matches_filter_;             //如果此测试与
//用户指定的筛选器。
internal::TestFactoryBase* const factory_;  //创建的工厂
//测试对象

//此字段是可变的，需要在运行
//第二次测试。
  TestResult result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestInfo);
};

//一个测试用例，由一个睾丸信息向量组成。
//
//测试用例不可复制。
class GTEST_API_ TestCase {
 public:
//创建具有给定名称的测试用例。
//
//测试用例没有默认的构造函数。总是使用这个
//用于创建测试用例对象的构造函数。
//
//争论：
//
//名称：测试用例的名称
//_type_param：测试类型参数的名称，如果
//这不是类型参数化测试。
//设置\u tc:指向设置测试用例的函数的指针
//分解：指向分解测试用例的函数的指针
  TestCase(const char* name, const char* a_type_param,
           Test::SetUpTestCaseFunc set_up_tc,
           Test::TearDownTestCaseFunc tear_down_tc);

//测试用例的析构函数。
  virtual ~TestCase();

//获取测试用例的名称。
  const char* name() const { return name_.c_str(); }

//返回参数类型的名称，如果不是
//类型参数化测试用例。
  const char* type_param() const {
    if (type_param_.get() != NULL)
      return type_param_->c_str();
    return NULL;
  }

//如果应该运行此测试用例中的任何测试，则返回true。
  bool should_run() const { return should_run_; }

//获取此测试用例中成功的测试数。
  int successful_test_count() const;

//获取此测试用例中失败的测试数。
  int failed_test_count() const;

//获取将在XML报告中报告的禁用测试数。
  int reportable_disabled_test_count() const;

//获取此测试用例中禁用的测试数。
  int disabled_test_count() const;

//获取要在XML报表中打印的测试数。
  int reportable_test_count() const;

//获取此测试用例中应该运行的测试数。
  int test_to_run_count() const;

//获取此测试用例中所有测试的数目。
  int total_test_count() const;

//如果测试用例通过，则返回true。
  bool Passed() const { return !Failed(); }

//如果测试用例失败，则返回true。
  bool Failed() const { return failed_test_count() > 0; }

//返回已用时间（毫秒）。
  TimeInMillis elapsed_time() const { return elapsed_time_; }

//返回所有测试中的第i个测试。我可以从0到
//总测试计数（）-1。如果我不在该范围内，则返回空值。
  const TestInfo* GetTestInfo(int i) const;

//返回保存期间记录的测试属性的测试结果
//执行setuptestcase和teardowntestcase。
  const TestResult& ad_hoc_test_result() const { return ad_hoc_test_result_; }

 private:
  friend class Test;
  friend class internal::UnitTestImpl;

//获取此testcase中testinfos的（可变）矢量。
  std::vector<TestInfo*>& test_info_list() { return test_info_list_; }

//获取此testcase中testinfos的（不可变）向量。
  const std::vector<TestInfo*>& test_info_list() const {
    return test_info_list_;
  }

//返回所有测试中的第i个测试。我可以从0到
//总测试计数（）-1。如果我不在该范围内，则返回空值。
  TestInfo* GetMutableTestInfo(int i);

//设置应运行的成员。
  void set_should_run(bool should) { should_run_ = should; }

//将testinfo添加到此测试用例。将删除测试信息
//测试用例对象的销毁。
  void AddTestInfo(TestInfo * test_info);

//清除此测试用例中所有测试的结果。
  void ClearResult();

//清除给定测试用例中所有测试的结果。
  static void ClearTestCaseResult(TestCase* test_case) {
    test_case->ClearResult();
  }

//运行此测试用例中的每个测试。
  void Run();

//为此测试用例运行SetupTestCase（）。需要这个包装纸
//用于捕获从SetupTestCase（）引发的异常。
  void RunSetUpTestCase() { (*set_up_tc_)(); }

//为此测试用例运行teardowntestcase（）。这个包装是
//需要捕获从TearDownTestCase（）引发的异常。
  void RunTearDownTestCase() { (*tear_down_tc_)(); }

//返回通过的真iff测试。
  static bool TestPassed(const TestInfo* test_info) {
    return test_info->should_run() && test_info->result()->Passed();
  }

//返回真正的iff测试失败。
  static bool TestFailed(const TestInfo* test_info) {
    return test_info->should_run() && test_info->result()->Failed();
  }

//返回true如果测试被禁用并将在XML中报告
//报告。
  static bool TestReportableDisabled(const TestInfo* test_info) {
    return test_info->is_reportable() && test_info->is_disabled_;
  }

//返回禁用的真iff测试。
  static bool TestDisabled(const TestInfo* test_info) {
    return test_info->is_disabled_;
  }

//返回true iff此测试将出现在XML报告中。
  static bool TestReportable(const TestInfo* test_info) {
    return test_info->is_reportable();
  }

//如果给定的测试应该运行，则返回true。
  static bool ShouldRunTest(const TestInfo* test_info) {
    return test_info->should_run();
  }

//在此测试用例中无序处理测试。
  void ShuffleTests(internal::Random* random);

//将测试顺序恢复到第一次随机播放之前。
  void UnshuffleTests();

//测试用例的名称。
  std::string name_;
//参数类型的名称，如果不是类型或
//类型参数化测试。
  const internal::scoped_ptr<const ::std::string> type_param_;
//按原始顺序排列的睾丸向量。它拥有
//向量中的元素。
  std::vector<TestInfo*> test_info_list_;
//为测试列表提供一个间接级别，以便
//洗牌和恢复测试顺序。这里面的第i个元素
//向量是无序测试列表中第i个测试的索引。
  std::vector<int> test_indices_;
//指向设置测试用例的函数的指针。
  Test::SetUpTestCaseFunc set_up_tc_;
//指向分解测试用例的函数的指针。
  Test::TearDownTestCaseFunc tear_down_tc_;
//如果此测试用例中的任何测试都应该运行，则返回true。
  bool should_run_;
//已用时间（毫秒）。
  TimeInMillis elapsed_time_;
//保留执行SetupTestCase期间记录的测试属性，以及
//拆下外壳。
  TestResult ad_hoc_test_result_;

//我们不允许复制测试用例。
  GTEST_DISALLOW_COPY_AND_ASSIGN_(TestCase);
};

//环境对象能够设置和分解
//环境。您应该将其子类化，以定义自己的
//环境（s）。
//
//环境对象在虚拟环境中进行设置和分解
//方法setup（）和teardown（）而不是构造函数和
//析构函数，如：
//
//1。你不能安全地从析构函数中抛出。这是个问题
//在某些情况下，在启用异常的情况下使用Google测试，并且
//我们可能希望使用异常实现assert_*
//可用。
//
//析构函数。
class Environment {
 public:
//由于我们需要对环境进行子类化，所以该职责是虚拟的。
  virtual ~Environment() {}

//重写此项以定义如何设置环境。
  virtual void SetUp() {}

//重写此项以定义如何删除环境。
  virtual void TearDown() {}
 private:
//如果看到有关重写以下函数的错误，或者
//关于它是私有的，您将SETUP（）拼写错误为SETUP（）。
  struct Setup_should_be_spelled_SetUp {};
  virtual Setup_should_be_spelled_SetUp* Setup() { return NULL; }
};

//用于跟踪测试执行的接口。方法组织在
//激发相应事件的顺序。
class TestEventListener {
 public:
  virtual ~TestEventListener() {}

//在任何测试活动开始前激发。
  virtual void OnTestProgramStart(const UnitTest& unit_test) = 0;

//在每次测试迭代开始前激发。可能不止
//如果设置了gtest_标志（repeat），则执行一次迭代。迭代就是迭代
//索引，从0开始。
  virtual void OnTestIterationStart(const UnitTest& unit_test,
                                    int iteration) = 0;

//在为每个测试迭代启动环境设置之前激发。
  virtual void OnEnvironmentsSetUpStart(const UnitTest& unit_test) = 0;

//在为每个测试迭代设置环境后激发。
  virtual void OnEnvironmentsSetUpEnd(const UnitTest& unit_test) = 0;

//在测试用例开始之前激发。
  virtual void OnTestCaseStart(const TestCase& test_case) = 0;

//在测试开始前激发。
  virtual void OnTestStart(const TestInfo& test_info) = 0;

//在失败的断言或成功（）调用后激发。
  virtual void OnTestPartResult(const TestPartResult& test_part_result) = 0;

//在测试结束后激发。
  virtual void OnTestEnd(const TestInfo& test_info) = 0;

//在测试用例结束后激发。
  virtual void OnTestCaseEnd(const TestCase& test_case) = 0;

//在开始每次测试迭代的环境崩溃之前激发。
  virtual void OnEnvironmentsTearDownStart(const UnitTest& unit_test) = 0;

//在每次测试迭代结束时，在环境崩溃后激发。
  virtual void OnEnvironmentsTearDownEnd(const UnitTest& unit_test) = 0;

//在每次测试迭代完成后激发。
  virtual void OnTestIterationEnd(const UnitTest& unit_test,
                                  int iteration) = 0;

//在所有测试活动结束后激发。
  virtual void OnTestProgramEnd(const UnitTest& unit_test) = 0;
};

//为只需要覆盖一个或两个的用户提供的便利类
//方法和不关心签名的可能更改
//它们重写的方法在生成期间不会被捕获。为了
//有关每个方法的注释，请参见TestEventListener的定义
//上面。
class EmptyTestEventListener : public TestEventListener {
 public:
  /*tual void ontestprogramstart（const unit test&/*单元测试*/）
  虚拟void ontestitutionStart（const unittest&/*unit-tes*/,

                                    /*/*迭代*/）
  虚拟void OnEnvironmentsSetupStart（const unittest&/*Unit-tes*/) {}

  /*环境的实际空隙设置（const unit test&/*单元测试*/）
  虚拟void ontestcaseStart（const testcase&/*test\u cas*/) {}

  /*tual void onteststart（const test info&/*测试信息*/）
  虚拟void contestpartresult（const testpartresult&/*测试部分结果*/) {}

  /*tual void ontested（const test info&/*测试信息*/）
  虚拟void ontestcaseEnd（const testcase&/*test_cas*/) {}

  /*环境温度下降开始时的实际空隙（const unit test&/*Unit_test*/）
  环境下的虚拟空间（const unittest和/*unit-tes*/) {}

  /*结构端的实际空隙（const unit test&/*单元测试*/，
                                  INT/*迭代*/) {}

  /*tual void ontestprogrammend（const unit test&/*单元测试*/）
}；

//TestEventListeners允许用户添加监听器来跟踪Google测试中的事件。
类gtest_api_uuTestEventListeners_
 公众：
  TestEventListeners（）；
  ~testeventListeners（）；

  //将事件侦听器追加到列表末尾。谷歌测试假设
  //侦听器的所有权（即当
  //测试程序完成）。
  void append（testeventlistener*listener）；

  //从列表中删除给定的事件侦听器并返回它。然后
  //成为调用方删除侦听器的责任。退换商品
  //如果列表中找不到侦听器，则为空。
  testeventlistener*版本（testeventlistener*listener）；

  //返回负责默认控制台的标准侦听器
  //输出。可以从侦听器列表中删除以默认关闭
  //控制台输出。请注意，从侦听器列表中删除此对象
  //使用release将其所有权转移给调用方，并使其
  //函数下次返回空值。
  testeventListener*默认_result_printer（）const_
    返回默认的“结果打印机”；
  }

  //返回负责默认XML输出的标准侦听器
  //由--gtest_output=xml标志控制。可以从
  //侦听器按要关闭默认XML输出的用户列出
  //由该标志控制，并用自定义标志替换它。注意
  //使用release从侦听器列表中移除此对象会传输其
  //对调用方的所有权，并使此函数在下一个函数中返回空值
  /时间。
  testeventListener*默认的_xml_generator（）const_
    返回默认的“xml”生成器；
  }

 私人：
  朋友类测试用例；
  朋友类测试信息；
  友元类内部：：DefaultGlobalTestPartResultReporter；
  friend类内部：：noexecdeathtest；
  友元类内部：：TestEventListenersAccessor；
  友元类内部：：UnitTestImpl；

  //返回将TestEventListener事件广播给所有人的转发器
  /订户。
  testeEventListener*repeater（）；

  //将默认的“结果”打印机属性设置为提供的侦听器。
  //侦听器也被添加到侦听器列表和上一个列表中
  //默认的_result_打印机将从中删除并删除。听众可以
  //也为空，在这种情况下不会将其添加到列表中。做
  //如果前一个和当前侦听器对象相同，则不执行任何操作。
  void setdefaultresultprinter（testeventlistener*listener）；

  //为提供的侦听器设置默认的_xml_generator属性。这个
  //Listener也被添加到Listener列表和上一个列表中
  //将从中删除并删除默认的_xml_生成器。听众可以
  //也为空，在这种情况下不会将其添加到列表中。做
  //如果前一个和当前侦听器对象相同，则不执行任何操作。
  void setdefaultxmlgenerator（testeventlistener*listener）；

  //控制中继器是否将事件转发到
  //列表中的侦听器。
  bool eventforwardingEnabled（）常量；
  void SuppressEventForwarding（）；

  //侦听器的实际列表。
  内部：：testeventrepeater*转发器\；
  //负责标准结果输出的侦听器。
  testeventlistener*默认结果打印机；
  //负责创建XML输出文件的侦听器。
  testeventlistener*默认\u xml \u生成器\；

  //我们不允许复制TestEventListeners。
  gtest不允许复制和分配（testeventlisteners）；
}；

//unittest由一个testcase向量组成。
/ /
//这是一个单例类。UnitTest的唯一实例是
//在首次调用UnitTest:：GetInstance（）时创建。这个
//从未删除实例。
/ /
//UnitTest不可复制。
/ /
//只要调用方法，这个类就是线程安全的
//根据他们的规格。
GTEST类API单元测试
 公众：
  //获取singleton unittest对象。第一次使用这种方法
  //被调用，构造并返回UnitTest对象。
  //连续调用将返回同一对象。
  静态UnitTest*GetInstance（）；

  //运行此UnitTest对象中的所有测试并打印结果。
  //如果成功则返回0，否则返回1。
  / /
  //只能从主线程调用此方法。
  / /
  //内部实现-不要在用户程序中使用。
  int run（）gtest必须使用结果

  //返回第一个test（）或test_f（）时的工作目录
  //已执行。UnitTest对象拥有该字符串。
  const char*原始工作目录（）const；

  //返回当前正在运行的测试的testcase对象，
  //如果没有运行测试，则为空。
  const test case*当前测试用例（）const
      gtest_lock_excluded_uu（互斥锁）；

  //返回当前正在运行的测试的testinfo对象，
  //如果没有运行测试，则为空。
  const test info*当前测试信息（）const
      gtest_lock_excluded_uu（互斥锁）；

  //返回当前测试运行开始时使用的随机种子。
  int random_seed（）常量；

如果gtest_有_参数_测试
  //返回用于跟踪
  //值参数化测试并实例化并注册它们。
  / /
  //内部实现-不要在用户程序中使用。
  内部：：参数化的测试用例注册表和参数化的测试用例注册表（）。
      gtest_lock_excluded_uu（互斥锁）；
endif//gtest_具有_参数_测试

  //获取成功的测试用例数。
  int successful_test_case_count（）const；

  //获取失败的测试用例数。
  int失败的\u test \u case \u count（）const；

  //获取所有测试用例的数目。
  int total_test_case_count（）常量；

  //获取包含至少一个测试的所有测试用例的数目
  //应该可以。
  int test_case_to_run_count（）const；

  //获取成功测试的数目。
  int successful_test_count（）const；

  //获取失败的测试数。
  int失败，test_count（）const；

  //获取将在XML报告中报告的禁用测试数。
  int reportable_disabled_test_count（）const；

  //获取禁用的测试数。
  int disabled_test_count（）const；

  //获取要在XML报表中打印的测试数。
  int reportable_test_count（）常量；

  //获取所有测试的数目。
  int total_test_count（）常量；

  //获取应运行的测试数。
  int test_to_run_count（）const；

  //获取测试程序的启动时间，从
  /UNIX时代。
  timeInMillis start_timestamp（）常量；

  //获取经过的时间（毫秒）。
  timeinmillis elapsed_time（）常量；

  //如果单元测试通过（即所有测试用例都通过），则返回true。
  bool passed（）常量；

  //如果单元测试失败（即某些测试用例失败），则返回true
  //或所有测试之外的某个内容失败）。
  bool failed（）常量；

  //获取所有测试用例中的第i个测试用例。我可以从0到
  //总的测试用例计数（）-1。如果我不在该范围内，则返回空值。
  const测试用例*gettestcase（int i）const；

  //返回包含有关测试失败和
  //在单个测试用例之外记录的属性。
  const testsult&ad_hoc_test_result（）const；

  //返回可用于跟踪事件的事件侦听器列表
  //在Google测试中。
  TestEventListeners&Listeners（）；

 私人：
  //注册并返回全局测试环境。当测试时
  //程序正在运行，所有全局测试环境都将在
  //它们的注册顺序。在程序中所有测试之后
  //已完成，所有全局测试环境将在
  //它们被注册的*反向*顺序。
  / /
  //UnitTest对象拥有给定环境的所有权。
  / /
  //只能从主线程调用此方法。
  环境*附录环境（环境*环境）；

  //将testpartresult添加到当前的testpresult对象。所有
  //google测试断言宏（例如assert_true、expect_eq等）
  //最后调用此函数来报告结果。用户代码
  //应该使用断言宏，而不是直接调用它。
  void addtestpartresult（testpartresult:：type result_type，
                         const char*文件名，
                         内线号，
                         const std：：字符串和消息，
                         const std:：string&os_stack_trace）
      gtest_lock_excluded_uu（互斥锁）；

  //从调用时向当前的testreult对象添加testproperty
  //在测试内部，调用时指向当前测试用例的特殊测试结果
  //从SETUPTESTCASE或TEARDOWNTESTCASE，或到全局属性集
  //在其他地方调用时。如果结果已包含具有
  //相同的键，该值将被更新。
  void recordproperty（const std:：string&key，const std:：string&value）；

  //获取所有测试用例中的第i个测试用例。我可以从0到
  //总的测试用例计数（）-1。如果我不在该范围内，则返回空值。
  测试用例*getmutabletestcase（int i）；

  //实现对象的访问器。
  内部：：UnitTestImpl*impl（）返回impl_
  const内部：：unittesimpl*impl（）const返回impl

  //这些类和函数是朋友，因为它们需要访问private
  //UnitTest的成员。
  朋友班考试；
  friend类内部：：assertHelper；
  友元类内部：：ScopedTrace；
  友元类内部：：streaminglistenertest；
  友元类内部：：UnitTestRecordPropertyTestHelper；
  朋友环境*addglobaltestenvironment（环境*env）；
  friend internal:：UnitTestImpl*内部：：GetUnitTestImpl（）；
  friend void internal:：reportFailureinUnknownLocation（）。
      testpartresult：：类型result_类型，
      const std：：字符串和消息）；

  //创建空的UnitTest。
  UNITESTEST（）；

  /或
  虚拟~UnitTest（）；

  //将作用域_trace（）定义的跟踪推送到每个线程上
  //Google测试跟踪堆栈。
  void pushgtesttrace（const内部：：traceinfo&trace）
      gtest_lock_excluded_uu（互斥锁）；

  //从每个线程的Google测试跟踪堆栈中弹出一个跟踪。
  void popgtesttrace（）。
      gtest_lock_excluded_uu（互斥锁）；

  //保护*impl_u中的可变状态。这是可变的，就像某些常量一样。
  //方法也需要锁定它。
  可变内部：：互斥互斥

  //不透明的实现对象。此字段从未更改过一次
  //对象已构造。这里我们不把它标为警察，因为
  //这样做将导致UnitTest的构造函数中出现警告。
  //impl中的可变状态受mutex保护。
  内部：：UnitTestImpl*impl_；

  //我们不允许复制UnitTest。
  gtest不允许复制和分配（unittest）；
}；

//一个方便的包装器，用于为测试添加环境
/程序。
/ /
//应该在调用运行之前调用此函数，可能在
//MIN（）。如果使用gtest_main，则需要在main（）之前调用它。
//开始生效。例如，可以定义全局
//变量如下：
/ /
//测试：：环境*常量foo_env=
//测试：：addglobaltestenvironment（new fooenvironment）；
/ /
//但是，我们强烈建议您编写自己的main（）和
//在那里调用addGlobalTestenvironment（），因为依赖于初始化
//的全局变量使代码更难读取，并可能导致
//在不同环境中注册多个环境时出现问题
//翻译单元和环境之间存在依赖关系
//（记住编译器不保证
//初始化来自不同翻译单元的全局变量）。
内联环境*addglobaltestenvironment（environment*env）
  返回unittest:：getInstance（）->addenvironment（env）；
}

//初始化Google测试。必须在呼叫之前调用此
//运行_all_tests（）。特别是，它为
//谷歌测试识别的标志。每当google测试标志
//可见，它从argv中移除，并且*argc递减。
/ /
//不返回任何值。相反，google测试标志变量是
/ /更新。
/ /
//第二次调用函数没有用户可见的效果。
gtest_api_uuvoid initgoogletest（int*argc，char**argv）；

//此重载版本可用于在中编译的Windows程序
//Unicode模式。
gtest_api_uuvoid initgoogletest（int*argc，wchar_t**argv）；

命名空间内部

//formatforcomparison<toprint，otheroperand>：：format（value）formats a
//作为比较断言的操作数的toprint类型的值
//（例如，断言_eq）。OtherOperand是中另一个操作数的类型
//比较，用于帮助确定
//设置值的格式。尤其是当值是C字符串时
//（char指针）而另一个操作数是stl字符串对象，我们
//希望将C字符串格式化为字符串，因为我们知道它是
//按值与字符串对象进行比较。如果值为char
//指针，但另一个操作数不是STL字符串对象，我们不
//知道指针是否应该指向以nul结尾的
//字符串，因此要将其作为指针打印以确保安全。
/ /
//内部实现-不要在用户程序中使用。

//默认情况。
template<typename-toprint，typename-otheroperand>
用于比较的类格式
 公众：
  静态：：std：：字符串格式（const-toprint&value）
    返回：：testing:：printToString（value）；
  }
}；

//数组。
template<typename-toprint，size_t n，typename-otheroperand>
class formatforcomparison<toprint[n]，otheroperand>
 公众：
  静态：：std：：字符串格式（const toprint*value）
    返回formatforcomparison<const-toprint*，otheroperand>：：format（value）；
  }
}；

//默认情况下，打印c字符串作为安全指针，我们不知道
//它们是否实际指向以nul结尾的字符串。

将gtest_impl_format_c_string_定义为_pointer_uu（chartype） \
  模板<typename otheroperand>>
  class formatforcomparison<chartype*，otheroperand>
   公开：
    static：：std：：字符串格式（chartype*value）\
      返回：：testing：：printToString（static_cast<const void*>（value））；\
    _
  }

gtest_impl_format_c_string_as_pointer_uu（char）；
gtest_impl_format_c_string_as_pointer_uu（const char）；
gtest_impl_format_c_string_as_pointer_uu（wchar_t）；
gtest_impl_format_c_string_as_pointer_uu（const wchar_t）；

undef gtest_impl_format_c_string_as_pointer_

//如果将C字符串与STL字符串对象进行比较，我们知道它的意思是
//指向以nul结尾的字符串，因此可以将其打印为字符串。

将gtest_impl_format_c_string_定义为_string_u（chartype，otherstringtype）
  模板<>\
  class formatforcomparison<chartype*，OtherStringType>\
   公开：
    static：：std：：字符串格式（chartype*value）\
      返回：：testing:：printtoString（value）；\
    _
  }

gtest_impl_format_c_string_as_string_u（char，：：std:：string）；
gtest_impl_format_c_string_as_string_u（const char，：：std：：string）；

如果gtest_具有_global_字符串
gtest_impl_format_c_string_as_string_u（char，：：string）；
gtest_impl_format_c_string_as_string_u（const char，：：string）；
第二节

如果GTEST诳U具有诳U Global诳String
gtest_impl_format_c_string_as_string_uu（wchar_t，：：wstring）；
gtest_impl_format_c_string_as_string_u（const wchar_t，：：wstring）；
第二节

如果GTEST_有_-std_-wstring
gtest_impl_format_c_string_as_string_uu（wchar_t，：：std:：wstring）；
gtest_impl_format_c_string_as_string_u（const wchar_t，：：std：：wstring）；
第二节

undef gtest_impl_format_c_string_as_string_

//设置比较断言的格式（例如断言eq、Expect-lt等）
//要在失败消息中使用的操作数。类型（但不是值）
//其他操作数可能会影响格式。这让我们
//将char*与另一个char*进行比较时，将其作为原始指针打印
//char*或void*，并在比较时将其打印为C字符串
//例如，针对std:：string对象。
/ /
//内部实现-不要在用户程序中使用。
template<typename t1，typename t2>
std：：比较失败消息的字符串格式（
    const t1&value，const t2&/*其他操作数*/) {

  return FormatForComparison<T1, T2>::Format(value);
}

//将错误生成代码与代码路径分开，以减少堆栈
//cmphelpereq的帧大小。这有助于减少一些消毒剂的开销
//在紧密循环中调用expect_u*
template <typename T1, typename T2>
AssertionResult CmpHelperEQFailure(const char* expected_expression,
                                   const char* actual_expression,
                                   const T1& expected, const T2& actual) {
  return EqFailure(expected_expression,
                   actual_expression,
                   FormatForComparisonFailureMessage(expected, actual),
                   FormatForComparisonFailureMessage(actual, expected),
                   false);
}

//_assert_expect_u eq的helper函数。
template <typename T1, typename T2>
AssertionResult CmpHelperEQ(const char* expected_expression,
                            const char* actual_expression,
                            const T1& expected,
                            const T2& actual) {
/*st_disable_msc_warnings_push_u（4389/*有符号/无符号不匹配*/）
  如果（预期=实际）
    返回assertionsuccess（）；
  }
gtest禁用msc警告

  返回cmphelpereqfailure（应为表达式，实际为表达式，应为，
                            实际）；
}

//对于此重载版本，我们允许使用匿名枚举
//在用gcc 4编译时，assert expect u eq中，作为匿名枚举
//可以隐式强制转换为biggestant。
gtest-api-assertionresult cmphelpereq（const char*应为表达式，
                                       const char*实际表达式，
                                       预计会有大问题，
                                       实际的重大影响）；

//用于assert expect u eq.模板参数的helper类
//lhs_is_null_literal is true iff the first变元to assert_eq（））
//是空指针文本。以下默认实现是
//因为lhs_为\u空\字面值为假。
template<bool lhs_is_null_literal>
EQhelper类
 公众：
  //此模板化版本适用于一般情况。
  template<typename t1，typename t2>
  静态断言结果比较（const char*应为\u表达式，
                                 const char*实际表达式，
                                 常量T1和预期值，
                                 常量t2和实际值）
    返回cmphelpereq（应为表达式，实际为表达式，应为，
                       实际）；
  }

  //对于此重载版本，我们允许使用匿名枚举
  //in assert expect u eq，当使用gcc 4编译时，作为匿名
  //枚举可以隐式强制转换为biggestent。
  / /
  //虽然它的主体看起来和上面的版本一样，但是我们
  //无法合并这两者，因为这会使匿名枚举不快乐。
  静态断言结果比较（const char*应为\u表达式，
                                 const char*实际表达式，
                                 预计会有大问题，
                                 重大实际）
    返回cmphelpereq（应为表达式，实际为表达式，应为，
                       实际）；
  }
}；

//当第一个参数断言_eq（）时使用此专门化
//是空指针文本，如空、假或0。
模板< >
类eqhelper<true>>
 公众：
  //我们定义了两个重载版本的compare（）。第一
  //当断言eq（）的第二个参数为
  //不是指针，例如assert_eq（0，anintFunction（））或
  //期望_eq（假，一个_bool）。
  template<typename t1，typename t2>
  静态断言结果比较（
      const char*应为表达式，
      const char*实际表达式，
      常量T1和预期值，
      常量t2和实际值，
      //如果t2
      //不是指针类型。我们需要这个，因为断言eq（空，我的指针）
      //展开以进行比较（“”，“”，null，my_ptr），这需要转换
      //与另一个重载中的secret*匹配，否则将使
      //此模板更匹配。
      typename enableif<！是“指针<t2>：：值>：：类型*=0）”；
    返回cmphelpereq（应为表达式，实际为表达式，应为，
                       实际）；
  }

  //当第二个参数assert_eq（）是
  //指针，例如assert_eq（空，一个_指针）。
  模板<typename t>
  静态断言结果比较（
      const char*应为表达式，
      const char*实际表达式，
      //我们以前有第二个模板参数而不是secret*。那
      //模板参数将推断为“long”，使其更好匹配
      //大于第一个重载，即使没有第一个重载的enableif。
      //不幸的是，带有-wconversion NULL的gcc在“将NULL传递给
      //非指针参数（即使是推导出的整型参数），因此
      //实现导致用户代码中出现警告。
      应输入secret*/*（空）*/,

      T* actual) {
//我们已经知道“expected”是一个空指针。
    return CmpHelperEQ(expected_expression, actual_expression,
                       static_cast<T*>(NULL), actual);
  }
};

//将错误生成代码与代码路径分开，以减少堆栈
//cmphelperop的帧大小。这有助于减少一些消毒剂的开销
//在紧密循环中调用Expect_op时。
template <typename T1, typename T2>
AssertionResult CmpHelperOpFailure(const char* expr1, const char* expr2,
                                   const T1& val1, const T2& val2,
                                   const char* op) {
  return AssertionFailure()
         << "Expected: (" << expr1 << ") " << op << " (" << expr2
         << "), actual: " << FormatForComparisonFailureMessage(val1, val2)
         << " vs " << FormatForComparisonFailureMessage(val2, val1);
}

//用于实现实现所需帮助器函数的宏
//断言？和期待？？这里只是为了避免复制和粘贴
//类似代码。
//
//对于每个模板化的助手函数，我们还定义了一个重载
//为了减少代码膨胀和允许
//要与断言预期使用的匿名枚举？编译时
//与GCC 4。
//
//内部实现-不要在用户程序中使用。

#define GTEST_IMPL_CMP_HELPER_(op_name, op)\
template <typename T1, typename T2>\
AssertionResult CmpHelper##op_name(const char* expr1, const char* expr2, \
                                   const T1& val1, const T2& val2) {\
  if (val1 op val2) {\
    return AssertionSuccess();\
  } else {\
    return CmpHelperOpFailure(expr1, expr2, val1, val2, #op);\
  }\
}\
GTEST_API_ AssertionResult CmpHelper##op_name(\
    const char* expr1, const char* expr2, BiggestInt val1, BiggestInt val2)

//内部实现-不要在用户程序中使用。

//实现断言预期ne的助手函数
GTEST_IMPL_CMP_HELPER_(NE, !=);
//实现断言预期le的助手函数
GTEST_IMPL_CMP_HELPER_(LE, <=);
//实现断言预期u lt的助手函数
GTEST_IMPL_CMP_HELPER_(LT, <);
//为断言预期u ge实现助手函数
GTEST_IMPL_CMP_HELPER_(GE, >=);
//实现断言预期u gt的帮助函数
GTEST_IMPL_CMP_HELPER_(GT, >);

#undef GTEST_IMPL_CMP_HELPER_

//_assert_expect_u streq的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTREQ(const char* expected_expression,
                                          const char* actual_expression,
                                          const char* expected,
                                          const char* actual);

//_assert_expect_u strcaseq的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTRCASEEQ(const char* expected_expression,
                                              const char* actual_expression,
                                              const char* expected,
                                              const char* actual);

//_assert_expect_u strne的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTRNE(const char* s1_expression,
                                          const char* s2_expression,
                                          const char* s1,
                                          const char* s2);

//_assert_expect_u strcasene的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTRCASENE(const char* s1_expression,
                                              const char* s2_expression,
                                              const char* s1,
                                              const char* s2);


//宽字符串上的*streq的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTREQ(const char* expected_expression,
                                          const char* actual_expression,
                                          const wchar_t* expected,
                                          const wchar_t* actual);

//宽字符串上的*字符串的帮助函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult CmpHelperSTRNE(const char* s1_expression,
                                          const char* s2_expression,
                                          const wchar_t* s1,
                                          const wchar_t* s2);

}  //命名空间内部

//issubstring（）和isnotsubstring（）将用作
//第一个参数到expect，assert u pred_format2（），而不是by
//他们自己。他们检查针是否是干草堆的一个子环。
//（空值仅被视为自身的子字符串），并返回
//失败时显示适当的错误消息。
//
//针、干草堆表达式参数是字符串化的。
//生成两个实参的表达式。
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack);
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack);
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack);

#if GTEST_HAS_STD_WSTRING
GTEST_API_ AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack);
GTEST_API_ AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack);
#endif  //GTEST_有_-std_-wstring

namespace internal {

//用于比较浮点的助手模板函数。
//
//模板参数：
//
//raw type：原始浮点类型（float或double）
//
//内部实现-不要在用户程序中使用。
template <typename RawType>
AssertionResult CmpHelperFloatingPointEQ(const char* expected_expression,
                                         const char* actual_expression,
                                         RawType expected,
                                         RawType actual) {
  const FloatingPoint<RawType> lhs(expected), rhs(actual);

  if (lhs.AlmostEquals(rhs)) {
    return AssertionSuccess();
  }

  ::std::stringstream expected_ss;
  expected_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
              << expected;

  ::std::stringstream actual_ss;
  actual_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
            << actual;

  return EqFailure(expected_expression,
                   actual_expression,
                   StringStreamToString(&expected_ss),
                   StringStreamToString(&actual_ss),
                   false);
}

//用于实现assert_near的helper函数。
//
//内部实现-不要在用户程序中使用。
GTEST_API_ AssertionResult DoubleNearPredFormat(const char* expr1,
                                                const char* expr2,
                                                const char* abs_error_expr,
                                                double val1,
                                                double val2,
                                                double abs_error);

//内部实现-不要在用户代码中使用。
//允许将消息流式传输到断言宏的类
class GTEST_API_ AssertHelper {
 public:
//构造函数。
  AssertHelper(TestPartResult::Type type,
               const char* file,
               int line,
               const char* message);
  ~AssertHelper();

//消息分配是启用断言的语义技巧
//流媒体；请参见下面的gtest_message_u宏。
  void operator=(const Message& message) const;

 private:
//我们将数据放入一个结构中，以便assertHelper类的大小可以
//尽可能小。这很重要，因为GCC不能
//即使是临时变量也要重新使用堆栈空间，因此每个变量都需要
//为另一个AssertHelper保留堆栈空间。
  struct AssertHelperData {
    AssertHelperData(TestPartResult::Type t,
                     const char* srcfile,
                     int line_num,
                     const char* msg)
        : type(t), file(srcfile), line(line_num), message(msg) { }

    TestPartResult::Type const type;
    const char* const file;
    int const line;
    std::string const message;

   private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(AssertHelperData);
  };

  AssertHelperData* const data_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(AssertHelper);
};

}  //命名空间内部

#if GTEST_HAS_PARAM_TEST
//所有值参数化测试所继承的纯接口类。
//值参数化类必须同时从：：testing:：test和继承
//：：测试：：WithParamInterface。在大多数情况下，这仅仅意味着继承
//从：：testing：：testwithParam，但更复杂的测试层次结构
//可能需要从不同级别的test和withParamInterface继承。
//
//此接口支持通过访问测试参数值
//getParam（）方法。
//
//与参数生成器定义函数之一（如range（））一起使用，
//values（）、valuesin（）、bool（）和combine（）。
//
//class footerst:public:：testing:：testwithParam<int>
//受保护的：
//FooTestTo（）{
////在此可以使用getParam（）。
//}
//虚拟~footst（）
////在此可以使用getParam（）。
//}
//virtual void setup（）
////在此可以使用getParam（）。
//}
//虚拟虚空拆解
////在此可以使用getParam（）。
//}
//}；
//测试_P（footerst，doesbar）
////此处不能使用getParam（）方法。
//福福；
//断言_true（foo.doesbar（getparam（））；
//}
//实例化_test_case_p（onetotenrange，footerst，：：testing:：range（1，10））；

template <typename T>
class WithParamInterface {
 public:
  typedef T ParamType;
  virtual ~WithParamInterface() {}

//当前参数值。也可在测试夹具中找到
//构造函数。这个成员函数是非静态的，即使它只是
//引用静态数据，以减少错误使用的机会
//类似于为测试写入“withParamInterface<bool>：：getParam（）”
//使用参数类型为int的fixture。
  const ParamType& GetParam() const {
    GTEST_CHECK_(parameter_ != NULL)
        << "GetParam() can only be called inside a value-parameterized test "
        << "-- did you intend to write TEST_P instead of TEST_F?";
    return *parameter_;
  }

 private:
//设置参数值。调用方负责确保值
//在当前测试期间保持活动和不变。
  static void SetParam(const ParamType* parameter) {
    parameter_ = parameter;
  }

//用于在测试生存期访问参数的静态值。
  static const ParamType* parameter_;

//testclass必须是withParamInterface<t>和test的子类。
  template <class TestClass> friend class internal::ParameterizedTestFactory;
};

template <typename T>
const T* WithParamInterface<T>::parameter_ = NULL;

//大多数值参数化类可以忽略
//withParamInterface，并且只能从：：testing:：testWithParam继承。

template <typename T>
class TestWithParam : public Test, public WithParamInterface<T> {
};

#endif  //gtest_有参数测试

//用于指示测试代码中成功/失败的宏。

//添加失败无条件地将失败添加到当前测试。
//成功产生成功-它不会自动使
//当前测试成功，因为只有当测试
//没有失败。
//
//Expect_*验证是否满足某个条件。如果不是，
//它的行为类似于添加失败。特别地：
//
//Expect_true验证布尔条件是否为真。
//Expect_false验证布尔条件是否为false。
//
//fail和assert_x与add_failure和expect_x相似，除了
//它们还将在出现故障时中止当前功能。人
//通常需要fail和assert的fail-fast行为，但是那些
//编写数据驱动的测试通常会发现自己使用了“添加失败”
//期待更多。

//用一般消息生成非致命故障。
#define ADD_FAILURE() GTEST_NONFATAL_FAILURE_("Failed")

//在给定的源文件位置生成非致命故障
//通用消息。
#define ADD_FAILURE_AT(file, line) \
  GTEST_MESSAGE_AT_(file, line, "Failed", \
                    ::testing::TestPartResult::kNonFatalFailure)

//用一般消息生成致命故障。
#define GTEST_FAIL() GTEST_FATAL_FAILURE_("Failed")

//将此宏定义为1以忽略fail（）的定义，fail（）是
//通用名称和与其他一些库冲突。
#if !GTEST_DONT_DEFINE_FAIL
# define FAIL() GTEST_FAIL()
#endif

//通过通用消息生成成功消息。
#define GTEST_SUCCEED() GTEST_SUCCESS_("Succeeded")

//将此宏定义为1以忽略success（）的定义，其中
//是通用名称，与其他库冲突。
#if !GTEST_DONT_DEFINE_SUCCEED
# define SUCCEED() GTEST_SUCCEED()
#endif

//用于测试异常的宏。
//
//*断言预期抛出（语句，预期异常）：
//测试语句是否引发预期的异常。
//*断言预期不抛出（语句）：
//测试语句是否不引发任何异常。
//＊断言期望任何抛出（语句）：
//测试语句是否引发异常。

#define EXPECT_THROW(statement, expected_exception) \
  GTEST_TEST_THROW_(statement, expected_exception, GTEST_NONFATAL_FAILURE_)
#define EXPECT_NO_THROW(statement) \
  GTEST_TEST_NO_THROW_(statement, GTEST_NONFATAL_FAILURE_)
#define EXPECT_ANY_THROW(statement) \
  GTEST_TEST_ANY_THROW_(statement, GTEST_NONFATAL_FAILURE_)
#define ASSERT_THROW(statement, expected_exception) \
  GTEST_TEST_THROW_(statement, expected_exception, GTEST_FATAL_FAILURE_)
#define ASSERT_NO_THROW(statement) \
  GTEST_TEST_NO_THROW_(statement, GTEST_FATAL_FAILURE_)
#define ASSERT_ANY_THROW(statement) \
  GTEST_TEST_ANY_THROW_(statement, GTEST_FATAL_FAILURE_)

//布尔断言。条件可以是布尔表达式或
//断言结果。有关如何将断言结果用于
//这些宏可以看到该类的注释。
#define EXPECT_TRUE(condition) \
  GTEST_TEST_BOOLEAN_(condition, #condition, false, true, \
                      GTEST_NONFATAL_FAILURE_)
#define EXPECT_FALSE(condition) \
  GTEST_TEST_BOOLEAN_(!(condition), #condition, true, false, \
                      GTEST_NONFATAL_FAILURE_)
#define ASSERT_TRUE(condition) \
  GTEST_TEST_BOOLEAN_(condition, #condition, false, true, \
                      GTEST_FATAL_FAILURE_)
#define ASSERT_FALSE(condition) \
  GTEST_TEST_BOOLEAN_(!(condition), #condition, true, false, \
                      GTEST_FATAL_FAILURE_)

//包括实现一系列
//通用谓词断言宏。
//版权所有2006，Google Inc.
//版权所有。
//
//以源和二进制形式重新分配和使用，有或无
//允许修改，前提是以下条件
//遇见：
//
//*源代码的再分配必须保留上述版权。
//注意，此条件列表和以下免责声明。
//*二进制形式的再分配必须复制上述内容
//版权声明、此条件列表和以下免责声明
//在提供的文件和/或其他材料中，
//分布。
//*无论是谷歌公司的名称还是其
//贡献者可用于支持或推广源自
//本软件未经事先明确书面许可。
//
//本软件由版权所有者和贡献者提供。
//“原样”和任何明示或暗示的保证，包括但不包括
//仅限于对适销性和适用性的暗示保证
//不承认特定目的。在任何情况下，版权
//所有人或出资人对任何直接、间接、附带的，
//特殊、惩戒性或后果性损害（包括但不包括
//仅限于采购替代货物或服务；使用损失，
//数据或利润；或业务中断），无论如何引起的
//责任理论，无论是合同责任、严格责任还是侵权责任。
//（包括疏忽或其他）因使用不当而引起的
//即使已告知此类损坏的可能性。

//该文件由命令于2011年10月31日自动生成。
//“Gen_gtest_pred_impl.py 5”。不要手工编辑！
//
//实现一系列通用谓词断言宏。

#ifndef GTEST_INCLUDE_GTEST_GTEST_PRED_IMPL_H_
#define GTEST_INCLUDE_GTEST_GTEST_PRED_IMPL_H_

//确保在gtest.h之前不包括此标题。
#ifndef GTEST_INCLUDE_GTEST_GTEST_H_
# error Do not include gtest_pred_impl.h directly.  Include gtest.h instead.
#endif  //gtest_包括gtest_gtest_h_

//此头实现一系列通用谓词断言
//宏指令：
//
//断言pred格式1（pred格式，v1）
//断言pred格式2（pred格式，v1，v2）
//…
//
//其中pred_格式是接受n的函数或函数（在
//断言pred格式值及其源表达式的大小写
//文本，并返回测试：：断言结果。参见定义
//例如，gtest.h中的断言eq。
//
//如果您不关心格式，可以使用更多
//限制版本：
//
//断言pred1（pred，v1）
//断言pred2（pred，v1，v2）
//…
//
//其中pred是返回bool的n元函数或函数，
//值v1，v2，…，必须支持<<运算符
//流式传输到std:：ostream。
//
//我们还定义了预期变化。
//
//目前我们只支持arity最多为5的谓词。
//如果需要，请发送电子邮件至googletestframework@googlegroups.com
//支持更高的算术。

//gtest_assert_u是所有断言的基本语句
//在此文件中减少。不要在代码中使用这个。

#define GTEST_ASSERT_(expression, on_failure) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (const ::testing::AssertionResult gtest_ar = (expression)) \
    ; \
  else \
    on_failure(gtest_ar.failure_message())


//用于实现expect assert u pred1的助手函数。不要使用
//这在你的代码中。
template <typename Pred,
          typename T1>
AssertionResult AssertPred1Helper(const char* pred_text,
                                  const char* e1,
                                  Pred pred,
                                  const T1& v1) {
  if (pred(v1)) return AssertionSuccess();

  return AssertionFailure() << pred_text << "("
                            << e1 << ") evaluates to false, where"
                            << "\n" << e1 << " evaluates to " << v1;
}

//用于实现expect assert u pred_format1的内部宏。
//不要在代码中使用这个。
#define GTEST_PRED_FORMAT1_(pred_format, v1, on_failure)\
  GTEST_ASSERT_(pred_format(#v1, v1), \
                on_failure)

//用于实现预期断言u pred1的内部宏。不要使用
//这在你的代码中。
#define GTEST_PRED1_(pred, v1, on_failure)\
  GTEST_ASSERT_(::testing::AssertPred1Helper(#pred, \
                                             #v1, \
                                             pred, \
                                             v1), on_failure)

//一元谓词断言宏。
#define EXPECT_PRED_FORMAT1(pred_format, v1) \
  GTEST_PRED_FORMAT1_(pred_format, v1, GTEST_NONFATAL_FAILURE_)
#define EXPECT_PRED1(pred, v1) \
  GTEST_PRED1_(pred, v1, GTEST_NONFATAL_FAILURE_)
#define ASSERT_PRED_FORMAT1(pred_format, v1) \
  GTEST_PRED_FORMAT1_(pred_format, v1, GTEST_FATAL_FAILURE_)
#define ASSERT_PRED1(pred, v1) \
  GTEST_PRED1_(pred, v1, GTEST_FATAL_FAILURE_)



//用于实现预期断言u pred2的助手函数。不要使用
//这在你的代码中。
template <typename Pred,
          typename T1,
          typename T2>
AssertionResult AssertPred2Helper(const char* pred_text,
                                  const char* e1,
                                  const char* e2,
                                  Pred pred,
                                  const T1& v1,
                                  const T2& v2) {
  if (pred(v1, v2)) return AssertionSuccess();

  return AssertionFailure() << pred_text << "("
                            << e1 << ", "
                            << e2 << ") evaluates to false, where"
                            << "\n" << e1 << " evaluates to " << v1
                            << "\n" << e2 << " evaluates to " << v2;
}

//用于实现expect assert u pred_format2的内部宏。
//不要在代码中使用这个。
#define GTEST_PRED_FORMAT2_(pred_format, v1, v2, on_failure)\
  GTEST_ASSERT_(pred_format(#v1, #v2, v1, v2), \
                on_failure)

//用于实现预期断言u pred2的内部宏。不要使用
//这在你的代码中。
#define GTEST_PRED2_(pred, v1, v2, on_failure)\
  GTEST_ASSERT_(::testing::AssertPred2Helper(#pred, \
                                             #v1, \
                                             #v2, \
                                             pred, \
                                             v1, \
                                             v2), on_failure)

//二进制谓词断言宏。
#define EXPECT_PRED_FORMAT2(pred_format, v1, v2) \
  GTEST_PRED_FORMAT2_(pred_format, v1, v2, GTEST_NONFATAL_FAILURE_)
#define EXPECT_PRED2(pred, v1, v2) \
  GTEST_PRED2_(pred, v1, v2, GTEST_NONFATAL_FAILURE_)
#define ASSERT_PRED_FORMAT2(pred_format, v1, v2) \
  GTEST_PRED_FORMAT2_(pred_format, v1, v2, GTEST_FATAL_FAILURE_)
#define ASSERT_PRED2(pred, v1, v2) \
  GTEST_PRED2_(pred, v1, v2, GTEST_FATAL_FAILURE_)



//用于实现预期断言u pred3的助手函数。不要使用
//这在你的代码中。
template <typename Pred,
          typename T1,
          typename T2,
          typename T3>
AssertionResult AssertPred3Helper(const char* pred_text,
                                  const char* e1,
                                  const char* e2,
                                  const char* e3,
                                  Pred pred,
                                  const T1& v1,
                                  const T2& v2,
                                  const T3& v3) {
  if (pred(v1, v2, v3)) return AssertionSuccess();

  return AssertionFailure() << pred_text << "("
                            << e1 << ", "
                            << e2 << ", "
                            << e3 << ") evaluates to false, where"
                            << "\n" << e1 << " evaluates to " << v1
                            << "\n" << e2 << " evaluates to " << v2
                            << "\n" << e3 << " evaluates to " << v3;
}

//用于实现expect assert u pred_format3的内部宏。
//不要在代码中使用这个。
#define GTEST_PRED_FORMAT3_(pred_format, v1, v2, v3, on_failure)\
  GTEST_ASSERT_(pred_format(#v1, #v2, #v3, v1, v2, v3), \
                on_failure)

//用于实现预期断言u pred3的内部宏。不要使用
//这在你的代码中。
#define GTEST_PRED3_(pred, v1, v2, v3, on_failure)\
  GTEST_ASSERT_(::testing::AssertPred3Helper(#pred, \
                                             #v1, \
                                             #v2, \
                                             #v3, \
                                             pred, \
                                             v1, \
                                             v2, \
                                             v3), on_failure)

//三元谓词断言宏。
#define EXPECT_PRED_FORMAT3(pred_format, v1, v2, v3) \
  GTEST_PRED_FORMAT3_(pred_format, v1, v2, v3, GTEST_NONFATAL_FAILURE_)
#define EXPECT_PRED3(pred, v1, v2, v3) \
  GTEST_PRED3_(pred, v1, v2, v3, GTEST_NONFATAL_FAILURE_)
#define ASSERT_PRED_FORMAT3(pred_format, v1, v2, v3) \
  GTEST_PRED_FORMAT3_(pred_format, v1, v2, v3, GTEST_FATAL_FAILURE_)
#define ASSERT_PRED3(pred, v1, v2, v3) \
  GTEST_PRED3_(pred, v1, v2, v3, GTEST_FATAL_FAILURE_)



//用于实现预期断言u pred4的助手函数。不要使用
//这在你的代码中。
template <typename Pred,
          typename T1,
          typename T2,
          typename T3,
          typename T4>
AssertionResult AssertPred4Helper(const char* pred_text,
                                  const char* e1,
                                  const char* e2,
                                  const char* e3,
                                  const char* e4,
                                  Pred pred,
                                  const T1& v1,
                                  const T2& v2,
                                  const T3& v3,
                                  const T4& v4) {
  if (pred(v1, v2, v3, v4)) return AssertionSuccess();

  return AssertionFailure() << pred_text << "("
                            << e1 << ", "
                            << e2 << ", "
                            << e3 << ", "
                            << e4 << ") evaluates to false, where"
                            << "\n" << e1 << " evaluates to " << v1
                            << "\n" << e2 << " evaluates to " << v2
                            << "\n" << e3 << " evaluates to " << v3
                            << "\n" << e4 << " evaluates to " << v4;
}

//用于实现expect assert u pred_format4的内部宏。
//不要在代码中使用这个。
#define GTEST_PRED_FORMAT4_(pred_format, v1, v2, v3, v4, on_failure)\
  GTEST_ASSERT_(pred_format(#v1, #v2, #v3, #v4, v1, v2, v3, v4), \
                on_failure)

//用于实现预期断言u pred4的内部宏。不要使用
//这在你的代码中。
#define GTEST_PRED4_(pred, v1, v2, v3, v4, on_failure)\
  GTEST_ASSERT_(::testing::AssertPred4Helper(#pred, \
                                             #v1, \
                                             #v2, \
                                             #v3, \
                                             #v4, \
                                             pred, \
                                             v1, \
                                             v2, \
                                             v3, \
                                             v4), on_failure)

//四元谓词断言宏。
#define EXPECT_PRED_FORMAT4(pred_format, v1, v2, v3, v4) \
  GTEST_PRED_FORMAT4_(pred_format, v1, v2, v3, v4, GTEST_NONFATAL_FAILURE_)
#define EXPECT_PRED4(pred, v1, v2, v3, v4) \
  GTEST_PRED4_(pred, v1, v2, v3, v4, GTEST_NONFATAL_FAILURE_)
#define ASSERT_PRED_FORMAT4(pred_format, v1, v2, v3, v4) \
  GTEST_PRED_FORMAT4_(pred_format, v1, v2, v3, v4, GTEST_FATAL_FAILURE_)
#define ASSERT_PRED4(pred, v1, v2, v3, v4) \
  GTEST_PRED4_(pred, v1, v2, v3, v4, GTEST_FATAL_FAILURE_)



//用于实现预期断言u pred5的助手函数。不要使用
//这在你的代码中。
template <typename Pred,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5>
AssertionResult AssertPred5Helper(const char* pred_text,
                                  const char* e1,
                                  const char* e2,
                                  const char* e3,
                                  const char* e4,
                                  const char* e5,
                                  Pred pred,
                                  const T1& v1,
                                  const T2& v2,
                                  const T3& v3,
                                  const T4& v4,
                                  const T5& v5) {
  if (pred(v1, v2, v3, v4, v5)) return AssertionSuccess();

  return AssertionFailure() << pred_text << "("
                            << e1 << ", "
                            << e2 << ", "
                            << e3 << ", "
                            << e4 << ", "
                            << e5 << ") evaluates to false, where"
                            << "\n" << e1 << " evaluates to " << v1
                            << "\n" << e2 << " evaluates to " << v2
                            << "\n" << e3 << " evaluates to " << v3
                            << "\n" << e4 << " evaluates to " << v4
                            << "\n" << e5 << " evaluates to " << v5;
}

//用于实现expect assert u pred_format5的内部宏。
//不要在代码中使用这个。
#define GTEST_PRED_FORMAT5_(pred_format, v1, v2, v3, v4, v5, on_failure)\
  GTEST_ASSERT_(pred_format(#v1, #v2, #v3, #v4, #v5, v1, v2, v3, v4, v5), \
                on_failure)

//用于实现预期断言u pred5的内部宏。不要使用
//这在你的代码中。
#define GTEST_PRED5_(pred, v1, v2, v3, v4, v5, on_failure)\
  GTEST_ASSERT_(::testing::AssertPred5Helper(#pred, \
                                             #v1, \
                                             #v2, \
                                             #v3, \
                                             #v4, \
                                             #v5, \
                                             pred, \
                                             v1, \
                                             v2, \
                                             v3, \
                                             v4, \
                                             v5), on_failure)

//5元谓词断言宏。
#define EXPECT_PRED_FORMAT5(pred_format, v1, v2, v3, v4, v5) \
  GTEST_PRED_FORMAT5_(pred_format, v1, v2, v3, v4, v5, GTEST_NONFATAL_FAILURE_)
#define EXPECT_PRED5(pred, v1, v2, v3, v4, v5) \
  GTEST_PRED5_(pred, v1, v2, v3, v4, v5, GTEST_NONFATAL_FAILURE_)
#define ASSERT_PRED_FORMAT5(pred_format, v1, v2, v3, v4, v5) \
  GTEST_PRED_FORMAT5_(pred_format, v1, v2, v3, v4, v5, GTEST_FATAL_FAILURE_)
#define ASSERT_PRED5(pred, v1, v2, v3, v4, v5) \
  GTEST_PRED5_(pred, v1, v2, v3, v4, v5, GTEST_FATAL_FAILURE_)



#endif  //gtest_包括gtest_gtest_pred_impl_h_

//用于测试等式和不等式的宏。
//
//*断言预期U eq（预期，实际）：预期测试==实际
//*断言期望u ne（v1，v2）：测试v1！= V2
//*断言预期lt（v1，v2）：测试v1<v2
//*断言预期预期（v1，v2）：测试v1<=v2
//＊断言预期gt（v1，v2）：测试v1>v2
//*断言预期GE（v1，v2）：测试v1>=v2
//
//如果不是，谷歌测试会打印测试的表达式和
//它们的实际值。值必须是兼容的内置类型，
//否则会出现编译器错误。“兼容”是指
//值可以通过相应的运算符进行比较。
//
//注：
//
//1。可以使用户定义的类型与
//断言期望uu？？（），但这需要重载
//比较运算符，因此谷歌C+
//使用指南。因此，建议您使用
//assert expect u true（）宏断言两个对象是
//相等。
//
//2。断言预期_？（）宏在上进行指针比较
//指针（尤其是C字符串）。因此，如果你使用它
//使用两个C字符串，您将测试它们在内存中的位置
//是相关的，而不是他们的内容是如何相关的。比较两个C
//字符串按内容，使用断言预期字符串*（）。
//
//三。断言预期_eq（预期，实际）优先于
//断言预期真（预期==实际），正如前者告诉您的那样
//当它失败时，实际值是什么，同样地，对于
//其他比较。
//
//4。不要依赖于断言预期的顺序？（？）
//评估他们的参数，这是未定义的。
//
//5。这些宏只计算一次参数。
//
//实例：
//
//期望_e（5，foo（））；
//期望_Eq（空，一个_指针）；
//断言lt（i，数组大小）；
//断言gt（records.size（），0）<“没有留下任何记录。”；

#define EXPECT_EQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal:: \
                      EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare, \
                      expected, actual)
#define EXPECT_NE(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperNE, expected, actual)
#define EXPECT_LE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLE, val1, val2)
#define EXPECT_LT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperLT, val1, val2)
#define EXPECT_GE(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGE, val1, val2)
#define EXPECT_GT(val1, val2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperGT, val1, val2)

#define GTEST_ASSERT_EQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal:: \
                      EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare, \
                      expected, actual)
#define GTEST_ASSERT_NE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperNE, val1, val2)
#define GTEST_ASSERT_LE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperLE, val1, val2)
#define GTEST_ASSERT_LT(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperLT, val1, val2)
#define GTEST_ASSERT_GE(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperGE, val1, val2)
#define GTEST_ASSERT_GT(val1, val2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperGT, val1, val2)

//define macro gtest_不要将断言xy定义为1以忽略
//断言xy（），它与某些用户自己的代码冲突。

#if !GTEST_DONT_DEFINE_ASSERT_EQ
# define ASSERT_EQ(val1, val2) GTEST_ASSERT_EQ(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_NE
# define ASSERT_NE(val1, val2) GTEST_ASSERT_NE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LE
# define ASSERT_LE(val1, val2) GTEST_ASSERT_LE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_LT
# define ASSERT_LT(val1, val2) GTEST_ASSERT_LT(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GE
# define ASSERT_GE(val1, val2) GTEST_ASSERT_GE(val1, val2)
#endif

#if !GTEST_DONT_DEFINE_ASSERT_GT
# define ASSERT_GT(val1, val2) GTEST_ASSERT_GT(val1, val2)
#endif

//C字符串比较。所有测试都处理空字符串和任何非空字符串
//不同。两个空值相等。
//
//*断言预期应力（s1，s2）：测试s1==s2
//*断言预期strne（s1，s2）：测试s1！= S2
//*断言预期strcaseq（s1，s2）：测试s1==s2，忽略case
//*断言预期strcasene（s1，s2）：测试s1！=s2，忽略大小写
//
//对于宽或窄字符串对象，可以使用
//断言期望uu？（）宏。
//
//不要依赖于参数的计算顺序，
//这是未定义的。
//
//这些宏只计算一次参数。

#define EXPECT_STREQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTREQ, expected, actual)
#define EXPECT_STRNE(s1, s2) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRNE, s1, s2)
#define EXPECT_STRCASEEQ(expected, actual) \
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASEEQ, expected, actual)
#define EXPECT_STRCASENE(s1, s2)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASENE, s1, s2)

#define ASSERT_STREQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTREQ, expected, actual)
#define ASSERT_STRNE(s1, s2) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRNE, s1, s2)
#define ASSERT_STRCASEEQ(expected, actual) \
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASEEQ, expected, actual)
#define ASSERT_STRCASENE(s1, s2)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperSTRCASENE, s1, s2)

//用于比较浮点数字的宏。
//
//*断言预期浮动_eq（预期，实际）：
//测试两个浮点值几乎相等。
//*
//测试两个双精度值几乎相等。
//*断言预期附近（v1，v2，abs_错误）：
//测试v1和v2是否在给定的距离内。
//
//Google测试使用基于ULP的比较自动选择默认值
//适用于操作数的错误绑定。见
//gtest internal.h中的floatingpoint模板类（如果是）
//对实现细节感兴趣。

#define EXPECT_FLOAT_EQ(expected, actual)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<float>, \
                      expected, actual)

#define EXPECT_DOUBLE_EQ(expected, actual)\
  EXPECT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<double>, \
                      expected, actual)

#define ASSERT_FLOAT_EQ(expected, actual)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<float>, \
                      expected, actual)

#define ASSERT_DOUBLE_EQ(expected, actual)\
  ASSERT_PRED_FORMAT2(::testing::internal::CmpHelperFloatingPointEQ<double>, \
                      expected, actual)

#define EXPECT_NEAR(val1, val2, abs_error)\
  EXPECT_PRED_FORMAT3(::testing::internal::DoubleNearPredFormat, \
                      val1, val2, abs_error)

#define ASSERT_NEAR(val1, val2, abs_error)\
  ASSERT_PRED_FORMAT3(::testing::internal::DoubleNearPredFormat, \
                      val1, val2, abs_error)

//这些谓词格式函数用于浮点值，以及
//可用于断言预期预测格式2*（），例如
//
//expect_pred_format2（测试：：doublele，foo（），5.0）；

//断言val1小于或几乎等于val2。失败
//否则。尤其是，如果val1或val2为NaN，则失败。
GTEST_API_ AssertionResult FloatLE(const char* expr1, const char* expr2,
                                   float val1, float val2);
GTEST_API_ AssertionResult DoubleLE(const char* expr1, const char* expr2,
                                    double val1, double val2);


#if GTEST_OS_WINDOWS

//宏用于测试hresult失败和成功，这些仅有用
//在Windows上，并依赖Windows SDK宏和API进行编译。
//
//*断言预期结果成功失败（expr）
//
//当expr意外失败或成功时，google test会打印
//预期结果和实际结果都具有可读性
//错误的字符串表示形式（如果可用），以及
//十六进制结果代码。
# define EXPECT_HRESULT_SUCCEEDED(expr) \
    EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))

# define ASSERT_HRESULT_SUCCEEDED(expr) \
    ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTSuccess, (expr))

# define EXPECT_HRESULT_FAILED(expr) \
    EXPECT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))

# define ASSERT_HRESULT_FAILED(expr) \
    ASSERT_PRED_FORMAT1(::testing::internal::IsHRESULTFailure, (expr))

#endif  //GTEST操作系统窗口

//执行语句并检查其是否生成新的致命错误的宏
//当前线程中的失败。
//
//＊断言预期没有致命失败（声明）；
//
//实例：
//
//预期_no_fatal_failure（process（））；
//断言没有致命的失败（process（））<“process（）failed”；
//
#define ASSERT_NO_FATAL_FAILURE(statement) \
    GTEST_TEST_NO_FATAL_FAILURE_(statement, GTEST_FATAL_FAILURE_)
#define EXPECT_NO_FATAL_FAILURE(statement) \
    GTEST_TEST_NO_FATAL_FAILURE_(statement, GTEST_NONFATAL_FAILURE_)

//导致跟踪（包括源文件路径、当前行
//编号和给定的信息）应包括在每次测试失败中。
//由当前作用域中的代码生成的消息。效果是
//当控件离开当前范围时撤消。
//
//消息参数可以是任何可流式传输到std:：ostream的内容。
//
//在实现中，我们将当前行号作为一部分
//虚拟变量名，因此允许多个作用域的
//出现在同一个块中-只要它们在不同的块上
//线。
#define SCOPED_TRACE(message) \
  ::testing::internal::ScopedTrace GTEST_CONCAT_TOKEN_(gtest_trace_, __LINE__)(\
    __FILE__, __LINE__, ::testing::Message() << (message))

//类型相等的编译时断言。
//StaticAssertTypeeq<type1，type2>（）编译iff type1和type2
//同一类型。它返回的值并不有趣。
//
//我们不将staticAssertTypeeq作为类模板，而是将其作为
//调用助手类模板的函数模板。这个
//防止用户误用staticAssertTypeeq<t1，t2>
//定义该类型的对象。
//
//警告：
//
//在类模板的方法中使用时，
//只有当方法为
//实例化。例如，给定：
//
//template<typename t>class foo_
//公众：
//void bar（）测试：：staticAssertTypeeq<int，t>（）；
//}；
//
//代码：
//
//void test1（）foo<bool>foo；
//
//不会生成编译器错误，因为foo<bool>：：bar（）从不
//实际实例化。相反，您需要：
//
//void test2（）foo<bool>foo；foo.bar（）；
//
//导致编译错误。
template <typename T1, typename T2>
bool StaticAssertTypeEq() {
  (void)internal::StaticAssertTypeEqHelper<T1, T2>();
  return true;
}

//定义测试。
//
//第一个参数是测试用例的名称，第二个参数是
//参数是测试用例中测试的名称。
//
//约定是以“test”结束测试用例名称。为了
//例如，可以将foo类的测试用例命名为footst。
//
//调用后，测试代码应出现在大括号之间
//这个宏。例子：
//
//测试（footerst，直接初始化）
//福福；
//预期为“真”（foo.statusOK（））；
//}

//注意，我们调用gettestypeid（）而不是gettypeid<
//：：testing：：test>（）此处获取testing：：test的类型ID。这个
//在使用google test as时是为了解决一个可疑的链接器错误
//Mac OS X上的框架。该错误导致gettypeid<
//：：testing:：test>（）返回不同的值，具体取决于
//调用来自Google测试框架本身或用户测试
//代码。gettestypeid（）保证始终返回相同的
//值，因为它总是从Google测试中调用gettypeid<>（）。
//框架。
#define GTEST_TEST(test_case_name, test_name)\
  GTEST_TEST_(test_case_name, test_name, \
              ::testing::Test, ::testing::internal::GetTestTypeId())

//将此宏定义为1以忽略test（）的定义，其中
//是通用名称，与其他库冲突。
#if !GTEST_DONT_DEFINE_TEST
# define TEST(test_case_name, test_name) GTEST_TEST(test_case_name, test_name)
#endif

//定义使用测试夹具的测试。
//
//第一个参数是测试夹具类的名称，它
//也可以兼作测试用例名称。第二个参数是
//测试用例中测试的名称。
//
//测试夹具类必须在前面声明。用户应该
//使用此宏后，他在大括号之间的测试代码。例子：
//
//类footerst:公共测试：：测试
//受保护的：
//virtual void setup（）b_u.addelement（3）；
//
//福奥A.
//福禄；
//}；
//
//测试_f（footerst，initializeScorectly）
//期望_true（a_.statusOK（））；
//}
//
//测试_f（footerst，returnselementcountercorrectly）
//期望_eq（0，a_.size（））；
//期望_eq（1，b_u.size（））；
//}

#define TEST_F(test_fixture, test_name)\
  GTEST_TEST_(test_fixture, test_name, test_fixture, \
              ::testing::internal::GetTypeId<test_fixture>())

}  //命名空间测试

//在main（）中使用此函数来运行所有测试。如果全部返回0
//测试成功，否则为1。
//
//应在命令行被调用后调用run_all_tests（）。
//由initGoogleTest（）解析。
//
//这个函数以前是一个宏；因此，它在全局中
//命名空间和具有全大写名称。
int RUN_ALL_TESTS() GTEST_MUST_USE_RESULT_;

inline int RUN_ALL_TESTS() {
  return ::testing::UnitTest::GetInstance()->Run();
}

#endif  //gtest_包括gtest_gtest_h_
