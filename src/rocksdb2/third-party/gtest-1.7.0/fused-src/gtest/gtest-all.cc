
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
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
//谷歌C++测试框架（谷歌测试）
//
//有时需要通过编译单个文件来构建Google测试。
//此文件用于此目的。

//禁止Clang Analyzer警告。
#ifndef __clang_analyzer__

//这一行确保了gtest.h可以自己编译，甚至
//当它熔合时。
#include "gtest/gtest.h"

//以下几行将拉入真实的gtest*.cc文件。
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

//版权所有2007，Google Inc.
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
//用于测试Google测试本身和使用Google测试的代码的实用程序
//（例如，建立在谷歌测试之上的框架）。

#ifndef GTEST_INCLUDE_GTEST_GTEST_SPI_H_
#define GTEST_INCLUDE_GTEST_GTEST_SPI_H_

namespace testing {

//这个助手类可以用来模拟谷歌测试失败报告。
//这样我们就可以测试Google测试或者建立在Google测试之上的代码。
//
//此类的对象将TestPartResult对象附加到
//每当Google测试时，构造函数中给定的testpartResultArray对象
//报告失败。它可以只截获
//在创建此对象的同一线程中生成，或者它可以截获
//所有生成的失败。这个模拟对象的范围可以用
//两个参数构造函数的第二个参数。
class GTEST_API_ ScopedFakeTestPartResultReporter
    : public TestPartResultReporterInterface {
 public:
//这个对象的两种可能的模拟模式。
  enum InterceptMode {
INTERCEPT_ONLY_CURRENT_THREAD,  //只截取线程本地失败。
INTERCEPT_ALL_THREADS           //截获所有失败。
  };

//Tor将此对象设置为使用的测试部件结果报告器
//通过谷歌测试。“result”参数指定在何处报告
//结果。此报告程序将只捕获在当前
//线程。贬低
  explicit ScopedFakeTestPartResultReporter(TestPartResultArray* result);

//同上，但您可以选择这个对象的拦截范围。
  ScopedFakeTestPartResultReporter(InterceptMode intercept_mode,
                                   TestPartResultArray* result);

//d'tor恢复先前的测试部件结果报告器。
  virtual ~ScopedFakeTestPartResultReporter();

//将TestPartResult对象追加到TestPartResultArray
//在构造函数中接收。
//
//此方法来自TestPartResultReporterInterface
//接口。
  virtual void ReportTestPartResult(const TestPartResult& result);
 private:
  void Init();

  const InterceptMode intercept_mode_;
  TestPartResultReporterInterface* old_reporter_;
  TestPartResultArray* const result_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ScopedFakeTestPartResultReporter);
};

namespace internal {

//用于实现Expect_Fatal_Failure（）和
//预期发生非致命性故障（）。它的析构函数验证给定的
//testpartResultArray只包含一个具有给定
//键入并包含给定的子字符串。如果不是这样的话，
//将生成非致命故障。
class GTEST_API_ SingleFailureChecker {
 public:
//构造函数记住参数。
  SingleFailureChecker(const TestPartResultArray* results,
                       TestPartResult::Type type,
                       const string& substr);
  ~SingleFailureChecker();
 private:
  const TestPartResultArray* const results_;
  const TestPartResult::Type type_;
  const string substr_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(SingleFailureChecker);
};

}  //命名空间内部

}  //命名空间测试

//用于测试Google测试断言或预期代码的一组宏
//以产生谷歌测试致命的失败。它证实了
//语句只会导致一个致命的Google测试失败，并带有“substr”
//是故障信息的一部分。
//
//这个宏有两个不同的版本。只期待致命的失败
//影响并考虑在当前线程中生成的失败，以及
//期望所有线程上的\致命的\失败\除了所有线程外，都是相同的。
//
//即使在语句
//引发异常或中止当前函数。
//
//已知限制：
//-“语句”不能引用本地非静态变量或
//当前对象的非静态成员。
//-“语句”不能返回值。
//-不能将失败消息流式传输到此宏。
//
//注意，即使下面两个的实现
//宏非常相似，我们无法将它们重构为使用
//helper宏，由于预处理器
//作品。在中扩展到未保护的comma测试的AcceptsMacro
//如果我们这样做，gtest-unittest.cc将无法编译。
#define EXPECT_FATAL_FAILURE(statement, substr) \
  do { \
    class GTestExpectFatalFailureHelper {\
     public:\
      static void Execute() { statement; }\
    };\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TestPartResult::kFatalFailure, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ONLY_CURRENT_THREAD, &gtest_failures);\
      GTestExpectFatalFailureHelper::Execute();\
    }\
  } while (::testing::internal::AlwaysFalse())

#define EXPECT_FATAL_FAILURE_ON_ALL_THREADS(statement, substr) \
  do { \
    class GTestExpectFatalFailureHelper {\
     public:\
      static void Execute() { statement; }\
    };\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TestPartResult::kFatalFailure, (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ALL_THREADS, &gtest_failures);\
      GTestExpectFatalFailureHelper::Execute();\
    }\
  } while (::testing::internal::AlwaysFalse())

//用于测试谷歌测试断言或代码的宏。
//生成谷歌测试非致命故障。它断言
//语句只会导致一个非致命的Google测试失败，并带有“substr”
//是故障信息的一部分。
//
//这个宏有两个不同的版本。只预期非致命性故障
//影响并考虑在当前线程中生成的失败，以及
//希望所有线程上的非致命故障都是相同的，但对所有线程都是相同的。
//
//“statement”允许引用局部变量和
//当前对象。
//
//即使在语句
//引发异常或中止当前函数。
//
//已知限制：
//-不能将失败消息流式传输到此宏。
//
//注意，即使下面两个的实现
//宏非常相似，我们无法将它们重构为使用
//helper宏，由于预处理器
//作品。如果我们这样做，当用户给出
//expect_nonfatal_failure（）包含宏的语句
//扩展到包含未保护逗号的代码。这个
//在gtest_unittest.cc中接受扩展到未保护的comma测试的宏
//抓住了。
//
//出于同样的原因，我们不得不写
//if（：：testing:：internal:：alwaystrue（））语句；
//而不是
//GTEST_抑制_Unreachable_code_warning_below_u（语句）
//以避免对无法访问的代码发出MSVC警告。
#define EXPECT_NONFATAL_FAILURE(statement, substr) \
  do {\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TestPartResult::kNonFatalFailure, \
        (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter:: \
          INTERCEPT_ONLY_CURRENT_THREAD, &gtest_failures);\
      if (::testing::internal::AlwaysTrue()) { statement; }\
    }\
  } while (::testing::internal::AlwaysFalse())

#define EXPECT_NONFATAL_FAILURE_ON_ALL_THREADS(statement, substr) \
  do {\
    ::testing::TestPartResultArray gtest_failures;\
    ::testing::internal::SingleFailureChecker gtest_checker(\
        &gtest_failures, ::testing::TestPartResult::kNonFatalFailure, \
        (substr));\
    {\
      ::testing::ScopedFakeTestPartResultReporter gtest_reporter(\
          ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ALL_THREADS, \
          &gtest_failures);\
      if (::testing::internal::AlwaysTrue()) { statement; }\
    }\
  } while (::testing::internal::AlwaysFalse())

#endif  //gtest_包括gtest_gtest_spi_h_

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <list>
#include <map>
#include <ostream>  //诺林
#include <sstream>
#include <vector>

#if GTEST_OS_LINUX

//todo（kenton@google.com）：使用autoconf检测
//GETTimeFay.（）
# define GTEST_HAS_GETTIMEOFDAY_ 1

# include <fcntl.h>  //诺林
# include <limits.h>  //诺林
# include <sched.h>  //诺林
//声明vsnprintf（）。此头在Windows上不可用。
# include <strings.h>  //诺林
# include <sys/mman.h>  //诺林
# include <sys/time.h>  //诺林
# include <unistd.h>  //诺林
# include <string>

#elif GTEST_OS_SYMBIAN
# define GTEST_HAS_GETTIMEOFDAY_ 1
# include <sys/time.h>  //诺林

#elif GTEST_OS_ZOS
# define GTEST_HAS_GETTIMEOFDAY_ 1
# include <sys/time.h>  //诺林

//在z/OS上，strcasecmp还需要strings.h。
# include <strings.h>  //诺林

#elif GTEST_OS_WINDOWS_MOBILE  //我们在CE窗户上。

# include <windows.h>  //诺林
# undef min

#elif GTEST_OS_WINDOWS  //我们在窗户上。

# include <io.h>  //诺林
# include <sys/timeb.h>  //诺林
# include <sys/types.h>  //诺林
# include <sys/stat.h>  //诺林

# if GTEST_OS_WINDOWS_MINGW
//mingw有getTimeOfDay（），但没有ftime64（）。
//todo（kenton@google.com）：使用autoconf检测
//GETTimeFay.（）
//todo（kenton@google.com）：还有其他方法可以让时间进入
//Windows，如GetTickCount（）或GetSystemTimeAsFileTime（）。工具链
//支持这些。考虑改用它们。
#  define GTEST_HAS_GETTIMEOFDAY_ 1
#  include <sys/time.h>  //诺林
# endif  //测试窗口

//cpplin认为头已经包含在内，所以我们希望
//安静下来。
# include <windows.h>  //诺林
# undef min

#else

//假设其他平台具有getTimeOfDay（）。
//todo（kenton@google.com）：使用autoconf检测
//GETTimeFay.（）
# define GTEST_HAS_GETTIMEOFDAY_ 1

//cpplin认为头已经包含在内，所以我们希望
//安静下来。
# include <sys/time.h>  //诺林
# include <unistd.h>  //诺林

#endif  //GTestOSLinux

#if GTEST_HAS_EXCEPTIONS
# include <stdexcept>
#endif

#if GTEST_CAN_STREAM_RESULTS_
# include <arpa/inet.h>  //诺林
# include <netdb.h>  //诺林
# include <sys/socket.h>  //诺林
# include <sys/types.h>  //诺林
#endif

//指示此翻译单元是Google测试的一部分
//实施。它必须在gtest内部inl.h之前
//包含，否则将出现编译器错误。这个技巧是为了
//防止用户意外包括gtest internal inl.h in
//他的密码。
#define GTEST_IMPLEMENTATION_ 1
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

//谷歌C++测试框架使用的实用函数和类。
//
//作者：wan@google.com（zhanyong wan）
//
//此文件包含纯Google测试的内部实现。拜托
//不要将其包含在用户程序中。

#ifndef GTEST_SRC_GTEST_INTERNAL_INL_H_
#define GTEST_SRC_GTEST_INTERNAL_INL_H_

//如果当前翻译单位是
//谷歌测试实现的一部分；否则它是未定义的。
#if !GTEST_IMPLEMENTATION_
//如果该文件包含在用户代码中，只需说“否”。
# error "gtest-internal-inl.h is part of Google Test's internal implementation."
# error "It must not be included except by Google Test itself."
#endif  //GTEST_实施

#ifndef _WIN32_WCE
# include <errno.h>
#endif  //！Wi32
#include <stddef.h>
#include <stdlib.h>  //对于strtoll/\u strtoul64/malloc/免费。
#include <string.h>  //为了移动。

#include <algorithm>
#include <string>
#include <vector>


#if GTEST_CAN_STREAM_RESULTS_
# include <arpa/inet.h>  //诺林
# include <netdb.h>  //诺林
#endif

#if GTEST_OS_WINDOWS
# include <windows.h>  //诺林
#endif  //GTEST操作系统窗口


namespace testing {

//声明标志。
//
//我们不希望用户在代码中修改此标志，但希望
//谷歌测试自己的单元测试能够访问它。因此我们
//在这里声明，而不是在gtest.h.
GTEST_DECLARE_bool_(death_test_use_fork);

namespace internal {

//从google测试中看到的gettestypeid（）的值
//图书馆。这仅用于测试gettestypeid（）。
GTEST_API_ extern const TypeId kTestTypeIdInGoogleTest;

//标志的名称（分析Google测试标志时需要）。
const char kAlsoRunDisabledTestsFlag[] = "also_run_disabled_tests";
const char kBreakOnFailureFlag[] = "break_on_failure";
const char kCatchExceptionsFlag[] = "catch_exceptions";
const char kColorFlag[] = "color";
const char kFilterFlag[] = "filter";
const char kListTestsFlag[] = "list_tests";
const char kOutputFlag[] = "output";
const char kPrintTimeFlag[] = "print_time";
const char kRandomSeedFlag[] = "random_seed";
const char kRepeatFlag[] = "repeat";
const char kShuffleFlag[] = "shuffle";
const char kStackTraceDepthFlag[] = "stack_trace_depth";
const char kStreamResultToFlag[] = "stream_result_to";
const char kThrowOnFailureFlag[] = "throw_on_failure";

//有效的随机种子必须位于[1，kmaxrandomseed]中。
const int kMaxRandomSeed = 99999;

//如果--help标志或等效形式是
//在命令行上指定。
GTEST_API_ extern bool g_help_flag;

//返回当前时间（毫秒）。
GTEST_API_ TimeInMillis GetTimeInMillis();

//返回真正的iff google测试应该在输出中使用颜色。
GTEST_API_ bool ShouldUseColor(bool stdout_is_tty);

//以毫秒为单位将给定时间格式化为秒。
GTEST_API_ std::string FormatTimeInMillisAsSeconds(TimeInMillis ms);

//将给定时间（毫秒）转换为ISO 8601中的日期字符串
//格式，不包含时区信息。注意：由于使用
//不可重入的localtime（）函数，此函数不是线程安全的。做
//不要在任何可以从多个线程调用的代码中使用它。
GTEST_API_ std::string FormatEpochTimeInMillisAsIso8601(TimeInMillis ms);

//以“--flag=value”的形式分析Int32标志的字符串。
//
//成功后，将标志的值存储在*value中，并返回
//真的。失败时，返回FALSE而不更改*值。
GTEST_API_ bool ParseInt32Flag(
    const char* str, const char* flag, Int32* value);

//根据返回范围[1，kmaxrandomseed]中的随机种子
//给定--gtest_random_seed标志值。
inline int GetRandomSeedFromFlag(Int32 random_seed_flag) {
  const unsigned int raw_seed = (random_seed_flag == 0) ?
      static_cast<unsigned int>(GetTimeInMillis()) :
      static_cast<unsigned int>(random_seed_flag);

//将实际种子规格化为范围[1，kmaxrandomseed]，以便
//打字很容易。
  const int normalized_seed =
      static_cast<int>((raw_seed - 1U) %
                       static_cast<unsigned int>(kMaxRandomSeed)) + 1;
  return normalized_seed;
}

//返回“seed”之后的第一个有效随机种子。行为是
//如果“seed”无效，则未定义。kmaxrandomseed之后的种子是
//被认为是1。
inline int GetNextRandomSeed(int seed) {
  GTEST_CHECK_(1 <= seed && seed <= kMaxRandomSeed)
      << "Invalid random seed " << seed << " - must be in [1, "
      << kMaxRandomSeed << "].";
  const int next_seed = seed + 1;
  return (next_seed > kMaxRandomSeed) ? 1 : next_seed;
}

//此类将所有Google测试标志的值保存在其职责中，并且
//将它们恢复到它的作用域。
class GTestFlagSaver {
 public:
//警察。
  GTestFlagSaver() {
    also_run_disabled_tests_ = GTEST_FLAG(also_run_disabled_tests);
    break_on_failure_ = GTEST_FLAG(break_on_failure);
    catch_exceptions_ = GTEST_FLAG(catch_exceptions);
    color_ = GTEST_FLAG(color);
    death_test_style_ = GTEST_FLAG(death_test_style);
    death_test_use_fork_ = GTEST_FLAG(death_test_use_fork);
    filter_ = GTEST_FLAG(filter);
    internal_run_death_test_ = GTEST_FLAG(internal_run_death_test);
    list_tests_ = GTEST_FLAG(list_tests);
    output_ = GTEST_FLAG(output);
    print_time_ = GTEST_FLAG(print_time);
    random_seed_ = GTEST_FLAG(random_seed);
    repeat_ = GTEST_FLAG(repeat);
    shuffle_ = GTEST_FLAG(shuffle);
    stack_trace_depth_ = GTEST_FLAG(stack_trace_depth);
    stream_result_to_ = GTEST_FLAG(stream_result_to);
    throw_on_failure_ = GTEST_FLAG(throw_on_failure);
  }

//任务不是虚拟的。不要从此类继承。
  ~GTestFlagSaver() {
    GTEST_FLAG(also_run_disabled_tests) = also_run_disabled_tests_;
    GTEST_FLAG(break_on_failure) = break_on_failure_;
    GTEST_FLAG(catch_exceptions) = catch_exceptions_;
    GTEST_FLAG(color) = color_;
    GTEST_FLAG(death_test_style) = death_test_style_;
    GTEST_FLAG(death_test_use_fork) = death_test_use_fork_;
    GTEST_FLAG(filter) = filter_;
    GTEST_FLAG(internal_run_death_test) = internal_run_death_test_;
    GTEST_FLAG(list_tests) = list_tests_;
    GTEST_FLAG(output) = output_;
    GTEST_FLAG(print_time) = print_time_;
    GTEST_FLAG(random_seed) = random_seed_;
    GTEST_FLAG(repeat) = repeat_;
    GTEST_FLAG(shuffle) = shuffle_;
    GTEST_FLAG(stack_trace_depth) = stack_trace_depth_;
    GTEST_FLAG(stream_result_to) = stream_result_to_;
    GTEST_FLAG(throw_on_failure) = throw_on_failure_;
  }

 private:
//用于保存标志原始值的字段。
  bool also_run_disabled_tests_;
  bool break_on_failure_;
  bool catch_exceptions_;
  std::string color_;
  std::string death_test_style_;
  bool death_test_use_fork_;
  std::string filter_;
  std::string internal_run_death_test_;
  bool list_tests_;
  std::string output_;
  bool print_time_;
  internal::Int32 random_seed_;
  internal::Int32 repeat_;
  bool shuffle_;
  internal::Int32 stack_trace_depth_;
  std::string stream_result_to_;
  bool throw_on_failure_;
} GTEST_ATTRIBUTE_UNUSED_;

//以UTF-8编码将Unicode码位转换为窄字符串。
//代码点参数的类型为uint32，因为wchar_t可能不是
//足够宽以包含代码点。
//如果代码点不是有效的Unicode代码点
//（即超出Unicode范围U+0至U+10ffff）将被转换。
//到“（无效的Unicode 0xXXXXXXXX）”。
GTEST_API_ std::string CodePointToUtf8(UInt32 code_point);

//以UTF-8编码将宽字符串转换为窄字符串。
//假定宽字符串具有以下编码：
//utf-16 if sizeof（wchar_t）==2（在Windows、Cygwin、Symbian OS上）
//utf-32 if sizeof（wchar_t）==4（在Linux上）
//参数str指向以空结尾的宽字符串。
//参数num_chars还可以限制数字
//已处理的wchar字符数。当整个字符串
//应该处理。
//如果字符串包含无效的Unicode代码点
//（即超出Unicode范围U+0至U+10ffff）它们将被输出
//作为'（无效的Unicode 0xXXXXXXXX）'。如果字符串采用UTF16编码
//并且包含无效的UTF-16代理项对，这些对中的值
//将被编码为基本正常平面上的单个Unicode字符。
GTEST_API_ std::string WideStringToUtf8(const wchar_t* str, int num_chars);

//读取gtest-shard-u-status-file环境变量，并创建该文件
//如果变量存在。如果此位置已存在文件，则
//函数将重写它。如果变量存在，但文件不能
//创建、打印错误并退出。
void WriteToShardStatusFileIfNeeded();

//通过检查相关的
//环境变量值。如果变量存在，
//但不一致（例如shard_index>=total_shards），打印
//一个错误并退出。如果在“死亡”测试的“子过程”中，切分是
//已禁用，因为它只能应用于原始测试
//过程。否则，我们可以过滤掉我们打算执行的死亡测试。
GTEST_API_ bool ShouldShard(const char* total_shards_str,
                            const char* shard_index_str,
                            bool in_subprocess_for_death_test);

//将环境变量var解析为int32。如果未设置，
//返回默认值。如果它不是Int32，则打印错误并
//流产。
GTEST_API_ Int32 Int32FromEnvOrDie(const char* env_var, Int32 default_val);

//给定碎片总数、碎片索引和测试ID，
//返回true iff测试应该在此碎片上运行。测试ID为
//分配给每个测试的一些任意但唯一的非负整数
//方法。假设0<=shard_index<total_shards。
GTEST_API_ bool ShouldRunTestOnShard(
    int total_shards, int shard_index, int test_id);

//STL容器实用程序。

//返回给定容器中满足以下条件的元素数
//给定的谓词。
template <class Container, typename Predicate>
inline int CountIf(const Container& c, Predicate predicate) {
//自上的libcstd中的std：：count_if（）起作为显式循环实现
//Solaris具有非标准签名。
  int count = 0;
  for (typename Container::const_iterator it = c.begin(); it != c.end(); ++it) {
    if (predicate(*it))
      ++count;
  }
  return count;
}

//对容器中的每个元素应用函数/函数。
template <class Container, typename Functor>
void ForEach(const Container& c, Functor functor) {
  std::for_each(c.begin(), c.end(), functor);
}

//返回向量的第i个元素，如果不是，则返回默认值
//范围[0，v.size（））。
template <typename E>
inline E GetElementOr(const std::vector<E>& v, int i, E default_value) {
  return (i < 0 || i >= static_cast<int>(v.size())) ? default_value : v[i];
}

//执行向量元素范围的就地无序移动。
//“begin”和“end”是作为STL样式范围的元素索引；
//即[开始，结束]是无序排列的，其中“结束”=size（）表示
//移动到矢量的末尾。
template <typename E>
void ShuffleRange(internal::Random* random, int begin, int end,
                  std::vector<E>* v) {
  const int size = static_cast<int>(v->size());
  GTEST_CHECK_(0 <= begin && begin <= size)
      << "Invalid shuffle range start " << begin << ": must be in range [0, "
      << size << "].";
  GTEST_CHECK_(begin <= end && end <= size)
      << "Invalid shuffle range finish " << end << ": must be in range ["
      << begin << ", " << size << "].";

//费希尔·耶茨洗牌，从
//http://en.wikipedia.org/wiki/fisher-yates-shuffle网站
  for (int range_width = end - begin; range_width >= 2; range_width--) {
    const int last_in_range = begin + range_width - 1;
    const int selected = begin + random->Generate(range_width);
    std::swap((*v)[selected], (*v)[last_in_range]);
  }
}

//对向量的元素执行就地无序排列。
template <typename E>
inline void Shuffle(internal::Random* random, std::vector<E>* v) {
  ShuffleRange(random, 0, static_cast<int>(v->size()), v);
}

//用于删除对象的函数。方便用作
//函子
template <typename T>
static void Delete(T* x) {
  delete x;
}

//根据已知键检查testproperty的键的谓词。
//
//testpropertykeyis是可复制的。
class TestPropertyKeyIs {
 public:
//构造函数。
//
//testpropertykeyis没有默认的构造函数。
  explicit TestPropertyKeyIs(const std::string& key) : key_(key) {}

//如果测试属性的测试名称与键\匹配，则返回true。
  bool operator()(const TestProperty& test_property) const {
    return test_property.key() == key_;
  }

 private:
  std::string key_;
};

//类UnitTestOptions。
//
//此类包含用于处理用户
//指定运行测试时。它只有静态成员。
//
//在大多数情况下，用户可以使用
//环境变量或命令行标志。例如，您可以设置
//使用gtest_筛选器或--gtest_筛选器测试筛选器。如果两者
//变量和标志存在，后者重写
//前者。
class GTEST_API_ UnitTestOptions {
 public:
//用于处理gtest_输出标志的函数。

//返回输出格式，对于正常打印输出，返回“”。
  static std::string GetOutputFormat();

//返回请求的输出文件的绝对路径，或
//默认（原始工作目录中的test_detail.xml）如果
//未显式指定。
  static std::string GetAbsolutePathToOutputFile();

//用于处理gtest_筛选器标志的函数。

//如果通配符模式与字符串匹配，则返回true。这个
//模式中的第一个“：”或“\0”字符标记它的结尾。
//
//这个递归算法不是很有效，但是很清楚
//很好地匹配测试名称，这是很短的。
  static bool PatternMatchesString(const char *pattern, const char *str);

//返回true iff用户指定的筛选器与测试用例匹配
//名称和测试名称。
  static bool FilterMatchesTest(const std::string &test_case_name,
                                const std::string &test_name);

#if GTEST_OS_WINDOWS
//用于支持gtest_catch_异常标志的函数。

//如果google测试应该处理
//给定SEH异常，或异常继续搜索。
//此函数用作除条件之外的其他条件。
  static int GTestShouldProcessSEH(DWORD exception_code);
#endif  //GTEST操作系统窗口

//如果“name”与全局样式的“：”分隔列表匹配，则返回true
//“过滤器”中的过滤器。
  static bool MatchesFilter(const std::string& name, const char* filter);
};

//返回当前应用程序的名称，如果是，则删除目录路径
//是存在的。由UnitTestOptions:：GetOutputfile使用。
GTEST_API_ FilePath GetCurrentExecutableName();

//用于将OS堆栈跟踪作为字符串获取的角色接口。
class OsStackTraceGetterInterface {
 public:
  OsStackTraceGetterInterface() {}
  virtual ~OsStackTraceGetterInterface() {}

//以std：：字符串形式返回当前操作系统堆栈跟踪。参数：
//
//最大深度-要包含的最大堆栈帧数
//在痕迹中。
//Skip_Count-要跳过的顶层帧的数目；不计算
//相对于最大深度。
  virtual string CurrentStackTrace(int max_depth, int skip_count) = 0;

//upOnLeavinggtest（）应在Google测试调用之前立即调用
//用户代码。它保存了一些关于当前堆栈的信息，
//currentStackTrace（）将用于查找和隐藏Google测试堆栈帧。
  virtual void UponLeavingGTest() = 0;

 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(OsStackTraceGetterInterface);
};

//osstacktracegetterinterface接口的工作实现。
class OsStackTraceGetter : public OsStackTraceGetterInterface {
 public:
  OsStackTraceGetter() : caller_frame_(NULL) {}

  virtual string CurrentStackTrace(int max_depth, int skip_count)
      GTEST_LOCK_EXCLUDED_(mutex_);

  virtual void UponLeavingGTest() GTEST_LOCK_EXCLUDED_(mutex_);

//插入此字符串以代替作为
//谷歌测试的实现。
  static const char* const kElidedFramesMarker;

 private:
Mutex mutex_;  //保护所有内部状态

//我们将堆栈帧保存在调用用户代码的帧下面。
//我们这样做是因为下面框架的地址
//用户代码在调用upOnLeavingGTest（）之间发生更改
//以及从用户代码中调用currentStackTrace（）。
  void* caller_frame_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(OsStackTraceGetter);
};

//关于谷歌测试跟踪点的信息。
struct TraceInfo {
  const char* file;
  int line;
  std::string message;
};

//这是UnitTestImpl中使用的默认全局测试部件结果报告程序。
//此类只能由UnitTestImpl使用。
class DefaultGlobalTestPartResultReporter
  : public TestPartResultReporterInterface {
 public:
  explicit DefaultGlobalTestPartResultReporter(UnitTestImpl* unit_test);
//实现TestPartResultReporterInterface。报告测试部件
//当前测试的结果。
  virtual void ReportTestPartResult(const TestPartResult& result);

 private:
  UnitTestImpl* const unit_test_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(DefaultGlobalTestPartResultReporter);
};

//这是中使用的每个线程测试部分结果报告器的默认值
//附录。此类只能由UnitTestImpl使用。
class DefaultPerThreadTestPartResultReporter
    : public TestPartResultReporterInterface {
 public:
  explicit DefaultPerThreadTestPartResultReporter(UnitTestImpl* unit_test);
//实现TestPartResultReporterInterface。实施只是
//委托给“单元测试”的当前全局测试部件结果报告程序。
  virtual void ReportTestPartResult(const TestPartResult& result);

 private:
  UnitTestImpl* const unit_test_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(DefaultPerThreadTestPartResultReporter);
};

//UnitTest类的私有实现。我们不保护
//互斥体下的方法，因为此类不可由
//用户和委托此类工作的UnitTest类
//适当的锁定。
class GTEST_API_ UnitTestImpl {
 public:
  explicit UnitTestImpl(UnitTest* parent);
  virtual ~UnitTestImpl();

//注册自己的testpartResultReporter有两种不同的方法。
//您可以注册自己的报告，以便只听取测试结果。
//
//默认情况下，每个线程的测试结果报告只传递一个新的
//向全局测试结果报告器发送testpartresult，该报告器注册
//当前运行测试的测试部件结果。

//返回全局测试部件结果报告器。
  TestPartResultReporterInterface* GetGlobalTestPartResultReporter();

//设置全局测试部件结果报告器。
  void SetGlobalTestPartResultReporter(
      TestPartResultReporterInterface* reporter);

//返回当前线程的测试部件结果报告器。
  TestPartResultReporterInterface* GetTestPartResultReporterForCurrentThread();

//为当前线程设置测试部件结果报告器。
  void SetTestPartResultReporterForCurrentThread(
      TestPartResultReporterInterface* reporter);

//获取成功的测试用例数。
  int successful_test_case_count() const;

//获取失败的测试用例数。
  int failed_test_case_count() const;

//获取所有测试用例的数目。
  int total_test_case_count() const;

//获取包含至少一个测试的所有测试用例的数目
//应该可以。
  int test_case_to_run_count() const;

//获取成功的测试数。
  int successful_test_count() const;

//获取失败的测试数。
  int failed_test_count() const;

//获取将在XML报告中报告的禁用测试数。
  int reportable_disabled_test_count() const;

//获取禁用的测试数。
  int disabled_test_count() const;

//获取要在XML报表中打印的测试数。
  int reportable_test_count() const;

//获取所有测试的数目。
  int total_test_count() const;

//获取应运行的测试数。
  int test_to_run_count() const;

//获取测试程序的启动时间，从
//UNIX时代
  TimeInMillis start_timestamp() const { return start_timestamp_; }

//获取已用时间（毫秒）。
  TimeInMillis elapsed_time() const { return elapsed_time_; }

//如果单元测试通过（即所有测试用例通过），则返回true。
  bool Passed() const { return !Failed(); }

//如果单元测试失败（即某些测试用例失败），则返回true
//或者所有测试之外的东西都失败了）。
  bool Failed() const {
    return failed_test_case_count() > 0 || ad_hoc_test_result()->Failed();
  }

//获取所有测试用例中的第i个测试用例。我可以从0到
//总测试用例计数（）-1。如果我不在该范围内，则返回空值。
  const TestCase* GetTestCase(int i) const {
    const int index = GetElementOr(test_case_indices_, i, -1);
    return index < 0 ? NULL : test_cases_[i];
  }

//获取所有测试用例中的第i个测试用例。我可以从0到
//总测试用例计数（）-1。如果我不在该范围内，则返回空值。
  TestCase* GetMutableTestCase(int i) {
    const int index = GetElementOr(test_case_indices_, i, -1);
    return index < 0 ? NULL : test_cases_[index];
  }

//提供对事件侦听器列表的访问。
  TestEventListeners* listeners() { return &listeners_; }

//返回当前正在运行的测试的测试结果，或者
//如果没有运行任何测试，则为即席测试的测试结果。
  TestResult* current_test_result();

//返回临时测试的测试结果。
  const TestResult* ad_hoc_test_result() const { return &ad_hoc_test_result_; }

//设置OS堆栈跟踪getter。
//
//如果输入和当前OS堆栈跟踪getter
//相同；否则，删除旧的getter并使
//输入电流吸气剂。
  void set_os_stack_trace_getter(OsStackTraceGetterInterface* getter);

//如果当前OS堆栈跟踪getter不为空，则返回该getter；
//否则，创建osstacktracegetter，使其成为当前
//把它拿回来。
  OsStackTraceGetterInterface* os_stack_trace_getter();

//以std：：字符串形式返回当前操作系统堆栈跟踪。
//
//要包含的最大堆栈帧数由指定
//gtest堆栈跟踪深度标志。skip_count参数
//指定要跳过的顶层帧的数目，但不指定
//根据要包含的帧数计数。
//
//例如，如果foo（）调用bar（），而bar（）又调用
//当前osstacktraceexcepttop（1），foo（）将包含在
//trace但bar（）和currentOsstackTraceExceptTop（）不会。
  std::string CurrentOsStackTraceExceptTop(int skip_count) GTEST_NO_INLINE_;

//查找并返回具有给定名称的测试用例。如果没有
//存在，创建一个并返回它。
//
//争论：
//
//测试用例名称：测试用例的名称
//type_param：测试类型参数的名称，如果
//这不是类型化测试或类型参数化测试。
//设置\u tc:指向设置测试用例的函数的指针
//分解：指向分解测试用例的函数的指针
  TestCase* GetTestCase(const char* test_case_name,
                        const char* type_param,
                        Test::SetUpTestCaseFunc set_up_tc,
                        Test::TearDownTestCaseFunc tear_down_tc);

//将testinfo添加到单元测试中。
//
//争论：
//
//设置\u tc:指向设置测试用例的函数的指针
//分解：指向分解测试用例的函数的指针
//测试信息：test info对象
  void AddTestInfo(Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc,
                   TestInfo* test_info) {
//为了支持线程安全死亡测试，我们需要
//当测试程序
//第一次被调用。我们不能在run_all_tests（）中这样做，因为
//用户可能在调用之前更改了当前目录
//运行所有测试（）。因此，我们捕获当前目录
//addtestinfo（），用于注册测试或测试
//在到达main（）之前。
    if (original_working_dir_.IsEmpty()) {
      original_working_dir_.Set(FilePath::GetCurrentDir());
      GTEST_CHECK_(!original_working_dir_.IsEmpty())
          << "Failed to get the current working directory.";
    }

    GetTestCase(test_info->test_case_name(),
                test_info->type_param(),
                set_up_tc,
                tear_down_tc)->AddTestInfo(test_info);
  }

#if GTEST_HAS_PARAM_TEST
//返回用于跟踪
//值参数化测试并实例化和注册它们。
  internal::ParameterizedTestCaseRegistry& parameterized_test_registry() {
    return parameterized_test_registry_;
  }
#endif  //gtest_有参数测试

//为当前正在运行的测试设置testcase对象。
  void set_current_test_case(TestCase* a_current_test_case) {
    current_test_case_ = a_current_test_case;
  }

//为当前正在运行的测试设置testinfo对象。如果
//当前的测试信息为空，断言结果将存储在
//特别测试结果。
  void set_current_test_info(TestInfo* a_current_test_info) {
    current_test_info_ = a_current_test_info;
  }

//注册使用test_p和定义的所有参数化测试
//实例化测试用例，为每个测试/参数创建常规测试
//组合。此方法可以多次调用；它具有保护
//防止多次注册测试。如果
//值参数化测试被禁用，注册表参数化测试为
//在场但什么都不做。
  void RegisterParameterizedTests();

//运行此UnitTest对象中的所有测试，打印结果，以及
//如果所有测试都成功，则返回true。如果有例外
//在测试期间引发，此测试被视为失败，但
//其余的测试仍将运行。
  bool RunAllTests();

//清除所有测试的结果，特别测试除外。
  void ClearNonAdHocTestResult() {
    ForEach(test_cases_, TestCase::ClearTestCaseResult);
  }

//清除特殊测试断言的结果。
  void ClearAdHocTestResult() {
    ad_hoc_test_result_.Clear();
  }

//在
//测试、测试用例或全局属性集的上下文。如果
//结果已包含具有相同键的属性，该值将为
//更新。
  void RecordProperty(const TestProperty& test_property);

  enum ReactionToSharding {
    HONOR_SHARDING_PROTOCOL,
    IGNORE_SHARDING_PROTOCOL
  };

//根据指定的用户匹配每个测试的全名
//筛选以确定是否应运行测试，然后记录
//在每个testcase和testinfo对象中生成结果。
//如果shard_tests==honour_sharding_协议，则进一步过滤测试
//基于环境中的切分变量。
//返回应运行的测试数。
  int FilterTests(ReactionToSharding shard_tests);

//打印与用户指定的筛选标志匹配的测试的名称。
  void ListTestsMatchingFilter();

  const TestCase* current_test_case() const { return current_test_case_; }
  TestInfo* current_test_info() { return current_test_info_; }
  const TestInfo* current_test_info() const { return current_test_info_; }

//返回需要设置/分解的环境向量
//在运行测试之前/之后。
  std::vector<Environment*>& environments() { return environments_; }

//每个线程的google测试跟踪堆栈的getter。
  std::vector<TraceInfo>& gtest_trace_stack() {
    return *(gtest_trace_stack_.pointer());
  }
  const std::vector<TraceInfo>& gtest_trace_stack() const {
    return gtest_trace_stack_.get();
  }

#if GTEST_HAS_DEATH_TEST
  void InitDeathTestSubprocessControlInfo() {
    internal_run_death_test_flag_.reset(ParseInternalRunDeathTestFlag());
  }
//返回指向已分析--gtest_内部_运行_死亡_测试的指针
//标志，如果未指定该标志，则为空。
//此信息仅在死亡测试子进程中有用。
//在调用initgoogletest之前不能调用。
  const InternalRunDeathTestFlag* internal_run_death_test_flag() const {
    return internal_run_death_test_flag_.get();
  }

//返回指向当前死亡测试工厂的指针。
  internal::DeathTestFactory* death_test_factory() {
    return death_test_factory_.get();
  }

  void SuppressTestEventsIfInSubprocess();

  friend class ReplaceDeathTestFactory;
#endif  //GTEST有死亡测试

//按照指定的方式初始化执行XML输出的事件侦听器
//单元测试选项。在initgoogletest之前不能调用。
  void ConfigureXmlOutput();

#if GTEST_CAN_STREAM_RESULTS_
//将流式测试结果的事件侦听器初始化为套接字。
//在initgoogletest之前不能调用。
  void ConfigureStreamingOutput();
#endif

//根据在中获取的标志值执行初始化
//仅分析GoogleStestFlagsOnly。在调用后从initgoogletest调用
//仅分析GoogleStestFlagsOnly。如果用户忽略调用initgoogletest
//此函数也从runalltests调用。因为这个函数可以
//多次调用，它必须是等幂的。
  void PostFlagParsingInit();

//获取当前测试迭代开始时使用的随机种子。
  int random_seed() const { return random_seed_; }

//获取随机数生成器。
  internal::Random* random() { return &random_; }

//洗牌所有测试用例，以及每个测试用例中的测试，
//确保先进行死亡测试。
  void ShuffleTests();

//将测试用例和测试恢复到第一次随机播放之前的顺序。
  void UnshuffleTests();

//返回当前gtest_标志（catch_exceptions）的值
//UnitTest:：Run（）启动。
  bool catch_exceptions() const { return catch_exceptions_; }

 private:
  friend class ::testing::UnitTest;

//由UnitTest:：Run（）用于捕获的状态
//启动时的gtest_标志（catch_exceptions）。
  void set_catch_exceptions(bool value) { catch_exceptions_ = value; }

//拥有此实现对象的UnitTest对象。
  UnitTest* const parent_;

//当第一个test（）或test_f（）是
//执行。
  internal::FilePath original_working_dir_;

//默认测试部件结果报告器。
  DefaultGlobalTestPartResultReporter default_global_test_part_result_reporter_;
  DefaultPerThreadTestPartResultReporter
      default_per_thread_test_part_result_reporter_;

//指向（但不拥有）全局测试部件结果报告器。
  TestPartResultReporterInterface* global_test_part_result_repoter_;

//保护对全局测试部分结果报告器的读写访问。
  internal::Mutex global_test_part_result_reporter_mutex_;

//指向（但不拥有）每线程测试部分结果报告器。
  internal::ThreadLocal<TestPartResultReporterInterface*>
      per_thread_test_part_result_reporter_;

//需要设置/分解的环境向量
//在运行测试之前/之后。
  std::vector<Environment*> environments_;

//测试用例原始顺序的向量。它拥有
//向量中的元素。
  std::vector<TestCase*> test_cases_;

//提供测试用例列表允许的间接级别
//简单的洗牌和恢复测试用例顺序。第i个
//这个向量的元素是第i个测试用例的索引
//
  std::vector<int> test_case_indices_;

#if GTEST_HAS_PARAM_TEST
//用于注册参数化值的ParameteredTregistry对象
//测验。
  internal::ParameterizedTestCaseRegistry parameterized_test_registry_;

//指示是否已调用RegisterParameterizedTests（）。
  bool parameterized_tests_registered_;
#endif  //gtest_有参数测试

//上次注册的死亡测试案例的索引。最初- 1。
  int last_death_test_case_;

//这指向当前运行的测试的测试用例。它
//
//当没有测试运行时，它被设置为空，并且google测试
//将断言结果存储在特别测试结果中。最初是空的。
  TestCase* current_test_case_;

//这指向当前运行的测试的testinfo。它
//随着谷歌测试一个接一个测试的进行而改变。什么时候？
//没有测试正在运行，这被设置为空，并且Google测试存储
//断言结果在即席测试结果中。最初是空的。
  TestInfo* current_test_info_;

//通常，用户只在测试或测试中编写断言，
//或者在测试或测试调用的函数中。因为google
//测试跟踪当前正在运行的测试，它可以
//将这样的断言与它所属的测试关联起来。
//
//如果在没有运行测试或测试时遇到断言，
//谷歌测试将断言结果归为假想的“临时”结果。
//测试，并将结果记录在特别测试结果中。
  TestResult ad_hoc_test_result_;

//可用于跟踪内部事件的事件侦听器列表
//谷歌测试。
  TestEventListeners listeners_;

//操作系统堆栈跟踪getter。当UnitTest
//对象已销毁。默认情况下，使用osstacktracegetter，
//但是用户可以将此字段设置为使用自定义getter，如果这是
//渴望的。
  OsStackTraceGetterInterface* os_stack_trace_getter_;

//已调用了true iff postfagParsingInit（）。
  bool post_flag_parse_init_performed_;

//在测试运行开始时使用的随机数种子。
  int random_seed_;

//我们的随机数生成器。
  internal::Random random_;

//测试程序启动时间，以ms为单位
//UNIX时代
  TimeInMillis start_timestamp_;

//测试运行的时间（毫秒）。
  TimeInMillis elapsed_time_;

#if GTEST_HAS_DEATH_TEST
//gtest_internal_run_death_测试标志的分解组件，
//在调用运行所有测试时进行分析。
  internal::scoped_ptr<InternalRunDeathTestFlag> internal_run_death_test_flag_;
  internal::scoped_ptr<internal::DeathTestFactory> death_test_factory_;
#endif  //GTEST有死亡测试

//由作用域跟踪（）宏创建的每个线程的跟踪堆栈。
  internal::ThreadLocal<std::vector<TraceInfo> > gtest_trace_stack_;

//当前runalltests（）的gtest_标志（catch_exceptions）的值
//开始。
  bool catch_exceptions_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(UnitTestImpl);
};  //类UnitTestImpl

//访问全局UnitTest的便利功能
//实现对象。
inline UnitTestImpl* GetUnitTestImpl() {
  return UnitTest::GetInstance()->impl();
}

#if GTEST_USES_SIMPLE_RE

//用于实现简单规则的内部助手函数
//表达式匹配器。
GTEST_API_ bool IsInSet(char ch, const char* str);
GTEST_API_ bool IsAsciiDigit(char ch);
GTEST_API_ bool IsAsciiPunct(char ch);
GTEST_API_ bool IsRepeat(char ch);
GTEST_API_ bool IsAsciiWhiteSpace(char ch);
GTEST_API_ bool IsAsciiWordChar(char ch);
GTEST_API_ bool IsValidEscape(char ch);
GTEST_API_ bool AtomMatchesChar(bool escaped, char pattern, char ch);
GTEST_API_ bool ValidateRegex(const char* regex);
GTEST_API_ bool MatchRegexAtHead(const char* regex, const char* str);
GTEST_API_ bool MatchRepetitionAndRegexAtHead(
    bool escaped, char ch, char repeat, const char* regex, const char* str);
GTEST_API_ bool MatchRegexAnywhere(const char* regex, const char* str);

#endif  //GTEST使用简单的

//解析Google测试标志的命令行，而不初始化
//谷歌测试的其他部分。
GTEST_API_ void ParseGoogleTestFlagsOnly(int* argc, char** argv);
GTEST_API_ void ParseGoogleTestFlagsOnly(int* argc, wchar_t** argv);

#if GTEST_HAS_DEATH_TEST

//返回描述上一个系统错误的消息，无论
//平台。
GTEST_API_ std::string GetLastErrnoDescription();

//尝试将字符串解析为正整数
//数字参数。如果可能，返回true。
//gtest_has_death_test表示我们有：：std:：string，因此我们可以使用
//它在这里。
template <typename Integer>
bool ParseNaturalNumber(const ::std::string& str, Integer* number) {
//如果给定的字符串不是以数字开头，则快速失败；
//这将绕过strtoxx的“可选前导空格和加号”
//或者减号的语义，这在这里是不可取的。
  if (str.empty() || !IsDigit(str[0])) {
    return false;
  }
  errno = 0;

  char* end;
//biggestconvertible是系统提供的最大整数类型
//字符串到数字的转换例程可以返回。

# if GTEST_OS_WINDOWS && !defined(__GNUC__)

//MSVC和C++Builder定义了γ-It64，而不是标准的长长。
  typedef unsigned __int64 BiggestConvertible;
  const BiggestConvertible parsed = _strtoui64(str.c_str(), &end, 10);

# else

typedef unsigned long long BiggestConvertible;  //诺林
  const BiggestConvertible parsed = strtoull(str.c_str(), &end, 10);

# endif  //测试Windows&！定义（uu gnuc_uuu）

  const bool parse_success = *end == '\0' && errno == 0;

//TODO（vladl@google.com）：将其转换为编译时断言
//可用。
  GTEST_CHECK_(sizeof(Integer) <= sizeof(parsed));

  const Integer result = static_cast<Integer>(parsed);
  if (parse_success && static_cast<BiggestConvertible>(result) == parsed) {
    *number = result;
    return true;
  }
  return false;
}
#endif  //GTEST有死亡测试

//测试结果包含一些应该隐藏的私有方法
//谷歌测试用户，但测试需要。这门课允许我们考试
//访问它们。
//
//此类仅用于测试Google测试自己的
//构造。不要直接或间接地在用户测试中使用它。
class TestResultAccessor {
 public:
  static void RecordProperty(TestResult* test_result,
                             const std::string& xml_element,
                             const TestProperty& property) {
    test_result->RecordProperty(xml_element, property);
  }

  static void ClearTestPartResults(TestResult* test_result) {
    test_result->ClearTestPartResults();
  }

  static const std::vector<testing::TestPartResult>& test_part_results(
      const TestResult& test_result) {
    return test_result.test_part_results();
  }
};

#if GTEST_CAN_STREAM_RESULTS_

//将测试结果流式传输到给定主机上的给定端口。
class StreamingListener : public EmptyTestEventListener {
 public:
//用于将字符串写入套接字的抽象基类。
  class AbstractSocketWriter {
   public:
    virtual ~AbstractSocketWriter() {}

//向套接字发送字符串。
    virtual void Send(const string& message) = 0;

//关闭插座。
    virtual void CloseConnection() {}

//向套接字发送字符串和换行符。
    void SendLn(const string& message) {
      Send(message + "\n");
    }
  };

//用于将字符串实际写入套接字的具体类。
  class SocketWriter : public AbstractSocketWriter {
   public:
    SocketWriter(const string& host, const string& port)
        : sockfd_(-1), host_name_(host), port_num_(port) {
      MakeConnection();
    }

    virtual ~SocketWriter() {
      if (sockfd_ != -1)
        CloseConnection();
    }

//向套接字发送字符串。
    virtual void Send(const string& message) {
      GTEST_CHECK_(sockfd_ != -1)
          << "Send() can be called only when there is a connection.";

      const int len = static_cast<int>(message.length());
      if (write(sockfd_, message.c_str(), len) != len) {
        GTEST_LOG_(WARNING)
            << "stream_result_to: failed to stream to "
            << host_name_ << ":" << port_num_;
      }
    }

   private:
//创建客户端套接字并连接到服务器。
    void MakeConnection();

//关闭插座。
    void CloseConnection() {
      GTEST_CHECK_(sockfd_ != -1)
          << "CloseConnection() can be called only when there is a connection.";

      close(sockfd_);
      sockfd_ = -1;
    }

int sockfd_;  //套接口描述字
    const string host_name_;
    const string port_num_;

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SocketWriter);
};  //类SocketWriter

//将str中的'='、'&'、'%'和'\n'字符转义为“%xx”。
  static string UrlEncode(const char* str);

  StreamingListener(const string& host, const string& port)
      : socket_writer_(new SocketWriter(host, port)) { Start(); }

  explicit StreamingListener(AbstractSocketWriter* socket_writer)
      : socket_writer_(socket_writer) { Start(); }

  /*内容程序启动（const unit test&/*单元测试*/）
    sendln（“event=testprogramstart”）；
  }

  void ontestprogrammend（const unit test&unit_test）
    //请注意，google test current只报告
    //测试迭代，不适用于整个测试程序。
    sendln（“event=testprogramend&passed=”+formatBool（unit_test.passed（））；

    //通知流服务器停止。
    socket_writer_u->closeConnection（）；
  }

  void ontestitutionstart（const unit test&/*单元测试*/, int iteration) {

    SendLn("event=TestIterationStart&iteration=" +
           StreamableToString(iteration));
  }

  /*内容结束（const unit test&unit_test，int/*迭代*/）
    sendln（“event=testerationend&passed=”+
           FormatBool（Unit_test.passed（））+“&elapsed_time=”+
           streamableToString（Unit_test.elapsed_time（））+“ms”）；
  }

  void ontestcasestart（const test case&test_case）
    sendln（std:：string（“event=testcasestart&name=”）+test_case.name（））；
  }

  void ontestcaseend（const test case&test_case）
    sendln（“event=testcaseend&passed=”+formatBool（test_case.passed（））
           +“&elapsed_time=”+streamableToString（test_case.elapsed_time（））
           +“ms”）；
  }

  void onteststart（const test info&test_info）
    sendln（std:：string（“event=teststart&name=”）+test_info.name（））；
  }

  void ontested（const test info&test_info）
    sendln（“event=testend&passed=”+
           formatBool（（test_info.result（））->passed（））+
           “&elapsed\u time=”+
           streamableToString（（test_info.result（））->elapsed_time（））+“ms”）；
  }

  void ontestpartresult（const test part result&test_part_result）
    const char*file_name=test_part_result.file_name（）；
    if（文件名=空）
      文件名=“”；
    sendln（“event=testpartresult&file=”+urlencode（文件名）+
           “&line=”+streamableToString（test_part_result.line_number（））+
           “&message=”+urlencode（test_part_result.message（））；
  }

 私人：
  //将给定的消息和换行符发送到套接字。
  void sendln（const string&message）socket_writer_u->sendln（message）；

  //在流开始时调用以通知接收器
  //我们使用的协议。
  void start（）sendln（“gtest_streaming_protocol_version=1.0”）；

  字符串格式bool（bool value）返回值？1”：“0”；

  const scoped_ptr<abstractSocketWriter>socket_writer_u；

  gtest不允许复制和分配（streaminglistener）；
//类流侦听器

endif//gtest_can_stream_results_

//命名空间内部
//命名空间测试

endif//gtest_src_gtest_internal_inl_h_
取消GTESTU实施

如果GTEST操作系统窗口
定义vsnprintf
endif//gtest_os_windows

命名空间测试

使用内部：：countif；
使用内部：：foreach；
使用内部：：getelementor；
使用内部：：shuffle；

/ /常量。

//其测试用例名称或测试名称与此筛选器匹配的测试是
//禁用，不运行。
static const char kdisabletestfilter[]=“已禁用”**/DISABLED_*";


//名称与此筛选器匹配的测试用例被视为死亡
//测试用例，并将在名称不为的测试用例之前运行
//匹配此筛选器。
/*tic const char kdeathestcasefilter[]=“*死亡测试：*死亡测试/*”；

//匹配所有内容的测试筛选器。
静态常量char kuniversalfilter[]=“*”；

//XML输出的默认输出文件。
static const char kdefaultoutputfile[]=“测试\u detail.xml”；

//测试碎片索引的环境变量名。
static const char ktestshardindindex[]=“GTest_shard_index”；
//测试碎片总数的环境变量名。
static const char ktesttotalshards[]=“GTest_total_shards”；
//测试碎片状态文件的环境变量名。
static const char ktestshardstatusfile[]=“GTest_shard_status_file”；

命名空间内部

//失败消息中用于指示
//堆栈跟踪。
const char kstacktracemarker[]=\n堆栈跟踪：\n”；

//如果--help标志或等效形式为
//在命令行上指定。
bool g_help_flag=false；

//命名空间内部

static const char*getdefaultfilter（）
  返回Kuniversalfilter；
}

gtest_define_bool_uu（
    同时运行禁用的测试，
    内部：：boolfromgtestenv（“还运行禁用的测试”，false），
    “除了正常运行的测试之外，还运行禁用的测试。”）；

gtest_define_bool_uu（
    失败时中断，
    内部：：boolfromgtestenv（“break_on_failure”，false），
    “如果失败的断言应为调试器断点，则为真。”）；

gtest_define_bool_uu（
    捕获异常，
    内部：：boolfromgtestenv（“catch_exceptions”，true），
    “真iff”gtest_name_
    “应捕获异常并将其视为测试失败。”）；

定义字符串
    颜色，
    内部：：StringFromGTestenv（“颜色”，“自动”），
    “是否在输出中使用颜色。有效值：是，否，，
    “还有汽车。”“自动”表示在输出为“时使用颜色”
    “发送到终端和术语环境变量”
    “设置为支持颜色的终端类型。”）；

定义字符串
    滤波器，
    内部：：StringFromGTestenv（“filter”，getDefaultFilter（）），
    “以冒号分隔的glob（非regex）模式列表”
    用于筛选要运行的测试，可选后跟
    “-”和A:负模式的分隔列表（测试到”
    “排除”。如果测试与其中一个阳性结果匹配，则运行该测试。”
    “模式和不匹配任何负模式。”）；

gtest定义bool（列出测试，错误，
                   “列出所有测试而不运行它们。”）；

定义字符串
    输出，
    内部：：StringFromGTestenv（“输出”，“），
    “一种格式（当前必须是\”xml\“），可以选择跟随”
    “通过冒号和输出文件名或目录。目录
    “由尾随的路径名分隔符指示。”
    “示例：”xml:filename.xml\“，\”xml:：directoryname/\“。”
    如果指定了目录，将创建输出文件
    在该目录中，使用基于测试的文件名
    可执行文件的名称，如有必要，通过添加使其唯一
    “数字”；

gtest_define_bool_uu（
    打印时间，
    内部：：boolfromgtestenv（“打印时间”，真），
    “真iff”gtest_name_
    “应在文本输出中显示经过的时间。”）；

gtest_define_int32_uu（
    随机种子，
    内部：：Int32FromGTestenv（“随机种子”，0），
    “洗牌测试订单时使用的随机数种子。必须在范围内”
    “[1，99999]或0根据当前时间使用种子。”）；

gtest_define_int32_uu（
    重复，
    内部：：Int32FromGTestenv（“重复”，1），
    “每次测试重复多少次。指定一个负数“
    “永远重复。有助于摆脱片状测试。

gtest_define_bool_uu（
    显示\内部\堆栈\帧，错误，
    “true iff”gtest“name”应包括内部堆栈帧，当”
    “打印测试失败堆栈跟踪。”）；

gtest_define_bool_uu（
    洗牌，
    内部：：boolfromgtestenv（“shuffle”，false），
    “真iff”gtest_name_
    “应随机化测试的每次运行顺序。”）；

gtest_define_int32_uu（
    叠加追踪深度，
    内部：：int32fromgtestenv（“堆栈跟踪深度”，kmaxstacktracedepth），
    “打印时堆栈帧的最大数目”
    “断言失败。有效范围为0到100，包含在内。“）；

定义字符串
    流结果到，
    内部：：StringFromGTestenv（“流结果\到”，“），
    “此标志指定要传输的主机名和端口号”
    “测试结果。例如：“localhost:555”。此标志仅在“上有效”
    “Linux”；

gtest_define_bool_uu（
    失败时抛出，
    内部：：boolfromgtestenv（“在失败时抛出”，错误），
    指定此标志时，失败的断言将引发异常
    “如果启用了异常或使用非零代码退出程序”
    “否则”

命名空间内部

//使用线性函数从[0，range]生成一个随机数
//同余生成器（lcg）。“range”为0或更大时崩溃
//大于kmaxrange。
uint32随机：：生成（uint32范围）
  //这些常量与glibc的rand（3）中使用的常量相同。
  State_u=（1103515245u*State_+12345u）%公里最大范围；

  gtest_检查（范围>0）
      <“无法生成范围[0，0]内的数字。”；
  gtest_check_u（范围<=kmaxrange）
      <“在[0]中生成一个数字，”<<range<<“）被请求，”
      <“但这只能在[0，”<<kmaxrange<<“）.

  //通过模数转换会带来一点向下的偏差，但是
  //很简单，线性同余生成器也不太好
  //首先。
  返回状态范围；
}

//gtestisinatized（）返回用户已初始化的true iff
//谷歌测试。有助于发现用户未初始化的错误
//在调用run_all_tests（）之前进行google测试。
/ /
//用户必须调用testing:：initgoogletest（）初始化google
/测试。g_init_gtest_count设置为次数
//已调用initgoogletest（）。我们不保护这个变量
//在互斥体下，因为它只在主线程中访问。
gtest_api_u int g_init_gtest_count=0；
静态bool gtestisinitiialized（）返回g_init_gtest_count！= 0；

//迭代测试用例的向量，并保持
//对每个调用给定int返回方法的结果。
//返回和。
静态int sumovertestcaselist（const std:：vector<testcase*>&case_list，
                               int（测试用例：：*方法）（）常量）
  In和= 0；
  对于（size_t i=0；i<case_list.size（）；i++）
    sum+=（case_list[i]->*方法）（）；
  }
  返回总和；
}

//如果测试用例通过，则返回true。
静态bool testcasepassed（const test case*测试用例）
  返回test_case->should_run（）&&test_case->passed（）；
}

//如果测试用例失败，则返回true。
静态bool testcasefailed（const test case*测试用例）
  返回test_case->should_run（）&&test_case->failed（）；
}

//返回真正的iff test_case，至少包含一个应该
/运行。
静态bool shouldruntestcase（const test case*测试用例）
  返回test_case->should_run（）；
}

//断言帮助器构造函数。
assertHelper:：assertHelper（testpartresult:：type类型，
                           const char*文件，
                           int行，
                           const char*消息）
    ：data_uu（new asserthelperdata（type，file，line，message））
}

assertHelper:：~assertHelper（）
  删除数据；
}

//消息分配，用于断言流支持。
void assertHelper:：operator=（const message&message）const_
  UnitTest:：GetInstance（）->
    addtestpartresult（数据_->type，数据_->file，数据_->line，
                      appendusermessage（数据->message，消息）
                      UnitTest:：GetInstance（）->Impl（））
                      ->当前跟踪例外项（1）
                      //跳过此函数本身的堆栈帧。
                      ）；/ /诺林
}

//链接指针的互斥体。
gtest_-api_gtest_-define_-static_-mutex_u（g_-linked_-ptr_-mutex）；

//在initgoogletest中获取了应用程序路径名。
std:：string g g_可执行文件路径；

//返回当前应用程序的名称，如果返回，则删除目录路径
/存在。
filePath getCurrentExecutableName（）
  文件路径结果；

如果GTEST操作系统窗口
  result.set（filepath（g_executable_path.removeextension（“exe”））；
否则
  result.set（文件路径（g_可执行文件路径））；
endif//gtest_os_windows

  返回result.removeDirectoryName（）；
}

//用于处理gtest_输出标志的函数。

//返回输出格式，对于正常打印输出，返回“”。
std:：string unittestOptions:：getOutputformat（）
  const char*const gtest_output_flag=gtest_flag（output）.c_str（）；
  if（gtest_output_flag==null）返回std:：string（“”）；

  const char*const colon=strchr（gtest_output_flag，：'）；
  返回（冒号=空）？
      std：：字符串（gtest_output_标志）：
      std:：string（gtest_output_标志，colon-gtest_output_标志）；
}

//返回请求的输出文件的名称，如果没有，则返回默认值
//已显式指定。
std:：string unittestOptions:：getAbsolutePathToOutputfile（）
  const char*const gtest_output_flag=gtest_flag（output）.c_str（）；
  if（gtest_output_flag==空）
    “返回”；

  const char*const colon=strchr（gtest_output_flag，：'）；
  if（冒号=空）
    返回内部：：文件路径：：concatpaths（
        内部：：文件路径（
            unittest:：getInstance（）->原始的工作目录（），
        内部：：filePath（kDefaultOutputfile））.string（）；

  内部：：文件路径输出\名称（冒号+1）；
  如果（！）output_name.isabsolutePath（））
    //TODO（wan@google.com）：在Windows上\some\path不是绝对路径
    //路径（其含义取决于当前驱动器），但是
    //以下将其转换为绝对路径的逻辑是错误的。
    /修理它。
    输出\名称=内部：：文件路径：：concatpaths（
        内部：：文件路径（UnitTest:：GetInstance（）->原始的工作目录（）），
        内部：：文件路径（冒号+1））；

  如果（！）输出_name.isdirectory（））
    返回输出_name.string（）；

  内部：：文件路径结果（内部：：文件路径：：GenerateUniqueFileName（
      输出名称，内部：：getcurrentExecutableName（），
      getoutputformat（）.c_str（））；
  返回result.string（）；
}

//如果通配符模式与字符串匹配，则返回true。这个
//模式中的第一个“：”或“\0”字符标记它的结尾。
/ /
//这个递归算法不是很有效，但是很清楚
//对于匹配测试名（即短测试名）来说已经足够好了。
bool unittestoptions:：patternmatchesstring（const char*模式，
                                           常量char*str）
  开关（*pattern）
    案例“0”：
    大小写“：”：/“：”或“\0”表示模式的结束。
      return*str='\0'；
    案例“？”：//匹配任何单个字符。
      返回* STR！='\0'&&patternMatchessString（pattern+1，str+1）；
    大小写“*”：//匹配任何字符串（可能为空）。
      返回（* STR！='\0'&&patternmatchesstring（pattern，str+1））
          模式匹配字符串（模式+1，str）；
    默认：/非特殊字符。匹配自己。
      返回*模式=*str&&
          pattern匹配字符串（pattern+1，str+1）；
  }
}

bool unittestOptions:：matchesFilter（）。
    const std：：字符串和名称，const char*筛选器）
  const char*cur_pattern=过滤器；
  为（；；）{
    if（patternmatchesstring（cur_pattern，name.c_str（））
      回归真实；
    }

    //在筛选器中查找下一个模式。
    cur_pattern=strchr（cur_pattern，：'）；

    //如果找不到更多的模式，则返回。
    if（cur_pattern==null）
      返回错误；
    }

    //跳过模式分隔符（：'字符）。
    Cury+ +；
  }
}

//返回true如果用户指定的筛选器与测试用例匹配
//名称和测试名称。
bool unittestoptions:：filtermatchestest（const std:：string&test_case_name，
                                        const std：：字符串和测试名称）
  const std:：string&full_name=test_case_name+“”+test_name.c_str（）；

  //split--gtest_filter at“-”，如果有，则将其分离为
  //正滤波器和负滤波器部分
  const char*const p=gtest_标志（filter）.c_str（）；
  const char*const dash=strchr（p，—'）；
  std：：字符串正；
  std：：字符串负；
  如果（破折号=空）
    正数=gtest_flag（filter）.c_str（）；//整个字符串是一个正数筛选器
    否定=“”；
  }否则{
    正数=std：：string（p，dash）；//到dash为止的所有内容
    负=std：：string（dash+1）；//破折号后的所有内容
    if（positive.empty（））
      //将“-test1”与“*-test1”相同
      阳性=Kuniversalfilter；
    }
  }

  //过滤器是以冒号分隔的模式列表。它匹配一个
  //测试其中是否有与测试匹配的模式。
  返回（matchesfilter（full_name，positive.c_str（））&&
          ！matchesFilter（全名，negative.c_str（））；
}

如果GTEST有
//如果google test应该处理
//给定SEH异常，否则继续搜索。
//此函数用作\例外条件。
int unittestoptions:：gtestshouldprocessseh（dword异常\u代码）
  //在以下情况下，Google测试应处理SEH异常：
  / / 1。用户想要它，并且
  / / 2。这不是断点异常，并且
  / / 3。这不是C++异常（VC++通过SEH实现它们）
  很明显）。
  / /
  //SEH异常代码用于C++异常。
  //（有关详细信息，请参阅http://support.microsoft.com/kb/185294）。
  常量dword kcxxxexceptioncode=0xe06d7363；

  bool should_handle=true；

  如果（！）gtest_标志（catch_异常）
    应_handle=false；
  else if（exception_code==exception_breakpoint）
    应_handle=false；
  else if（exception_code==kcxxxexceptioncode）
    应_handle=false；

  返回应该处理？异常执行处理程序：异常继续搜索；
}
endif//gtest_有

//命名空间内部

//tor将此对象设置为
//谷歌测试。“result”参数指定在何处报告
/结果。只截取当前线程中的失败。
ScopedFaketsPartResultReporter:：ScopedFaketsPartResultReporter（）。
    testpartResultArray*结果）
    ：intercept_mode_u（intercept_only_current_thread），
      结果
  （）；
}

//tor将此对象设置为
//谷歌测试。“result”参数指定在何处报告
/结果。
ScopedFaketsPartResultReporter:：ScopedFaketsPartResultReporter（）。
    截取模式截取模式，testpartResultArray*结果）
    ：Intercept_模式（Intercept_模式）
      结果
  （）；
}

void ScopedFaketsPartResultReporter:：Init（）
  内部：：UnitTestImpl*const impl=内部：：GetUnitTestImpl（）；
  if（intercept_模式==intercept_所有_线程）
    旧的_reporter_u=impl->GetGlobalTestPartResultReporter（）；
    impl->setglobaltestpartresultreporter（this）；
  }否则{
    旧的_reporter_uu=impl->getTestPartResultReporterForCurrentThread（）；
    impl->settestpartresultsreporterforcurrenthread（this）；
  }
}

//tor恢复google test使用的测试部件结果报告程序
以前。
ScopedFaketsPartResultReporter:：~ScopedFaketsPartResultReporter（）
  内部：：UnitTestImpl*const impl=内部：：GetUnitTestImpl（）；
  if（intercept_模式==intercept_所有_线程）
    impl->setglobaltestpartresultreporter（旧的_reporter_u）；
  }否则{
    impl->settestpartresultsreporterforcurrenthread（旧的_reporter_u）；
  }
}

//增加测试部件结果计数并记住结果。
//此方法来自TestPartResultReporterInterface接口。
void ScopedFaketsPartResultReporter:：ReportTestPartResult（）。
    const测试部分结果和结果）
  结果->append（result）；
}

命名空间内部

//返回：：testing:：test的类型ID。我们应该称之为
//而不是get type id<：testing:：test>（）以获取的类型ID
//测试：：测试。这是为了解决一个可疑的链接器错误，当
//在Mac OS X上使用Google测试作为框架。该错误导致
//GettypeID<：测试：：测试>（）返回不同的值，具体取决于
//关于调用是来自Google测试框架本身还是
//来自用户测试代码。GetTestTypeID（）保证始终
//返回相同的值，因为它总是从
//gtest.cc，它在Google测试框架内。
typeid gettestypeid（）
  返回gettypeid<test>（）；
}

//从google测试中看到的gettestypeid（）的值
//库。这仅用于测试gettestypeid（）。
extern const typeid ktesttypeidingogletest=gettestypeid（）；

//此谓词格式化程序检查“results”是否包含测试部分
//给定类型的失败，并且失败消息包含
//给定子字符串。
断言结果HasOneFailure（const char*/*results_expr*/,

                              /*st char*/*键入_expr*/，
                              const char*/*子字符串\u expr*/,

                              const TestPartResultArray& results,
                              TestPartResult::Type type,
                              const string& substr) {
  const std::string expected(type == TestPartResult::kFatalFailure ?
                        "1 fatal failure" :
                        "1 non-fatal failure");
  Message msg;
  if (results.size() != 1) {
    msg << "Expected: " << expected << "\n"
        << "  Actual: " << results.size() << " failures";
    for (int i = 0; i < results.size(); i++) {
      msg << "\n" << results.GetTestPartResult(i);
    }
    return AssertionFailure() << msg;
  }

  const TestPartResult& r = results.GetTestPartResult(0);
  if (r.type() != type) {
    return AssertionFailure() << "Expected: " << expected << "\n"
                              << "  Actual:\n"
                              << r;
  }

  if (strstr(r.message(), substr.c_str()) == NULL) {
    return AssertionFailure() << "Expected: " << expected << " containing \""
                              << substr << "\"\n"
                              << "  Actual:\n"
                              << r;
  }

  return AssertionSuccess();
}

//singlefailurechecker的构造函数记住在哪里查找
//测试部件结果，我们期望的故障类型，以及
//故障消息应包含子字符串。
SingleFailureChecker:: SingleFailureChecker(
    const TestPartResultArray* results,
    TestPartResult::Type type,
    const string& substr)
    : results_(results),
      type_(type),
      substr_(substr) {}

//SingleFailureChecker的析构函数验证给定的
//testpartResultArray只包含一个具有给定
//键入并包含给定的子字符串。如果不是这样的话，
//将生成非致命故障。
SingleFailureChecker::~SingleFailureChecker() {
  EXPECT_PRED_FORMAT3(HasOneFailure, *results_, type_, substr_);
}

DefaultGlobalTestPartResultReporter::DefaultGlobalTestPartResultReporter(
    UnitTestImpl* unit_test) : unit_test_(unit_test) {}

void DefaultGlobalTestPartResultReporter::ReportTestPartResult(
    const TestPartResult& result) {
  unit_test_->current_test_result()->AddTestPartResult(result);
  unit_test_->listeners()->repeater()->OnTestPartResult(result);
}

DefaultPerThreadTestPartResultReporter::DefaultPerThreadTestPartResultReporter(
    UnitTestImpl* unit_test) : unit_test_(unit_test) {}

void DefaultPerThreadTestPartResultReporter::ReportTestPartResult(
    const TestPartResult& result) {
  unit_test_->GetGlobalTestPartResultReporter()->ReportTestPartResult(result);
}

//返回全局测试部件结果报告器。
TestPartResultReporterInterface*
UnitTestImpl::GetGlobalTestPartResultReporter() {
  internal::MutexLock lock(&global_test_part_result_reporter_mutex_);
  return global_test_part_result_repoter_;
}

//设置全局测试部件结果报告器。
void UnitTestImpl::SetGlobalTestPartResultReporter(
    TestPartResultReporterInterface* reporter) {
  internal::MutexLock lock(&global_test_part_result_reporter_mutex_);
  global_test_part_result_repoter_ = reporter;
}

//返回当前线程的测试部件结果报告器。
TestPartResultReporterInterface*
UnitTestImpl::GetTestPartResultReporterForCurrentThread() {
  return per_thread_test_part_result_reporter_.get();
}

//为当前线程设置测试部件结果报告器。
void UnitTestImpl::SetTestPartResultReporterForCurrentThread(
    TestPartResultReporterInterface* reporter) {
  per_thread_test_part_result_reporter_.set(reporter);
}

//获取成功的测试用例数。
int UnitTestImpl::successful_test_case_count() const {
  return CountIf(test_cases_, TestCasePassed);
}

//获取失败的测试用例数。
int UnitTestImpl::failed_test_case_count() const {
  return CountIf(test_cases_, TestCaseFailed);
}

//获取所有测试用例的数目。
int UnitTestImpl::total_test_case_count() const {
  return static_cast<int>(test_cases_.size());
}

//获取包含至少一个测试的所有测试用例的数目
//应该可以。
int UnitTestImpl::test_case_to_run_count() const {
  return CountIf(test_cases_, ShouldRunTestCase);
}

//获取成功的测试数。
int UnitTestImpl::successful_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::successful_test_count);
}

//获取失败的测试数。
int UnitTestImpl::failed_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::failed_test_count);
}

//获取将在XML报告中报告的禁用测试数。
int UnitTestImpl::reportable_disabled_test_count() const {
  return SumOverTestCaseList(test_cases_,
                             &TestCase::reportable_disabled_test_count);
}

//获取禁用的测试数。
int UnitTestImpl::disabled_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::disabled_test_count);
}

//获取要在XML报表中打印的测试数。
int UnitTestImpl::reportable_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::reportable_test_count);
}

//获取所有测试的数目。
int UnitTestImpl::total_test_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::total_test_count);
}

//获取应运行的测试数。
int UnitTestImpl::test_to_run_count() const {
  return SumOverTestCaseList(test_cases_, &TestCase::test_to_run_count);
}

//以std：：字符串形式返回当前操作系统堆栈跟踪。
//
//要包含的最大堆栈帧数由指定
//gtest堆栈跟踪深度标志。skip_count参数
//指定要跳过的顶层帧的数目，但不指定
//根据要包含的帧数计数。
//
//例如，如果foo（）调用bar（），而bar（）又调用
//当前osstacktraceexcepttop（1），foo（）将包含在
//trace但bar（）和currentOsstackTraceExceptTop（）不会。
std::string UnitTestImpl::CurrentOsStackTraceExceptTop(int skip_count) {
  (void)skip_count;
  return "";
}

//返回当前时间（毫秒）。
TimeInMillis GetTimeInMillis() {
#if GTEST_OS_WINDOWS_MOBILE || defined(__BORLANDC__)
//1970-01-01和1601-01-01之间的差异（毫秒）。
//网址：http://assidual.blogspot.com/2005/04/epoch.html
  const TimeInMillis kJavaEpochToWinFileTimeDelta =
    static_cast<TimeInMillis>(116444736UL) * 100000UL;
  const DWORD kTenthMicrosInMilliSecond = 10000;

  SYSTEMTIME now_systime;
  FILETIME now_filetime;
  ULARGE_INTEGER now_int64;
//托多（kenton@google.com）：这不应该只是使用
//GetSystemTimeAsFileTime（）？
  GetSystemTime(&now_systime);
  if (SystemTimeToFileTime(&now_systime, &now_filetime)) {
    now_int64.LowPart = now_filetime.dwLowDateTime;
    now_int64.HighPart = now_filetime.dwHighDateTime;
    now_int64.QuadPart = (now_int64.QuadPart / kTenthMicrosInMilliSecond) -
      kJavaEpochToWinFileTimeDelta;
    return now_int64.QuadPart;
  }
  return 0;
#elif GTEST_OS_WINDOWS && !GTEST_HAS_GETTIMEOFDAY_
  __timeb64 now;

//msvc 8不支持ftime64（），因此我们希望取消警告4996
//（已弃用的函数）。
//todo（kenton@google.com）：使用gettickCount（）？或使用
//系统时间到文件时间（）
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4996)
  _ftime64(&now);
  GTEST_DISABLE_MSC_WARNINGS_POP_()

  return static_cast<TimeInMillis>(now.time) * 1000 + now.millitm;
#elif GTEST_HAS_GETTIMEOFDAY_
  struct timeval now;
  gettimeofday(&now, NULL);
  return static_cast<TimeInMillis>(now.tv_sec) * 1000 + now.tv_usec / 1000;
#else
# error "Don't know how to get the current time on your system."
#endif
}

//公用事业

//类字符串。

#if GTEST_OS_WINDOWS_MOBILE
//从给定的ansi字符串创建一个utf-16宽字符串，分配
//内存使用新的。调用方负责删除返回
//使用delete[]的值。返回宽字符串，如果
//输入为空。
LPCWSTR String::AnsiToUtf16(const char* ansi) {
  if (!ansi) return NULL;
  const int length = strlen(ansi);
  const int unicode_length =
      MultiByteToWideChar(CP_ACP, 0, ansi, length,
                          NULL, 0);
  WCHAR* unicode = new WCHAR[unicode_length + 1];
  MultiByteToWideChar(CP_ACP, 0, ansi, length,
                      unicode, unicode_length);
  unicode[unicode_length] = 0;
  return unicode;
}

//从给定的宽字符串创建一个ANSI字符串，并分配
//内存使用新的。调用方负责删除返回
//使用delete[]的值。返回ansi字符串，如果
//输入为空。
const char* String::Utf16ToAnsi(LPCWSTR utf16_str)  {
  if (!utf16_str) return NULL;
  const int ansi_length =
      WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,
                          NULL, 0, NULL, NULL);
  char* ansi = new char[ansi_length + 1];
  WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,
                      ansi, ansi_length, NULL, NULL);
  ansi[ansi_length] = 0;
  return ansi;
}

#endif  //GTEST操作系统Windows移动

//比较两个C字符串。返回具有相同内容的真iff。
//
//与strcmp（）不同，此函数可以处理空参数。空值
//C字符串被认为与任何非空C字符串不同，
//包括空字符串。
bool String::CStringEquals(const char * lhs, const char * rhs) {
  if ( lhs == NULL ) return rhs == NULL;

  if ( rhs == NULL ) return false;

  return strcmp(lhs, rhs) == 0;
}

#if GTEST_HAS_STD_WSTRING || GTEST_HAS_GLOBAL_WSTRING

//使用utf-8将宽字符数组转换为窄字符串
//编码，并将结果流式传输到给定的消息对象。
static void StreamWideCharsToMessage(const wchar_t* wstr, size_t length,
                                     Message* msg) {
for (size_t i = 0; i != length; ) {  //诺林
    if (wstr[i] != L'\0') {
      *msg << WideStringToUtf8(wstr + i, static_cast<int>(length - i));
      while (i != length && wstr[i] != L'\0')
        i++;
    } else {
      *msg << '\0';
      i++;
    }
  }
}

#endif  //GTEST_有_-std_-wstring_GTEST_有_-global_-wstring

}  //命名空间内部

//构造空消息。
//我们单独分配stringstream，因为否则每个使用
//过程中的assert/expect向过程的
//堆栈帧在某些情况下会导致巨大的堆栈帧；gcc不重用
//堆栈空间。
Message::Message() : ss_(new ::std::stringstream) {
//默认情况下，我们希望打印时有足够的精度
//一条消息的双精度。
  *ss_ << std::setprecision(std::numeric_limits<double>::digits10 + 2);
}

//这两个重载允许将宽C字符串流式传输到消息
//使用UTF-8编码。
Message& Message::operator <<(const wchar_t* wide_c_str) {
  return *this << internal::String::ShowWideCString(wide_c_str);
}
Message& Message::operator <<(wchar_t* wide_c_str) {
  return *this << internal::String::ShowWideCString(wide_c_str);
}

#if GTEST_HAS_STD_WSTRING
//使用utf-8将给定的宽字符串转换为窄字符串
//编码，并将结果流式传输到此消息对象。
Message& Message::operator <<(const ::std::wstring& wstr) {
  internal::StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  //GTEST_有_-std_-wstring

#if GTEST_HAS_GLOBAL_WSTRING
//使用utf-8将给定的宽字符串转换为窄字符串
//编码，并将结果流式传输到此消息对象。
Message& Message::operator <<(const ::wstring& wstr) {
  internal::StreamWideCharsToMessage(wstr.c_str(), wstr.length(), this);
  return *this;
}
#endif  //GTEST拥有全球性的

//获取流式传输到此对象的文本，直至达到std：：string。
//缓冲区中的每个\0'字符都替换为\\0。
std::string Message::GetString() const {
  return internal::StringStreamToString(ss_.get());
}

//断言结果构造函数。
//在Expect_true/false（断言_结果）中使用。
AssertionResult::AssertionResult(const AssertionResult& other)
    : success_(other.success_),
      message_(other.message_.get() != NULL ?
               new ::std::string(*other.message_) :
               static_cast< ::std::string*>(NULL)) {
}

//交换两个断言结果。
void AssertionResult::swap(AssertionResult& other) {
  using std::swap;
  swap(success_, other.success_);
  swap(message_, other.message_);
}

//返回断言的否定。与expect/assert_false一起使用。
AssertionResult AssertionResult::operator!() const {
  AssertionResult negation(!success_);
  if (message_.get() != NULL)
    negation << *message_;
  return negation;
}

//生成成功的断言结果。
AssertionResult AssertionSuccess() {
  return AssertionResult(true);
}

//生成失败的断言结果。
AssertionResult AssertionFailure() {
  return AssertionResult(false);
}

//使用给定的失败消息生成失败的断言结果。
//已弃用；请使用assertionfailure（）<<message。
AssertionResult AssertionFailure(const Message& message) {
  return AssertionFailure() << message;
}

namespace internal {

namespace edit_distance {
std::vector<EditType> CalculateOptimalEdits(const std::vector<size_t>& left,
                                            const std::vector<size_t>& right) {
  std::vector<std::vector<double> > costs(
      left.size() + 1, std::vector<double>(right.size() + 1));
  std::vector<std::vector<EditType> > best_move(
      left.size() + 1, std::vector<EditType>(right.size() + 1));

//填充为空权限。
  for (size_t l_i = 0; l_i < costs.size(); ++l_i) {
    costs[l_i][0] = static_cast<double>(l_i);
    best_move[l_i][0] = kRemove;
  }
//填充左侧为空。
  for (size_t r_i = 1; r_i < costs[0].size(); ++r_i) {
    costs[0][r_i] = static_cast<double>(r_i);
    best_move[0][r_i] = kAdd;
  }

  for (size_t l_i = 0; l_i < left.size(); ++l_i) {
    for (size_t r_i = 0; r_i < right.size(); ++r_i) {
      if (left[l_i] == right[r_i]) {
//找到一根火柴消费它。
        costs[l_i + 1][r_i + 1] = costs[l_i][r_i];
        best_move[l_i + 1][r_i + 1] = kMatch;
        continue;
      }

      const double add = costs[l_i + 1][r_i];
      const double remove = costs[l_i][r_i + 1];
      const double replace = costs[l_i][r_i];
      if (add < remove && add < replace) {
        costs[l_i + 1][r_i + 1] = add + 1;
        best_move[l_i + 1][r_i + 1] = kAdd;
      } else if (remove < add && remove < replace) {
        costs[l_i + 1][r_i + 1] = remove + 1;
        best_move[l_i + 1][r_i + 1] = kRemove;
      } else {
//我们使替换比添加/删除成本低一点
//他们的优先权。
        costs[l_i + 1][r_i + 1] = replace + 1.00001;
        best_move[l_i + 1][r_i + 1] = kReplace;
      }
    }
  }

//重建最佳路径。我们按相反的顺序做。
  std::vector<EditType> best_path;
  for (size_t l_i = left.size(), r_i = right.size(); l_i > 0 || r_i > 0;) {
    EditType move = best_move[l_i][r_i];
    best_path.push_back(move);
    l_i -= move != kAdd;
    r_i -= move != kRemove;
  }
  std::reverse(best_path.begin(), best_path.end());
  return best_path;
}

namespace {

//帮助器类，通过重复数据消除将字符串转换为ID。
class InternalStrings {
 public:
  size_t GetId(const std::string& str) {
    IdMap::iterator it = ids_.find(str);
    if (it != ids_.end()) return it->second;
    size_t id = ids_.size();
    return ids_[str] = id;
  }

 private:
  typedef std::map<std::string, size_t> IdMap;
  IdMap ids_;
};

}  //命名空间

std::vector<EditType> CalculateOptimalEdits(
    const std::vector<std::string>& left,
    const std::vector<std::string>& right) {
  std::vector<size_t> left_ids, right_ids;
  {
    InternalStrings intern_table;
    for (size_t i = 0; i < left.size(); ++i) {
      left_ids.push_back(intern_table.GetId(left[i]));
    }
    for (size_t i = 0; i < right.size(); ++i) {
      right_ids.push_back(intern_table.GetId(right[i]));
    }
  }
  return CalculateOptimalEdits(left_ids, right_ids);
}

namespace {

//helper类，它保存一个hunk的状态并将其输出到
//溪流。
//如果可能，它会重新排序添加/删除，以将所有删除都分组在所有删除之前
//添加。它还将printint之前的hunk头添加到流中。
class Hunk {
 public:
  Hunk(size_t left_start, size_t right_start)
      : left_start_(left_start),
        right_start_(right_start),
        adds_(),
        removes_(),
        common_() {}

  void PushLine(char edit, const char* line) {
    switch (edit) {
      case ' ':
        ++common_;
        FlushEdits();
        hunk_.push_back(std::make_pair(' ', line));
        break;
      case '-':
        ++removes_;
        hunk_removes_.push_back(std::make_pair('-', line));
        break;
      case '+':
        ++adds_;
        hunk_adds_.push_back(std::make_pair('+', line));
        break;
    }
  }

  void PrintTo(std::ostream* os) {
    PrintHeader(os);
    FlushEdits();
    for (std::list<std::pair<char, const char*> >::const_iterator it =
             hunk_.begin();
         it != hunk_.end(); ++it) {
      *os << it->first << it->second << "\n";
    }
  }

  bool has_edits() const { return adds_ || removes_; }

 private:
  void FlushEdits() {
    hunk_.splice(hunk_.end(), hunk_removes_);
    hunk_.splice(hunk_.end(), hunk_adds_);
  }

//为一个Hunk打印一个统一的diff头文件。
//格式是
//“@@-<left\u start>，<left\u length>+<right\u start>，<right\u length>@@”
//如果不必要，省略左/右部分。
  void PrintHeader(std::ostream* ss) const {
    *ss << "@@ ";
    if (removes_) {
      *ss << "-" << left_start_ << "," << (removes_ + common_);
    }
    if (removes_ && adds_) {
      *ss << " ";
    }
    if (adds_) {
      *ss << "+" << right_start_ << "," << (adds_ + common_);
    }
    *ss << " @@\n";
  }

  size_t left_start_, right_start_;
  size_t adds_, removes_, common_;
  std::list<std::pair<char, const char*> > hunk_, hunk_adds_, hunk_removes_;
};

}  //命名空间

//以统一的diff格式创建diff块列表。
//每个Hunk都有一个由上面的printHeader生成的头和一个带有
//前缀为“”的行表示无更改，“-”表示删除，“+”表示
//加法。
//“context”表示diff周围所需的未更改前缀/后缀。
//如果两个大块足够接近以至于它们的上下文重叠，那么它们是
//加入了一个混蛋。
std::string CreateUnifiedDiff(const std::vector<std::string>& left,
                              const std::vector<std::string>& right,
                              size_t context) {
  const std::vector<EditType> edits = CalculateOptimalEdits(left, right);

  size_t l_i = 0, r_i = 0, edit_i = 0;
  std::stringstream ss;
  while (edit_i < edits.size()) {
//查找第一个编辑。
    while (edit_i < edits.size() && edits[edit_i] == kMatch) {
      ++l_i;
      ++r_i;
      ++edit_i;
    }

//找到要包含在Hunk中的第一行。
    const size_t prefix_context = std::min(l_i, context);
    Hunk hunk(l_i - prefix_context + 1, r_i - prefix_context + 1);
    for (size_t i = prefix_context; i > 0; --i) {
      hunk.PushLine(' ', left[l_i - i].c_str());
    }

//重复编辑，直到我们找到足够的后缀用于块或输入
//结束了。
    size_t n_suffix = 0;
    for (; edit_i < edits.size(); ++edit_i) {
      if (n_suffix >= context) {
//只有在下一个庞克非常接近时才继续。
        std::vector<EditType>::const_iterator it = edits.begin() + edit_i;
        while (it != edits.end() && *it == kMatch) ++it;
        if (it == edits.end() || (it - edits.begin()) - edit_i >= context) {
//没有下一个编辑，或者太远了。
          break;
        }
      }

      EditType edit = edits[edit_i];
//找到不匹配项时重置计数。
      n_suffix = edit == kMatch ? n_suffix + 1 : 0;

      if (edit == kMatch || edit == kRemove || edit == kReplace) {
        hunk.PushLine(edit == kMatch ? ' ' : '-', left[l_i].c_str());
      }
      if (edit == kAdd || edit == kReplace) {
        hunk.PushLine('+', right[r_i].c_str());
      }

//根据编辑类型推进索引。
      l_i += edit != kAdd;
      r_i += edit != kRemove;
    }

    if (!hunk.has_edits()) {
//我们完了。我们不想要这个混蛋。
      break;
    }

    hunk.PrintTo(&ss);
  }
  return ss.str();
}

}  //命名空间编辑距离

namespace {

//在eqfailure（）中接收的值的字符串表示形式已经
//逃脱。在逃逸的边界上把它们分开。让所有其他逃犯离开
//字符相同。
std::vector<std::string> SplitEscapedString(const std::string& str) {
  std::vector<std::string> lines;
  size_t start = 0, end = str.size();
  if (end > 2 && str[0] == '"' && str[end - 1] == '"') {
    ++start;
    --end;
  }
  bool escaped = false;
  for (size_t i = start; i + 1 < end; ++i) {
    if (escaped) {
      escaped = false;
      if (str[i] == 'n') {
        lines.push_back(str.substr(start, i - start - 1));
        start = i + 1;
      }
    } else {
      escaped = str[i] == '\\';
    }
  }
  lines.push_back(str.substr(start, end - start));
  return lines;
}

}  //命名空间

//构造并返回相等断言的消息
//（例如，断言eq、预期streq等）失败。
//
//前四个参数是断言中使用的表达式
//以及它们的值，作为字符串。例如，对于断言eq（foo，bar）
//其中foo为5，bar为6，我们有：
//
//应输入_表达式：“foo”
//实际_表达式：“bar”
//预期值：“5”
//实际值：“6”
//
//如果断言是
//*如果为真，字符串“（忽略大小写）”将
//插入到消息中。
AssertionResult EqFailure(const char* expected_expression,
                          const char* actual_expression,
                          const std::string& expected_value,
                          const std::string& actual_value,
                          bool ignoring_case) {
  Message msg;
  msg << "Value of: " << actual_expression;
  if (actual_value != actual_expression) {
    msg << "\n  Actual: " << actual_value;
  }

  msg << "\nExpected: " << expected_expression;
  if (ignoring_case) {
    msg << " (ignoring case)";
  }
  if (expected_value != expected_expression) {
    msg << "\nWhich is: " << expected_value;
  }

  if (!expected_value.empty() && !actual_value.empty()) {
    const std::vector<std::string> expected_lines =
        SplitEscapedString(expected_value);
    const std::vector<std::string> actual_lines =
        SplitEscapedString(actual_value);
    if (expected_lines.size() > 1 || actual_lines.size() > 1) {
      msg << "\nWith diff:\n"
          << edit_distance::CreateUnifiedDiff(expected_lines, actual_lines);
    }
  }

  return AssertionFailure() << msg;
}

//构造布尔断言的失败消息，如expect_true。
std::string GetBoolAssertionFailureMessage(
    const AssertionResult& assertion_result,
    const char* expression_text,
    const char* actual_predicate_value,
    const char* expected_predicate_value) {
  const char* actual_message = assertion_result.message();
  Message msg;
  msg << "Value of: " << expression_text
      << "\n  Actual: " << actual_predicate_value;
  if (actual_message[0] != '\0')
    msg << " (" << actual_message << ")";
  msg << "\nExpected: " << expected_predicate_value;
  return msg.GetString();
}

//用于实现assert_near的helper函数。
AssertionResult DoubleNearPredFormat(const char* expr1,
                                     const char* expr2,
                                     const char* abs_error_expr,
                                     double val1,
                                     double val2,
                                     double abs_error) {
  const double diff = fabs(val1 - val2);
  if (diff <= abs_error) return AssertionSuccess();

//TODO（WAN）：如果表达式的值是
//已经是文字了。
  return AssertionFailure()
      << "The difference between " << expr1 << " and " << expr2
      << " is " << diff << ", which exceeds " << abs_error_expr << ", where\n"
      << expr1 << " evaluates to " << val1 << ",\n"
      << expr2 << " evaluates to " << val2 << ", and\n"
      << abs_error_expr << " evaluates to " << abs_error << ".";
}


//用于实现floatle（）和doublele（）的帮助器模板。
template <typename RawType>
AssertionResult FloatingPointLE(const char* expr1,
                                const char* expr2,
                                RawType val1,
                                RawType val2) {
//如果val1小于val2，则返回SUCCESS，
  if (val1 < val2) {
    return AssertionSuccess();
  }

//或者如果val1几乎等于val2。
  const FloatingPoint<RawType> lhs(val1), rhs(val2);
  if (lhs.AlmostEquals(rhs)) {
    return AssertionSuccess();
  }

//请注意，如果VAL1或
//Val2是NaN，因为IEEE浮点标准要求
//任何涉及NaN的谓词都必须返回false。

  ::std::stringstream val1_ss;
  val1_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val1;

  ::std::stringstream val2_ss;
  val2_ss << std::setprecision(std::numeric_limits<RawType>::digits10 + 2)
          << val2;

  return AssertionFailure()
      << "Expected: (" << expr1 << ") <= (" << expr2 << ")\n"
      << "  Actual: " << StringStreamToString(&val1_ss) << " vs "
      << StringStreamToString(&val2_ss);
}

}  //命名空间内部

//断言val1小于或几乎等于val2。失败
//否则。尤其是，如果val1或val2为NaN，则失败。
AssertionResult FloatLE(const char* expr1, const char* expr2,
                        float val1, float val2) {
  return internal::FloatingPointLE<float>(expr1, expr2, val1, val2);
}

//断言val1小于或几乎等于val2。失败
//否则。尤其是，如果val1或val2为NaN，则失败。
AssertionResult DoubleLE(const char* expr1, const char* expr2,
                         double val1, double val2) {
  return internal::FloatingPointLE<double>(expr1, expr2, val1, val2);
}

namespace internal {

//带int或enum的assert expect u eq的helper函数
//争论。
AssertionResult CmpHelperEQ(const char* expected_expression,
                            const char* actual_expression,
                            BiggestInt expected,
                            BiggestInt actual) {
  if (expected == actual) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   FormatForComparisonFailureMessage(expected, actual),
                   FormatForComparisonFailureMessage(actual, expected),
                   false);
}

//用于实现实现所需帮助器函数的宏
//断言？和期待？？使用整数或枚举参数。它就在这里
//只是为了避免复制和粘贴类似的代码。
#define GTEST_IMPL_CMP_HELPER_(op_name, op)\
AssertionResult CmpHelper##op_name(const char* expr1, const char* expr2, \
                                   BiggestInt val1, BiggestInt val2) {\
  if (val1 op val2) {\
    return AssertionSuccess();\
  } else {\
    return AssertionFailure() \
        << "Expected: (" << expr1 << ") " #op " (" << expr2\
        << "), actual: " << FormatForComparisonFailureMessage(val1, val2)\
        << " vs " << FormatForComparisonFailureMessage(val2, val1);\
  }\
}

//使用int或
//枚举参数。
GTEST_IMPL_CMP_HELPER_(NE, !=)
//使用int或
//枚举参数。
GTEST_IMPL_CMP_HELPER_(LE, <=)
//使用int或
//枚举参数。
GTEST_IMPL_CMP_HELPER_(LT, < )
//使用int或
//枚举参数。
GTEST_IMPL_CMP_HELPER_(GE, >=)
//使用int或
//枚举参数。
GTEST_IMPL_CMP_HELPER_(GT, > )

#undef GTEST_IMPL_CMP_HELPER_

//_assert_expect_u streq的helper函数。
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const char* expected,
                               const char* actual) {
  if (String::CStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   PrintToString(expected),
                   PrintToString(actual),
                   false);
}

//_assert_expect_u strcaseq的helper函数。
AssertionResult CmpHelperSTRCASEEQ(const char* expected_expression,
                                   const char* actual_expression,
                                   const char* expected,
                                   const char* actual) {
  if (String::CaseInsensitiveCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   PrintToString(expected),
                   PrintToString(actual),
                   true);
}

//_assert_expect_u strne的helper函数。
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const char* s1,
                               const char* s2) {
  if (!String::CStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    return AssertionFailure() << "Expected: (" << s1_expression << ") != ("
                              << s2_expression << "), actual: \""
                              << s1 << "\" vs \"" << s2 << "\"";
  }
}

//_assert_expect_u strcasene的helper函数。
AssertionResult CmpHelperSTRCASENE(const char* s1_expression,
                                   const char* s2_expression,
                                   const char* s1,
                                   const char* s2) {
  if (!String::CaseInsensitiveCStringEquals(s1, s2)) {
    return AssertionSuccess();
  } else {
    return AssertionFailure()
        << "Expected: (" << s1_expression << ") != ("
        << s2_expression << ") (ignoring case), actual: \""
        << s1 << "\" vs \"" << s2 << "\"";
  }
}

}  //命名空间内部

namespace {

//用于实现issubstring（）和isnotsubstring（）的helper函数。

//这组重载函数返回真正的iff指针是
//干草堆的子串。空值被视为自身的子字符串
//只有。

bool IsSubstringPred(const char* needle, const char* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return strstr(haystack, needle) != NULL;
}

bool IsSubstringPred(const wchar_t* needle, const wchar_t* haystack) {
  if (needle == NULL || haystack == NULL)
    return needle == haystack;

  return wcsstr(haystack, needle) != NULL;
}

//这里的stringtype可以是：：std:：string或：：std:：wstring。
template <typename StringType>
bool IsSubstringPred(const StringType& needle,
                     const StringType& haystack) {
  return haystack.find(needle) != StringType::npos;
}

//此函数实现ISSUBString（）或ISNOTSSUBString（），
//取决于预期的“要”子字符串参数的值。
//这里的StringType可以是const char*、const wchar_t*、：：std：：string，
//或：：std：：wstring。
template <typename StringType>
AssertionResult IsSubstringImpl(
    bool expected_to_be_substring,
    const char* needle_expr, const char* haystack_expr,
    const StringType& needle, const StringType& haystack) {
  if (IsSubstringPred(needle, haystack) == expected_to_be_substring)
    return AssertionSuccess();

  const bool is_wide_string = sizeof(needle[0]) > 1;
  const char* const begin_string_quote = is_wide_string ? "L\"" : "\"";
  return AssertionFailure()
      << "Value of: " << needle_expr << "\n"
      << "  Actual: " << begin_string_quote << needle << "\"\n"
      << "Expected: " << (expected_to_be_substring ? "" : "not ")
      << "a substring of " << haystack_expr << "\n"
      << "Which is: " << begin_string_quote << haystack << "\"";
}

}  //命名空间

//issubstring（）和isnotsubstring（）检查指针是否为
//干草堆的子串（空值被视为自身的子串
//仅限），并在失败时返回适当的错误消息。

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const char* needle, const char* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const wchar_t* needle, const wchar_t* haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::string& needle, const ::std::string& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}

#if GTEST_HAS_STD_WSTRING
AssertionResult IsSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(true, needle_expr, haystack_expr, needle, haystack);
}

AssertionResult IsNotSubstring(
    const char* needle_expr, const char* haystack_expr,
    const ::std::wstring& needle, const ::std::wstring& haystack) {
  return IsSubstringImpl(false, needle_expr, haystack_expr, needle, haystack);
}
#endif  //GTEST_有_-std_-wstring

namespace internal {

#if GTEST_OS_WINDOWS

namespace {

//ishresult successfailure谓词的帮助函数
AssertionResult HRESULTFailureHelper(const char* expr,
                                     const char* expected,
long hr) {  //诺林
# if GTEST_OS_WINDOWS_MOBILE

//Windows CE不支持格式化消息。
  const char error_text[] = "";

# else

//查找人类可读的系统消息以获取hresult代码
//因为我们不向formatmessage传递任何参数，所以
//希望插入扩展。
  const DWORD kFlags = FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD kBufSize = 4096;
//获取此hresult的系统可读消息字符串。
  char error_text[kBufSize] = { '\0' };
  DWORD message_length = ::FormatMessageA(kFlags,
0,  //没有消息来源，我们在问系统
hr,  //误差
0,  //无线条宽度限制
error_text,  //输出缓冲器
kBufSize,  //BUF尺寸
NULL);  //没有插入参数
//修剪尾随空白（formatmessage留下尾随CR-LF）
  for (; message_length && IsSpace(error_text[message_length - 1]);
          --message_length) {
    error_text[message_length - 1] = '\0';
  }

# endif  //GTEST操作系统Windows移动

  const std::string error_hex("0x" + String::FormatHexInt(hr));
  return ::testing::AssertionFailure()
      << "Expected: " << expr << " " << expected << ".\n"
      << "  Actual: " << error_hex << " " << error_text << "\n";
}

}  //命名空间

AssertionResult IsHRESULTSuccess(const char* expr, long hr) {  //诺林
  if (SUCCEEDED(hr)) {
    return AssertionSuccess();
  }
  return HRESULTFailureHelper(expr, "succeeds", hr);
}

AssertionResult IsHRESULTFailure(const char* expr, long hr) {  //诺林
  if (FAILED(hr)) {
    return AssertionSuccess();
  }
  return HRESULTFailureHelper(expr, "fails", hr);
}

#endif  //GTEST操作系统窗口

//用于在中编码Unicode文本（宽字符串）的实用程序函数
//UTF-8。

//一个Unicode码位最多可以有21位，并以UTF-8编码。
//这样地：
//
//码位长度编码
//0-7位0xXXXXXX
//8-11位110xxxxx 10xxxx
//12-16位1110XXXX 10XXXX 10XXXX 10XXXX
//17-21位11110xxx 10xxxx 10xxxx 10xxxx 10xxxx

//一个单字节UTF-8序列可以表示的最大码位。
const UInt32 kMaxCodePoint1 = (static_cast<UInt32>(1) <<  7) - 1;

//两个字节的UTF-8序列可以表示的最大码位。
const UInt32 kMaxCodePoint2 = (static_cast<UInt32>(1) << (5 + 6)) - 1;

//三字节UTF-8序列可以表示的最大码位。
const UInt32 kMaxCodePoint3 = (static_cast<UInt32>(1) << (4 + 2*6)) - 1;

//四字节UTF-8序列可以表示的最大码位。
const UInt32 kMaxCodePoint4 = (static_cast<UInt32>(1) << (3 + 3*6)) - 1;

//从位模式中切掉n个最低位。返回n
//最低比特。作为副作用，原始位模式将
//右移n位。
inline UInt32 ChopLowBits(UInt32* bits, int n) {
  const UInt32 low_bits = *bits & ((static_cast<UInt32>(1) << n) - 1);
  *bits >>= n;
  return low_bits;
}

//以UTF-8编码将Unicode码位转换为窄字符串。
//代码点参数的类型为uint32，因为wchar_t可能不是
//足够宽以包含代码点。
//如果代码点不是有效的Unicode代码点
//（即超出Unicode范围U+0至U+10ffff）将被转换。
//到“（无效的Unicode 0xXXXXXXXX）”。
std::string CodePointToUtf8(UInt32 code_point) {
  if (code_point > kMaxCodePoint4) {
    return "(Invalid Unicode 0x" + String::FormatHexInt(code_point) + ")";
  }

char str[5];  //足够大，可以容纳最大的有效代码点。
  if (code_point <= kMaxCodePoint1) {
    str[1] = '\0';
str[0] = static_cast<char>(code_point);                          //0xXXXXXX
  } else if (code_point <= kMaxCodePoint2) {
    str[2] = '\0';
str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[0] = static_cast<char>(0xC0 | code_point);                   //110×XXXX
  } else if (code_point <= kMaxCodePoint3) {
    str[3] = '\0';
str[2] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[0] = static_cast<char>(0xE0 | code_point);                   //1110XXXX
} else {  //代码点<=kmaxcodepoint4
    str[4] = '\0';
str[3] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[2] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[1] = static_cast<char>(0x80 | ChopLowBits(&code_point, 6));  //10XXXXXX
str[0] = static_cast<char>(0xF0 | code_point);                   //111xxxx
  }
  return str;
}

//以下两个功能仅在系统
//使用UTF-16进行宽字符串编码。所有支持的系统
//对于16位的wchar_t（Windows、Cygwin、Symbian OS），请使用utf-16。

//确定参数是否构成UTF-16代理项对
//因此应该组合成一个Unicode码位
//使用CreateCodePointFromUTF16代理网关对。
inline bool IsUtf16SurrogatePair(wchar_t first, wchar_t second) {
  return sizeof(wchar_t) == 2 &&
      (first & 0xFC00) == 0xD800 && (second & 0xFC00) == 0xDC00;
}

//从UTF16代理项对创建Unicode代码点。
inline UInt32 CreateCodePointFromUtf16SurrogatePair(wchar_t first,
                                                    wchar_t second) {
  const UInt32 mask = (1 << 10) - 1;
  return (sizeof(wchar_t) == 2) ?
      (((first & mask) << 10) | (second & mask)) + 0x10000 :
//当条件为
//错误，但我们提供了一个合理的默认值。
      static_cast<UInt32>(first);
}

//以UTF-8编码将宽字符串转换为窄字符串。
//假定宽字符串具有以下编码：
//utf-16 if sizeof（wchar_t）==2（在Windows、Cygwin、Symbian OS上）
//utf-32 if sizeof（wchar_t）==4（在Linux上）
//参数str指向以空结尾的宽字符串。
//参数num_chars还可以限制数字
//已处理的wchar字符数。当整个字符串
//应该处理。
//如果字符串包含无效的Unicode代码点
//（即超出Unicode范围U+0至U+10ffff）它们将被输出
//作为'（无效的Unicode 0xXXXXXXXX）'。如果字符串采用UTF16编码
//并且包含无效的UTF-16代理项对，这些对中的值
//将被编码为基本正常平面上的单个Unicode字符。
std::string WideStringToUtf8(const wchar_t* str, int num_chars) {
  if (num_chars == -1)
    num_chars = static_cast<int>(wcslen(str));

  ::std::stringstream stream;
  for (int i = 0; i < num_chars; ++i) {
    UInt32 unicode_code_point;

    if (str[i] == L'\0') {
      break;
    } else if (i + 1 < num_chars && IsUtf16SurrogatePair(str[i], str[i + 1])) {
      unicode_code_point = CreateCodePointFromUtf16SurrogatePair(str[i],
                                                                 str[i + 1]);
      i++;
    } else {
      unicode_code_point = static_cast<UInt32>(str[i]);
    }

    stream << CodePointToUtf8(unicode_code_point);
  }
  return StringStreamToString(&stream);
}

//使用UTF-8编码将宽C字符串转换为std:：string。
//空值将转换为“（空）”。
std::string String::ShowWideCString(const wchar_t * wide_c_str) {
  if (wide_c_str == NULL)  return "(null)";

  return internal::WideStringToUtf8(wide_c_str, -1);
}

//比较两个宽C字符串。返回真正的iff它们有相同的
//内容。
//
//与wcscmp（）不同，此函数可以处理空参数。空值
//C字符串被认为与任何非空C字符串不同，
//包括空字符串。
bool String::WideCStringEquals(const wchar_t * lhs, const wchar_t * rhs) {
  if (lhs == NULL) return rhs == NULL;

  if (rhs == NULL) return false;

  return wcscmp(lhs, rhs) == 0;
}

//宽字符串上的*streq的helper函数。
AssertionResult CmpHelperSTREQ(const char* expected_expression,
                               const char* actual_expression,
                               const wchar_t* expected,
                               const wchar_t* actual) {
  if (String::WideCStringEquals(expected, actual)) {
    return AssertionSuccess();
  }

  return EqFailure(expected_expression,
                   actual_expression,
                   PrintToString(expected),
                   PrintToString(actual),
                   false);
}

//宽字符串上的*字符串的帮助函数。
AssertionResult CmpHelperSTRNE(const char* s1_expression,
                               const char* s2_expression,
                               const wchar_t* s1,
                               const wchar_t* s2) {
  if (!String::WideCStringEquals(s1, s2)) {
    return AssertionSuccess();
  }

  return AssertionFailure() << "Expected: (" << s1_expression << ") != ("
                            << s2_expression << "), actual: "
                            << PrintToString(s1)
                            << " vs " << PrintToString(s2);
}

//比较两个C字符串，忽略大小写。返回真实的iff
//相同的内容。
//
//与strcasecmp（）不同，此函数可以处理空参数。一
//空C字符串被认为与任何非空C字符串不同，
//包括空字符串。
bool String::CaseInsensitiveCStringEquals(const char * lhs, const char * rhs) {
  if (lhs == NULL)
    return rhs == NULL;
  if (rhs == NULL)
    return false;
  return posix::StrCaseCmp(lhs, rhs) == 0;
}

//比较两个宽C字符串，忽略大小写。返回真正的如果他们
//内容相同。
//
//与wcscasecmp（）不同，此函数可以处理空参数。
//空C字符串被认为不同于任何非空宽C字符串，
//包括空字符串。
//注意：不同平台上的实现略有不同。
//在Windows上，此方法使用根据lc类型进行比较的wcsicmp
//环境变量。在GNU平台上，此方法使用wcscasecmp
//根据当前区域设置的lc类型类别进行比较。
//在MacOS X上，它使用Towlower，它还使用
//当前区域设置。
bool String::CaseInsensitiveWideCStringEquals(const wchar_t* lhs,
                                              const wchar_t* rhs) {
  if (lhs == NULL) return rhs == NULL;

  if (rhs == NULL) return false;

#if GTEST_OS_WINDOWS
  return _wcsicmp(lhs, rhs) == 0;
#elif GTEST_OS_LINUX && !GTEST_OS_LINUX_ANDROID
  return wcscasecmp(lhs, rhs) == 0;
#else
//Android、Mac OS X和Cygwin没有定义wcscasecmp。
//其他未知的操作系统也可能无法定义它。
  wint_t left, right;
  do {
    left = towlower(*lhs++);
    right = towlower(*rhs++);
  } while (left && left == right);
  return left == right;
#endif  //操作系统选择器
}

//返回以给定后缀结尾的真iff str，忽略大小写。
//任何字符串都被认为以空后缀结尾。
bool String::EndsWithCaseInsensitive(
    const std::string& str, const std::string& suffix) {
  const size_t str_len = str.length();
  const size_t suffix_len = suffix.length();
  return (str_len >= suffix_len) &&
         CaseInsensitiveCStringEquals(str.c_str() + str_len - suffix_len,
                                      suffix.c_str());
}

//将int值格式化为“%02d”。
std::string String::FormatIntWidth2(int value) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(2) << value;
  return ss.str();
}

//将int值格式化为“%x”。
std::string String::FormatHexInt(int value) {
  std::stringstream ss;
  ss << std::hex << std::uppercase << value;
  return ss.str();
}

//将字节格式化为“%02X”。
std::string String::FormatByte(unsigned char value) {
  std::stringstream ss;
  ss << std::setfill('0') << std::setw(2) << std::hex << std::uppercase
     << static_cast<unsigned int>(value);
  return ss.str();
}

//将stringstream中的缓冲区转换为std:：string，并转换nul
//一路到“\\0”的字节数。
std::string StringStreamToString(::std::stringstream* ss) {
  const ::std::string& str = ss->str();
  const char* const start = str.c_str();
  const char* const end = start + str.length();

  std::string result;
  result.reserve(2 * (end - start));
  for (const char* ch = start; ch != end; ++ch) {
    if (*ch == '\0') {
result += "\\0";  //将nul替换为“\\0”；
    } else {
      result += *ch;
    }
  }

  return result;
}

//将用户提供的消息附加到Google测试生成的消息。
std::string AppendUserMessage(const std::string& gtest_msg,
                              const Message& user_msg) {
//如果用户消息不为空，则追加该消息。
  const std::string user_msg_string = user_msg.GetString();
  if (user_msg_string.empty()) {
    return gtest_msg;
  }

  return gtest_msg + "\n" + user_msg_string;
}

}  //命名空间内部

//类测试结果

//创建空的测试结果。
TestResult::TestResult()
    : death_test_count_(0),
      elapsed_time_(0) {
}

//德托。
TestResult::~TestResult() {
}

//返回所有结果中的第i个测试部分结果。我可以
//范围从0到total_part_count（）-1。如果我不在这个范围内，
//中止程序。
const TestPartResult& TestResult::GetTestPartResult(int i) const {
  if (i < 0 || i >= total_part_count())
    internal::posix::Abort();
  return test_part_results_.at(i);
}

//返回第i个测试属性。我可以从0到
//测试_属性_count（）-1。如果我不在这个范围内，中止
//程序。
const TestProperty& TestResult::GetTestProperty(int i) const {
  if (i < 0 || i >= test_property_count())
    internal::posix::Abort();
  return test_properties_.at(i);
}

//清除测试部件结果。
void TestResult::ClearTestPartResults() {
  test_part_results_.clear();
}

//将测试部件结果添加到列表中。
void TestResult::AddTestPartResult(const TestPartResult& test_part_result) {
  test_part_results_.push_back(test_part_result);
}

//向列表中添加测试属性。如果属性的键与
//已表示提供的属性，此测试属性的值
//替换该键的旧值。
void TestResult::RecordProperty(const std::string& xml_element,
                                const TestProperty& test_property) {
  if (!ValidateTestProperty(xml_element, test_property)) {
    return;
  }
  internal::MutexLock lock(&test_properites_mutex_);
  const std::vector<TestProperty>::iterator property_with_matching_key =
      std::find_if(test_properties_.begin(), test_properties_.end(),
                   internal::TestPropertyKeyIs(test_property.key()));
  if (property_with_matching_key == test_properties_.end()) {
    test_properties_.push_back(test_property);
    return;
  }
  property_with_matching_key->SetValue(test_property.value());
}

//XML的<testsuites>元素中使用的保留属性列表
//输出。
static const char* const kReservedTestSuitesAttributes[] = {
  "disabled",
  "errors",
  "failures",
  "name",
  "random_seed",
  "tests",
  "time",
  "timestamp"
};

//XML的<testsuite>元素中使用的保留属性列表
//输出。
static const char* const kReservedTestSuiteAttributes[] = {
  "disabled",
  "errors",
  "failures",
  "name",
  "tests",
  "time"
};

//XML输出的<testcase>元素中使用的保留属性列表。
static const char* const kReservedTestCaseAttributes[] = {
  "classname",
  "name",
  "status",
  "time",
  "type_param",
  "value_param"
};

template <int kSize>
std::vector<std::string> ArrayAsVector(const char* const (&array)[kSize]) {
  return std::vector<std::string>(array, array + kSize);
}

static std::vector<std::string> GetReservedAttributesForElement(
    const std::string& xml_element) {
  if (xml_element == "testsuites") {
    return ArrayAsVector(kReservedTestSuitesAttributes);
  } else if (xml_element == "testsuite") {
    return ArrayAsVector(kReservedTestSuiteAttributes);
  } else if (xml_element == "testcase") {
    return ArrayAsVector(kReservedTestCaseAttributes);
  } else {
    GTEST_CHECK_(false) << "Unrecognized xml_element provided: " << xml_element;
  }
//无法访问此代码，但有些编译器可能没有意识到这一点。
  return std::vector<std::string>();
}

static std::string FormatWordList(const std::vector<std::string>& words) {
  Message word_list;
  for (size_t i = 0; i < words.size(); ++i) {
    if (i > 0 && words.size() > 2) {
      word_list << ", ";
    }
    if (i == words.size() - 1) {
      word_list << "and ";
    }
    word_list << "'" << words[i] << "'";
  }
  return word_list.GetString();
}

bool ValidateTestPropertyName(const std::string& property_name,
                              const std::vector<std::string>& reserved_names) {
  if (std::find(reserved_names.begin(), reserved_names.end(), property_name) !=
          reserved_names.end()) {
    ADD_FAILURE() << "Reserved key used in RecordProperty(): " << property_name
                  << " (" << FormatWordList(reserved_names)
                  << " are reserved by " << GTEST_NAME_ << ")";
    return false;
  }
  return true;
}

//如果键是名为的元素的保留属性，则添加失败
//XMLX元素。如果属性有效，则返回true。
bool TestResult::ValidateTestProperty(const std::string& xml_element,
                                      const TestProperty& test_property) {
  return ValidateTestPropertyName(test_property.key(),
                                  GetReservedAttributesForElement(xml_element));
}

//清除对象。
void TestResult::Clear() {
  test_part_results_.clear();
  test_properties_.clear();
  death_test_count_ = 0;
  elapsed_time_ = 0;
}

//如果测试失败，则返回true。
bool TestResult::Failed() const {
  for (int i = 0; i < total_part_count(); ++i) {
    if (GetTestPartResult(i).failed())
      return true;
  }
  return false;
}

//如果测试部分最终失败，则返回true。
static bool TestPartFatallyFailed(const TestPartResult& result) {
  return result.fatally_failed();
}

//如果测试最终失败，则返回true。
bool TestResult::HasFatalFailure() const {
  return CountIf(test_part_results_, TestPartFatallyFailed) > 0;
}

//如果测试部件非致命失败，则返回true。
static bool TestPartNonfatallyFailed(const TestPartResult& result) {
  return result.nonfatally_failed();
}

//返回true如果测试有非致命错误。
bool TestResult::HasNonfatalFailure() const {
  return CountIf(test_part_results_, TestPartNonfatallyFailed) > 0;
}

//获取所有测试部件的编号。这是数字的和
//成功的测试部件和失败的测试部件的数量。
int TestResult::total_part_count() const {
  return static_cast<int>(test_part_results_.size());
}

//返回测试属性的数目。
int TestResult::test_property_count() const {
  return static_cast<int>(test_properties_.size());
}

//课堂测试

//创建测试对象。

//选择器保存所有Google测试标志的值。
Test::Test()
    : gtest_flag_saver_(new internal::GTestFlagSaver) {
}

//d'tor恢复所有Google测试标志的值。
Test::~Test() {
  delete gtest_flag_saver_;
}

//设置测试夹具。
//
//子类可能会覆盖此项。
void Test::SetUp() {
}

//拆下测试夹具。
//
//子类可能会覆盖此项。
void Test::TearDown() {
}

//允许记录用户提供的键值对以供以后输出。
void Test::RecordProperty(const std::string& key, const std::string& value) {
  UnitTest::GetInstance()->RecordProperty(key, value);
}

//允许记录用户提供的键值对以供以后输出。
void Test::RecordProperty(const std::string& key, int value) {
  Message value_message;
  value_message << value;
  RecordProperty(key, value_message.GetString().c_str());
}

namespace internal {

void ReportFailureInUnknownLocation(TestPartResult::Type result_type,
                                    const std::string& message) {
//此函数是UnitTest的朋友，因此可以访问
//添加测试部分结果。
  UnitTest::GetInstance()->AddTestPartResult(
      result_type,
NULL,  //没有关于发生异常的源文件的信息。
-1,    //我们不知道是哪一行导致了异常。
      message,
"");   //也没有堆栈跟踪。
}

}  //命名空间内部

//Google测试要求同一测试用例中的所有测试都使用相同的测试
//夹具类。此函数检查当前测试是否具有
//与当前测试用例中的第一个测试相同的fixture类。如果
//是的，它返回true；否则它将生成一个Google测试失败，并且
//返回false。
bool Test::HasSameFixtureClass() {
  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  const TestCase* const test_case = impl->current_test_case();

//有关当前测试用例中第一个测试的信息。
  const TestInfo* const first_test_info = test_case->test_info_list()[0];
  const internal::TypeId first_fixture_id = first_test_info->fixture_class_id_;
  const char* const first_test_name = first_test_info->name();

//有关当前测试的信息。
  const TestInfo* const this_test_info = impl->current_test_info();
  const internal::TypeId this_fixture_id = this_test_info->fixture_class_id_;
  const char* const this_test_name = this_test_info->name();

  if (this_fixture_id != first_fixture_id) {
//第一个测试是使用测试定义的吗？
    const bool first_is_TEST = first_fixture_id == internal::GetTestTypeId();
//此测试是使用测试定义的吗？
    const bool this_is_TEST = this_fixture_id == internal::GetTestTypeId();

    if (first_is_TEST || this_is_TEST) {
//测试和测试都出现在同一个测试用例中，这是不正确的。
//告诉用户如何修复此问题。

//获取测试名称和测试名称。注意
//第一个“是”测试和“是”测试不能同时为真，因为
//两个测试的夹具ID不同。
      const char* const TEST_name =
          first_is_TEST ? first_test_name : this_test_name;
      const char* const TEST_F_name =
          first_is_TEST ? this_test_name : first_test_name;

      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class, so mixing TEST_F and TEST in the same test case is\n"
          << "illegal.  In test case " << this_test_info->test_case_name()
          << ",\n"
          << "test " << TEST_F_name << " is defined using TEST_F but\n"
          << "test " << TEST_name << " is defined using TEST.  You probably\n"
          << "want to change the TEST to TEST_F or move it to another test\n"
          << "case.";
    } else {
//具有相同名称的两个fixture类出现在两个不同的
//命名空间，这是不允许的。告诉用户如何修复此问题。
      ADD_FAILURE()
          << "All tests in the same test case must use the same test fixture\n"
          << "class.  However, in test case "
          << this_test_info->test_case_name() << ",\n"
          << "you defined test " << first_test_name
          << " and test " << this_test_name << "\n"
          << "using two different test fixture classes.  This can happen if\n"
          << "the two classes are from different namespaces or translation\n"
          << "units and have the same name.  You should probably rename one\n"
          << "of the classes to put the tests into different test cases.";
    }
    return false;
  }

  return true;
}

#if GTEST_HAS_SEH

//向当前测试添加“异常引发”致命失败。这个
//函数通过输出参数指针返回其结果，因为vc++
//禁止在函数的堆栈上创建具有析构函数的对象
//使用“尝试”（参见错误C2712）。
static std::string* FormatSehExceptionMessage(DWORD exception_code,
                                              const char* location) {
  Message message;
  message << "SEH exception with code 0x" << std::setbase(16) <<
    exception_code << std::setbase(10) << " thrown in " << location << ".";

  return new std::string(message.GetString());
}

#endif  //格斯塔夫哈斯塞赫

namespace internal {

#if GTEST_HAS_EXCEPTIONS

//向当前测试添加“异常引发”致命失败。
static std::string FormatCxxExceptionMessage(const char* description,
                                             const char* location) {
  Message message;
  if (description != NULL) {
    message << "C++ exception with description \"" << description << "\"";
  } else {
    message << "Unknown C++ exception";
  }
  message << " thrown in " << location << ".";

  return message.GetString();
}

static std::string PrintTestPartResultToString(
    const TestPartResult& test_part_result);

GoogleTestFailureException::GoogleTestFailureException(
    const TestPartResult& failure)
    : ::std::runtime_error(PrintTestPartResultToString(failure).c_str()) {}

#endif  //gtest_有\u例外

//我们将这些助手函数作为IBM的XLC放在内部名称空间中。
//如果代码声明为静态的，编译器将拒绝该代码。

//运行给定的方法并处理它抛出的SEH异常，当
//支持seh；如果
//SEH例外。（微软编译器无法处理SEH和C++）
//同一函数中的异常。因此，我们提供一个单独的
//用于处理SEH异常的包装函数。）
template <class T, typename Result>
Result HandleSehExceptionsInMethodIfSupported(
    T* object, Result (T::*method)(), const char* location) {
#if GTEST_HAS_SEH
  __try {
    return (object->*method)();
} __except (internal::UnitTestOptions::GTestShouldProcessSEH(  //诺林
      GetExceptionCode())) {
//我们在堆上创建异常消息，因为VC++禁止
//使用_uuTry在函数的堆栈上创建带有析构函数的对象
//（参见错误C2712）。
    std::string* exception_message = FormatSehExceptionMessage(
        GetExceptionCode(), location);
    internal::ReportFailureInUnknownLocation(TestPartResult::kFatalFailure,
                                             *exception_message);
    delete exception_message;
    return static_cast<Result>(0);
  }
#else
  (void)location;
  return (object->*method)();
#endif  //格斯塔夫哈斯塞赫
}

//运行给定的方法，捕获并报告C++和/或SEH样式
//异常（如果支持）；返回类型的0值
//如果出现SEH异常，则返回结果。
template <class T, typename Result>
Result HandleExceptionsInMethodIfSupported(
    T* object, Result (T::*method)(), const char* location) {
//注意：用户代码会影响Google测试的处理方式。
//通过设置gtest_标志（catch_exceptions），但仅在
//开始运行所有测试。技术上可以检查旗子
//在捕获异常并报告或重新引发
//基于标志值的异常：
//
//尝试{
////执行测试方法。
//} catch（…）{
//if（gtest_标志（catch_异常））
////将异常报告为失败。
//其他的
//throw；//重新引发原始异常。
//}
//
//但是，此标志的目的是允许程序进入
//引发异常时的调试器。在大多数平台上，
//控件进入catch块，异常源信息为
//丢失，调试器将在
//重新抛出这个函数——而不是在原来的
//在被测试的代码中抛出语句。因此，我们
//提前检查，牺牲影响谷歌测试的能力
//在引发异常的方法中处理异常。
  if (internal::GetUnitTestImpl()->catch_exceptions()) {
#if GTEST_HAS_EXCEPTIONS
    try {
      return HandleSehExceptionsInMethodIfSupported(object, method, location);
} catch (const internal::GoogleTestFailureException&) {  //诺林
//此异常类型只能由失败的Google引发
//测试断言，目的是让另一个测试
//框架捕获它。所以我们只是把它扔了。
      throw;
} catch (const std::exception& e) {  //诺林
      internal::ReportFailureInUnknownLocation(
          TestPartResult::kFatalFailure,
          FormatCxxExceptionMessage(e.what(), location));
} catch (...) {  //诺林
      internal::ReportFailureInUnknownLocation(
          TestPartResult::kFatalFailure,
          FormatCxxExceptionMessage(NULL, location));
    }
    return static_cast<Result>(0);
#else
    return HandleSehExceptionsInMethodIfSupported(object, method, location);
#endif  //gtest_有\u例外
  } else {
    return (object->*method)();
  }
}

}  //命名空间内部

//运行测试并更新测试结果。
void Test::Run() {
  if (!HasSameFixtureClass()) return;

  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  impl->os_stack_trace_getter()->UponLeavingGTest();
  internal::HandleExceptionsInMethodIfSupported(this, &Test::SetUp, "SetUp()");
//只有当setup（）成功时，我们才会运行测试。
  if (!HasFatalFailure()) {
    impl->os_stack_trace_getter()->UponLeavingGTest();
    internal::HandleExceptionsInMethodIfSupported(
        this, &Test::TestBody, "the test body");
  }

//但是，我们要尽可能地清理。因此我们将
//始终调用TearDown（），即使setup（）或测试体
//失败。
  impl->os_stack_trace_getter()->UponLeavingGTest();
  internal::HandleExceptionsInMethodIfSupported(
      this, &Test::TearDown, "TearDown()");
}

//如果当前测试有致命错误，则返回true。
bool Test::HasFatalFailure() {
  return internal::GetUnitTestImpl()->current_test_result()->HasFatalFailure();
}

//返回true如果当前测试有非致命错误。
bool Test::HasNonfatalFailure() {
  return internal::GetUnitTestImpl()->current_test_result()->
      HasNonfatalFailure();
}

//类测试信息

//构造一个testinfo对象。它拥有测试工厂的所有权
//对象。
TestInfo::TestInfo(const std::string& a_test_case_name,
                   const std::string& a_name,
                   const char* a_type_param,
                   const char* a_value_param,
                   internal::TypeId fixture_class_id,
                   internal::TestFactoryBase* factory)
    : test_case_name_(a_test_case_name),
      name_(a_name),
      type_param_(a_type_param ? new std::string(a_type_param) : NULL),
      value_param_(a_value_param ? new std::string(a_value_param) : NULL),
      fixture_class_id_(fixture_class_id),
      should_run_(false),
      is_disabled_(false),
      matches_filter_(false),
      factory_(factory),
      result_() {}

//销毁testinfo对象。
TestInfo::~TestInfo() { delete factory_; }

namespace internal {

//创建一个新的testinfo对象并将其注册到google test；
//返回创建的对象。
//
//争论：
//
//测试用例名称：测试用例的名称
//名称：测试名称
//type_param：测试类型参数的名称，如果
//这不是类型化测试或类型参数化测试。
//value_param：测试值参数的文本表示，
//如果不是值参数化测试，则为空。
//fixture_class_id：测试fixture类的ID
//设置\u tc:指向设置测试用例的函数的指针
//分解：指向分解测试用例的函数的指针
//工厂：指向创建测试对象的工厂的指针。
//新创建的testinfo实例将假定
//工厂对象的所有权。
TestInfo* MakeAndRegisterTestInfo(
    const char* test_case_name,
    const char* name,
    const char* type_param,
    const char* value_param,
    TypeId fixture_class_id,
    SetUpTestCaseFunc set_up_tc,
    TearDownTestCaseFunc tear_down_tc,
    TestFactoryBase* factory) {
  TestInfo* const test_info =
      new TestInfo(test_case_name, name, type_param, value_param,
                   fixture_class_id, factory);
  GetUnitTestImpl()->AddTestInfo(set_up_tc, tear_down_tc, test_info);
  return test_info;
}

#if GTEST_HAS_PARAM_TEST
void ReportInvalidTestCaseType(const char* test_case_name,
                               const char* file, int line) {
  Message errors;
  errors
      << "Attempted redefinition of test case " << test_case_name << ".\n"
      << "All tests in the same test case must use the same test fixture\n"
      << "class.  However, in test case " << test_case_name << ", you tried\n"
      << "to define a test using a fixture class different from the one\n"
      << "used earlier. This can happen if the two fixture classes are\n"
      << "from different namespaces and have the same name. You should\n"
      << "probably rename one of the classes to put the tests into different\n"
      << "test cases.";

  fprintf(stderr, "%s %s", FormatFileLocation(file, line).c_str(),
          errors.GetString().c_str());
}
#endif  //gtest_有参数测试

}  //命名空间内部

namespace {

//根据已知的
//价值。
//
//这仅用于测试用例类的实现。我们放
//它在匿名名称空间中防止污染外部
//命名空间。
//
//testnameis是可复制的。
class TestNameIs {
 public:
//构造函数。
//
//testnameis没有默认的构造函数。
  explicit TestNameIs(const char* name)
      : name_(name) {}

//如果测试信息的测试名称与名称匹配，则返回true。
  bool operator()(const TestInfo * test_info) const {
    return test_info && test_info->name() == name_;
  }

 private:
  std::string name_;
};

}  //命名空间

namespace internal {

//此方法扩展用宏测试注册的所有参数化测试
//并将测试用例实例化为常规测试并注册这些测试。
//这将在程序运行时执行一次。
void UnitTestImpl::RegisterParameterizedTests() {
#if GTEST_HAS_PARAM_TEST
  if (!parameterized_tests_registered_) {
    parameterized_test_registry_.RegisterTests();
    parameterized_tests_registered_ = true;
  }
#endif
}

}  //命名空间内部

//创建测试对象，运行它，记录它的结果，然后
//删除它。
void TestInfo::Run() {
  if (!should_run_) return;

//告诉UnitTest存储测试结果的位置。
  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  impl->set_current_test_info(this);

  TestEventListener* repeater = UnitTest::GetInstance()->listeners().repeater();

//通知单元测试事件侦听器测试即将开始。
  repeater->OnTestStart(*this);

  const TimeInMillis start = internal::GetTimeInMillis();

  impl->os_stack_trace_getter()->UponLeavingGTest();

//创建测试对象。
  Test* const test = internal::HandleExceptionsInMethodIfSupported(
      factory_, &internal::TestFactoryBase::CreateTest,
      "the test fixture's constructor");

//仅当创建了测试对象并且
//构造函数未生成致命错误。
  if ((test != NULL) && !Test::HasFatalFailure()) {
//这并不是因为所有可以抛出的用户代码都被包装在
//异常处理代码。
    test->Run();
  }

//删除测试对象。
  impl->os_stack_trace_getter()->UponLeavingGTest();
  internal::HandleExceptionsInMethodIfSupported(
      test, &Test::DeleteSelf_, "the test fixture's destructor");

  result_.set_elapsed_time(internal::GetTimeInMillis() - start);

//通知单元测试事件侦听器测试刚刚完成。
  repeater->OnTestEnd(*this);

//告诉UnitTest停止将断言结果与此关联
//测试。
  impl->set_current_test_info(NULL);
}

//类测试用例

//获取此测试用例中成功的测试数。
int TestCase::successful_test_count() const {
  return CountIf(test_info_list_, TestPassed);
}

//获取此测试用例中失败的测试数。
int TestCase::failed_test_count() const {
  return CountIf(test_info_list_, TestFailed);
}

//获取将在XML报告中报告的禁用测试数。
int TestCase::reportable_disabled_test_count() const {
  return CountIf(test_info_list_, TestReportableDisabled);
}

//获取此测试用例中禁用的测试数。
int TestCase::disabled_test_count() const {
  return CountIf(test_info_list_, TestDisabled);
}

//获取要在XML报表中打印的测试数。
int TestCase::reportable_test_count() const {
  return CountIf(test_info_list_, TestReportable);
}

//获取此测试用例中应该运行的测试数。
int TestCase::test_to_run_count() const {
  return CountIf(test_info_list_, ShouldRunTest);
}

//获取所有测试的数目。
int TestCase::total_test_count() const {
  return static_cast<int>(test_info_list_.size());
}

//创建具有给定名称的测试用例。
//
//争论：
//
//名称：测试用例的名称
//a_type_param：测试用例类型参数的名称，如果
//这不是类型化或类型参数化测试用例。
//设置\u tc:指向设置测试用例的函数的指针
//分解：指向分解测试用例的函数的指针
TestCase::TestCase(const char* a_name, const char* a_type_param,
                   Test::SetUpTestCaseFunc set_up_tc,
                   Test::TearDownTestCaseFunc tear_down_tc)
    : name_(a_name),
      type_param_(a_type_param ? new std::string(a_type_param) : NULL),
      set_up_tc_(set_up_tc),
      tear_down_tc_(tear_down_tc),
      should_run_(false),
      elapsed_time_(0) {
}

//测试用例的析构函数。
TestCase::~TestCase() {
//删除集合中的每个测试。
  ForEach(test_info_list_, internal::Delete<TestInfo>);
}

//返回所有测试中的第i个测试。我可以从0到
//总测试计数（）-1。如果我不在该范围内，则返回空值。
const TestInfo* TestCase::GetTestInfo(int i) const {
  const int index = GetElementOr(test_indices_, i, -1);
  return index < 0 ? NULL : test_info_list_[index];
}

//返回所有测试中的第i个测试。我可以从0到
//总测试计数（）-1。如果我不在该范围内，则返回空值。
TestInfo* TestCase::GetMutableTestInfo(int i) {
  const int index = GetElementOr(test_indices_, i, -1);
  return index < 0 ? NULL : test_info_list_[index];
}

//将测试添加到此测试用例。将删除测试
//测试用例对象的销毁。
void TestCase::AddTestInfo(TestInfo * test_info) {
  test_info_list_.push_back(test_info);
  test_indices_.push_back(static_cast<int>(test_indices_.size()));
}

//运行此测试用例中的每个测试。
void TestCase::Run() {
  if (!should_run_) return;

  internal::UnitTestImpl* const impl = internal::GetUnitTestImpl();
  impl->set_current_test_case(this);

  TestEventListener* repeater = UnitTest::GetInstance()->listeners().repeater();

  repeater->OnTestCaseStart(*this);
  impl->os_stack_trace_getter()->UponLeavingGTest();
  internal::HandleExceptionsInMethodIfSupported(
      this, &TestCase::RunSetUpTestCase, "SetUpTestCase()");

  const internal::TimeInMillis start = internal::GetTimeInMillis();
  for (int i = 0; i < total_test_count(); i++) {
    GetMutableTestInfo(i)->Run();
  }
  elapsed_time_ = internal::GetTimeInMillis() - start;

  impl->os_stack_trace_getter()->UponLeavingGTest();
  internal::HandleExceptionsInMethodIfSupported(
      this, &TestCase::RunTearDownTestCase, "TearDownTestCase()");

  repeater->OnTestCaseEnd(*this);
  impl->set_current_test_case(NULL);
}

//清除此测试用例中所有测试的结果。
void TestCase::ClearResult() {
  ad_hoc_test_result_.Clear();
  ForEach(test_info_list_, TestInfo::ClearTestResult);
}

//在此测试用例中无序处理测试。
void TestCase::ShuffleTests(internal::Random* random) {
  Shuffle(random, &test_indices_);
}

//将测试顺序恢复到第一次随机播放之前。
void TestCase::UnshuffleTests() {
  for (size_t i = 0; i < test_indices_.size(); i++) {
    test_indices_[i] = static_cast<int>(i);
  }
}

//格式化可数名词。根据其数量，或者
//使用单数形式或复数形式。例如
//
//formatCountableNoun（1，“formula”，“formuli”）返回“1 formula”。
//formatCountable名词（5，“book”，“books”）返回“5 books”。
static std::string FormatCountableNoun(int count,
                                       const char * singular_form,
                                       const char * plural_form) {
  return internal::StreamableToString(count) + " " +
      (count == 1 ? singular_form : plural_form);
}

//设置测试计数的格式。
static std::string FormatTestCount(int test_count) {
  return FormatCountableNoun(test_count, "test", "tests");
}

//格式化测试用例的计数。
static std::string FormatTestCaseCount(int test_case_count) {
  return FormatCountableNoun(test_case_count, "test case", "test cases");
}

//将testpartresult:：type枚举转换为对人友好的字符串
//代表。knonfatalfailure和kfatalfailure都被翻译成
//“失败”，因为用户通常不关心区别
//在查看测试结果时介于两者之间。
static const char * TestPartResultTypeToString(TestPartResult::Type type) {
  switch (type) {
    case TestPartResult::kSuccess:
      return "Success";

    case TestPartResult::kNonFatalFailure:
    case TestPartResult::kFatalFailure:
#ifdef _MSC_VER
      return "error: ";
#else
      return "Failure\n";
#endif
    default:
      return "Unknown result type";
  }
}

namespace internal {

//将testpartresult打印到std:：string。
static std::string PrintTestPartResultToString(
    const TestPartResult& test_part_result) {
  return (Message()
          << internal::FormatFileLocation(test_part_result.file_name(),
                                          test_part_result.line_number())
          << " " << TestPartResultTypeToString(test_part_result.type())
          << test_part_result.message()).GetString();
}

//打印testpartresult。
static void PrintTestPartResult(const TestPartResult& test_part_result) {
  const std::string& result =
      PrintTestPartResultToString(test_part_result);
  printf("%s\n", result.c_str());
  fflush(stdout);
//如果测试程序在Visual Studio或调试器中运行，则
//以下语句将测试部件结果消息添加到输出
//窗口，以便用户可以双击它以跳转到
//对应的源代码位置；否则它们什么也不做。
#if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE
//在Windows Mobile上，我们不将OutputDebugString*（）称为打印
//到stdout已经由outputDebugString（）完成了-我们没有
//希望同一封邮件打印两次。
  ::OutputDebugStringA(result.c_str());
  ::OutputDebugStringA("\n");
#endif
}

//类PrettyUnitTestResultPrinter

enum GTestColor {
  COLOR_DEFAULT,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW
};

#if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE && \
    !GTEST_OS_WINDOWS_PHONE && !GTEST_OS_WINDOWS_RT

//返回给定颜色的字符属性。
WORD GetColorAttribute(GTestColor color) {
  switch (color) {
    case COLOR_RED:    return FOREGROUND_RED;
    case COLOR_GREEN:  return FOREGROUND_GREEN;
    case COLOR_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
    default:           return 0;
  }
}

#else

//返回给定颜色的ANSI颜色代码。颜色默认为
//输入无效。
const char* GetAnsiColorCode(GTestColor color) {
  switch (color) {
    case COLOR_RED:     return "1";
    case COLOR_GREEN:   return "2";
    case COLOR_YELLOW:  return "3";
    default:            return NULL;
  };
}

#endif  //测试Windows&！GTEST操作系统Windows移动

//返回真正的iff google测试应该在输出中使用颜色。
bool ShouldUseColor(bool stdout_is_tty) {
  const char* const gtest_color = GTEST_FLAG(color).c_str();

  if (String::CaseInsensitiveCStringEquals(gtest_color, "auto")) {
#if GTEST_OS_WINDOWS
//在Windows上，通常不设置术语变量，但是
//那里的控制台支持颜色。
    return stdout_is_tty;
#else
//在非Windows平台上，我们依赖于变量这个术语。
    const char* const term = posix::GetEnv("TERM");
    const bool term_supports_color =
        String::CStringEquals(term, "xterm") ||
        String::CStringEquals(term, "xterm-color") ||
        String::CStringEquals(term, "xterm-256color") ||
        String::CStringEquals(term, "screen") ||
        String::CStringEquals(term, "screen-256color") ||
        String::CStringEquals(term, "linux") ||
        String::CStringEquals(term, "cygwin");
    return stdout_is_tty && term_supports_color;
#endif  //GTEST操作系统窗口
  }

  return String::CaseInsensitiveCStringEquals(gtest_color, "yes") ||
      String::CaseInsensitiveCStringEquals(gtest_color, "true") ||
      String::CaseInsensitiveCStringEquals(gtest_color, "t") ||
      String::CStringEquals(gtest_color, "1");
//我们将“是”、“真”、“T”和“1”作为“是”的意思。如果
//值既不是这些值，也不是“自动”值，我们将其视为“否”。
//保守一点。
}

//用于将彩色字符串打印到stdout的帮助程序。注意，在Windows上，我们
//不能简单地发出特殊字符并使终端更改颜色。
//此例程必须实际发出字符，而不是返回字符串
//它在打印时会被着色，就像在Linux上一样。
void ColoredPrintf(GTestColor color, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

#if GTEST_OS_WINDOWS_MOBILE || GTEST_OS_SYMBIAN || GTEST_OS_ZOS || \
    GTEST_OS_IOS || GTEST_OS_WINDOWS_PHONE || GTEST_OS_WINDOWS_RT
  const bool use_color = AlwaysFalse();
#else
  static const bool in_color_mode =
      ShouldUseColor(posix::IsATTY(posix::FileNo(stdout)) != 0);
  const bool use_color = in_color_mode && (color != COLOR_DEFAULT);
#endif  //gtest_os_windows_mobile_gtest_os_symbian_gtest_os_zos
//“！=0'比较是满足MSVC 7.1的必要条件。

  if (!use_color) {
    vprintf(fmt, args);
    va_end(args);
    return;
  }

#if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_MOBILE && \
    !GTEST_OS_WINDOWS_PHONE && !GTEST_OS_WINDOWS_RT
  const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

//获取当前文本颜色。
  CONSOLE_SCREEN_BUFFER_INFO buffer_info;
  GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
  const WORD old_color_attrs = buffer_info.wAttributes;

//我们需要在每个
//setconsoletexttribute调用，以防它影响已经存在的文本
//已打印，但尚未到达控制台。
  fflush(stdout);
  SetConsoleTextAttribute(stdout_handle,
                          GetColorAttribute(color) | FOREGROUND_INTENSITY);
  vprintf(fmt, args);

  fflush(stdout);
//恢复文本颜色。
  SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
  printf("\033[0;3%sm", GetAnsiColorCode(color));
  vprintf(fmt, args);
printf("\033[m");  //将终端重置为默认值。
#endif  //测试Windows&！GTEST操作系统Windows移动
  va_end(args);
}

//在Google测试的文本输出中打印的文本和--gunit-list-u测试
//输出以标记测试的类型参数和值参数。
static const char kTypeParamLabel[] = "TypeParam";
static const char kValueParamLabel[] = "GetParam()";

void PrintFullTestCommentIfPresent(const TestInfo& test_info) {
  const char* const type_param = test_info.type_param();
  const char* const value_param = test_info.value_param();

  if (type_param != NULL || value_param != NULL) {
    printf(", where ");
    if (type_param != NULL) {
      printf("%s = %s", kTypeParamLabel, type_param);
      if (value_param != NULL)
        printf(" and ");
    }
    if (value_param != NULL) {
      printf("%s = %s", kValueParamLabel, value_param);
    }
  }
}

//此类实现了TestEventListener接口。
//
//类PrettyUnitTestResultPrinter是可复制的。
class PrettyUnitTestResultPrinter : public TestEventListener {
 public:
  PrettyUnitTestResultPrinter() {}
  static void PrintTestName(const char * test_case, const char * test) {
    printf("%s.%s", test_case, test);
  }

//以下方法重写TestEventListener类中的内容。
  /*tual void ontestprogramstart（const unit test&/*单元测试*/）
  虚拟void ontestitutionStart（const unit test&unit_test，int迭代）；
  虚拟void onenvironmentssetupstart（const unit test&unit_test）；
  虚拟void OnEnvironmentsSetupend（const unittest&/*Unit-tes*/) {}

  virtual void OnTestCaseStart(const TestCase& test_case);
  virtual void OnTestStart(const TestInfo& test_info);
  virtual void OnTestPartResult(const TestPartResult& result);
  virtual void OnTestEnd(const TestInfo& test_info);
  virtual void OnTestCaseEnd(const TestCase& test_case);
  virtual void OnEnvironmentsTearDownStart(const UnitTest& unit_test);
  /*环境温度下降时的实际空隙（const unit test&/*unity_test*/）
  虚拟void ontestitutionend（const unit test&unit_test，int迭代）；
  虚拟void ontestprogrammend（const unittest和/*unit-tes*/) {}


 private:
  static void PrintFailedTests(const UnitTest& unit_test);
};

//在每次测试迭代开始前激发。
void PrettyUnitTestResultPrinter::OnTestIterationStart(
    const UnitTest& unit_test, int iteration) {
  if (GTEST_FLAG(repeat) != 1)
    printf("\nRepeating all tests (iteration %d) . . .\n\n", iteration + 1);

  const char* const filter = GTEST_FLAG(filter).c_str();

//如果不是，则打印过滤器*。这提醒用户
//可以跳过测试。
  if (!String::CStringEquals(filter, kUniversalFilter)) {
    ColoredPrintf(COLOR_YELLOW,
                  "Note: %s filter = %s\n", GTEST_NAME_, filter);
  }

  if (internal::ShouldShard(kTestTotalShards, kTestShardIndex, false)) {
    const Int32 shard_index = Int32FromEnvOrDie(kTestShardIndex, -1);
    ColoredPrintf(COLOR_YELLOW,
                  "Note: This is test shard %d of %s.\n",
                  static_cast<int>(shard_index) + 1,
                  internal::posix::GetEnv(kTestTotalShards));
  }

  if (GTEST_FLAG(shuffle)) {
    ColoredPrintf(COLOR_YELLOW,
                  "Note: Randomizing tests' orders with a seed of %d .\n",
                  unit_test.random_seed());
  }

  ColoredPrintf(COLOR_GREEN,  "[==========] ");
  printf("Running %s from %s.\n",
         FormatTestCount(unit_test.test_to_run_count()).c_str(),
         FormatTestCaseCount(unit_test.test_case_to_run_count()).c_str());
  fflush(stdout);
}

void PrettyUnitTestResultPrinter::OnEnvironmentsSetUpStart(
    /*ST单元测试&/*单元测试*/）
  coloredprintf（颜色_green，“[------]”）；
  printf（“全局测试环境设置。\n”）；
  FFLUH（STDUT）；
}

void prettyUnitTestResultPrinter:：ontestcaseStart（const test case&test_case）
  const std：：字符串计数=
      formatCountable名词（test_case.test_to_run_count（），“test”，“tests”）；
  coloredprintf（颜色_green，“[------]”）；
  printf（“%s来自%s”，counts.c_str（），test_case.name（））；
  if（test_case.type_param（）==空）
    PrTNF（“\n”）；
  }否则{
    printf（“，其中%s=%s\n”，ktypeParamLabel，test_case.type_param（））；
  }
  FFLUH（STDUT）；
}

void prettyUnitTestResultPrinter:：ontestStart（const test info&test_info）
  coloredprintf（颜色_green，“[run]”）；
  printTestName（test_info.test_case_name（），test_info.name（））；
  PrTNF（“\n”）；
  FFLUH（STDUT）；
}

//在断言失败后调用。
void prettyUnitTestResultPrinter:：ontestPartResult（）。
    const测试部分结果和结果）
  //如果测试部分成功，我们不需要做任何事情。
  if（result.type（）==testpartresult:：ksuccess）
    返回；

  //打印来自断言的失败消息（例如，预期此消息并得到该消息）。
  打印测试部件结果（结果）；
  FFLUH（STDUT）；
}

void prettyUnitTestResultPrinter:：ontested（const test info&test_info）
  if（test_info.result（）->passed（））
    coloredprintf（颜色_green，“[确定]”）；
  }否则{
    coloredprintf（color_red，“[失败]”）；
  }
  printTestName（test_info.test_case_name（），test_info.name（））；
  if（test_info.result（）->failed（））
    打印完整的测试评论（测试信息）；

  if（gtest_flag（print_time））
    printf（“%s ms”），内部：：streamabletoString（
           test_info.result（）->elapsed_time（）.c_str（））；
  }否则{
    PrTNF（“\n”）；
  }
  FFLUH（STDUT）；
}

void prettyUnitTestResultPrinter:：ontestCaseEnd（const test case&test_case）
  如果（！）gtest_flag（print_time））返回；

  const std：：字符串计数=
      formatCountable名词（test_case.test_to_run_count（），“test”，“tests”）；
  coloredprintf（颜色_green，“[------]”）；
  printf（“%s来自%s（总计%s ms））\n\n”，
         counts.c_str（），test_case.name（），
         内部：：streamableToString（test_case.elapsed_time（）.c_str（））；
  FFLUH（STDUT）；
}

void prettyUnitTestResultPrinter:：OnEnvironmentStearDownStart（
    单位测试和/单位测试*/) {

  ColoredPrintf(COLOR_GREEN,  "[----------] ");
  printf("Global test environment tear-down\n");
  fflush(stdout);
}

//用于打印失败测试列表的内部帮助程序。
void PrettyUnitTestResultPrinter::PrintFailedTests(const UnitTest& unit_test) {
  const int failed_test_count = unit_test.failed_test_count();
  if (failed_test_count == 0) {
    return;
  }

  for (int i = 0; i < unit_test.total_test_case_count(); ++i) {
    const TestCase& test_case = *unit_test.GetTestCase(i);
    if (!test_case.should_run() || (test_case.failed_test_count() == 0)) {
      continue;
    }
    for (int j = 0; j < test_case.total_test_count(); ++j) {
      const TestInfo& test_info = *test_case.GetTestInfo(j);
      if (!test_info.should_run() || test_info.result()->Passed()) {
        continue;
      }
      ColoredPrintf(COLOR_RED, "[  FAILED  ] ");
      printf("%s.%s", test_case.name(), test_info.name());
      PrintFullTestCommentIfPresent(test_info);
      printf("\n");
    }
  }
}

void PrettyUnitTestResultPrinter::OnTestIterationEnd(const UnitTest& unit_test,
                                                     /*/*迭代*/）
  coloredprintf（颜色_green，“[======”）；
  printf（“%s从%s运行。”，
         格式测试计数（Unit_test.test_to_run_count（））.c_str（），
         格式test case count（unit_test.test_case_to_run_count（））.c_str（）；
  if（gtest_flag（print_time））
    printf（“%s ms total）”，
           内部：：streamableToString（unit_test.elapsed_time（））.c_str（））；
  }
  PrTNF（“\n”）；
  coloredprintf（颜色_green，“[通过]”）；
  printf（“%s.\n”，formattestcount（unit_test.successful_test_count（））.c_str（））；

  int num_failures=单元_test.failed_test_count（）；
  如果（！）单元测试通过（））
    const int failed_test_count=unit_test.failed_test_count（）；
    coloredprintf（color_red，“[失败]”）；
    printf（“%s，如下所列：\n”，formattestcount（failed_test_count）.c_str（））；
    打印失败测试（单元测试）；
    printf（“\n%2d失败%s\n”，num_失败，
                        num_failures==1？“test”：“测试”；
  }

  int num_disabled=单元_test.reportable_disabled_test_count（）；
  如果（num_禁用&！gtest_标志（也可以运行禁用的_测试）
    如果（！）NUMIN失败）{
      printf（“\n”）；//如果没有显示故障标志，则添加一个间隔符。
    }
    彩色打印F（黄色，
                  “您已禁用了%d个%s\n\n”，
                  禁用数字
                  num_disabled==1？“test”：“测试”；
  }
  //确保之前打印Google测试输出，例如HeapChecker输出。
  FFLUH（STDUT）；
}

//结束PrettyUnitTestResultPrinter

//类testeventrepeater
/ /
//此类将事件转发给其他事件侦听器。
类TestEventRepeater:公共TestEventListener_
 公众：
  testeventrepeater（）：转发_启用_u（真）
  虚拟~testeventrepeater（）；
  void append（testeventlistener*listener）；
  testeventlistener*版本（testeventlistener*listener）；

  //控制是否将事件转发给侦听器。设置为假
  //在死亡测试子进程中。
  bool forwarding_enabled（）const返回转发_enabled_
  void set_forwarding_enabled（bool enable）forwarding_enabled_=enable；

  虚拟void ontestprogramstart（const unit test&unit_test）；
  虚拟void ontestitutionStart（const unit test&unit_test，int迭代）；
  虚拟void onenvironmentssetupstart（const unit test&unit_test）；
  虚拟虚空环境设置（const unit test&unit_test）；
  虚拟void ontestcaseStart（const test case&test_case）；
  虚拟void onteststart（const test info&test_info）；
  虚拟void contestpartresult（const testpartresult&result）；
  虚拟void ontested（const test info&test_info）；
  虚拟void ontestcaseend（const test case&test_case）；
  虚拟虚空环境下开始（const unit test&unit_test）；
  虚拟虚空环境下端（const unit test&unit_test）；
  虚拟void ontestitutionend（const unit test&unit_test，int迭代）；
  虚拟void ontestprogrammend（const unit test&unit_test）；

 私人：
  //控制是否将事件转发给侦听器。设置为假
  //在死亡测试子进程中。
  bool转发\已启用\；
  //接收事件的侦听器列表。
  std:：vector<testeventlistener*>监听器

  gtest不允许复制和分配（testeventrepeater）；
}；

testeventrepeater:：~testeventrepeater（）
  foreach（listeners_u，delete<testeventlistener>）；
}

void testeventrepeater:：append（testeventlistener*listener）
  倾听者推回（倾听者）；
}

//todo（vladl@google.com）：将搜索功能分解为vector:：find。
testeventlistener*testeventrepeater:：release（testeventlistener*listener）
  对于（size_t i=0；i<listeners_.size（）；++i）
    if（listeners_[i]==listener）
      listeners_u.erase（listeners_u.begin（）+i）；
      返回监听器；
    }
  }

  返回空；
}

//由于大多数方法非常相似，所以使用宏来减少样板文件。
//这定义了将调用转发给所有侦听器的成员。
定义GTEST_转发器_方法_u（名称、类型）
void testeventrepeater:：name（const type&parameter）\
  如果（转发已启用）
    对于（size_t i=0；i<listeners_.size（）；i++）
      监听器名称（参数）；\
    }
  }
}
//这定义了一个成员，该成员将调用反向转发给所有侦听器
/订购。
定义gtest_reverse_repeater_method_u（名称，类型）
void testeventrepeater:：name（const type&parameter）\
  如果（转发已启用）
    for（int i=static_cast<int>（listeners_.size（））-1；i>=0；i--）
      监听器名称（参数）；\
    }
  }
}

gtest_转发器\方法（ontestprogramstart，unittest）
GTESTU转发器U方法（OnEnvironmentsSetupStart，UnitTest）
gtest_repeater_方法（ontestcasestart，testcase）
gtest_repeater_method_u（onteststart，testinfo）
gtest_repeater_方法（ontestpartresult，testpartresult）
GTESTU转发器U方法（OnEnvironmentStearDownStart，UnitTest）
GTESTU REVERSEU RESTERU METHODUU（OnEnvironmentsSetupend，UnitTest）
GTESTU REVERSEU RESTERU METHODUUU（OnEnvironmentStearDownend，UnitTest）
gtest_reverse_repeater_method_uu（Ontestend，testinfo）
gtest_reverse_repeater_method_u（ontestcasesend，测试用例）
GTESTU REVERSEU RESTERU METHODUU（OnTest程序、UnitTest）

Undef gtest_转发器_方法
Undef gtest_Reverse_Repeater_方法

void testeventrepeater:：ontestitutionstart（const unit test&unit_test，
                                             int迭代）
  如果（转发已启用）
    对于（size_t i=0；i<listeners_.size（）；i++）
      Listeners_[i]->OneStitutionStart（单元测试、迭代）；
    }
  }
}

void testeventrepeater:：ontestitutionend（const unit test&unit_test，
                                           int迭代）
  如果（转发已启用）
    for（int i=static_cast<int>（listeners_.size（））-1；i>=0；i--）
      Listeners_[i]->OneStitutionEnd（单元测试、迭代）；
    }
  }
}

//结束testeventrepeater

//此类生成一个XML输出文件。
类xmlUnitTestResultPrinter:public EmptyTestEventListener_
 公众：
  显式xmluntestresultprinter（const char*输出文件）；

  虚拟void ontestitutionend（const unit test&unit_test，int迭代）；

 私人：
  //是C是规范化为空格字符的空白字符
  //当它出现在XML属性值中时？
  静态bool IsnormalizableWhitespace（char c）
    返回c==0x9 c==0xa c==0xd；
  }

  //C是否可以出现在格式良好的XML文档中？
  静态bool isvalidxmlCharacter（char c）
    返回isnormalizablewhitespace（c）c>=0x20；
  }

  //返回输入字符串str.if的XML转义副本
  //is_attribute is true，the text is means to appear as a attribute
  //值，并通过替换保留可规范化的空白
  //带有字符引用。
  static std:：string escapeXML（const std:：string&str，bool为_属性）；

  //返回给定的字符串，删除了xml中的所有无效字符。
  static std:：string removeInvalidXML字符（const std:：string&str）；

  //str为属性值时，方便包装escapeXML。
  static std:：string escapeXMLTattribute（const std:：string&str）
    返回escapeXML（str，true）；
  }

  //str不是属性值时，方便包装escapeXML。
  static std:：string escapexmltext（const char*str）
    返回escapeXML（str，false）；
  }

  //验证给定属性是否属于给定元素，以及
  //将属性作为XML流。
  静态void outputXmlattribute（std:：ostream*流，
                                 const std：：字符串和元素名称，
                                 const std：：字符串和名称，
                                 const std：：字符串和值）；

  //流式处理XML CDATA节，根据需要转义无效的CDATA序列。
  静态void输出xmlcdataSection（：：std:：ostream*流，const char*数据）；

  //流式处理testinfo对象的XML表示形式。
  静态void输出xmleTestInfo（：：std:：ostream*流，
                                const char*测试用例名称，
                                const test info和test_info）；

  //打印testcase对象的XML表示形式
  静态void printxmltestcase（：：std:：ostream*流，
                               const测试用例和测试用例）；

  //打印单元测试的XML摘要以输出流。
  静态void printxmluntest（：：std:：ostream*流，
                               单位测试和单位测试）；

  //生成一个字符串，将结果中的测试属性表示为空格
  //基于属性key=“value”对分隔的XML属性。
  //当std:：string不为空时，它在开头包含一个空格，
  //从以前的属性中分隔此属性。
  static std：：string testpropertiesasxmlattributes（const testreult&result）；

  //输出文件。
  const std：：字符串输出文件

  gtest不允许复制和分配（xmluntestresultprinter）；
}；

//创建新的XmlUnitTestResultPrinter。
xmlUnitTestResultPrinter:：xmlUnitTestResultPrinter（const char*输出文件）
    ：输出_文件_u（输出_文件）
  if（output_file_u.c_str（）==null output_file_u.empty（））
    fprintf（stderr，“xml输出文件不能为空\n”）；
    FFLUH（STDRR）；
    退出（退出失败）；
  }
}

//在单元测试结束后调用。
void xmlUnitTestResultPrinter:：OnStitutionEnd（const unit test&unit_test，
                                                  INT/*迭代*/) {

  FILE* xmlout = NULL;
  FilePath output_file(output_file_);
  FilePath output_dir(output_file.RemoveFileName());

  if (output_dir.CreateDirectoriesRecursively()) {
    xmlout = posix::FOpen(output_file_.c_str(), "w");
  }
  if (xmlout == NULL) {
//TODO（WAN）：报告失败原因。
//
//我们现在不这么做，因为：
//
//1。没有迫切的需要。
//2。让errno变量线程在
//所有三个操作系统（Linux、Windows和Mac OS）。
//三。以线程安全的方式解释errno的含义，
//我们需要strError_r（）函数，该函数在
//窗户。
    fprintf(stderr,
            "Unable to open file \"%s\"\n",
            output_file_.c_str());
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
  std::stringstream stream;
  PrintXmlUnitTest(&stream, unit_test);
  fprintf(xmlout, "%s", StringStreamToString(&stream).c_str());
  fclose(xmlout);
}

//返回输入字符串str的XML转义副本。if is_属性
//为真，文本将显示为属性值，并且
//可规范化的空白通过用字符替换来保留
//参考文献。
//
//str中的无效XML字符（如果有）将从输出中删除。
//预计大多数（如果不是全部）文本都由
//模块将由普通英语文本组成。
//如果修改此模块以生成1.1版XML输出，
//大多数无效字符可以使用字符引用保留。
//TODO（广域网）：有一个微创的，人类可读的可能会更好。
//无效字符的转义方案，而不是删除它们。
std::string XmlUnitTestResultPrinter::EscapeXml(
    const std::string& str, bool is_attribute) {
  Message m;

  for (size_t i = 0; i < str.size(); ++i) {
    const char ch = str[i];
    switch (ch) {
      case '<':
        m << "&lt;";
        break;
      case '>':
        m << "&gt;";
        break;
      case '&':
        m << "&amp;";
        break;
      case '\'':
        if (is_attribute)
          m << "&apos;";
        else
          m << '\'';
        break;
      case '"':
        if (is_attribute)
          m << "&quot;";
        else
          m << '"';
        break;
      default:
        if (IsValidXmlCharacter(ch)) {
          if (is_attribute && IsNormalizableWhitespace(ch))
            m << "&#x" << String::FormatByte(static_cast<unsigned char>(ch))
              << ";";
          else
            m << ch;
        }
        break;
    }
  }

  return m.GetString();
}

//返回给定的字符串，删除XML中所有无效字符。
//当前从字符串中删除了无效字符。安
//另一种方法是用某些字符替换它们，例如。还是？.
std::string XmlUnitTestResultPrinter::RemoveInvalidXmlCharacters(
    const std::string& str) {
  std::string output;
  output.reserve(str.size());
  for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    if (IsValidXmlCharacter(*it))
      output.push_back(*it);

  return output;
}

//以下例程生成UnitTest的XML表示形式
//对象。
//
//谷歌测试概念就是这样映射到DTD的：
//
//<testsuites name=“alltests”><--对应于UnitTest对象
//<testsuite name=“testcase name”><--对应于testcase对象
//<testcase name=“test name”><--对应于testinfo对象
//<failure message=“…”>…<failure>
//<failure message=“…”>…<failure>
//<failure message=“…”>…<failure>
//<--个别断言失败
//</TestCase>
//</TestSuffe>
//</TestSuth>

//以毫秒为单位将给定时间格式化为秒。
std::string FormatTimeInMillisAsSeconds(TimeInMillis ms) {
  ::std::stringstream ss;
  ss << ms/1000.0;
  return ss.str();
}

static bool PortableLocaltime(time_t seconds, struct tm* out) {
#if defined(_MSC_VER)
  return localtime_s(out, &seconds) == 0;
#elif defined(__MINGW32__) || defined(__MINGW64__)
//mingw<time.h>既不提供本地时间也不提供本地时间，但使用
//windows的localtime（），它有一个线程本地tm缓冲区。
struct tm* tm_ptr = localtime(&seconds);  //诺林
  if (tm_ptr == NULL)
    return false;
  *out = *tm_ptr;
  return true;
#else
  return localtime_r(&seconds, out) != NULL;
#endif
}

//将给定的epoch时间（毫秒）转换为ISO中的日期字符串
//8601格式，无时区信息。
std::string FormatEpochTimeInMillisAsIso8601(TimeInMillis ms) {
  struct tm time_struct;
  if (!PortableLocaltime(static_cast<time_t>(ms / 1000), &time_struct))
    return "";
//年：月：日：月：秒
  return StreamableToString(time_struct.tm_year + 1900) + "-" +
      String::FormatIntWidth2(time_struct.tm_mon + 1) + "-" +
      String::FormatIntWidth2(time_struct.tm_mday) + "T" +
      String::FormatIntWidth2(time_struct.tm_hour) + ":" +
      String::FormatIntWidth2(time_struct.tm_min) + ":" +
      String::FormatIntWidth2(time_struct.tm_sec);
}

//流式处理XML CDATA节，根据需要转义无效的CDATA序列。
void XmlUnitTestResultPrinter::OutputXmlCDataSection(::std::ostream* stream,
                                                     const char* data) {
  const char* segment = data;
  *stream << "<![CDATA[";
  for (;;) {
    const char* const next_segment = strstr(segment, "]]>");
    if (next_segment != NULL) {
      stream->write(
          segment, static_cast<std::streamsize>(next_segment - segment));
      *stream << "]]>]]&gt;<![CDATA[";
      segment = next_segment + strlen("]]>");
    } else {
      *stream << segment;
      break;
    }
  }
  *stream << "]]>";
}

void XmlUnitTestResultPrinter::OutputXmlAttribute(
    std::ostream* stream,
    const std::string& element_name,
    const std::string& name,
    const std::string& value) {
  const std::vector<std::string>& allowed_names =
      GetReservedAttributesForElement(element_name);

  GTEST_CHECK_(std::find(allowed_names.begin(), allowed_names.end(), name) !=
                   allowed_names.end())
      << "Attribute " << name << " is not allowed for element <" << element_name
      << ">.";

  *stream << " " << name << "=\"" << EscapeXmlAttribute(value) << "\"";
}

//打印testinfo对象的XML表示形式。
//TODO（WAN）：使用普通打印机打印属性也有价值。
void XmlUnitTestResultPrinter::OutputXmlTestInfo(::std::ostream* stream,
                                                 const char* test_case_name,
                                                 const TestInfo& test_info) {
  const TestResult& result = *test_info.result();
  const std::string kTestcase = "testcase";

  *stream << "    <testcase";
  OutputXmlAttribute(stream, kTestcase, "name", test_info.name());

  if (test_info.value_param() != NULL) {
    OutputXmlAttribute(stream, kTestcase, "value_param",
                       test_info.value_param());
  }
  if (test_info.type_param() != NULL) {
    OutputXmlAttribute(stream, kTestcase, "type_param", test_info.type_param());
  }

  OutputXmlAttribute(stream, kTestcase, "status",
                     test_info.should_run() ? "run" : "notrun");
  OutputXmlAttribute(stream, kTestcase, "time",
                     FormatTimeInMillisAsSeconds(result.elapsed_time()));
  OutputXmlAttribute(stream, kTestcase, "classname", test_case_name);
  *stream << TestPropertiesAsXmlAttributes(result);

  int failures = 0;
  for (int i = 0; i < result.total_part_count(); ++i) {
    const TestPartResult& part = result.GetTestPartResult(i);
    if (part.failed()) {
      if (++failures == 1) {
        *stream << ">\n";
      }
      const string location = internal::FormatCompilerIndependentFileLocation(
          part.file_name(), part.line_number());
      const string summary = location + "\n" + part.summary();
      *stream << "      <failure message=\""
              << EscapeXmlAttribute(summary.c_str())
              << "\" type=\"\">";
      const string detail = location + "\n" + part.message();
      OutputXmlCDataSection(stream, RemoveInvalidXmlCharacters(detail).c_str());
      *stream << "</failure>\n";
    }
  }

  if (failures == 0)
    *stream << " />\n";
  else
    *stream << "    </testcase>\n";
}

//打印测试用例对象的XML表示形式
void XmlUnitTestResultPrinter::PrintXmlTestCase(std::ostream* stream,
                                                const TestCase& test_case) {
  const std::string kTestsuite = "testsuite";
  *stream << "  <" << kTestsuite;
  OutputXmlAttribute(stream, kTestsuite, "name", test_case.name());
  OutputXmlAttribute(stream, kTestsuite, "tests",
                     StreamableToString(test_case.reportable_test_count()));
  OutputXmlAttribute(stream, kTestsuite, "failures",
                     StreamableToString(test_case.failed_test_count()));
  OutputXmlAttribute(
      stream, kTestsuite, "disabled",
      StreamableToString(test_case.reportable_disabled_test_count()));
  OutputXmlAttribute(stream, kTestsuite, "errors", "0");
  OutputXmlAttribute(stream, kTestsuite, "time",
                     FormatTimeInMillisAsSeconds(test_case.elapsed_time()));
  *stream << TestPropertiesAsXmlAttributes(test_case.ad_hoc_test_result())
          << ">\n";

  for (int i = 0; i < test_case.total_test_count(); ++i) {
    if (test_case.GetTestInfo(i)->is_reportable())
      OutputXmlTestInfo(stream, test_case.name(), *test_case.GetTestInfo(i));
  }
  *stream << "  </" << kTestsuite << ">\n";
}

//打印单元测试的XML摘要以输出流。
void XmlUnitTestResultPrinter::PrintXmlUnitTest(std::ostream* stream,
                                                const UnitTest& unit_test) {
  const std::string kTestsuites = "testsuites";

  *stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  *stream << "<" << kTestsuites;

  OutputXmlAttribute(stream, kTestsuites, "tests",
                     StreamableToString(unit_test.reportable_test_count()));
  OutputXmlAttribute(stream, kTestsuites, "failures",
                     StreamableToString(unit_test.failed_test_count()));
  OutputXmlAttribute(
      stream, kTestsuites, "disabled",
      StreamableToString(unit_test.reportable_disabled_test_count()));
  OutputXmlAttribute(stream, kTestsuites, "errors", "0");
  OutputXmlAttribute(
      stream, kTestsuites, "timestamp",
      FormatEpochTimeInMillisAsIso8601(unit_test.start_timestamp()));
  OutputXmlAttribute(stream, kTestsuites, "time",
                     FormatTimeInMillisAsSeconds(unit_test.elapsed_time()));

  if (GTEST_FLAG(shuffle)) {
    OutputXmlAttribute(stream, kTestsuites, "random_seed",
                       StreamableToString(unit_test.random_seed()));
  }

  *stream << TestPropertiesAsXmlAttributes(unit_test.ad_hoc_test_result());

  OutputXmlAttribute(stream, kTestsuites, "name", "AllTests");
  *stream << ">\n";

  for (int i = 0; i < unit_test.total_test_case_count(); ++i) {
    if (unit_test.GetTestCase(i)->reportable_test_count() > 0)
      PrintXmlTestCase(stream, *unit_test.GetTestCase(i));
  }
  *stream << "</" << kTestsuites << ">\n";
}

//生成一个字符串，将结果中的测试属性表示为空格
//基于属性key=“value”对的分隔XML属性。
std::string XmlUnitTestResultPrinter::TestPropertiesAsXmlAttributes(
    const TestResult& result) {
  Message attributes;
  for (int i = 0; i < result.test_property_count(); ++i) {
    const TestProperty& property = result.GetTestProperty(i);
    attributes << " " << property.key() << "="
        << "\"" << EscapeXmlAttribute(property.value()) << "\"";
  }
  return attributes.GetString();
}

//结束XmlUnitTestResultPrinter

#if GTEST_CAN_STREAM_RESULTS_

//检查str是否包含“='、'&'、'%'或'\n'字符。如果是的话，
//将它们替换为“%xx”，其中xx是它们的十六进制值。为了
//例如，将“=”替换为“%3d”。该算法为O（strlen（str））。
//在时间和空间中——这很重要，因为输入str可能包含
//任意长的测试失败消息和堆栈跟踪。
string StreamingListener::UrlEncode(const char* str) {
  string result;
  result.reserve(strlen(str) + 1);
  for (char ch = *str; ch != '\0'; ch = *++str) {
    switch (ch) {
      case '%':
      case '=':
      case '&':
      case '\n':
        result.append("%" + String::FormatByte(static_cast<unsigned char>(ch)));
        break;
      default:
        result.push_back(ch);
        break;
    }
  }
  return result;
}

void StreamingListener::SocketWriter::MakeConnection() {
  GTEST_CHECK_(sockfd_ == -1)
      << "MakeConnection() can't be called when there is already a connection.";

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;    //允许IPv4和IPv6地址。
  hints.ai_socktype = SOCK_STREAM;
  addrinfo* servinfo = NULL;

//使用getaddrinfo（）获取的IP地址的链接列表
//给定的主机名。
  const int error_num = getaddrinfo(
      host_name_.c_str(), port_num_.c_str(), &hints, &servinfo);
  if (error_num != 0) {
    GTEST_LOG_(WARNING) << "stream_result_to: getaddrinfo() failed: "
                        << gai_strerror(error_num);
  }

//循环遍历所有结果，并连接到第一个我们可以连接的结果。
  for (addrinfo* cur_addr = servinfo; sockfd_ == -1 && cur_addr != NULL;
       cur_addr = cur_addr->ai_next) {
    sockfd_ = socket(
        cur_addr->ai_family, cur_addr->ai_socktype, cur_addr->ai_protocol);
    if (sockfd_ != -1) {
//将客户端套接字连接到服务器套接字。
      if (connect(sockfd_, cur_addr->ai_addr, cur_addr->ai_addrlen) == -1) {
        close(sockfd_);
        sockfd_ = -1;
      }
    }
  }

freeaddrinfo(servinfo);  //全部完成了这个结构

  if (sockfd_ == -1) {
    GTEST_LOG_(WARNING) << "stream_result_to: failed to connect to "
                        << host_name_ << ":" << port_num_;
  }
}

//类尾流侦听器
#endif  //GTEST可以传输结果

//ScopedTrace类

//将给定的源文件位置和消息推送到每个线程上
//由谷歌测试维护的跟踪堆栈。
ScopedTrace::ScopedTrace(const char* file, int line, const Message& message)
    GTEST_LOCK_EXCLUDED_(&UnitTest::mutex_) {
  TraceInfo trace;
  trace.file = file;
  trace.line = line;
  trace.message = message.GetString();

  UnitTest::GetInstance()->PushGTestTrace(trace);
}

//弹出由控制器推送的信息。
ScopedTrace::~ScopedTrace()
    GTEST_LOCK_EXCLUDED_(&UnitTest::mutex_) {
  UnitTest::GetInstance()->PopGTestTrace();
}


//类osstacktracegetter

//以std：：字符串形式返回当前操作系统堆栈跟踪。参数：
//
//最大深度-要包含的最大堆栈帧数
//在痕迹中。
//Skip_Count-要跳过的顶层帧的数目；不计算
//相对于最大深度。
//
/*使用osstacktracegetter:：currentstacktrace（int/*最大深度*/，
                                             int/*跳过计数*/)

    GTEST_LOCK_EXCLUDED_(mutex_) {
  return "";
}

void OsStackTraceGetter::UponLeavingGTest()
    GTEST_LOCK_EXCLUDED_(mutex_) {
}

const char* const
OsStackTraceGetter::kElidedFramesMarker =
    "... " GTEST_NAME_ " internal frames ...";

//在其
//并删除其析构函数中的文件。
class ScopedPrematureExitFile {
 public:
  explicit ScopedPrematureExitFile(const char* premature_exit_filepath)
      : premature_exit_filepath_(premature_exit_filepath) {
//如果指定了提前退出文件的路径…
    if (premature_exit_filepath != NULL && *premature_exit_filepath != '\0') {
//创建包含单个“0”字符的文件。输入输出
//错误被忽略了，因为我们无能为力
//不要因为这个考试不及格。
      FILE* pfile = posix::FOpen(premature_exit_filepath, "w");
      fwrite("0", 1, 1, pfile);
      fclose(pfile);
    }
  }

  ~ScopedPrematureExitFile() {
    if (premature_exit_filepath_ != NULL && *premature_exit_filepath_ != '\0') {
      remove(premature_exit_filepath_);
    }
  }

 private:
  const char* const premature_exit_filepath_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ScopedPrematureExitFile);
};

}  //命名空间内部

//类TestEventListeners

TestEventListeners::TestEventListeners()
    : repeater_(new internal::TestEventRepeater()),
      default_result_printer_(NULL),
      default_xml_generator_(NULL) {
}

TestEventListeners::~TestEventListeners() { delete repeater_; }

//返回负责默认控制台的标准侦听器
//输出。可以从侦听器列表中删除以默认关闭
//控制台输出。请注意，从侦听器列表中删除此对象
//通过释放，它的所有权转移给用户。
void TestEventListeners::Append(TestEventListener* listener) {
  repeater_->Append(listener);
}

//从列表中删除给定的事件侦听器并返回它。然后
//成为调用方删除侦听器的责任。退换商品
//如果在列表中找不到侦听器，则为空。
TestEventListener* TestEventListeners::Release(TestEventListener* listener) {
  if (listener == default_result_printer_)
    default_result_printer_ = NULL;
  else if (listener == default_xml_generator_)
    default_xml_generator_ = NULL;
  return repeater_->Release(listener);
}

//返回将TestEventListener事件广播到所有
//订户。
TestEventListener* TestEventListeners::repeater() { return repeater_; }

//将默认的打印机属性设置为提供的侦听器。
//监听器也被添加到监听器列表和上一个
//从打印机中删除默认的“结果”打印机。听众可以
//如果为空，则不会将其添加到列表中。做
//如果前一个和当前侦听器对象相同，则不执行任何操作。
void TestEventListeners::SetDefaultResultPrinter(TestEventListener* listener) {
  if (default_result_printer_ != listener) {
//将此方法传递给已在
//名单。
    delete Release(default_result_printer_);
    default_result_printer_ = listener;
    if (listener != NULL)
      Append(listener);
  }
}

//设置提供的侦听器的默认\xml \u生成器属性。这个
//监听器也被添加到监听器列表和上一个
//从中删除并删除默认的_XML_生成器。听众可以
//如果为空，则不会将其添加到列表中。做
//如果前一个和当前侦听器对象相同，则不执行任何操作。
void TestEventListeners::SetDefaultXmlGenerator(TestEventListener* listener) {
  if (default_xml_generator_ != listener) {
//将此方法传递给已在
//名单。
    delete Release(default_xml_generator_);
    default_xml_generator_ = listener;
    if (listener != NULL)
      Append(listener);
  }
}

//控制中继器是否将事件转发到
//列表中的侦听器。
bool TestEventListeners::EventForwardingEnabled() const {
  return repeater_->forwarding_enabled();
}

void TestEventListeners::SuppressEventForwarding() {
  repeater_->set_forwarding_enabled(false);
}

//类单元测试

//获取Singleton UnitTest对象。这种方法是第一次
//调用时，构造并返回UnitTest对象。连续的
//调用将返回同一对象。
//
//我们不会在mutex下保护它，因为用户不应该这样做
//在main（）开始之前调用此函数，从返回点开始
//价值永远不会改变。
UnitTest* UnitTest::GetInstance() {
//在优化模式下使用MSVC 7.1编译时，会破坏
//退出程序时UnitTest对象会弄乱退出代码，
//导致成功的测试似乎失败。我们必须使用
//在这种情况下，使用不同的实现来绕过编译器错误。
//这个实现使编译器满意，代价是
//正在泄漏UnitTest对象。

//Cd+Guilder坚持一个公共析构函数
//默认实现。使用此实现保持良好的OO
//使用私有析构函数设计。

#if (_MSC_VER == 1310 && !defined(_DEBUG)) || defined(__BORLANDC__)
  static UnitTest* const instance = new UnitTest;
  return instance;
#else
  static UnitTest instance;
  return &instance;
#endif  //（_msc_ver==1310&！已定义（_debug））已定义（uuBorlandc_uuuu）
}

//获取成功的测试用例数。
int UnitTest::successful_test_case_count() const {
  return impl()->successful_test_case_count();
}

//获取失败的测试用例数。
int UnitTest::failed_test_case_count() const {
  return impl()->failed_test_case_count();
}

//获取所有测试用例的数目。
int UnitTest::total_test_case_count() const {
  return impl()->total_test_case_count();
}

//获取包含至少一个测试的所有测试用例的数目
//应该可以。
int UnitTest::test_case_to_run_count() const {
  return impl()->test_case_to_run_count();
}

//获取成功的测试数。
int UnitTest::successful_test_count() const {
  return impl()->successful_test_count();
}

//获取失败的测试数。
int UnitTest::failed_test_count() const { return impl()->failed_test_count(); }

//获取将在XML报告中报告的禁用测试数。
int UnitTest::reportable_disabled_test_count() const {
  return impl()->reportable_disabled_test_count();
}

//获取禁用的测试数。
int UnitTest::disabled_test_count() const {
  return impl()->disabled_test_count();
}

//获取要在XML报表中打印的测试数。
int UnitTest::reportable_test_count() const {
  return impl()->reportable_test_count();
}

//获取所有测试的数目。
int UnitTest::total_test_count() const { return impl()->total_test_count(); }

//获取应运行的测试数。
int UnitTest::test_to_run_count() const { return impl()->test_to_run_count(); }

//获取测试程序的启动时间，从
//UNIX时代
internal::TimeInMillis UnitTest::start_timestamp() const {
    return impl()->start_timestamp();
}

//获取已用时间（毫秒）。
internal::TimeInMillis UnitTest::elapsed_time() const {
  return impl()->elapsed_time();
}

//如果单元测试通过（即所有测试用例通过），则返回true。
bool UnitTest::Passed() const { return impl()->Passed(); }

//如果单元测试失败（即某些测试用例失败），则返回true
//或者所有测试之外的东西都失败了）。
bool UnitTest::Failed() const { return impl()->Failed(); }

//获取所有测试用例中的第i个测试用例。我可以从0到
//总测试用例计数（）-1。如果我不在该范围内，则返回空值。
const TestCase* UnitTest::GetTestCase(int i) const {
  return impl()->GetTestCase(i);
}

//返回包含有关测试失败和
//在单个测试用例之外记录的属性。
const TestResult& UnitTest::ad_hoc_test_result() const {
  return *impl()->ad_hoc_test_result();
}

//获取所有测试用例中的第i个测试用例。我可以从0到
//总测试用例计数（）-1。如果我不在该范围内，则返回空值。
TestCase* UnitTest::GetMutableTestCase(int i) {
  return impl()->GetMutableTestCase(i);
}

//返回可用于跟踪事件的事件侦听器列表
//谷歌内部测试。
TestEventListeners& UnitTest::listeners() {
  return *impl()->listeners();
}

//注册并返回全局测试环境。当测试时
//程序正在运行，所有全局测试环境都将在
//命令他们注册。程序中的所有测试
//完成后，所有全局测试环境将在
//*按相反的顺序注册。
//
//UnitTest对象拥有给定环境的所有权。
//
//我们不会在mutex_u下保护它，因为我们只支持调用它
//从主线程。
Environment* UnitTest::AddEnvironment(Environment* env) {
  if (env == NULL) {
    return NULL;
  }

  impl_->environments().push_back(env);
  return env;
}

//将TestPartResult添加到当前的TestResult对象。全谷歌测试
//断言宏（例如assert_true、expect_eq等）最终调用
//这是为了报告他们的结果。用户代码应使用
//断言宏而不是直接调用它。
void UnitTest::AddTestPartResult(
    TestPartResult::Type result_type,
    const char* file_name,
    int line_number,
    const std::string& message,
    const std::string& os_stack_trace) GTEST_LOCK_EXCLUDED_(mutex_) {
  Message msg;
  msg << message;

  internal::MutexLock lock(&mutex_);
  if (impl_->gtest_trace_stack().size() > 0) {
    msg << "\n" << GTEST_NAME_ << " trace:";

    for (int i = static_cast<int>(impl_->gtest_trace_stack().size());
         i > 0; --i) {
      const internal::TraceInfo& trace = impl_->gtest_trace_stack()[i - 1];
      msg << "\n" << internal::FormatFileLocation(trace.file, trace.line)
          << " " << trace.message;
    }
  }

  if (os_stack_trace.c_str() != NULL && !os_stack_trace.empty()) {
    msg << internal::kStackTraceMarker << os_stack_trace;
  }

  const TestPartResult result =
    TestPartResult(result_type, file_name, line_number,
                   msg.GetString().c_str());
  impl_->GetTestPartResultReporterForCurrentThread()->
      ReportTestPartResult(result);

  if (result_type != TestPartResult::kSuccess) {
//失败时的测试中断优先于
//测试失败时抛出。这允许用户设置后者
//在代码中（可能是为了使用Google测试断言
//使用另一个测试框架）并在
//用于调试的命令行。
    if (GTEST_FLAG(break_on_failure)) {
#if GTEST_OS_WINDOWS && !GTEST_OS_WINDOWS_PHONE && !GTEST_OS_WINDOWS_RT
//在Windows上使用debugbreak仍允许gtest进入调试器
//当一个故障发生时，同时-gtest_break_on_failure和
//指定了--gtest_catch_异常标志。
      DebugBreak();
#else
//通过易失性指针取消对空值的引用，以防止编译器
//从移除。我们使用这个而不是abort（）或内置陷阱（）来
//可移植性：Symbian没有很好地实现abort（），并且一些调试器
//不要正确地捕获abort（）。
      *static_cast<volatile int*>(NULL) = 1;
#endif  //GTEST操作系统窗口
    } else if (GTEST_FLAG(throw_on_failure)) {
#if GTEST_HAS_EXCEPTIONS
      throw internal::GoogleTestFailureException(result);
#else
//无法调用abort（），因为它在调试模式下生成弹出窗口
//不能在VC 7.1或更低版本中抑制。
      exit(1);
#endif
    }
  }
}

//从调用时向当前的TestResult对象添加TestProperty
//在一个测试中，调用当前测试用例的特殊测试结果
//从SETUPTESTCASE或TEARDOWNTESTCASE，或到全局属性集
//在其他地方调用时。如果结果已包含具有
//相同的键，值将被更新。
void UnitTest::RecordProperty(const std::string& key,
                              const std::string& value) {
  impl_->RecordProperty(TestProperty(key, value));
}

//运行此UnitTest对象中的所有测试并打印结果。
//如果成功，则返回0，否则返回1。
//
//我们不会在mutex_u下保护它，因为我们只支持调用它
//从主线程。
int UnitTest::Run() {
  const bool in_death_test_child_process =
      internal::GTEST_FLAG(internal_run_death_test).length() > 0;

//Google测试实现了这个协议来捕获一个测试
//程序在返回到Google测试之前退出：
//
//1。一开始，谷歌测试就创建了一个绝对路径
//由环境变量指定
//测试过早退出文件。
//2。当谷歌测试完成工作后，它会删除文件。
//
//这允许测试运行程序在
//运行基于Google测试的测试程序并检查是否存在
//在测试执行结束时查看文件是否
//过早退出。

//如果我们正在进行儿童死亡测试，不要
//创建/删除过早退出文件，因为这样做是不必要的
//并且会混淆父进程。否则，创建/删除
//进入/离开此功能时的文件。如果程序
//不知何故，在该函数有机会返回之前退出，
//过早退出文件将被保留为未删除，导致测试运行程序
//了解提前退出文件协议以报告
//测试失败。
  const internal::ScopedPrematureExitFile premature_exit_file(
      in_death_test_child_process ?
      NULL : internal::posix::GetEnv("TEST_PREMATURE_EXIT_FILE"));

//捕获gtest_标志的值（catch_exceptions）。这个值将是
//在程序执行期间使用。
  impl()->set_catch_exceptions(GTEST_FLAG(catch_exceptions));

#if GTEST_HAS_SEH
//用户希望Google测试捕获
//测试，或者这是在死亡测试子项的上下文中执行的
//过程。无论哪种情况，用户都不希望看到弹出对话框
//关于车祸——这是意料之中的。
  if (impl()->catch_exceptions() || in_death_test_child_process) {
# if !GTEST_OS_WINDOWS_MOBILE && !GTEST_OS_WINDOWS_PHONE && !GTEST_OS_WINDOWS_RT
//ce上不存在setErrorMode。
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT |
                 SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
# endif  //！GTEST操作系统Windows移动

# if (defined(_MSC_VER) || GTEST_OS_WINDOWS_MINGW) && !GTEST_OS_WINDOWS_MOBILE
//可以使用_abort（）终止死亡测试子项。在Windows上，
//_abort（）可以显示带有警告消息的对话框。这迫使
//中止消息以转到stderr。
    _set_error_mode(_OUT_TO_STDERR);
# endif

# if _MSC_VER >= 1400 && !GTEST_OS_WINDOWS_MOBILE
//在调试版本中，Visual Studio弹出一个单独的对话框
//提供调试中止程序的选项。我们需要压制
//否则，此对话框将针对每个Expect/Assert_死亡声明弹出
//执行。谷歌测试将通知用户任何意外
//通过stderr失败。
//
//VC++没有在8.0版之前定义_set_abort_behavior（）。
//以前的VC版本的用户将遭受
//点击无数的调试对话框。
//TODO（vladl@google.com）：在
//使用VC 7.1或更低版本编译时的调试模式。
    if (!GTEST_FLAG(break_on_failure))
      _set_abort_behavior(
0x0,                                    //清除以下标志：
_WRITE_ABORT_MSG | _CALL_REPORTFAULT);  //弹出窗口，核心转储。
# endif
  }
#endif  //格斯塔夫哈斯塞赫

  return internal::HandleExceptionsInMethodIfSupported(
      impl(),
      &internal::UnitTestImpl::RunAllTests,
      "auxiliary test code (environments or event listeners)") ? 0 : 1;
}

//当第一个test（）或test_f（）是
//执行。
const char* UnitTest::original_working_dir() const {
  return impl_->original_working_dir_.c_str();
}

//返回当前正在运行的测试的testcase对象，
//如果没有运行测试，则为空。
const TestCase* UnitTest::current_test_case() const
    GTEST_LOCK_EXCLUDED_(mutex_) {
  internal::MutexLock lock(&mutex_);
  return impl_->current_test_case();
}

//返回当前正在运行的测试的testinfo对象，
//如果没有运行测试，则为空。
const TestInfo* UnitTest::current_test_info() const
    GTEST_LOCK_EXCLUDED_(mutex_) {
  internal::MutexLock lock(&mutex_);
  return impl_->current_test_info();
}

//返回当前测试运行开始时使用的随机种子。
int UnitTest::random_seed() const { return impl_->random_seed(); }

#if GTEST_HAS_PARAM_TEST
//返回用于跟踪
//值参数化测试并实例化和注册它们。
internal::ParameterizedTestCaseRegistry&
    UnitTest::parameterized_test_registry()
        GTEST_LOCK_EXCLUDED_(mutex_) {
  return impl_->parameterized_test_registry();
}
#endif  //gtest_有参数测试

//创建空的UnitTest。
UnitTest::UnitTest() {
  impl_ = new internal::UnitTestImpl(this);
}

//UnitTest的析构函数。
UnitTest::~UnitTest() {
  delete impl_;
}

//将作用域_trace（）定义的跟踪推送到每个线程
//谷歌测试跟踪堆栈。
void UnitTest::PushGTestTrace(const internal::TraceInfo& trace)
    GTEST_LOCK_EXCLUDED_(mutex_) {
  internal::MutexLock lock(&mutex_);
  impl_->gtest_trace_stack().push_back(trace);
}

//从每个线程的Google测试跟踪堆栈中弹出跟踪。
void UnitTest::PopGTestTrace()
    GTEST_LOCK_EXCLUDED_(mutex_) {
  internal::MutexLock lock(&mutex_);
  impl_->gtest_trace_stack().pop_back();
}

namespace internal {

UnitTestImpl::UnitTestImpl(UnitTest* parent)
    : parent_(parent),
      /*st_disable_msc_warnings_push_u（4355/*在初始值设定项中使用此项*/）
      默认的“全局测试”部分“结果”报告器（此项），
      默认的_-per-u-thread_-test_-part_-result_-reporter_uu（this），
      gtest禁用msc警告
      全球测试部分结果报告
          &default_global_test_part_result_reporter_ux（默认_全局_测试_部分_结果_u报告者），
      每线程测试部分结果报告者
          默认值为“每线程测试部分结果报告者”，
如果gtest_有_参数_测试
      参数化测试注册表
      参数化的_tests_registered_uu（假），
endif//gtest_具有_参数_测试
      最后一个死亡测试案例
      当前测试用例（空）
      当前测试信息（空）
      即席测试结果
      OS堆栈\跟踪\获取器\（空），
      post_flag_parse_init_performed_u（假），
      随机_seed_u（0），//将在首次使用前被标志覆盖。
      随机_uu0，//将在首次使用前重新种子。
      开始时间戳（0），
      已用时间（0）
如果GTEST覕U有覕死亡覕测试
      死亡测试工厂（新默认死亡测试工厂）
第二节
      //将在首次使用前被标志重写。
      catch_exceptions_u（假）
  listeners（）->setDefaultResultPrinter（新的prettyUnitTestResultPrinter）；
}

UnitTestImpl:：~UnitTestImpl（）
  //删除每个测试用例。
  foreach（测试用例，内部：：delete<testcase>）；

  //删除每个环境。
  foreach（环境，内部：：delete<environment>）；

  删除操作系统堆栈\跟踪\获取器\；
}

//在
//测试上下文，调用时指向当前测试用例的即席测试结果
//从setuptestcase/teardowntestcase或到全局属性集
/否则。如果结果已经包含具有相同键的属性，
//将更新该值。
void unittestmpl:：recordproperty（const test property&test_property）
  std:：string xml_元素；
  testResult*test_result；//适合属性记录的testResult。

  如果（当前测试信息）{NULL）{
    xml_element=“测试用例”；
    测试结果&（当前测试信息->result）；
  否则，如果（当前测试用例）{NULL）{
    xml_element=“测试套件”；
    测试结果&（当前测试用例->ad-hoc测试结果）；
  }否则{
    xml_element=“测试套件”；
    测试结果=特别测试结果
  }
  test_result->recordproperty（xml_element，test_property）；
}

如果GTEST覕U有覕死亡覕测试
//如果控件当前处于死亡测试中，则禁用事件转发
/ /子过程。在initgoogletest之前不能调用。
void UnitTestImpl:：SuppressTestEventSifinSubProcess（）
  如果（内部运行死亡测试标记获取（）！=空）
    listeners（）->SuppressEventForwarding（）；
}
endif//gtest_具有_死亡_测试

//按照指定的方式初始化执行XML输出的事件侦听器
//UnitTestOptions。在initgoogletest之前不能调用。
void unittesimpl:：configurexmloutput（）
  const std:：string&output_format=unittestOptions:：getOutputformat（）；
  if（output_format==“xml”）
    listeners（）->setDefaultXmlGenerator（新的xmlUnitTestResultPrinter（
        UnitTestOptions:：GetAbsolutePathToOutputfile（）.c_str（））；
  否则，如果（输出_格式！=“”“{”
    printf（“警告：无法识别的输出格式\”“%s”“被忽略。\n”，
           output_format.c_str（））；
    FFLUH（STDUT）；
  }
}

如果gtest可以传输结果
//以字符串形式初始化流式测试结果的事件侦听器。
//不能在initgoogletest之前调用。
void UnitTestImpl:：configureStreamingOutput（）
  const std:：string&target=gtest_标志（stream_result_to）；
  如果（！）target.empty（））
    const size_t pos=target.find（'：'）；
    如果（POS）！=std：：字符串：：npos）
      listeners（）->append（new streaminglistener（target.substr（0，pos）），
                                                target.substr（pos+1））；
    }否则{
      printf（“警告：已忽略无法识别的流目标\”“%s”“）.\n”，
             target.c_str（））；
      FFLUH（STDUT）；
    }
  }
}
endif//gtest_can_stream_results_

//根据在中获取的标志值执行初始化
//仅分析GoogleStestFlagsOnly。在调用后从initgoogletest调用
//仅分析GoogleStestFlagsOnly。如果用户忽略调用initgoogletest
//此函数也从runalltests调用。因为这个函数可以
//多次调用，它必须是等幂的。
void UnitTestImpl:：PostFlagParsingInit（）
  //确保此函数不执行多次。
  如果（！）发布_标志_parse_init_performed_
    post_flag_parse_init_performed_u=true；

如果GTEST覕U有覕死亡覕测试
    initDeathTestSubprocessControlInfo（）；
    SuppressTestEventSifinSubProcess（）；
endif//gtest_具有_死亡_测试

    //注册参数化测试。这使得参数化测试
    //不运行就可用于UnitTest反射API
    //运行所有测试。
    registerParameterizedTests（）；

    //为XML输出配置侦听器。这使用户可以
    //在调用运行所有测试之前关闭默认的XML输出。
    配置exmloutput（）；

如果gtest可以传输结果
    //为将测试结果流式传输到指定服务器配置侦听器。
    配置流输出（）；
endif//gtest_can_stream_results_
  }
}

//根据已知的
//值。
/ /
//这仅用于UnitTest类的实现。我们放
//位于匿名命名空间中，以防止污染外部
//命名空间。
/ /
//testcasename是可复制的。
类testcasenameis_
 公众：
  //构造函数。
  显式testcasenameis（const std:：string&name）
      ：姓名

  //如果测试用例的名称与名称匹配，则返回true。
  bool operator（）（const test case*test_case）const_
    返回测试用例！=空&&strcmp（test_case->name（），name_u.c_str（））=0；
  }

 私人：
  std：：字符串名称\；
}；

//查找并返回具有给定名称的测试用例。如果没有
//存在，创建一个并返回它。是打电话的
//确保仅在
//测试没有无序排列。
/ /
//参数：
/ /
//测试用例名称：测试用例的名称
//type_param：测试用例类型参数的名称，如果
//这不是类型化或类型参数化测试用例。
//设置\u tc:指向设置测试用例的函数的指针
//拆下\u tc：指向拆下测试用例的函数的指针
test case*unittestmpl:：gettestcase（const char*测试用例名称，
                                    const char*类型\u参数，
                                    测试：：设置testcasefunc设置\u tc，
                                    测试：：拆卸下外壳
  //我们能找到一个具有给定名称的测试用例吗？
  const std：：vector<test case*>：：const_迭代器test_case=
      std:：find_if（test_cases_u.begin（），test_cases_.end（），
                   testcasenameis（test_case_name））；

  如果（测试用例）！=测试用例
    返回*测试用例；

  //不，我们创建一个。
  测试用例*常量新测试用例=
      新的测试用例（测试用例名称，输入参数，设置参数，删除参数）；

  //这是一个死亡测试案例吗？
  if（内部：：UnitTestOptions：：MatchesFilter（测试用例名称，
                                               kdeathtestcasefilter））
    是的。在最后一个死亡测试用例之后插入测试用例
    //到目前为止定义的。只有当测试用例没有
    //被洗牌了。否则我们可能会进行死亡测试
    //非死亡测试后。
    +上一个死亡测试案例；
    测试用例插入（测试用例开始（）+最后一个死亡测试用例，
                       Nexo Test-Type）；
  }否则{
    //否。追加到列表末尾。
    测试用例推回（新的测试用例）；
  }

  test_case_indexes_u.push_back（static_cast<int>（test_case_indexes_u.size（））；
  返回新的测试用例；
}

//用于设置/分解给定环境的帮助程序。他们
//用于foreach（）函数。
静态void设置环境（environment*env）env->setup（）；
静态void teardown environment（environment*env）env->teardown（）；

//运行此UnitTest对象中的所有测试，打印结果，以及
//如果所有测试都成功，则返回true。如果有例外
//在测试期间引发，该测试被视为失败，但
//其余测试仍将运行。
/ /
//当启用参数化测试时，它将展开并注册
//首先在registerParameterizedTests（）中进行参数化测试。
//从runalltests（）调用的所有其他函数都可以安全地假定
//参数化测试已准备好计数和运行。
bool unittestmpl:：runalltests（）
  //确保调用了initgoogletest（）。
  如果（！）gtestisinitization（））
    PrtTf（“%s”）
           “此测试程序没有调用：：testing:：initgoogletest”
           “在调用run_all_tests（）之前。请修复它。\n”）。
    返回错误；
  }

  //如果指定了--help标志，则不要运行任何测试。
  if（g_帮助_标志）
    回归真实；

  //重复对post标志解析初始化的调用，以防
  //用户没有调用initgoogletest。
  postflagParsingInit（）；

  //即使未启用切分，测试运行人员也可能希望使用
  //gtest_shard_status_文件，查询测试是否支持分片
  //协议。
  内部：：WriteToHardStatusFileIfNeeded（）；

  //真的，如果我们正处于运行线程安全样式的子进程中
  /死亡测试。
  bool in_subprocess_for_death_test=false；

如果GTEST覕U有覕死亡覕测试
  在_subprocess_for_death_test中=（内部_run_death_test_flag_u.get（）！= NULL）；
endif//gtest_具有_死亡_测试

  const bool should_shard=应该硬（ktesttotalshards，ktestshardindex，
                                        在“死亡测试”的“子过程”中）；

  //将完整的测试名称与筛选器进行比较，以确定
  //要运行的测试。
  const bool has_tests_to_run=filtertests（should_shard
                                              ？尊重分享协议
                                              ：忽略_sharding_协议）>0；

  //如果指定了--gtest_list_tests标志，则列出测试并退出。
  if（gtest_flag（list_tests））
    //必须在调用*filtertests（）之后调用*这个函数。
    listestsMatchingFilter（）；
    回归真实；
  }

  随机“种子”标志（随机播放）？
      getrandomseedfromflag（gtest_flag（random_seed））：0；

  //真iff至少有一个测试失败。
  bool failed=false；

  testeventListener*repeater=listeners（）->repeater（）；

  开始时间戳=getTimeInMillis（）；
  转发器->OnTestProgramStart（*Parent_uu）；

  //重复测试多少次？我们不想重复它们
  //当我们进入死亡测试的子进程时。
  const int repeat=在“死亡”测试的“子进程”中？1:gtest_标志（重复）；
  //如果重复计数为负，则永远重复。
  const bool forever=重复<0；
  对于（int i=0；永远i！=重复；i++）
    //我们希望保留由特殊测试生成的失败
    //在运行_all_tests（）之前执行的断言。
    clearnonadhoctestresult（）；

    const timeinmillis start=getTimeinmillis（）；

    //根据请求无序处理测试用例和测试。
    if（has_tests_to_run&&gtest_flag（shuffle））
      random（）->重置（随机种子）；
      //这应该在调用OneStitutionStart（）之前完成，
      //这样测试事件侦听器就可以看到实际的测试顺序
      //在事件中。
      SUFFLITEST（）；
    }

    //告诉单元测试事件侦听器测试即将开始。
    转发器->OneStitutionStart（*Parent_uui）；

    //如果至少要运行一个测试，则运行每个测试用例。
    如果（有测试运行）
      //预先设置所有环境。
      中继器->OnEnvironmentsSetupStart（*Parent_uu）；
      foreach（环境，设置环境）；
      转发器->OnEnvironmentsSetupend（*Parent_uu）；

      //仅当全局期间没有致命错误时才运行测试
      /设置。
      如果（！）测试：：hasfatafilure（））
        for（int test_index=0；test_index<total_test_case_count（）；
             TestObjult+++）{
          getmutabletestcase（test_index）->run（）；
        }
      }

      //然后按相反的顺序拆下所有环境。
      中继器->Onenvironmentsteardownstart（*parent_uu）；
      std：：对于每个（environments_.rbegin（），environments_.rend（），
                    泪液环境）；
      中继器->OnEnvironmentStearDownend（*Parent_uu）；
    }

    elapsed_time_uu=getTimeInMillis（）-开始；

    //告诉单元测试事件侦听器测试刚刚完成。
    转发器->OneStitutionEnd（*Parent_uui）；

    //获取结果并将其清除。
    如果（！）PasSee（））{
      失败=真；
    }

    //迭代后恢复原始测试顺序。这个
    //允许用户快速重新设置在
    //第n次迭代，不重复第一次（n-1）迭代。
    //这不包含在“if（gtest_flag（shuffle））”中。}，在
    //如果用户在某个地方更改了标志的值，
    //（取消测试的缓冲总是安全的）。
    unsuffletests（）；

    if（gtest_flag（shuffle））
      //为每个迭代选择一个新的随机种子。
      随机种子（random）
    }
  }

  转发器->ontestprogrammend（*parent_u）；

  回来！失败；
}

//读取gtest_shard_status_file环境变量，并创建该文件
//如果变量存在。如果此位置已存在文件，则
//函数将重写它。如果变量存在，但文件不能
//创建，打印错误并退出。
void writeToHardstatusFileIfNeeded（）
  const char*const test_shard_file=posix:：getenv（ktestshardstatusfile）；
  如果（测试碎片文件！{NULL）{
    文件*const file=posix:：fopen（test_shard_file，“w”）；
    if（file==null）
      彩色打印F（红色，
                    “无法写入测试碎片状态文件\”“%s”“”
                    “由%s环境变量指定。\n”，
                    测试shard文件，ktestshardstatusfile）；
      FFLUH（STDUT）；
      退出（退出失败）；
    }
    FSEX（文件）；
  }
}

//通过检查相关的
//环境变量值。如果变量存在，
//但不一致（即shard_index>=total_shards），打印
//一个错误并退出。如果在“死亡”测试的“子过程”中，切分是
//已禁用，因为它只能应用于原始测试
//过程。否则，我们可以过滤掉我们打算执行的死亡测试。
bool shouldshard（const char*total_shards_env，
                 常量char*shard_index_env，
                 bool in_subprocess_for_death_test）
  if（in_subprocess_for_death_test）
    返回错误；
  }

  const int32 total_shards=int32fromenvordie（total_shards_env，-1）；
  const int32 shard_index=int32fromenvordie（shard_index_env，-1）；

  if（总碎片数=-1&&shard_index==-1）
    返回错误；
  else if（total_shards==-1&&shard_index！= -1）{
    const message msg=message（）。
      <“无效的环境变量：您有”
      <<ktestshardindex<<“=”<<shard_index
      “<<”，但已离开“<<ktesttotalshards<<”unset.\n”；
    coloredprintf（color_red，msg.getstring（）.c_str（））；
    FFLUH（STDUT）；
    退出（退出失败）；
  否则，如果（总碎片！=-1&&shard_index==-1）
    const message msg=message（）。
      <“无效的环境变量：您有”
      <<ktesttotalshards<“=”<<total\u shards
      <<“，but have left”<<ktestshardindex<“unset.\n”；
    coloredprintf（color_red，msg.getstring（）.c_str（））；
    FFLUH（STDUT）；
    退出（退出失败）；
  else if（shard_index<0_shard_index>=total_shards）
    const message msg=message（）。
      <“无效的环境变量：我们需要0<=”
      <<ktestshardindex<<“<”<<ktesttotalshards
      <<“，但您有”<<ktestshardindex<<“=”<<shard_index
      <<“，，<<ktesttotalshards<<“=”<<total_shards<<“.\n”；
    coloredprintf（color_red，msg.getstring（）.c_str（））；
    FFLUH（STDUT）；
    退出（退出失败）；
  }

  返回总碎片>1；
}

//将环境变量var解析为int32。如果未设置，
//返回默认值。如果不是Int32，则打印错误
/和中止。
int32 int32fromenvordie（const char*var，int32 default_
  const char*str_val=posix:：getenv（var）；
  if（str_val==null）
    返回默认值；
  }

  IT32结果；
  如果（！）parseInt32（message（）<“环境变量的值”<<var，
                  str_val，&result））
    退出（退出失败）；
  }
  返回结果；
}

//给定碎片总数、碎片索引和测试ID，
//返回true iff测试应在此碎片上运行。测试ID为
//分配给每个测试的任意但唯一的非负整数
//方法。假设0<=shard_index<total_shards。
bool shouldruntestonshard（int total_shards，int shard_index，int test_id）
  返回（test_id%total_shards）==shard_index；
}

//将每个测试的名称与用户指定的筛选器进行比较
//决定是否运行测试，然后将结果记录在
//每个testcase和testinfo对象。
//如果shard_tests==true，则进一步基于sharding筛选测试
//环境中的变量-请参见
//http://code.google.com/p/googletest/wiki/googletestadvancedguide。
//返回应该运行的测试数。
int unittestmpl:：filtertests（reactiventsharding shard_tests）
  const int32 total_shards=shard_tests==honour_sharding_协议？
      int32fromenvordie（ktesttotalshards，-1）：-1；
  const int32 shard_index=shard_tests==honour_sharding_协议？
      int32fromenvordie（ktestshardindex，-1）：-1；

  //num_runnable_tests是将
  //在所有碎片上运行（即匹配筛选器，未禁用）。
  //num_selected_tests是要运行的测试数
  这个碎片。
  int num_runnable_tests=0；
  int num_selected_tests=0；
  对于（size_t i=0；i<test_cases_.size（）；i++）
    测试用例*常量测试用例=测试用例；
    const std:：string&test_case_name=test_case->name（）；
    测试用例->设置应该运行（错误）；

    对于（size_t j=0；j<test_case->test_info_list（）.size（）；j++）
      test info*const test_info=test_case->test_info_list（）[j]；
      const std:：string test_name（test_info->name（））；
      //如果测试用例名称或测试名称匹配，则禁用测试
      //kdisabletestfilter。
      const bool is_disabled=
          内部：：UnitTestOptions:：MatchesFilter（测试用例名称，
                                                   kdisabletestfilter）_
          内部：：UnitTestOptions：：MatchesFilter（测试名称，
                                                   kdisabletestfilter）；
      测试_info->is_disabled_u=is_disabled；

      const bool matches_filter=
          内部：：UnitTestOptions:：FilterMatchestest（测试用例名称，
                                                       测试名）；
      测试_-info->matches_-filter_u=matches_-filter；

      const bool可运行=
          （gtest_标志（也可以运行禁用的_测试）！ISS-禁用）& &
          匹配滤波器；

      const bool is_selected=可运行&&
          （shard_tests==忽略_sharding_协议
           应该进行搜索（总碎片，碎片指数，
                                num_runnable_tests））；

      num_runnable_tests+=可运行；
      num_selected_tests+=为_selected；

      测试_-info->should_-run_u=is_selected；
      test_case->set_should_run（test_case->should_run（）为_selected）；
    }
  }
  返回num_selected_tests；
}

//通过全部替换在一行上打印给定的C字符串\n'
//字符串为“\\n”的字符。如果输出超过
//最大长度字符，只打印第一个最大长度字符
/和“…”。
静态void printononeline（const char*str，int max_length）
  如果（STR）！{NULL）{
    对于（int i=0；*str！='\0'；++str）
      如果（i>=max_length）
        PrtTf（“…”）；
        断裂；
      }
      if（*str='\n'）
        PrTNF（“\n”）；
        I+＝2；
      }否则{
        printf（“%c”，*str）；
        +i；
      }
    }
  }
}

//打印与用户指定的筛选器标志匹配的测试的名称。
void UnitTestImpl:：ListTestsMatchingFilter（）
  //每个类型/值参数最多打印这么多字符。
  常量int kmaxparamlength=250；

  对于（size_t i=0；i<test_cases_.size（）；i++）
    const test case*const test_case=测试_cases_u[i]；
    bool printed_test_case_name=false；

    对于（size_t j=0；j<test_case->test_info_list（）.size（）；j++）
      const测试信息*const测试信息=
          test_case->test_info_list（）[j]；
      if（test_info->matches_filter_
        如果（！）打印的测试用例名称）
          打印的测试用例名称=真；
          printf（“%s.”，test_case->name（））；
          如果（test_case->type_param（）！{NULL）{
            printf（“%s=”，ktypeParamLabel）；
            //我们将类型参数打印在一行上，
            //程序容易解析的输出。
            printononeline（test_case->type_param（），kmaxparamlength）；
          }
          PrTNF（“\n”）；
        }
        printf（“%s”，test_info->name（））；
        如果（test_info->value_param（）！{NULL）{
          printf（“%s=”，kValueParamLabel）；
          //我们在一行上打印value参数，以使
          //输出易于程序分析。
          printononeline（test_info->value_param（），kmaxparamlength）；
        }
        PrTNF（“\n”）；
      }
    }
  }
  FFLUH（STDUT）；
}

//设置OS堆栈跟踪getter。
/ /
//如果输入和当前OS堆栈跟踪getter为
//相同；否则，删除旧getter并使输入
//当前吸气剂。
void unittesimpl：：设置_os_stack_trace_getter（
    osstacktracegetterinterface*getter）_
  如果（操作系统堆栈\跟踪\获取器\！=吸气剂）{
    删除操作系统堆栈\跟踪\获取器\；
    操作系统堆栈\跟踪\获取器\获取器；
  }
}

//如果当前OS堆栈跟踪getter不为空，则返回该getter；
//否则，创建osstacktracegetter，使其成为当前
//getter，并返回它。
osstacktracegetterinterface*unittesimpl:：os_stack_trace_getter（）
  if（os_stack_trace_getter_==null）
    os_stack_trace_getter_uu=新的os stack trace getter；
  }

  返回操作系统堆栈\跟踪\获取器\；
}

//返回当前正在运行的测试的测试结果，或者
//如果没有运行任何测试，则为即席测试的测试结果。
测试结果*UnitTestImpl:：current_test_result（）
  返回当前测试信息？
      &（当前_test_info_uu->result_u）：&ad_hoc_test_result_uu；
}

//无序处理所有测试用例，以及每个测试用例中的测试，
//确保仍然首先运行死亡测试。
void unittestmpl:：shuffletests（）
  //洗牌死亡测试用例。
  shuffleRange（random（），0，last_death_test_case_uu1，&test_case_indexes_uu）；

  //无序处理非死亡测试用例。
  shuffleRange（random（），last_death_test_case_u1，
               static_cast<int>（test_cases_.size（）），&test_case_indexes_uu）；

  //对每个测试用例中的测试进行无序处理。
  对于（size_t i=0；i<test_cases_.size（）；i++）
    test_cases_[i]->shuffletests（random（））；
  }
}

//将测试用例和测试恢复到第一次随机播放之前的顺序。
void unittestmpl:：unsuffletests（）
  对于（size_t i=0；i<test_cases_.size（）；i++）
    //取消对每个测试用例中的测试的缓冲。
    测试用例—>unsuffletests（）；
    //重置每个测试用例的索引。
    test_case_indexes_[i]=static_cast<int>（i）；
  }
}

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
标准：：string getcurrentostacktraceexcepttop（unittest*/*单位*/,

                                            int skip_count) {
//另外，我们还通过skip_count+1跳过这个包装函数。
//用户真正想要跳过的内容。
  return GetUnitTestImpl()->CurrentOsStackTraceExceptTop(skip_count + 1);
}

//由gtest使用，禁止宏下的“无法访问”代码“警告”
//禁止显示无法访问的代码警告。
namespace {
class ClassUniqueToAlwaysTrue {};
}

bool IsTrue(bool condition) { return condition; }

bool AlwaysTrue() {
#if GTEST_HAS_EXCEPTIONS
//这个条件总是错误的，所以alwaystrue（）从不实际抛出，
//但它使编译器认为它可能会抛出。
  if (IsTrue(false))
    throw ClassUniqueToAlwaysTrue();
#endif  //gtest_有\u例外
  return true;
}

//如果*pstr以给定前缀开头，则将*pstr修改为正确的。
//超过前缀并返回true；否则保持*pstr不变
//然后返回false。pstr、*pstr和prefix都不能为空。
bool SkipPrefix(const char* prefix, const char** pstr) {
  const size_t prefix_len = strlen(prefix);
  if (strncmp(*pstr, prefix, prefix_len) == 0) {
    *pstr += prefix_len;
    return true;
  }
  return false;
}

//将字符串解析为命令行标志。字符串应该
//格式--flag=value。当def_optional为true时，“=value”
//零件可以省略。
//
//返回标志的值，如果分析失败，则返回空值。
const char* ParseFlagValue(const char* str,
                           const char* flag,
                           bool def_optional) {
//str和flag不能为空。
  if (str == NULL || flag == NULL) return NULL;

//标志必须以“-”开头，后跟gtest标记前缀。
  const std::string flag_str = std::string("--") + GTEST_FLAG_PREFIX_ + flag;
  const size_t flag_len = flag_str.length();
  if (strncmp(str, flag_str.c_str(), flag_len) != 0) return NULL;

//跳过标志名。
  const char* flag_end = str + flag_len;

//当def_optional为true时，没有“=value”部分是可以的。
  if (def_optional && (flag_end[0] == '\0')) {
    return flag_end;
  }

//如果def_可选为真，并且在
//标志名，或者如果def_optional为false，则后面必须有“=”
//标志名。
  if (flag_end[0] != '=') return NULL;

//返回“=”后的字符串。
  return flag_end + 1;
}

//分析bool标志的字符串，其形式为
//“--flag=value”或“--flag”。
//
//在前一种情况下，只要值为真
//不以“0”、“f”或“f”开头。
//
//在后一种情况下，该值被视为真。
//
//成功后，将标志的值存储在*value中，并返回
//真的。失败时，返回FALSE而不更改*值。
bool ParseBoolFlag(const char* str, const char* flag, bool* value) {
//获取作为字符串的标志值。
  const char* const value_str = ParseFlagValue(str, flag, true);

//分析失败时中止。
  if (value_str == NULL) return false;

//将字符串值转换为bool。
  *value = !(*value_str == '0' || *value_str == 'f' || *value_str == 'F');
  return true;
}

//分析Int32标志的字符串，格式为
//“标志=值”。
//
//成功后，将标志的值存储在*value中，并返回
//真的。失败时，返回FALSE而不更改*值。
bool ParseInt32Flag(const char* str, const char* flag, Int32* value) {
//获取作为字符串的标志值。
  const char* const value_str = ParseFlagValue(str, flag, false);

//分析失败时中止。
  if (value_str == NULL) return false;

//将*值设置为标志的值。
  return ParseInt32(Message() << "The value of flag --" << flag,
                    value_str, value);
}

//为字符串标志解析字符串，格式为
//“标志=值”。
//
//成功后，将标志的值存储在*value中，并返回
//真的。失败时，返回FALSE而不更改*值。
bool ParseStringFlag(const char* str, const char* flag, std::string* value) {
//获取作为字符串的标志值。
  const char* const value_str = ParseFlagValue(str, flag, false);

//分析失败时中止。
  if (value_str == NULL) return false;

//将*值设置为标志的值。
  *value = value_str;
  return true;
}

//确定字符串是否具有Google测试用于其
//标志，即以gtest_flag_prefix_u或gtest_flag_prefix_dash_u开头。
//如果google test检测到命令行标志有其前缀但没有
//已识别，它将打印帮助消息。以开头的标志
//gtest_internal_prefix_u后跟“internal_u”被认为是google测试
//内部标志，不触发帮助消息。
static bool HasGoogleTestFlagPrefix(const char* str) {
  return (SkipPrefix("--", &str) ||
          SkipPrefix("-", &str) ||
          SkipPrefix("/", &str)) &&
         !SkipPrefix(GTEST_FLAG_PREFIX_ "internal_", &str) &&
         (SkipPrefix(GTEST_FLAG_PREFIX_, &str) ||
          SkipPrefix(GTEST_FLAG_PREFIX_DASH_, &str));
}

//打印包含代码编码文本的字符串。以下逃亡
//序列可在字符串中用于控制文本颜色：
//
//@@打印一个“@”字符。
//@r将颜色更改为红色。
//@g将颜色改为绿色。
//@y将颜色改为黄色。
//@d更改为默认的终端文本颜色。
//
//todo（wan@google.com）：一旦我们添加stdout，就为它编写测试
//捕捉到谷歌测试。
static void PrintColorEncoded(const char* str) {
GTestColor color = COLOR_DEFAULT;  //当前颜色。

//从概念上讲，我们将字符串拆分为由转义分隔的段
//序列。然后我们一次打印一段。在最后
//每次迭代，str指针都前进到
//下一节。
  for (;;) {
    const char* p = strchr(str, '@');
    if (p == NULL) {
      ColoredPrintf(color, "%s", str);
      return;
    }

    ColoredPrintf(color, "%s", std::string(str, p).c_str());

    const char ch = p[1];
    str = p + 2;
    if (ch == '@') {
      ColoredPrintf(color, "@");
    } else if (ch == 'D') {
      color = COLOR_DEFAULT;
    } else if (ch == 'R') {
      color = COLOR_RED;
    } else if (ch == 'G') {
      color = COLOR_GREEN;
    } else if (ch == 'Y') {
      color = COLOR_YELLOW;
    } else {
      --str;
    }
  }
}

static const char kColorEncodedHelpMessage[] =
"This program contains tests written using " GTEST_NAME_ ". You can use the\n"
"following command line flags to control its behavior:\n"
"\n"
"Test Selection:\n"
"  @G--" GTEST_FLAG_PREFIX_ "list_tests@D\n"
"      List the names of all tests instead of running them. The name of\n"
"      TEST(Foo, Bar) is \"Foo.Bar\".\n"
"  @G--" GTEST_FLAG_PREFIX_ "filter=@YPOSTIVE_PATTERNS"
    "[@G-@YNEGATIVE_PATTERNS]@D\n"
"      Run only the tests whose name matches one of the positive patterns but\n"
"      none of the negative patterns. '?' matches any single character; '*'\n"
"      matches any substring; ':' separates two patterns.\n"
"  @G--" GTEST_FLAG_PREFIX_ "also_run_disabled_tests@D\n"
"      Run all disabled tests too.\n"
"\n"
"Test Execution:\n"
"  @G--" GTEST_FLAG_PREFIX_ "repeat=@Y[COUNT]@D\n"
"      Run the tests repeatedly; use a negative count to repeat forever.\n"
"  @G--" GTEST_FLAG_PREFIX_ "shuffle@D\n"
"      Randomize tests' orders on every iteration.\n"
"  @G--" GTEST_FLAG_PREFIX_ "random_seed=@Y[NUMBER]@D\n"
"      Random number seed to use for shuffling test orders (between 1 and\n"
"      99999, or 0 to use a seed based on the current time).\n"
"\n"
"Test Output:\n"
"  @G--" GTEST_FLAG_PREFIX_ "color=@Y(@Gyes@Y|@Gno@Y|@Gauto@Y)@D\n"
"      Enable/disable colored output. The default is @Gauto@D.\n"
"  -@G-" GTEST_FLAG_PREFIX_ "print_time=0@D\n"
"      Don't print the elapsed time of each test.\n"
"  @G--" GTEST_FLAG_PREFIX_ "output=xml@Y[@G:@YDIRECTORY_PATH@G"
    GTEST_PATH_SEP_ "@Y|@G:@YFILE_PATH]@D\n"
"      Generate an XML report in the given directory or with the given file\n"
"      name. @YFILE_PATH@D defaults to @Gtest_details.xml@D.\n"
#if GTEST_CAN_STREAM_RESULTS_
"  @G--" GTEST_FLAG_PREFIX_ "stream_result_to=@YHOST@G:@YPORT@D\n"
"      Stream test results to the given server.\n"
#endif  //GTEST可以传输结果
"\n"
"Assertion Behavior:\n"
#if GTEST_HAS_DEATH_TEST && !GTEST_OS_WINDOWS
"  @G--" GTEST_FLAG_PREFIX_ "death_test_style=@Y(@Gfast@Y|@Gthreadsafe@Y)@D\n"
"      Set the default death test style.\n"
#endif  //GTEST有死亡测试&！GTEST操作系统窗口
"  @G--" GTEST_FLAG_PREFIX_ "break_on_failure@D\n"
"      Turn assertion failures into debugger break-points.\n"
"  @G--" GTEST_FLAG_PREFIX_ "throw_on_failure@D\n"
"      Turn assertion failures into C++ exceptions.\n"
"  @G--" GTEST_FLAG_PREFIX_ "catch_exceptions=0@D\n"
"      Do not report exceptions as test failures. Instead, allow them\n"
"      to crash the program or throw a pop-up (on Windows).\n"
"\n"
"Except for @G--" GTEST_FLAG_PREFIX_ "list_tests@D, you can alternatively set "
    "the corresponding\n"
"environment variable of a flag (all letters in upper-case). For example, to\n"
"disable colored text output, you can either specify @G--" GTEST_FLAG_PREFIX_
    "color=no@D or set\n"
"the @G" GTEST_FLAG_PREFIX_UPPER_ "COLOR@D environment variable to @Gno@D.\n"
"\n"
"For more information, please read the " GTEST_NAME_ " documentation at\n"
"@G" GTEST_PROJECT_URL_ "@D. If you find a bug in " GTEST_NAME_ "\n"
"(not one in your own code or tests), please report it to\n"
"@G<" GTEST_DEV_EMAIL_ ">@D.\n";

//解析Google测试标志的命令行，而不初始化
//谷歌测试的其他部分。类型参数chartype可以是
//实例化为char或wchar_t。
template <typename CharType>
void ParseGoogleTestFlagsOnlyImpl(int* argc, CharType** argv) {
  for (int i = 1; i < *argc; i++) {
    const std::string arg_string = StreamableToString(argv[i]);
    const char* const arg = arg_string.c_str();

    using internal::ParseBoolFlag;
    using internal::ParseInt32Flag;
    using internal::ParseStringFlag;

//我们看到谷歌测试标志了吗？
    if (ParseBoolFlag(arg, kAlsoRunDisabledTestsFlag,
                      &GTEST_FLAG(also_run_disabled_tests)) ||
        ParseBoolFlag(arg, kBreakOnFailureFlag,
                      &GTEST_FLAG(break_on_failure)) ||
        ParseBoolFlag(arg, kCatchExceptionsFlag,
                      &GTEST_FLAG(catch_exceptions)) ||
        ParseStringFlag(arg, kColorFlag, &GTEST_FLAG(color)) ||
        ParseStringFlag(arg, kDeathTestStyleFlag,
                        &GTEST_FLAG(death_test_style)) ||
        ParseBoolFlag(arg, kDeathTestUseFork,
                      &GTEST_FLAG(death_test_use_fork)) ||
        ParseStringFlag(arg, kFilterFlag, &GTEST_FLAG(filter)) ||
        ParseStringFlag(arg, kInternalRunDeathTestFlag,
                        &GTEST_FLAG(internal_run_death_test)) ||
        ParseBoolFlag(arg, kListTestsFlag, &GTEST_FLAG(list_tests)) ||
        ParseStringFlag(arg, kOutputFlag, &GTEST_FLAG(output)) ||
        ParseBoolFlag(arg, kPrintTimeFlag, &GTEST_FLAG(print_time)) ||
        ParseInt32Flag(arg, kRandomSeedFlag, &GTEST_FLAG(random_seed)) ||
        ParseInt32Flag(arg, kRepeatFlag, &GTEST_FLAG(repeat)) ||
        ParseBoolFlag(arg, kShuffleFlag, &GTEST_FLAG(shuffle)) ||
        ParseInt32Flag(arg, kStackTraceDepthFlag,
                       &GTEST_FLAG(stack_trace_depth)) ||
        ParseStringFlag(arg, kStreamResultToFlag,
                        &GTEST_FLAG(stream_result_to)) ||
        ParseBoolFlag(arg, kThrowOnFailureFlag,
                      &GTEST_FLAG(throw_on_failure))
        ) {
//对。将argv列表的其余部分向左移动一个。注释
//argv有（*argc+1）个元素，最后一个元素总是
//无效的。以下循环将尾随的空元素移动为
//好。
      for (int j = i; j != *argc; j++) {
        argv[j] = argv[j + 1];
      }

//减少参数计数。
      (*argc)--;

//我们还需要在刚刚移除迭代器时对其进行减量处理。
//元素。
      i--;
    } else if (arg_string == "--help" || arg_string == "-h" ||
               arg_string == "-?" || arg_string == "/?" ||
               HasGoogleTestFlagPrefix(arg)) {
//帮助标志和无法识别的Google测试标志（不包括
//内部的）触发帮助显示。
      g_help_flag = true;
    }
  }

  if (g_help_flag) {
//我们在这里打印帮助而不是运行中的所有测试（），作为
//如果用户正在使用Google，则根本无法调用后者。
//使用另一个测试框架进行测试。
    PrintColorEncoded(kColorEncodedHelpMessage);
  }
}

//解析Google测试标志的命令行，而不初始化
//谷歌测试的其他部分。
void ParseGoogleTestFlagsOnly(int* argc, char** argv) {
  ParseGoogleTestFlagsOnlyImpl(argc, argv);
}
void ParseGoogleTestFlagsOnly(int* argc, wchar_t** argv) {
  ParseGoogleTestFlagsOnlyImpl(argc, argv);
}

//initGoogleTest（）的内部实现。
//
//类型参数char type可以实例化为char或
//瓦查特
template <typename CharType>
void InitGoogleTestImpl(int* argc, CharType** argv) {
  g_init_gtest_count++;

//我们不想运行两次初始化代码。
  if (g_init_gtest_count != 1) return;

  if (*argc <= 0) return;

  internal::g_executable_path = internal::StreamableToString(argv[0]);

#if GTEST_HAS_DEATH_TEST

  g_argvs.clear();
  for (int i = 0; i != *argc; i++) {
    g_argvs.push_back(StreamableToString(argv[i]));
  }

#endif  //GTEST有死亡测试

  ParseGoogleTestFlagsOnly(argc, argv);
  GetUnitTestImpl()->PostFlagParsingInit();
}

}  //命名空间内部

//初始化Google测试。必须在呼叫之前调用此
//运行所有测试（）。特别是，它为
//谷歌测试识别的标志。每当google测试标志
//可见，它从argv中移除，并且*argc递减。
//
//没有返回值。相反，google测试标志变量是
//更新。
//
//第二次调用函数没有用户可见的效果。
void InitGoogleTest(int* argc, char** argv) {
  internal::InitGoogleTestImpl(argc, argv);
}

//此重载版本可在编译的Windows程序中使用。
//Unicode模式。
void InitGoogleTest(int* argc, wchar_t** argv) {
  internal::InitGoogleTestImpl(argc, argv);
}

}  //命名空间测试
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
//作者：wan@google.com（zhanyong wan），vladl@google.com（vlad losev）
//
//此文件实现死亡测试。


#if GTEST_HAS_DEATH_TEST

# if GTEST_OS_MAC
#  include <crt_externs.h>
# endif  //格斯特罗斯奥斯麦克

# include <errno.h>
# include <fcntl.h>
# include <limits.h>

# if GTEST_OS_LINUX
#  include <signal.h>
# endif  //GTestOSLinux

# include <stdarg.h>

# if GTEST_OS_WINDOWS
#  include <windows.h>
# else
#  include <sys/mman.h>
#  include <sys/wait.h>
# endif  //GTEST操作系统窗口

# if GTEST_OS_QNX
#  include <spawn.h>
# endif  //格斯塔斯奥斯QNX

#endif  //GTEST有死亡测试


//指示此翻译单元是Google测试的一部分
//实施。它必须在gtest内部inl.h之前
//包含，否则将出现编译器错误。这个把戏是为了
//防止在
//用户代码。
#define GTEST_IMPLEMENTATION_ 1
#undef GTEST_IMPLEMENTATION_

namespace testing {

//常量。

//默认的死亡测试样式。
static const char kDefaultDeathTestStyle[] = "fast";

GTEST_DEFINE_string_(
    death_test_style,
    internal::StringFromGTestEnv("death_test_style", kDefaultDeathTestStyle),
    "Indicates how to run a death test in a forked child process: "
    "\"threadsafe\" (child process re-executes the test binary "
    "from the beginning, running only the specific death test) or "
    "\"fast\" (child process runs the death test immediately "
    "after forking).");

GTEST_DEFINE_bool_(
    death_test_use_fork,
    internal::BoolFromGTestEnv("death_test_use_fork", false),
    "Instructs to use fork()/_exit() instead of clone() in death tests. "
    "Ignored and always uses fork() on POSIX systems where clone() is not "
    "implemented. Useful when running under valgrind or similar tools if "
    "those do not support clone(). Valgrind 3.3.1 will just fail if "
    "it sees an unsupported combination of clone() flags. "
    "It is not recommended to use this flag w/o valgrind though it will "
    "work in 99% of the cases. Once valgrind is fixed, this flag will "
    "most likely be removed.");

namespace internal {
GTEST_DEFINE_string_(
    internal_run_death_test, "",
    "Indicates the file, line number, temporal index of "
    "the single death test to run, and a file descriptor to "
    "which a success code may be sent, all separated by "
    "the '|' characters.  This flag is specified if and only if the current "
    "process is a sub-process launched for running a thread-safe "
    "death test.  FOR INTERNAL USE ONLY.");
}  //命名空间内部

#if GTEST_HAS_DEATH_TEST

namespace internal {

//仅对快速死亡测试有效。指示代码正在
//儿童快速式死亡测试过程。
static bool g_in_fast_death_test_child = false;

//返回一个布尔值，该值指示调用方当前是否
//在死亡测试子进程的上下文中执行。工具如
//Valgrind堆检查程序可能需要这个来修改他们在死亡中的行为。
//测验。重要提示：这是一个内部实用程序。使用它可能会破坏
//执行死亡测试。用户代码不能使用它。
bool InDeathTestChild() {
# if GTEST_OS_WINDOWS

//在Windows上，死亡测试是线程安全的，不管
//死亡测试样式标志。
  return !GTEST_FLAG(internal_run_death_test).empty();

# else

  if (GTEST_FLAG(death_test_style) == "threadsafe")
    return !GTEST_FLAG(internal_run_death_test).empty();
  else
    return g_in_fast_death_test_child;
#endif
}

}  //命名空间内部

//已退出代码构造函数。
ExitedWithCode::ExitedWithCode(int exit_code) : exit_code_(exit_code) {
}

//退出代码函数调用运算符。
bool ExitedWithCode::operator()(int exit_status) const {
# if GTEST_OS_WINDOWS

  return exit_status == exit_code_;

# else

  return WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == exit_code_;

# endif  //GTEST操作系统窗口
}

# if !GTEST_OS_WINDOWS
//被信号构造函数杀死。
KilledBySignal::KilledBySignal(int signum) : signum_(signum) {
}

//由信号函数调用运算符取消。
bool KilledBySignal::operator()(int exit_status) const {
  return WIFSIGNALED(exit_status) && WTERMSIG(exit_status) == signum_;
}
# endif  //！GTEST操作系统窗口

namespace internal {

//死亡测试所需的实用程序。

//生成给定退出代码的文本描述，格式为
//由等待（2）指定。
static std::string ExitSummary(int exit_code) {
  Message m;

# if GTEST_OS_WINDOWS

  m << "Exited with exit status " << exit_code;

# else

  if (WIFEXITED(exit_code)) {
    m << "Exited with exit status " << WEXITSTATUS(exit_code);
  } else if (WIFSIGNALED(exit_code)) {
    m << "Terminated by signal " << WTERMSIG(exit_code);
  }
#  ifdef WCOREDUMP
  if (WCOREDUMP(exit_code)) {
    m << " (core dumped)";
  }
#  endif
# endif  //GTEST操作系统窗口

  return m.GetString();
}

//如果exit_status描述一个已终止的进程，则返回true
//通过一个信号，或正常退出，退出代码为非零。
bool ExitedUnsuccessfully(int exit_status) {
  return !ExitedWithCode(0)(exit_status);
}

# if !GTEST_OS_WINDOWS
//当死亡测试发现超过
//一个线程正在运行，或无法确定线程数
//执行给定的语句。这是
//调用方不传递线程计数1。
static std::string DeathTestThreadWarning(size_t thread_count) {
  Message msg;
  msg << "Death tests use fork(), which is unsafe particularly"
      << " in a threaded context. For this test, " << GTEST_NAME_ << " ";
  if (thread_count == 0)
    msg << "couldn't detect the number of threads.";
  else
    msg << "detected " << thread_count << " threads.";
  return msg.GetString();
}
# endif  //！GTEST操作系统窗口

//标记用于报告未死亡的死亡测试的字符。
static const char kDeathTestLived = 'L';
static const char kDeathTestReturned = 'R';
static const char kDeathTestThrew = 'T';
static const char kDeathTestInternalError = 'I';

//描述死亡测试可能的所有方法的枚举。
//得出结论。Dead表示在执行测试时进程停止
//代码；活动的意思是进程超出了测试代码的末尾；
//返回意味着测试语句试图执行返回
//语句，这是不允许的；抛出表示测试语句
//通过引发异常返回控件。进行中意味着测试
//尚未结束。
//todo（vladl@google.com）：统一名称和可能的值
//Abortreason、DeathTestOutcome和上面的标志字符。
enum DeathTestOutcome { IN_PROGRESS, DIED, LIVED, RETURNED, THREW };

//中止程序的例程，该程序可以安全地从
//exec样式的死亡测试子进程，在这种情况下，错误
//消息将传播回父进程。否则，
//消息只是打印到stderr。在这两种情况下，程序
//然后退出，状态为1。
void DeathTestAbort(const std::string& message) {
//在POSIX系统上，可以从线程安全样式调用此函数
//死亡测试子进程，它在一个非常小的堆栈上操作。使用
//用于任何其他非小内存需求的堆。
  const InternalRunDeathTestFlag* const flag =
      GetUnitTestImpl()->internal_run_death_test_flag();
  if (flag != NULL) {
    FILE* parent = posix::FDOpen(flag->write_fd(), "w");
    fputc(kDeathTestInternalError, parent);
    fprintf(parent, "%s", message.c_str());
    fflush(parent);
    _exit(1);
  } else {
    fprintf(stderr, "%s", message.c_str());
    fflush(stderr);
    posix::Abort();
  }
}

//如果断言
//失败。
# define GTEST_DEATH_TEST_CHECK_(expression) \
  do { \
    if (!::testing::internal::IsTrue(expression)) { \
      DeathTestAbort( \
          ::std::string("CHECK failed: File ") + __FILE__ +  ", line " \
          + ::testing::internal::StreamableToString(__LINE__) + ": " \
          + #expression); \
    } \
  } while (::testing::internal::AlwaysFalse())

//这个宏类似于gtest_death_test_check，但它是用于
//评估满足两个条件的任何系统调用：它必须返回
//-故障时为1，中断时将errno设置为eintr，并且
//应该再试一次。宏扩展为循环，循环重复
//只要表达式的计算结果为-1并设置
//从errno到eintr。如果表达式的计算结果为-1，但errno为
//除eintr外，死亡测试者被称为。
# define GTEST_DEATH_TEST_CHECK_SYSCALL_(expression) \
  do { \
    int gtest_retval; \
    do { \
      gtest_retval = (expression); \
    } while (gtest_retval == -1 && errno == EINTR); \
    if (gtest_retval == -1) { \
      DeathTestAbort( \
          ::std::string("CHECK failed: File ") + __FILE__ + ", line " \
          + ::testing::internal::StreamableToString(__LINE__) + ": " \
          + #expression + " != -1"); \
    } \
  } while (::testing::internal::AlwaysFalse())

//返回描述errno中最后一个系统错误的消息。
std::string GetLastErrnoDescription() {
    return errno == 0 ? "" : posix::StrError(errno);
}

//从死亡测试父进程调用此函数以读取失败
//来自死亡测试子进程的消息，并用致命的
//严重程度。在Windows上，从管道句柄读取消息。在其他
//平台，从文件描述符读取。
static void FailFromInternalError(int fd) {
  Message error;
  char buffer[256];
  int num_read;

  do {
    while ((num_read = posix::Read(fd, buffer, 255)) > 0) {
      buffer[num_read] = '\0';
      error << buffer;
    }
  } while (num_read == -1 && errno == EINTR);

  if (num_read == 0) {
    GTEST_LOG_(FATAL) << error.GetString();
  } else {
    const int last_error = errno;
    GTEST_LOG_(FATAL) << "Error while reading death test internal: "
                      << GetLastErrnoDescription() << " [" << last_error << "]";
  }
}

//死亡测试建造师。增加正在运行的死亡测试计数
//用于当前测试。
DeathTest::DeathTest() {
  TestInfo* const info = GetUnitTestImpl()->current_test_info();
  if (info == NULL) {
    DeathTestAbort("Cannot run a death test outside of a TEST or "
                   "TEST_F construct");
  }
}

//通过调度到当前
//死亡测试工厂。
bool DeathTest::Create(const char* statement, const RE* regex,
                       const char* file, int line, DeathTest** test) {
  return GetUnitTestImpl()->death_test_factory()->Create(
      statement, regex, file, line, test);
}

const char* DeathTest::LastMessage() {
  return last_death_test_message_.c_str();
}

void DeathTest::set_last_death_test_message(const std::string& message) {
  last_death_test_message_ = message;
}

std::string DeathTest::last_death_test_message_;

//为某些死亡功能提供跨平台实现。
class DeathTestImpl : public DeathTest {
 protected:
  DeathTestImpl(const char* a_statement, const RE* a_regex)
      : statement_(a_statement),
        regex_(a_regex),
        spawned_(false),
        status_(-1),
        outcome_(IN_PROGRESS),
        read_fd_(-1),
        write_fd_(-1) {}

//read_fd_u应该由派生类关闭和清除。
  ~DeathTestImpl() { GTEST_DEATH_TEST_CHECK_(read_fd_ == -1); }

  void Abort(AbortReason reason);
  virtual bool Passed(bool status_ok);

  const char* statement() const { return statement_; }
  const RE* regex() const { return regex_; }
  bool spawned() const { return spawned_; }
  void set_spawned(bool is_spawned) { spawned_ = is_spawned; }
  int status() const { return status_; }
  void set_status(int a_status) { status_ = a_status; }
  DeathTestOutcome outcome() const { return outcome_; }
  void set_outcome(DeathTestOutcome an_outcome) { outcome_ = an_outcome; }
  int read_fd() const { return read_fd_; }
  void set_read_fd(int fd) { read_fd_ = fd; }
  int write_fd() const { return write_fd_; }
  void set_write_fd(int fd) { write_fd_ = fd; }

//仅在父进程中调用。读取死亡结果代码
//通过管道测试子进程，解释它以设置结果
//成员，并关闭read_fd_uu。输出诊断并终止于
//意外代码的情况。
  void ReadAndInterpretStatusByte();

 private:
//此对象正在测试的代码的文本内容。这个班
//不拥有此字符串，不应尝试删除它。
  const char* const statement_;
//测试输出必须匹配的正则表达式。死亡证明书
//不拥有此对象，不应尝试删除它。
  const RE* const regex_;
//如果已成功生成死亡测试子进程，则为true。
  bool spawned_;
//子进程的退出状态。
  int status_;
//死亡测试的结论。
  DeathTestOutcome outcome_;
//子进程管道读取端的描述符。它是
//在子进程中始终为-1。这个孩子一直写着
//写入管道。
  int read_fd_;
//子进程的管道写入端到父进程的描述符。
//在父进程中始终为-1。父级保留其结尾
//管道输入读数
  int write_fd_;
};

//仅在父进程中调用。读取死亡结果代码
//通过管道测试子进程，解释它以设置结果
//成员，并关闭read_fd_uu。输出诊断并终止于
//意外代码的情况。
void DeathTestImpl::ReadAndInterpretStatusByte() {
  char flag;
  int bytes_read;

//此处的read（）将一直阻塞，直到数据可用为止（表示
//死亡测试失败）或管道关闭（表示
//这是成功的），所以之前在家长那里也可以这么说。
//子进程已退出。
  do {
    bytes_read = posix::Read(read_fd(), &flag, 1);
  } while (bytes_read == -1 && errno == EINTR);

  if (bytes_read == 0) {
    set_outcome(DIED);
  } else if (bytes_read == 1) {
    switch (flag) {
      case kDeathTestReturned:
        set_outcome(RETURNED);
        break;
      case kDeathTestThrew:
        set_outcome(THREW);
        break;
      case kDeathTestLived:
        set_outcome(LIVED);
        break;
      case kDeathTestInternalError:
FailFromInternalError(read_fd());  //不返回。
        break;
      default:
        GTEST_LOG_(FATAL) << "Death test child process reported "
                          << "unexpected status byte ("
                          << static_cast<unsigned int>(flag) << ")";
    }
  } else {
    GTEST_LOG_(FATAL) << "Read from death test child process failed: "
                      << GetLastErrnoDescription();
  }
  GTEST_DEATH_TEST_CHECK_SYSCALL_(posix::Close(read_fd()));
  set_read_fd(-1);
}

//表示应该退出的死亡测试代码没有。
//只应在死亡测试子进程中调用。
//将状态字节写入子级的状态文件描述符，然后
//调用退出（1）。
void DeathTestImpl::Abort(AbortReason reason) {
//父进程将死亡测试视为失败，如果
//它在我们的管道中找到任何数据。因此，这里我们编写一个标志字节
//到管道，然后退出。
  const char status_ch =
      reason == TEST_DID_NOT_DIE ? kDeathTestLived :
      reason == TEST_THREW_EXCEPTION ? kDeathTestThrew : kDeathTestReturned;

  GTEST_DEATH_TEST_CHECK_SYSCALL_(posix::Write(write_fd(), &status_ch, 1));
//我们在这里泄漏描述符是因为在某些平台上（例如，
//当构建为Windows dll时，全局对象的析构函数仍将
//调用_exit（）后运行。在这样的系统上，写入
//从UnitTestImpl的析构函数间接关闭，导致Double
//如果它也在这里关闭，则关闭。在调试配置上，双关闭
//可以断言。由于这里没有要刷新的进程内缓冲区，因此
//进程终止后依赖操作系统关闭描述符
//当析构函数不运行时。
_exit(1);  //没有任何正常出口挂钩的出口（我们应该撞车）
}

//返回死亡测试的stderr输出的缩进副本。
//这使得死亡测试输出行与常规日志行区别开来。
//容易得多。
static ::std::string FormatDeathTestOutput(const ::std::string& output) {
  ::std::string ret;
  for (size_t at = 0; ; ) {
    const size_t line_end = output.find('\n', at);
    ret += "[  DEATH   ] ";
    if (line_end == ::std::string::npos) {
      ret += output.substr(at);
      break;
    }
    ret += output.substr(at, line_end + 1 - at);
    at = line_end + 1;
  }
  return ret;
}

//评估死亡测试的成功或失败，使用两种方法
//以前设置的成员和一个参数：
//
//私有数据成员：
//结果：描述死亡测试方法的枚举
//结论：死、活、扔或返回。死亡测试
//在后三种情况下失败。
//状态：子进程的退出状态。不，它在
//以wait（2）指定的格式。在Windows上，这是
//为exitprocess（）api或数字代码提供的值
//终止程序的异常。
//regex：要应用于的正则表达式对象
//测试捕获的标准错误输出；死亡测试
//如果不匹配则失败。
//
//论点：
//状态_OK:如果退出状态在以下上下文中是可接受的，则为真
//这个特别的死亡测试，如果是错误的话就失败了
//
//如果满足上述所有条件，则返回true。否则，
//第一个失败条件，按照上面给出的顺序，是
//报道。还设置最后一个死亡测试消息字符串。
bool DeathTestImpl::Passed(bool status_ok) {
  if (!spawned())
    return false;

  const std::string error_message = GetCapturedStderr();

  bool success = false;
  Message buffer;

  buffer << "Death test: " << statement() << "\n";
  switch (outcome()) {
    case LIVED:
      buffer << "    Result: failed to die.\n"
             << " Error msg:\n" << FormatDeathTestOutput(error_message);
      break;
    case THREW:
      buffer << "    Result: threw an exception.\n"
             << " Error msg:\n" << FormatDeathTestOutput(error_message);
      break;
    case RETURNED:
      buffer << "    Result: illegal return in test statement.\n"
             << " Error msg:\n" << FormatDeathTestOutput(error_message);
      break;
    case DIED:
      if (status_ok) {
        const bool matched = RE::PartialMatch(error_message.c_str(), *regex());
        if (matched) {
          success = true;
        } else {
          buffer << "    Result: died but not with expected error.\n"
                 << "  Expected: " << regex()->pattern() << "\n"
                 << "Actual msg:\n" << FormatDeathTestOutput(error_message);
        }
      } else {
        buffer << "    Result: died but not with expected exit code:\n"
               << "            " << ExitSummary(status()) << "\n"
               << "Actual msg:\n" << FormatDeathTestOutput(error_message);
      }
      break;
    case IN_PROGRESS:
    default:
      GTEST_LOG_(FATAL)
          << "DeathTest::Passed somehow called before conclusion of test";
  }

  DeathTest::set_last_death_test_message(buffer.GetString());
  return success;
}

# if GTEST_OS_WINDOWS
//WindowsDeathTest在Windows上执行死亡测试。由于
//在Windows上启动新进程的细节，死亡测试
//总是使用threadsafe，google test会考虑
//--gtest_death_test_style=快速设置等于
//——gtest_death_test_style=threadsafe那里。
//
//一些实现说明：如Linux版本、Windows
//实现使用管道进行子到父的通信。但由于
//窗户上管道的细节，需要一些额外的步骤：
//
//1。父级创建通信管道并将句柄存储到
//结束了。
//2。父级启动子级并向其提供信息
//需要获取管道写入端的句柄。
//三。子级获取管道的写入端并向父级发出信号
//使用Windows事件。
//4。现在，父级可以释放管道一侧的写入端。如果
//这在步骤3之前完成，对象的引用计数下降到
//0，它被销毁，阻止孩子获得它。这个
//父级现在必须释放它，或者在
//当子管道终止时，管道不会返回。
//5。父级通过管道读取子级的输出（结果代码和
//来自管道及其stderr的任何可能的错误消息，然后
//确定测试是否失败。
//
//注意：区分win32 api调用与本地方法和函数
//调用，前者在全局命名空间中显式解析。
//
class WindowsDeathTest : public DeathTestImpl {
 public:
  WindowsDeathTest(const char* a_statement,
                   const RE* a_regex,
                   const char* file,
                   int line)
      : DeathTestImpl(a_statement, a_regex), file_(file), line_(line) {}

//所有这些虚拟函数都是从死亡测试继承的。
  virtual int Wait();
  virtual TestRole AssumeRole();

 private:
//死亡测试所在文件的名称。
  const char* const file_;
//死亡测试所在的行号。
  const int line_;
//将管道的写入端处理到子进程。
  AutoHandle write_handle_;
//子进程句柄。
  AutoHandle child_handle_;
//子进程用来通知父进程
//获取了管道写入端的句柄。看到这个之后
//事件父级可以释放自己的句柄以确保
//readfile（）在子级终止时调用return。
  AutoHandle event_handle_;
};

//等待死亡测试中的孩子退出，返回其退出
//状态，如果不存在子进程，则为0。作为副作用，设置
//结果数据成员。
int WindowsDeathTest::Wait() {
  if (!spawned())
    return 0;

//等待直到子级发出它已获取写入端的信号
//否则它会死。
  const HANDLE wait_handles[2] = { child_handle_.Get(), event_handle_.Get() };
  switch (::WaitForMultipleObjects(2,
                                   wait_handles,
FALSE,  //等待任何句柄。
                                   INFINITE)) {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
      break;
    default:
GTEST_DEATH_TEST_CHECK_(false);  //不应该到这里。
  }

//子级已获取管道的写入端或已退出。
//我们松开侧面的手柄继续。
  write_handle_.Reset();
  event_handle_.Reset();

  ReadAndInterpretStatusByte();

//等待子进程退出（如果尚未退出）。这个
//如果子级已退出，则立即返回，无论
//以前对WaitForMultipleObjects的调用是否在此上同步
//处理或不处理。
  GTEST_DEATH_TEST_CHECK_(
      WAIT_OBJECT_0 == ::WaitForSingleObject(child_handle_.Get(),
                                             INFINITE));
  DWORD status_code;
  GTEST_DEATH_TEST_CHECK_(
      ::GetExitCodeProcess(child_handle_.Get(), &status_code) != FALSE);
  child_handle_.Reset();
  set_status(static_cast<int>(status_code));
  return status();
}

//Windows死亡测试的assumerole进程。它创造了一个孩子
//与当前进程具有相同可执行文件以运行
//死亡测试。子进程被赋予--gtest_过滤器和
//--gtest_internal_run_death_测试标记，以便它知道运行
//仅限当前死亡测试。
DeathTest::TestRole WindowsDeathTest::AssumeRole() {
  const UnitTestImpl* const impl = GetUnitTestImpl();
  const InternalRunDeathTestFlag* const flag =
      impl->internal_run_death_test_flag();
  const TestInfo* const info = impl->current_test_info();
  const int death_test_index = info->result()->death_test_count();

  if (flag != NULL) {
//ParseInternalRundeathTestFlag（）已执行所有必需的
//处理。
    set_write_fd(flag->write_fd());
    return EXECUTE_TEST;
  }

//WindowsDeathTest使用匿名管道来传递
//死亡测试。
  SECURITY_ATTRIBUTES handles_are_inheritable = {
    sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
  HANDLE read_handle, write_handle;
  GTEST_DEATH_TEST_CHECK_(
      ::CreatePipe(&read_handle, &write_handle, &handles_are_inheritable,
0)  //默认缓冲区大小。
      != FALSE);
  set_read_fd(::_open_osfhandle(reinterpret_cast<intptr_t>(read_handle),
                                O_RDONLY));
  write_handle_.Reset(write_handle);
  event_handle_.Reset(::CreateEvent(
      &handles_are_inheritable,
TRUE,    //事件将自动重置为无信号状态。
FALSE,   //初始状态是无信号的。
NULL));  //偶数是未命名的。
  GTEST_DEATH_TEST_CHECK_(event_handle_.Get() != NULL);
  const std::string filter_flag =
      std::string("--") + GTEST_FLAG_PREFIX_ + kFilterFlag + "=" +
      info->test_case_name() + "." + info->name();
  const std::string internal_flag =
      std::string("--") + GTEST_FLAG_PREFIX_ + kInternalRunDeathTestFlag +
      "=" + file_ + "|" + StreamableToString(line_) + "|" +
      StreamableToString(death_test_index) + "|" +
      StreamableToString(static_cast<unsigned int>(::GetCurrentProcessId())) +
//大小T与32位和64位指针的宽度相同
//Windows平台。
//请参阅http://msdn.microsoft.com/en-us/library/tcxf1dw6.aspx。
      "|" + StreamableToString(reinterpret_cast<size_t>(write_handle)) +
      "|" + StreamableToString(reinterpret_cast<size_t>(event_handle_.Get()));

char executable_path[_MAX_PATH + 1];  //诺林
  GTEST_DEATH_TEST_CHECK_(
      _MAX_PATH + 1 != ::GetModuleFileNameA(NULL,
                                            executable_path,
                                            _MAX_PATH));

  std::string command_line =
      std::string(::GetCommandLineA()) + " " + filter_flag + " \"" +
      internal_flag + "\"";

  DeathTest::set_last_death_test_message("");

  CaptureStderr();
//刷新日志缓冲区，因为日志流与子级共享。
  FlushInfoLog();

//子进程将与父进程共享标准句柄。
  STARTUPINFOA startup_info;
  memset(&startup_info, 0, sizeof(STARTUPINFO));
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
  startup_info.hStdOutput = ::GetStdHandle(STD_OUTPUT_HANDLE);
  startup_info.hStdError = ::GetStdHandle(STD_ERROR_HANDLE);

  PROCESS_INFORMATION process_info;
  GTEST_DEATH_TEST_CHECK_(::CreateProcessA(
      executable_path,
      const_cast<char*>(command_line.c_str()),
NULL,   //重新调整的进程句柄不可继承。
NULL,   //重新调整的线程句柄不可继承。
TRUE,   //子级继承所有可继承句柄（用于写入句柄）。
0x0,    //默认创建标志。
NULL,   //继承父级的环境。
      UnitTest::GetInstance()->original_working_dir(),
      &startup_info,
      &process_info) != FALSE);
  child_handle_.Reset(process_info.hProcess);
  ::CloseHandle(process_info.hThread);
  set_spawned(true);
  return OVERSEE_TEST;
}
# else  //我们不在窗户上。

//ForkingDeathTest为大多数抽象的
//死亡测试接口的方法。只有assumerole方法是
//左未定义。
class ForkingDeathTest : public DeathTestImpl {
 public:
  ForkingDeathTest(const char* statement, const RE* regex);

//所有这些虚拟函数都是从死亡测试继承的。
  virtual int Wait();

 protected:
  void set_child_pid(pid_t child_pid) { child_pid_ = child_pid; }

 private:
//死亡测试期间子进程的PID；子进程本身为0。
  pid_t child_pid_;
};

//构造ForkingDeathTest。
ForkingDeathTest::ForkingDeathTest(const char* a_statement, const RE* a_regex)
    : DeathTestImpl(a_statement, a_regex),
      child_pid_(-1) {}

//等待死亡测试中的孩子退出，返回其退出
//状态，如果不存在子进程，则为0。作为副作用，设置
//结果数据成员。
int ForkingDeathTest::Wait() {
  if (!spawned())
    return 0;

  ReadAndInterpretStatusByte();

  int status_value;
  GTEST_DEATH_TEST_CHECK_SYSCALL_(waitpid(child_pid_, &status_value, 0));
  set_status(status_value);
  return status_value;
}

//一个具体的死亡测试类，它分叉，然后立即运行测试
//在子进程中。
class NoExecDeathTest : public ForkingDeathTest {
 public:
  NoExecDeathTest(const char* a_statement, const RE* a_regex) :
      ForkingDeathTest(a_statement, a_regex) { }
  virtual TestRole AssumeRole();
};

//fork和run死亡测试的assumerole进程。它实现了一个
//直截了当的fork，用一个简单的管道来传输状态字节。
DeathTest::TestRole NoExecDeathTest::AssumeRole() {
  const size_t thread_count = GetThreadCount();
  if (thread_count != 1) {
    GTEST_LOG_(WARNING) << DeathTestThreadWarning(thread_count);
  }

  int pipe_fd[2];
  GTEST_DEATH_TEST_CHECK_(pipe(pipe_fd) != -1);

  DeathTest::set_last_death_test_message("");
  CaptureStderr();
//当我们在下面分叉处理时，日志文件缓冲区被复制，但是
//文件描述符是共享的。我们在这里刷新所有日志文件，以便关闭
//子进程中的文件描述符不会丢弃
//父进程中描述符和缓冲区之间的同步。
//这尽可能靠近货叉，以避免出现比赛情况
//在死亡测试之前有多个线程正在运行，另一个线程
//线程写入日志文件。
  FlushInfoLog();

  const pid_t child_pid = fork();
  GTEST_DEATH_TEST_CHECK_(child_pid != -1);
  set_child_pid(child_pid);
  if (child_pid == 0) {
    GTEST_DEATH_TEST_CHECK_SYSCALL_(close(pipe_fd[0]));
    set_write_fd(pipe_fd[1]);
//将子进程中的所有日志重定向到stderr以防止
//同时写入日志文件。我们在父级捕获stderr
//处理并将子进程的输出附加到日志中。
    LogToStderr();
//事件转发给事件侦听器API的侦听器必须关闭
//在死亡测试子流程中。
    GetUnitTestImpl()->listeners()->SuppressEventForwarding();
    g_in_fast_death_test_child = true;
    return EXECUTE_TEST;
  } else {
    GTEST_DEATH_TEST_CHECK_SYSCALL_(close(pipe_fd[1]));
    set_read_fd(pipe_fd[0]);
    set_spawned(true);
    return OVERSEE_TEST;
  }
}

//一个具体的死亡测试类，它分叉并重新执行主
//从一开始就用命令行标志设置程序
//只有这个特定的死亡测试才能运行。
class ExecDeathTest : public ForkingDeathTest {
 public:
  ExecDeathTest(const char* a_statement, const RE* a_regex,
                const char* file, int line) :
      ForkingDeathTest(a_statement, a_regex), file_(file), line_(line) { }
  virtual TestRole AssumeRole();
 private:
  static ::std::vector<testing::internal::string>
  GetArgvsForDeathTestChildProcess() {
    ::std::vector<testing::internal::string> args = GetInjectableArgvs();
    return args;
  }
//死亡测试所在文件的名称。
  const char* const file_;
//死亡测试所在的行号。
  const int line_;
};

//用于累积命令行参数的实用程序类。
class Arguments {
 public:
  Arguments() {
    args_.push_back(NULL);
  }

  ~Arguments() {
    for (std::vector<char*>::iterator i = args_.begin(); i != args_.end();
         ++i) {
      free(*i);
    }
  }
  void AddArgument(const char* argument) {
    args_.insert(args_.end() - 1, posix::StrDup(argument));
  }

  template <typename Str>
  void AddArguments(const ::std::vector<Str>& arguments) {
    for (typename ::std::vector<Str>::const_iterator i = arguments.begin();
         i != arguments.end();
         ++i) {
      args_.insert(args_.end() - 1, posix::StrDup(i->c_str()));
    }
  }
  char* const* Argv() {
    return &args_[0];
  }

 private:
  std::vector<char*> args_;
};

//包含子进程的参数的结构
//线程安全式死亡测试过程。
struct ExecDeathTestArgs {
char* const* argv;  //子级调用exec的命令行参数
int close_fd;       //要关闭的文件描述符；管道的读取端
};

#  if GTEST_OS_MAC
inline char** GetEnviron() {
//当google测试在macos x上构建为框架时，环境变量
//不可用。苹果的文档（Man Environ）建议使用
//而不是nsgetenviron（）。
  return *_NSGetEnviron();
}
#  else
//一些POSIX平台希望您声明environ。外部“C”制造
//它位于全局命名空间中。
extern "C" char** environ;
inline char** GetEnviron() { return environ; }
#  endif  //格斯特罗斯奥斯麦克

#  if !GTEST_OS_QNX
//线程安全式死亡测试子进程的主要功能。
//此函数在clone（）-ed进程中调用，因此必须避免
//任何潜在的不安全操作，如malloc或libc函数。
static int ExecDeathTestChildMain(void* child_arg) {
  ExecDeathTestArgs* const args = static_cast<ExecDeathTestArgs*>(child_arg);
  GTEST_DEATH_TEST_CHECK_SYSCALL_(close(args->close_fd));

//我们需要在相同的环境中执行测试程序，
//它最初是被调用的。因此我们改为原来的
//首先是工作目录。
  const char* const original_dir =
      UnitTest::GetInstance()->original_working_dir();
//我们可以安全地调用chdir（），因为它是直接的系统调用。
  if (chdir(original_dir) != 0) {
    DeathTestAbort(std::string("chdir(\"") + original_dir + "\") failed: " +
                   GetLastErrnoDescription());
    return EXIT_FAILURE;
  }

//我们可以安全地调用execve（），因为它是直接的系统调用。我们
//无法使用execvp（），因为它是libc函数，因此可能
//不安全的。由于execve（）不搜索路径，因此用户必须
//通过至少包含
//一个路径分隔符。
  execve(args->argv[0], args->argv, GetEnviron());
  DeathTestAbort(std::string("execve(") + args->argv[0] + ", ...) in " +
                 original_dir + " failed: " +
                 GetLastErrnoDescription());
  return EXIT_FAILURE;
}
#  endif  //！格斯塔斯奥斯QNX

//两个共同决定堆栈方向的实用程序
//生长。
//这可以通过一个递归更优雅地完成。
//功能，但我们要防止
//智能编译器优化递归。
//
//为了防止GCC 4.6嵌入，需要gtest_no_inline_u
//stacklowerthanaddress进入stackgrowsdown，而stackgrowsdown不提供
//正确答案。
void StackLowerThanAddress(const void* ptr, bool* result) GTEST_NO_INLINE_;
void StackLowerThanAddress(const void* ptr, bool* result) {
  int dummy;
  *result = (&dummy < ptr);
}

//确保地址消毒剂不会干扰这里的堆栈。
GTEST_ATTRIBUTE_NO_SANITIZE_ADDRESS_
bool StackGrowsDown() {
  int dummy;
  bool result;
  StackLowerThanAddress(&dummy, &result);
  return result;
}

//在中生成与当前进程具有相同可执行文件的子进程
//线程安全的方式，并指示它运行死亡测试。这个
//实现使用fork（2）+exec。在克隆（2）所在的系统上
//可用，它是用来代替，稍微更线程安全。在QNX上，
//fork只支持单线程环境，因此此函数使用
//在那里产卵。如果
//有什么问题吗？
static pid_t ExecDeathTestSpawnChild(char* const* argv, int close_fd) {
  ExecDeathTestArgs args = { argv, close_fd };
  pid_t child_pid = -1;

#  if GTEST_OS_QNX
//获取当前目录并将其设置为在子目录中关闭
//过程。
  const int cwd_fd = open(".", O_RDONLY);
  GTEST_DEATH_TEST_CHECK_(cwd_fd != -1);
  GTEST_DEATH_TEST_CHECK_SYSCALL_(fcntl(cwd_fd, F_SETFD, FD_CLOEXEC));
//我们需要在相同的环境中执行测试程序，
//它最初是被调用的。因此我们改为原来的
//首先是工作目录。
  const char* const original_dir =
      UnitTest::GetInstance()->original_working_dir();
//我们可以安全地调用chdir（），因为它是直接的系统调用。
  if (chdir(original_dir) != 0) {
    DeathTestAbort(std::string("chdir(\"") + original_dir + "\") failed: " +
                   GetLastErrnoDescription());
    return EXIT_FAILURE;
  }

  int fd_flags;
//将close_fd设置为在生成后关闭。
  GTEST_DEATH_TEST_CHECK_SYSCALL_(fd_flags = fcntl(close_fd, F_GETFD));
  GTEST_DEATH_TEST_CHECK_SYSCALL_(fcntl(close_fd, F_SETFD,
                                        fd_flags | FD_CLOEXEC));
  struct inheritance inherit = {0};
//生成是一个系统调用。
  child_pid = spawn(args.argv[0], 0, NULL, &inherit, args.argv, GetEnviron());
//还原当前工作目录。
  GTEST_DEATH_TEST_CHECK_(fchdir(cwd_fd) != -1);
  GTEST_DEATH_TEST_CHECK_SYSCALL_(close(cwd_fd));

#  else   //格斯塔斯奥斯QNX
#   if GTEST_OS_LINUX
//当fork（）或clone（）执行时接收到sigprof信号时，
//进程可能会挂起。为了避免这种情况，我们在这里忽略sigprof并重新启用
//在对fork（）/clone（）的调用完成之后。
  struct sigaction saved_sigprof_action;
  struct sigaction ignore_sigprof_action;
  memset(&ignore_sigprof_action, 0, sizeof(ignore_sigprof_action));
  sigemptyset(&ignore_sigprof_action.sa_mask);
  ignore_sigprof_action.sa_handler = SIG_IGN;
  GTEST_DEATH_TEST_CHECK_SYSCALL_(sigaction(
      SIGPROF, &ignore_sigprof_action, &saved_sigprof_action));
#   endif  //GTestOSLinux

#   if GTEST_HAS_CLONE
  const bool use_fork = GTEST_FLAG(death_test_use_fork);

  if (!use_fork) {
    static const bool stack_grows_down = StackGrowsDown();
    const size_t stack_size = getpagesize();
//mac上没有定义mmap-anonymous，所以我们使用map-anon。
    void* const stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                             MAP_ANON | MAP_PRIVATE, -1, 0);
    GTEST_DEATH_TEST_CHECK_(stack != MAP_FAILED);

//以字节为单位的最大堆栈对齐方式：对于向下增长的堆栈，此
//从堆栈空间大小中减去数量以获得地址
//在堆栈空间内，并且与我们关心的所有系统对齐
//关于。据我所知，没有比堆栈对齐更大的ABI
//超过64。我们假设堆栈和堆栈大小已经对齐
//K轴堆叠对齐。
    const size_t kMaxStackAlignment = 64;
    void* const stack_top =
        static_cast<char*>(stack) +
            (stack_grows_down ? stack_size - kMaxStackAlignment : 0);
    GTEST_DEATH_TEST_CHECK_(stack_size > kMaxStackAlignment &&
        reinterpret_cast<intptr_t>(stack_top) % kMaxStackAlignment == 0);

    child_pid = clone(&ExecDeathTestChildMain, stack_top, SIGCHLD, &args);

    GTEST_DEATH_TEST_CHECK_(munmap(stack, stack_size) != -1);
  }
#   else
  const bool use_fork = true;
#   endif  //格斯塔哈斯克隆

  if (use_fork && (child_pid = fork()) == 0) {
      ExecDeathTestChildMain(&args);
      _exit(0);
  }
#  endif  //格斯塔斯奥斯QNX
#  if GTEST_OS_LINUX
  GTEST_DEATH_TEST_CHECK_SYSCALL_(
      sigaction(SIGPROF, &saved_sigprof_action, NULL));
#  endif  //GTestOSLinux

  GTEST_DEATH_TEST_CHECK_(child_pid != -1);
  return child_pid;
}

//fork和exec死亡测试的assumerole进程。它重新执行
//从一开始的主程序，设置--gtest_过滤器
//和--gtest_internal_run_death_test flags只引起电流
//重新进行死亡测试。
DeathTest::TestRole ExecDeathTest::AssumeRole() {
  const UnitTestImpl* const impl = GetUnitTestImpl();
  const InternalRunDeathTestFlag* const flag =
      impl->internal_run_death_test_flag();
  const TestInfo* const info = impl->current_test_info();
  const int death_test_index = info->result()->death_test_count();

  if (flag != NULL) {
    set_write_fd(flag->write_fd());
    return EXECUTE_TEST;
  }

  int pipe_fd[2];
  GTEST_DEATH_TEST_CHECK_(pipe(pipe_fd) != -1);
//清除管道写入端的close on exec标志，以免
//当子进程执行exec时，它将关闭：
  GTEST_DEATH_TEST_CHECK_(fcntl(pipe_fd[1], F_SETFD, 0) != -1);

  const std::string filter_flag =
      std::string("--") + GTEST_FLAG_PREFIX_ + kFilterFlag + "="
      + info->test_case_name() + "." + info->name();
  const std::string internal_flag =
      std::string("--") + GTEST_FLAG_PREFIX_ + kInternalRunDeathTestFlag + "="
      + file_ + "|" + StreamableToString(line_) + "|"
      + StreamableToString(death_test_index) + "|"
      + StreamableToString(pipe_fd[1]);
  Arguments args;
  args.AddArguments(GetArgvsForDeathTestChildProcess());
  args.AddArgument(filter_flag.c_str());
  args.AddArgument(internal_flag.c_str());

  DeathTest::set_last_death_test_message("");

  CaptureStderr();
//有关下一行的原因，请参见noexecdeahtest:：assumerole中的注释。
//是必要的。
  FlushInfoLog();

  const pid_t child_pid = ExecDeathTestSpawnChild(args.Argv(), pipe_fd[0]);
  GTEST_DEATH_TEST_CHECK_SYSCALL_(close(pipe_fd[1]));
  set_child_pid(child_pid);
  set_read_fd(pipe_fd[0]);
  set_spawned(true);
  return OVERSEE_TEST;
}

# endif  //！GTEST操作系统窗口

//创建一个具体的DeathTest派生类，该类依赖于
//--gtest_death_test_样式标志，并设置指向的指针
//通过“test”参数把它的地址。如果测试应该
//跳过，将该指针设置为空。返回true，除非
//标志设置为无效值。
bool DefaultDeathTestFactory::Create(const char* statement, const RE* regex,
                                     const char* file, int line,
                                     DeathTest** test) {
  UnitTestImpl* const impl = GetUnitTestImpl();
  const InternalRunDeathTestFlag* const flag =
      impl->internal_run_death_test_flag();
  const int death_test_index = impl->current_test_info()
      ->increment_death_test_count();

  if (flag != NULL) {
    if (death_test_index > flag->index()) {
      DeathTest::set_last_death_test_message(
          "Death test count (" + StreamableToString(death_test_index)
          + ") somehow exceeded expected maximum ("
          + StreamableToString(flag->index()) + ")");
      return false;
    }

    if (!(flag->file() == file && flag->line() == line &&
          flag->index() == death_test_index)) {
      *test = NULL;
      return true;
    }
  }

# if GTEST_OS_WINDOWS

  if (GTEST_FLAG(death_test_style) == "threadsafe" ||
      GTEST_FLAG(death_test_style) == "fast") {
    *test = new WindowsDeathTest(statement, regex, file, line);
  }

# else

  if (GTEST_FLAG(death_test_style) == "threadsafe") {
    *test = new ExecDeathTest(statement, regex, file, line);
  } else if (GTEST_FLAG(death_test_style) == "fast") {
    *test = new NoExecDeathTest(statement, regex);
  }

# endif  //GTEST操作系统窗口

else {  //nolint-这比if中不平衡的括号更容易读取。
    DeathTest::set_last_death_test_message(
        "Unknown death test style \"" + GTEST_FLAG(death_test_style)
        + "\" encountered");
    return false;
  }

  return true;
}

//在给定分隔符上拆分给定字符串，填充给定的
//带字段的向量。GTEST有死亡测试意味着我们有
//：：std：：string，所以我们可以在这里使用它。
static void SplitString(const ::std::string& str, char delimiter,
                        ::std::vector< ::std::string>* dest) {
  ::std::vector< ::std::string> parsed;
  ::std::string::size_type pos = 0;
  while (::testing::internal::AlwaysTrue()) {
    const ::std::string::size_type colon = str.find(delimiter, pos);
    if (colon == ::std::string::npos) {
      parsed.push_back(str.substr(pos));
      break;
    } else {
      parsed.push_back(str.substr(pos, colon - pos));
      pos = colon + 1;
    }
  }
  dest->swap(parsed);
}

# if GTEST_OS_WINDOWS
//从提供的参数重新创建管道和事件句柄，
//向事件发出信号，并返回环绕管道的文件描述符
//把手。此函数仅在子进程中调用。
int GetStatusFileDescriptor(unsigned int parent_process_id,
                            size_t write_handle_as_size_t,
                            size_t event_handle_as_size_t) {
  AutoHandle parent_process_handle(::OpenProcess(PROCESS_DUP_HANDLE,
FALSE,  //不可继承。
                                                   parent_process_id));
  if (parent_process_handle.Get() == INVALID_HANDLE_VALUE) {
    DeathTestAbort("Unable to open parent process " +
                   StreamableToString(parent_process_id));
  }

//TODO（vladl@google.com）：将以下检查替换为
//编译时断言（如果可用）。
  GTEST_CHECK_(sizeof(HANDLE) <= sizeof(size_t));

  const HANDLE write_handle =
      reinterpret_cast<HANDLE>(write_handle_as_size_t);
  HANDLE dup_write_handle;

//新初始化的句柄只能在父级中访问
//过程。为了在孩子体内获得一个可访问的，我们需要使用
//重复手柄。
  if (!::DuplicateHandle(parent_process_handle.Get(), write_handle,
                         ::GetCurrentProcess(), &dup_write_handle,
0x0,    //已忽略请求的权限，因为
//使用重复的访问。
FALSE,  //请求不可继承的处理程序。
                         DUPLICATE_SAME_ACCESS)) {
    DeathTestAbort("Unable to duplicate the pipe handle " +
                   StreamableToString(write_handle_as_size_t) +
                   " from the parent process " +
                   StreamableToString(parent_process_id));
  }

  const HANDLE event_handle = reinterpret_cast<HANDLE>(event_handle_as_size_t);
  HANDLE dup_event_handle;

  if (!::DuplicateHandle(parent_process_handle.Get(), event_handle,
                         ::GetCurrentProcess(), &dup_event_handle,
                         0x0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS)) {
    DeathTestAbort("Unable to duplicate the event handle " +
                   StreamableToString(event_handle_as_size_t) +
                   " from the parent process " +
                   StreamableToString(parent_process_id));
  }

  const int write_fd =
      ::_open_osfhandle(reinterpret_cast<intptr_t>(dup_write_handle), O_APPEND);
  if (write_fd == -1) {
    DeathTestAbort("Unable to convert pipe handle " +
                   StreamableToString(write_handle_as_size_t) +
                   " to a file descriptor");
  }

//向父级发出信号，表示已获取管道的写入端
//因此，父级可以释放自己的写入端。
  ::SetEvent(dup_event_handle);

  return write_fd;
}
# endif  //GTEST操作系统窗口

//返回新创建的带有字段的InternalRundeathTestFlag对象
//从gtest_标志（内部_运行_死亡_测试）标志初始化，如果
//已指定标志；否则返回空值。
InternalRunDeathTestFlag* ParseInternalRunDeathTestFlag() {
  if (GTEST_FLAG(internal_run_death_test) == "") return NULL;

//gtest_has_death_test表示我们有：：std：：string，因此我们
//可以在这里使用。
  int line = -1;
  int index = -1;
  ::std::vector< ::std::string> fields;
  SplitString(GTEST_FLAG(internal_run_death_test).c_str(), '|', &fields);
  int write_fd = -1;

# if GTEST_OS_WINDOWS

  unsigned int parent_process_id = 0;
  size_t write_handle_as_size_t = 0;
  size_t event_handle_as_size_t = 0;

  if (fields.size() != 6
      || !ParseNaturalNumber(fields[1], &line)
      || !ParseNaturalNumber(fields[2], &index)
      || !ParseNaturalNumber(fields[3], &parent_process_id)
      || !ParseNaturalNumber(fields[4], &write_handle_as_size_t)
      || !ParseNaturalNumber(fields[5], &event_handle_as_size_t)) {
    DeathTestAbort("Bad --gtest_internal_run_death_test flag: " +
                   GTEST_FLAG(internal_run_death_test));
  }
  write_fd = GetStatusFileDescriptor(parent_process_id,
                                     write_handle_as_size_t,
                                     event_handle_as_size_t);
# else

  if (fields.size() != 4
      || !ParseNaturalNumber(fields[1], &line)
      || !ParseNaturalNumber(fields[2], &index)
      || !ParseNaturalNumber(fields[3], &write_fd)) {
    DeathTestAbort("Bad --gtest_internal_run_death_test flag: "
        + GTEST_FLAG(internal_run_death_test));
  }

# endif  //GTEST操作系统窗口

  return new InternalRunDeathTestFlag(fields[0], line, index, write_fd);
}

}  //命名空间内部

#endif  //GTEST有死亡测试

}  //命名空间测试
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
//作者：keith.ray@gmail.com（keith ray）


#include <stdlib.h>

#if GTEST_OS_WINDOWS_MOBILE
# include <windows.h>
#elif GTEST_OS_WINDOWS
# include <direct.h>
# include <io.h>
#elif GTEST_OS_SYMBIAN
//symbian openc在sys/syslimits.h中具有路径“max”
# include <sys/syslimits.h>
#else
# include <limits.h>
# include <climits>  //一些Linux发行版在这里定义了路径最大值。
#endif  //GTEST操作系统Windows移动

#if GTEST_OS_WINDOWS
# define GTEST_PATH_MAX_ _MAX_PATH
#elif defined(PATH_MAX)
# define GTEST_PATH_MAX_ PATH_MAX
#elif defined(_XOPEN_PATH_MAX)
# define GTEST_PATH_MAX_ _XOPEN_PATH_MAX
#else
# define GTEST_PATH_MAX_ _POSIX_PATH_MAX
#endif  //GTEST操作系统窗口


namespace testing {
namespace internal {

#if GTEST_OS_WINDOWS
//在Windows上，“\\”是标准路径分隔符，但有许多工具和
//Windows API还接受“/”作为备用路径分隔符。除非另有规定
//注意，文件路径可以包含路径分隔符或混合
//他们当中。
const char kPathSeparator = '\\';
const char kAlternatePathSeparator = '/';
const char kAlternatePathSeparatorString[] = "/";
# if GTEST_OS_WINDOWS_MOBILE
//Windows CE没有当前目录。你不应该使用
//Windows CE测试中的当前目录，但至少
//提供合理的回退。
const char kCurrentDirectoryString[] = "\\";
//Windows CE未定义无效的\u文件\u属性
const DWORD kInvalidFileAttributes = 0xffffffff;
# else
const char kCurrentDirectoryString[] = ".\\";
# endif  //GTEST操作系统Windows移动
#else
const char kPathSeparator = '/';
const char kCurrentDirectoryString[] = "./";
#endif  //GTEST操作系统窗口

//返回给定字符是否为有效的路径分隔符。
static bool IsPathSeparator(char c) {
#if GTEST_HAS_ALT_PATH_SEP_
  return (c == kPathSeparator) || (c == kAlternatePathSeparator);
#else
  return c == kPathSeparator;
#endif
}

//返回当前工作目录，如果不成功，则返回“”。
FilePath FilePath::GetCurrentDir() {
#if GTEST_OS_WINDOWS_MOBILE || GTEST_OS_WINDOWS_PHONE || GTEST_OS_WINDOWS_RT
//Windows CE没有当前目录，因此我们只返回
//合理的东西。
  return FilePath(kCurrentDirectoryString);
#elif GTEST_OS_WINDOWS
  char cwd[GTEST_PATH_MAX_ + 1] = { '\0' };
  return FilePath(_getcwd(cwd, sizeof(cwd)) == NULL ? "" : cwd);
#else
  char cwd[GTEST_PATH_MAX_ + 1] = { '\0' };
  char* result = getcwd(cwd, sizeof(cwd));
# if GTEST_OS_NACL
//getcwd可能会因为沙箱而在nacl中失败，所以返回一些
//合理。用户可能为getcwd提供了一个垫片实现，
//但是，只有在检测到故障时才会回退。
  return FilePath(result == NULL ? kCurrentDirectoryString : cwd);
# endif  //果蝇
  return FilePath(result == NULL ? "" : cwd);
#endif  //GTEST操作系统Windows移动
}

//返回删除了不区分大小写扩展名的文件路径的副本。
//示例：filepath（“dir/file.exe”）.removeextension（“exe”）返回
//文件路径（“dir/file”）。如果不区分大小写的扩展名不是
//找到，返回原始文件路径的副本。
FilePath FilePath::RemoveExtension(const char* extension) const {
  const std::string dot_extension = std::string(".") + extension;
  if (String::EndsWithCaseInsensitive(pathname_, dot_extension)) {
    return FilePath(pathname_.substr(
        0, pathname_.length() - dot_extension.length()));
  }
  return *this;
}

//返回指向中最后一个出现的有效路径分隔符的指针
//文件名。例如，在Windows上，'/'和'\'都是有效路径
//分离器。如果找不到路径分隔符，则返回空值。
const char* FilePath::FindLastPathSeparator() const {
  const char* const last_sep = strrchr(c_str(), kPathSeparator);
#if GTEST_HAS_ALT_PATH_SEP_
  const char* const last_alt_sep = strrchr(c_str(), kAlternatePathSeparator);
//比较只有一个为空的两个指针是未定义的。
  if (last_alt_sep != NULL &&
      (last_sep == NULL || last_alt_sep > last_sep)) {
    return last_alt_sep;
  }
#endif
  return last_sep;
}

//返回删除目录部分的文件路径的副本。
//示例：file path（“path/to/file”）.removedirectoryname（）返回
//文件路径（“文件”）。如果没有目录部分（“just_a_file”），则返回
//文件路径未修改。如果没有文件部分（“just_a_dir/”），则
//返回空文件路径（“”）。
//在Windows平台上，“\”是路径分隔符，否则是“/”。
FilePath FilePath::RemoveDirectoryName() const {
  const char* const last_sep = FindLastPathSeparator();
  return last_sep ? FilePath(last_sep + 1) : *this;
}

//removeFileName返回已删除文件名的目录路径。
//示例：file path（“path/to/file”）.removefilename（）返回“path/to/”。
//如果文件路径是“a \u文件”或/a \u文件，则removeFileName返回
//filepath（“./”）或，在Windows上，filepath（“.\\”）。如果文件路径
//没有文件，比如“just/a/dir/”，它返回未修改的文件路径。
//在Windows平台上，“\”是路径分隔符，否则是“/”。
FilePath FilePath::RemoveFileName() const {
  const char* const last_sep = FindLastPathSeparator();
  std::string dir;
  if (last_sep) {
    dir = std::string(c_str(), last_sep + 1 - c_str());
  } else {
    dir = kCurrentDirectoryString;
  }
  return FilePath(dir);
}

//用于在目录中为XML输出命名文件的helper函数。

//给定directory=“dir”，base_name=“test”，number=0，
//extension=“xml”，返回“dir/test.xml”。如果数字大于
//大于零（例如，12），返回“dir/test_12.xml”。
//在Windows平台上，使用\作为分隔符，而不是/。
FilePath FilePath::MakeFileName(const FilePath& directory,
                                const FilePath& base_name,
                                int number,
                                const char* extension) {
  std::string file;
  if (number == 0) {
    file = base_name.string() + "." + extension;
  } else {
    file = base_name.string() + "_" + StreamableToString(number)
        + "." + extension;
  }
  return ConcatPaths(directory, FilePath(file));
}

//给定directory=“dir”，relative_path=“test.xml”，返回“dir/test.xml”。
//在Windows上，使用\作为分隔符，而不是/。
FilePath FilePath::ConcatPaths(const FilePath& directory,
                               const FilePath& relative_path) {
  if (directory.IsEmpty())
    return relative_path;
  const FilePath dir(directory.RemoveTrailingPathSeparator());
  return FilePath(dir.string() + kPathSeparator + relative_path.string());
}

//如果路径名描述了文件系统中可查找的内容，则返回true，
//文件、目录或其他。
bool FilePath::FileOrDirectoryExists() const {
#if GTEST_OS_WINDOWS_MOBILE
  LPCWSTR unicode = String::AnsiToUtf16(pathname_.c_str());
  const DWORD attributes = GetFileAttributes(unicode);
  delete [] unicode;
  return attributes != kInvalidFileAttributes;
#else
  posix::StatStruct file_stat;
  return posix::Stat(pathname_.c_str(), &file_stat) == 0;
#endif  //GTEST操作系统Windows移动
}

//如果路径名描述文件系统中的目录，则返回true
//这是存在的。
bool FilePath::DirectoryExists() const {
  bool result = false;
#if GTEST_OS_WINDOWS
//如果路径是上的根目录，则不要去掉尾随分隔符
//Windows（如“C:\\”）。
  const FilePath& path(IsRootDirectory() ? *this :
                                           RemoveTrailingPathSeparator());
#else
  const FilePath& path(*this);
#endif

#if GTEST_OS_WINDOWS_MOBILE
  LPCWSTR unicode = String::AnsiToUtf16(path.c_str());
  const DWORD attributes = GetFileAttributes(unicode);
  delete [] unicode;
  if ((attributes != kInvalidFileAttributes) &&
      (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
    result = true;
  }
#else
  posix::StatStruct file_stat;
  result = posix::Stat(path.c_str(), &file_stat) == 0 &&
      posix::IsDir(file_stat);
#endif  //GTEST操作系统Windows移动

  return result;
}

//如果pathname描述根目录，则返回true。（窗户有一个
//每个磁盘驱动器的根目录。）
bool FilePath::IsRootDirectory() const {
#if GTEST_OS_WINDOWS
//todo（wan@google.com）：在Windows上
//\\server\share可以是根目录，但不能是
//当前目录。妥善处理。
  return pathname_.length() == 3 && IsAbsolutePath();
#else
  return pathname_.length() == 1 && IsPathSeparator(pathname_.c_str()[0]);
#endif
}

//如果pathname描述绝对路径，则返回true。
bool FilePath::IsAbsolutePath() const {
  const char* const name = pathname_.c_str();
#if GTEST_OS_WINDOWS
  return pathname_.length() >= 3 &&
     ((name[0] >= 'a' && name[0] <= 'z') ||
      (name[0] >= 'A' && name[0] <= 'Z')) &&
     name[1] == ':' &&
     IsPathSeparator(name[2]);
#else
  return IsPathSeparator(name[0]);
#endif
}

//返回当前不存在的文件的路径名。路径名
//将是directory/base_name.extension或
//directory/base_name_<number>。如果directory/base_name.extension
//已经存在。在找到路径名之前，编号将递增。
//这还不存在。
//示例：“dir/foo_test.xml”或“dir/foo_test_1.xml”。
//如果两个或多个进程调用此命令，则可能存在争用条件
//同时使用函数--它们可以选择相同的文件名。
FilePath FilePath::GenerateUniqueFileName(const FilePath& directory,
                                          const FilePath& base_name,
                                          const char* extension) {
  FilePath full_pathname;
  int number = 0;
  do {
    full_pathname.Set(MakeFileName(directory, base_name, number++, extension));
  } while (full_pathname.FileOrDirectoryExists());
  return full_pathname;
}

//如果filepath以路径分隔符结尾，则返回true，这表示
//它旨在表示一个目录。否则返回false。
//这不会检查目录（或文件）是否确实存在。
bool FilePath::IsDirectory() const {
  return !pathname_.empty() &&
         IsPathSeparator(pathname_.c_str()[pathname_.length() - 1]);
}

//创建目录以便路径存在。如果成功或如果
//目录已经存在；如果无法创建目录，则返回false
//出于任何原因。
bool FilePath::CreateDirectoriesRecursively() const {
  if (!this->IsDirectory()) {
    return false;
  }

  if (pathname_.length() == 0 || this->DirectoryExists()) {
    return true;
  }

  const FilePath parent(this->RemoveTrailingPathSeparator().RemoveFileName());
  return parent.CreateDirectoriesRecursively() && this->CreateFolder();
}

//创建目录以使路径存在。如果成功或
//如果目录已经存在；如果无法创建
//目录的任何原因，包括如果父目录
//存在。未命名为“createddirectory”，因为这是Windows上的宏。
bool FilePath::CreateFolder() const {
#if GTEST_OS_WINDOWS_MOBILE
  FilePath removed_sep(this->RemoveTrailingPathSeparator());
  LPCWSTR unicode = String::AnsiToUtf16(removed_sep.c_str());
  int result = CreateDirectory(unicode, NULL) ? 0 : -1;
  delete [] unicode;
#elif GTEST_OS_WINDOWS
  int result = _mkdir(pathname_.c_str());
#else
  int result = mkdir(pathname_.c_str(), 0777);
#endif  //GTEST操作系统Windows移动

  if (result == -1) {
return this->DirectoryExists();  //如果目录存在，则错误正常。
  }
return true;  //没有错误。
}

//如果输入名称具有尾随分隔符字符，请将其移除并返回
//名称，否则返回未修改的名称字符串。
//在Windows平台上，使用\作为分隔符，其他平台使用/。
FilePath FilePath::RemoveTrailingPathSeparator() const {
  return IsDirectory()
      ? FilePath(pathname_.substr(0, pathname_.length() - 1))
      : *this;
}

//删除路径名中可能存在的所有冗余分隔符。
//例如，“bar//foo”变为“bar/foo”。不排除其他
//路径名中可能涉及“.”或“..”的冗余。
//todo（wan@google.com）：处理Windows网络共享（例如\\server\share）。
void FilePath::Normalize() {
  if (pathname_.c_str() == NULL) {
    pathname_ = "";
    return;
  }
  const char* src = pathname_.c_str();
  char* const dest = new char[pathname_.length() + 1];
  char* dest_ptr = dest;
  memset(dest_ptr, 0, pathname_.length() + 1);

  while (*src != '\0') {
    *dest_ptr = *src;
    if (!IsPathSeparator(*src)) {
      src++;
    } else {
#if GTEST_HAS_ALT_PATH_SEP_
      if (*dest_ptr == kAlternatePathSeparator) {
        *dest_ptr = kPathSeparator;
      }
#endif
      while (IsPathSeparator(*src))
        src++;
    }
    dest_ptr++;
  }
  *dest_ptr = '\0';
  pathname_ = dest;
  delete[] dest;
}

}  //命名空间内部
}  //命名空间测试
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
//作者：wan@google.com（zhanyong wan）


#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if GTEST_OS_WINDOWS
# include <windows.h>
# include <io.h>
# include <sys/stat.h>
# include <map>  //用于ThreadLocal。
#else
# include <unistd.h>
#endif  //GTEST操作系统窗口

#if GTEST_OS_MAC
# include <mach/mach_init.h>
# include <mach/task.h>
# include <mach/vm_map.h>
#endif  //格斯特罗斯奥斯麦克

#if GTEST_OS_QNX
# include <devctl.h>
# include <fcntl.h>
# include <sys/procfs.h>
#endif  //格斯塔斯奥斯QNX


//指示此翻译单元是Google测试的一部分
//实施。它必须在gtest内部inl.h之前
//包含，否则将出现编译器错误。这个把戏是为了
//防止在
//用户代码。
#define GTEST_IMPLEMENTATION_ 1
#undef GTEST_IMPLEMENTATION_

namespace testing {
namespace internal {

#if defined(_MSC_VER) || defined(__BORLANDC__)
//MSVC和C++Builder不提供STDRRYFLIENO的定义。
const int kStdOutFileno = 1;
const int kStdErrFileno = 2;
#else
const int kStdOutFileno = STDOUT_FILENO;
const int kStdErrFileno = STDERR_FILENO;
#endif  //三桅纵帆船

#if GTEST_OS_MAC

//返回进程中运行的线程数，或0表示
//我们检测不到它。
size_t GetThreadCount() {
  const task_t task = mach_task_self();
  mach_msg_type_number_t thread_count;
  thread_act_array_t thread_list;
  const kern_return_t status = task_threads(task, &thread_list, &thread_count);
  if (status == KERN_SUCCESS) {
//任务线程在线程列表中分配资源，我们需要释放它们
//避免泄漏。
    vm_deallocate(task,
                  reinterpret_cast<vm_address_t>(thread_list),
                  sizeof(thread_t) * thread_count);
    return static_cast<size_t>(thread_count);
  } else {
    return 0;
  }
}

#elif GTEST_OS_QNX

//返回进程中运行的线程数，或0表示
//我们检测不到它。
size_t GetThreadCount() {
  const int fd = open("/proc/self/as", O_RDONLY);
  if (fd < 0) {
    return 0;
  }
  procfs_info process_info;
  const int status =
      devctl(fd, DCMD_PROC_INFO, &process_info, sizeof(process_info), NULL);
  close(fd);
  if (status == EOK) {
    return static_cast<size_t>(process_info.num_threads);
  } else {
    return 0;
  }
}

#else

size_t GetThreadCount() {
//没有可移植的方法来检测线程的数量，所以我们只是
//返回0表示我们无法检测到它。
  return 0;
}

#endif  //格斯特罗斯奥斯麦克

#if GTEST_IS_THREADSAFE && GTEST_OS_WINDOWS

void SleepMilliseconds(int n) {
  ::Sleep(n);
}

AutoHandle::AutoHandle()
    : handle_(INVALID_HANDLE_VALUE) {}

AutoHandle::AutoHandle(Handle handle)
    : handle_(handle) {}

AutoHandle::~AutoHandle() {
  Reset();
}

AutoHandle::Handle AutoHandle::Get() const {
  return handle_;
}

void AutoHandle::Reset() {
  Reset(INVALID_HANDLE_VALUE);
}

void AutoHandle::Reset(HANDLE handle) {
//用我们已经拥有的相同句柄重置是无效的。
  if (handle_ != handle) {
    if (IsCloseable()) {
      ::CloseHandle(handle_);
    }
    handle_ = handle;
  } else {
    GTEST_CHECK_(!IsCloseable())
        << "Resetting a valid handle to itself is likely a programmer error "
            "and thus not allowed.";
  }
}

bool AutoHandle::IsCloseable() const {
//不同的Windows API可以使用这些值中的任何一个来表示
//无效句柄。
  return handle_ != NULL && handle_ != INVALID_HANDLE_VALUE;
}

Notification::Notification()
: event_(::CreateEvent(NULL,   //默认安全属性。
TRUE,   //不要自动复位。
FALSE,  //初始未设置。
NULL)) {  //匿名事件。
  GTEST_CHECK_(event_.Get() != NULL);
}

void Notification::Notify() {
  GTEST_CHECK_(::SetEvent(event_.Get()) != FALSE);
}

void Notification::WaitForNotification() {
  GTEST_CHECK_(
      ::WaitForSingleObject(event_.Get(), INFINITE) == WAIT_OBJECT_0);
}

Mutex::Mutex()
    : owner_thread_id_(0),
      type_(kDynamic),
      critical_section_init_phase_(0),
      critical_section_(new CRITICAL_SECTION) {
  ::InitializeCriticalSection(critical_section_);
}

Mutex::~Mutex() {
//静态互斥锁是故意泄漏的。尝试是不安全的
//把它们清理干净。
//Todo（Yukawa）：切换到Slim读写器（SRW）锁，这需要
//无需清理，但仅在Vista和更高版本上可用。
//http://msdn.microsoft.com/en-us/library/windows/desktop/aa904937.aspx
  if (type_ == kDynamic) {
    ::DeleteCriticalSection(critical_section_);
    delete critical_section_;
    critical_section_ = NULL;
  }
}

void Mutex::Lock() {
  ThreadSafeLazyInit();
  ::EnterCriticalSection(critical_section_);
  owner_thread_id_ = ::GetCurrentThreadId();
}

void Mutex::Unlock() {
  ThreadSafeLazyInit();
//我们不保护写信给所有者，因为它是
//调用方负责确保当前线程保持
//调用此命令时互斥。
  owner_thread_id_ = 0;
  ::LeaveCriticalSection(critical_section_);
}

//如果当前线程持有互斥体，则不执行任何操作。否则，崩溃
//很有可能。
void Mutex::AssertHeld() {
  ThreadSafeLazyInit();
  GTEST_CHECK_(owner_thread_id_ == ::GetCurrentThreadId())
      << "The current thread is not holding the mutex @" << this;
}

//初始化静态互斥中的所有者线程和关键部分。
void Mutex::ThreadSafeLazyInit() {
//动态互斥体在构造函数中初始化。
  if (type_ == kStatic) {
    switch (
        ::InterlockedCompareExchange(&critical_section_init_phase_, 1L, 0L)) {
      case 0:
//如果交换前关键的\部分\初始阶段\是0，我们
//是第一个测试它并需要执行初始化的。
        owner_thread_id_ = 0;
        critical_section_ = new CRITICAL_SECTION;
        ::InitializeCriticalSection(critical_section_);
//更新关键\部分\初始\相位\至2以发出信号
//初始化完成。
        GTEST_CHECK_(::InterlockedCompareExchange(
                          &critical_section_init_phase_, 2L, 1L) ==
                      1L);
        break;
      case 1:
//其他人已经在初始化互斥体；旋转直到他们
//完成了。
        while (::InterlockedCompareExchange(&critical_section_init_phase_,
                                            2L,
                                            2L) != 2L) {
//可能会将线程的剩余时间片转换为其他时间片
//线程。
          ::Sleep(0);
        }
        break;

      case 2:
break;  //互斥体已经初始化，可以使用了。

      default:
        GTEST_CHECK_(false)
            << "Unexpected value of critical_section_init_phase_ "
            << "while initializing a static mutex.";
    }
  }
}

namespace {

class ThreadWithParamSupport : public ThreadWithParamBase {
 public:
  static HANDLE CreateThread(Runnable* runnable,
                             Notification* thread_can_start) {
    ThreadMainParam* param = new ThreadMainParam(runnable, thread_can_start);
    DWORD thread_id;
//Todo（Yukawa）：考虑改用_Beginthreadex。
    HANDLE thread_handle = ::CreateThread(
NULL,    //默认安全性。
0,       //默认堆栈大小。
        &ThreadWithParamSupport::ThreadMain,
param,   //threadmainstatic的参数
0x0,     //默认创建标志。
&thread_id);  //需要有效指针才能使调用在Win98下工作。
    GTEST_CHECK_(thread_handle != NULL) << "CreateThread failed with error "
                                        << ::GetLastError() << ".";
    if (thread_handle == NULL) {
      delete param;
    }
    return thread_handle;
  }

 private:
  struct ThreadMainParam {
    ThreadMainParam(Runnable* runnable, Notification* thread_can_start)
        : runnable_(runnable),
          thread_can_start_(thread_can_start) {
    }
    scoped_ptr<Runnable> runnable_;
//不拥有。
    Notification* thread_can_start_;
  };

  static DWORD WINAPI ThreadMain(void* ptr) {
//转让所有权。
    scoped_ptr<ThreadMainParam> param(static_cast<ThreadMainParam*>(ptr));
    if (param->thread_can_start_ != NULL)
      param->thread_can_start_->WaitForNotification();
    param->runnable_->Run();
    return 0;
  }

//禁止实例化。
  ThreadWithParamSupport();

  GTEST_DISALLOW_COPY_AND_ASSIGN_(ThreadWithParamSupport);
};

}  //命名空间

ThreadWithParamBase::ThreadWithParamBase(Runnable *runnable,
                                         Notification* thread_can_start)
      : thread_(ThreadWithParamSupport::CreateThread(runnable,
                                                     thread_can_start)) {
}

ThreadWithParamBase::~ThreadWithParamBase() {
  Join();
}

void ThreadWithParamBase::Join() {
  GTEST_CHECK_(::WaitForSingleObject(thread_.Get(), INFINITE) == WAIT_OBJECT_0)
      << "Failed to join the thread with error " << ::GetLastError() << ".";
}

//将线程映射到一组具有值的threadidtothreadLocals
//在该线程上实例化，并在线程退出时通知它们。一
//threadlocal实例应保持到其拥有的所有线程
//上的值已终止。
class ThreadLocalRegistryImpl {
 public:
//将线程本地实例注册为在当前线程上具有值。
//返回一个可用于从其他线程标识线程的值。
  static ThreadLocalValueHolderBase* GetValueOnCurrentThread(
      const ThreadLocalBase* thread_local_instance) {
    DWORD current_thread = ::GetCurrentThreadId();
    MutexLock lock(&mutex_);
    ThreadIdToThreadLocals* const thread_to_thread_locals =
        GetThreadLocalsMapLocked();
    ThreadIdToThreadLocals::iterator thread_local_pos =
        thread_to_thread_locals->find(current_thread);
    if (thread_local_pos == thread_to_thread_locals->end()) {
      thread_local_pos = thread_to_thread_locals->insert(
          std::make_pair(current_thread, ThreadLocalValues())).first;
      StartWatcherThreadFor(current_thread);
    }
    ThreadLocalValues& thread_local_values = thread_local_pos->second;
    ThreadLocalValues::iterator value_pos =
        thread_local_values.find(thread_local_instance);
    if (value_pos == thread_local_values.end()) {
      value_pos =
          thread_local_values
              .insert(std::make_pair(
                  thread_local_instance,
                  linked_ptr<ThreadLocalValueHolderBase>(
                      thread_local_instance->NewValueForCurrentThread())))
              .first;
    }
    return value_pos->second.get();
  }

  static void OnThreadLocalDestroyed(
      const ThreadLocalBase* thread_local_instance) {
    std::vector<linked_ptr<ThreadLocalValueHolderBase> > value_holders;
//在保持锁的同时清除threadLocalValues数据结构，但是
//推迟销毁ThreadLocalValueHolderBase。
    {
      MutexLock lock(&mutex_);
      ThreadIdToThreadLocals* const thread_to_thread_locals =
          GetThreadLocalsMapLocked();
      for (ThreadIdToThreadLocals::iterator it =
          thread_to_thread_locals->begin();
          it != thread_to_thread_locals->end();
          ++it) {
        ThreadLocalValues& thread_local_values = it->second;
        ThreadLocalValues::iterator value_pos =
            thread_local_values.find(thread_local_instance);
        if (value_pos != thread_local_values.end()) {
          value_holders.push_back(value_pos->second);
          thread_local_values.erase(value_pos);
//这个“如果”最多只能成功一次，所以理论上我们
//可以在这里跳出循环，但我们不费心这么做。
        }
      }
    }
//在锁外部，让“value-holders”的析构函数释放
//线程本地值保持器状态。
  }

  static void OnThreadExit(DWORD thread_id) {
    GTEST_CHECK_(thread_id != 0) << ::GetLastError();
    std::vector<linked_ptr<ThreadLocalValueHolderBase> > value_holders;
//在保持
//锁定，但推迟销毁threadLocalValueHolderBase。
    {
      MutexLock lock(&mutex_);
      ThreadIdToThreadLocals* const thread_to_thread_locals =
          GetThreadLocalsMapLocked();
      ThreadIdToThreadLocals::iterator thread_local_pos =
          thread_to_thread_locals->find(thread_id);
      if (thread_local_pos != thread_to_thread_locals->end()) {
        ThreadLocalValues& thread_local_values = thread_local_pos->second;
        for (ThreadLocalValues::iterator value_pos =
            thread_local_values.begin();
            value_pos != thread_local_values.end();
            ++value_pos) {
          value_holders.push_back(value_pos->second);
        }
        thread_to_thread_locals->erase(thread_local_pos);
      }
    }
//在锁外部，让“value-holders”的析构函数释放
//线程本地值保持器状态。
  }

 private:
//在特定线程中，将ThreadLocal对象映射到其值。
  typedef std::map<const ThreadLocalBase*,
                   linked_ptr<ThreadLocalValueHolderBase> > ThreadLocalValues;
//将所有threaddotthreadlocals存储到线程中具有值的线程中，索引方式为
//线程的ID。
  typedef std::map<DWORD, ThreadLocalValues> ThreadIdToThreadLocals;

//保留我们传递的线程ID和线程句柄
//StartWatcherThreadfor到WatcherThreadFunc。
  typedef std::pair<DWORD, HANDLE> ThreadIdAndHandle;

  static void StartWatcherThreadFor(DWORD thread_id) {
//返回的句柄将保留在线程映射中，并由
//WatcherThreadFunc中的Watcher_线程。
    HANDLE thread = ::OpenThread(SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                                 FALSE,
                                 thread_id);
    GTEST_CHECK_(thread != NULL);
//我们需要将有效的线程ID指针传递给它的createThread
//在Win98下正常工作。
    DWORD watcher_thread_id;
    HANDLE watcher_thread = ::CreateThread(
NULL,   //默认安全性。
0,      //默认堆栈大小
        &ThreadLocalRegistryImpl::WatcherThreadFunc,
        reinterpret_cast<LPVOID>(new ThreadIdAndHandle(thread_id, thread)),
        CREATE_SUSPENDED,
        &watcher_thread_id);
    GTEST_CHECK_(watcher_thread != NULL);
//为观察线程赋予与我们相同的优先级，以避免
//被它挡住了。
    ::SetThreadPriority(watcher_thread,
                        ::GetThreadPriority(::GetCurrentThread()));
    ::ResumeThread(watcher_thread);
    ::CloseHandle(watcher_thread);
  }

//监视从给定线程退出并通知这些线程
//有关线程终止的threaddothreadLocals。
  static DWORD WINAPI WatcherThreadFunc(LPVOID param) {
    const ThreadIdAndHandle* tah =
        reinterpret_cast<const ThreadIdAndHandle*>(param);
    GTEST_CHECK_(
        ::WaitForSingleObject(tah->second, INFINITE) == WAIT_OBJECT_0);
    OnThreadExit(tah->first);
    ::CloseHandle(tah->second);
    delete tah;
    return 0;
  }

//返回线程本地实例的映射。
  static ThreadIdToThreadLocals* GetThreadLocalsMapLocked() {
    mutex_.AssertHeld();
    static ThreadIdToThreadLocals* map = new ThreadIdToThreadLocals;
    return map;
  }

//保护对GetThreadLocalsMapLocked（）及其返回值的访问。
  static Mutex mutex_;
//保护对getthreadmaplocked（）及其返回值的访问。
  static Mutex thread_map_mutex_;
};

Mutex ThreadLocalRegistryImpl::mutex_(Mutex::kStaticMutex);
Mutex ThreadLocalRegistryImpl::thread_map_mutex_(Mutex::kStaticMutex);

ThreadLocalValueHolderBase* ThreadLocalRegistry::GetValueOnCurrentThread(
      const ThreadLocalBase* thread_local_instance) {
  return ThreadLocalRegistryImpl::GetValueOnCurrentThread(
      thread_local_instance);
}

void ThreadLocalRegistry::OnThreadLocalDestroyed(
      const ThreadLocalBase* thread_local_instance) {
  ThreadLocalRegistryImpl::OnThreadLocalDestroyed(thread_local_instance);
}

#endif  //gtest_是线程安全和gtest_OS_Windows

#if GTEST_USES_POSIX_RE

//执行RE。目前只需要做死亡测试。

RE::~RE() {
  if (is_valid_) {
//重新生成无效的regex可能会崩溃，因为内容
//regex的未定义。因为正则表达式本质上是
//同样，一个不能有效（或无效）没有另一个
//也是如此。
    regfree(&partial_regex_);
    regfree(&full_regex_);
  }
  free(const_cast<char*>(pattern_));
}

//返回与整个str重新匹配的真iff正则表达式。
bool RE::FullMatch(const char* str, const RE& re) {
  if (!re.is_valid_) return false;

  regmatch_t match;
  return regexec(&re.full_regex_, str, 1, &match, 0) == 0;
}

//返回true iff正则表达式，重新匹配str的子字符串
//（包括str本身）。
bool RE::PartialMatch(const char* str, const RE& re) {
  if (!re.is_valid_) return false;

  regmatch_t match;
  return regexec(&re.partial_regex_, str, 1, &match, 0) == 0;
}

//从其字符串表示形式初始化RE。
void RE::Init(const char* regex) {
  pattern_ = posix::StrDup(regex);

//保留足够的字节以保存用于
//完全匹配。
  const size_t full_regex_len = strlen(regex) + 10;
  char* const full_pattern = new char[full_regex_len];

  snprintf(full_pattern, full_regex_len, "^(%s)$", regex);
  is_valid_ = regcomp(&full_regex_, full_pattern, REG_EXTENDED) == 0;
//我们要调用regcomp（&partial_regex_…），即使
//上一个表达式返回false。否则，部分“regex”可能
//如果未正确初始化，可能会在
//释放。
//
//posix regex的一些实现（例如至少在
//cygwin的版本）不接受空字符串作为有效字符串
//正则表达式为了安全起见，我们将其改为等效形式“（）”。
  if (is_valid_) {
    const char* const partial_regex = (*regex == '\0') ? "()" : regex;
    is_valid_ = regcomp(&partial_regex_, partial_regex, REG_EXTENDED) == 0;
  }
  EXPECT_TRUE(is_valid_)
      << "Regular expression \"" << regex
      << "\" is not a valid POSIX Extended regular expression.";

  delete[] full_pattern;
}

#elif GTEST_USES_SIMPLE_RE

//返回str中任何地方出现的真正iff ch（不包括
//正在终止'\0'字符）。
bool IsInSet(char ch, const char* str) {
  return ch != '\0' && strchr(str, ch) != NULL;
}

//返回属于给定分类的真iff ch。不像
//<ctype.h>中的类似函数，这些函数不受
//当前区域设置。
bool IsAsciiDigit(char ch) { return '0' <= ch && ch <= '9'; }
bool IsAsciiPunct(char ch) {
  return IsInSet(ch, "^-!\"#$%&'()*+,./:;<=>?@[\\]_`{|}~");
}
bool IsRepeat(char ch) { return IsInSet(ch, "?*+"); }
bool IsAsciiWhiteSpace(char ch) { return IsInSet(ch, " \f\n\r\t\v"); }
bool IsAsciiWordChar(char ch) {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') ||
      ('0' <= ch && ch <= '9') || ch == '_';
}

//返回true iff“\\c”是受支持的转义序列。
bool IsValidEscape(char c) {
  return (IsAsciiPunct(c) || IsInSet(c, "dDfnrsStvwW"));
}

//返回给定原子的true iff（由转义和模式指定）
//匹配ch。如果原子无效，则结果未定义。
bool AtomMatchesChar(bool escaped, char pattern_char, char ch) {
if (escaped) {  //“\\p”，其中p是图案\u字符。
    switch (pattern_char) {
      case 'd': return IsAsciiDigit(ch);
      case 'D': return !IsAsciiDigit(ch);
      case 'f': return ch == '\f';
      case 'n': return ch == '\n';
      case 'r': return ch == '\r';
      case 's': return IsAsciiWhiteSpace(ch);
      case 'S': return !IsAsciiWhiteSpace(ch);
      case 't': return ch == '\t';
      case 'v': return ch == '\v';
      case 'w': return IsAsciiWordChar(ch);
      case 'W': return !IsAsciiWordChar(ch);
    }
    return IsAsciiPunct(pattern_char) && pattern_char == ch;
  }

  return (pattern_char == '.' && ch != '\n') || pattern_char == ch;
}

//validateregex（）用于格式化错误消息的helper函数。
std::string FormatRegexSyntaxError(const char* regex, int index) {
  return (Message() << "Syntax error at index " << index
          << " in simple regular expression \"" << regex << "\": ").GetString();
}

//生成非致命错误，如果regex无效，则返回false；
//否则返回true。
bool ValidateRegex(const char* regex) {
  if (regex == NULL) {
//TODO（wan@google.com）：修复源文件在
//断言失败，无法匹配在用户中使用regex的位置
//代码。
    ADD_FAILURE() << "NULL is not a valid simple regular expression.";
    return false;
  }

  bool is_valid = true;

//真的IFF？、*或+可以跟在前面的原子后面。
  bool prev_repeatable = false;
  for (int i = 0; regex[i]; i++) {
if (regex[i] == '\\') {  //转义序列
      i++;
      if (regex[i] == '\0') {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i - 1)
                      << "'\\' cannot appear at the end.";
        return false;
      }

      if (!IsValidEscape(regex[i])) {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i - 1)
                      << "invalid escape sequence \"\\" << regex[i] << "\".";
        is_valid = false;
      }
      prev_repeatable = true;
} else {  //不是转义序列。
      const char ch = regex[i];

      if (ch == '^' && i > 0) {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i)
                      << "'^' can only appear at the beginning.";
        is_valid = false;
      } else if (ch == '$' && regex[i + 1] != '\0') {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i)
                      << "'$' can only appear at the end.";
        is_valid = false;
      } else if (IsInSet(ch, "()[]{}|")) {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i)
                      << "'" << ch << "' is unsupported.";
        is_valid = false;
      } else if (IsRepeat(ch) && !prev_repeatable) {
        ADD_FAILURE() << FormatRegexSyntaxError(regex, i)
                      << "'" << ch << "' can only follow a repeatable token.";
        is_valid = false;
      }

      prev_repeatable = !IsInSet(ch, "^$?*+");
    }
  }

  return is_valid;
}

//匹配重复的regex原子，后跟有效的简单正则
//表达式。如果转义为false，则将regex原子定义为c，
//否则。重复是重复元字符（？，*
//或+）。如果str包含太多，则行为未定义
//要按大小索引的字符，在这种情况下，测试将
//不管怎样，可能会超时。我们可以接受这样的限制
//std:：string也有。
bool MatchRepetitionAndRegexAtHead(
    bool escaped, char c, char repeat, const char* regex,
    const char* str) {
  const size_t min_count = (repeat == '+') ? 1 : 0;
  const size_t max_count = (repeat == '?') ? 1 :
      static_cast<size_t>(-1) - 1;
//我们不能调用numeric_limits:：max（），因为它与
//窗口上的max（）宏。

  for (size_t i = 0; i <= max_count; ++i) {
//我们知道原子匹配str中的前i个字符。
    if (i >= min_count && MatchRegexAtHead(regex, str + i)) {
//我们头上有足够的火柴，尾巴也有。
//因为我们只关心*模式是否匹配str
//（与*如何*匹配相反），无需查找
//贪婪的匹配。
      return true;
    }
    if (str[i] == '\0' || !AtomMatchesChar(escaped, c, str[i]))
      return false;
  }
  return false;
}

//返回与str前缀匹配的true iff regex。regex必须是
//有效的简单正则表达式，不能以“^”开头，或
//结果未定义。
bool MatchRegexAtHead(const char* regex, const char* str) {
if (*regex == '\0')  //空的regex匹配任何前缀。
    return true;

//“$”只匹配字符串的结尾。注意，regex是
//有效保证在“$”之后没有任何内容。
  if (*regex == '$')
    return *str == '\0';

//regex中的第一件事是转义序列吗？
  const bool escaped = *regex == '\\';
  if (escaped)
    ++regex;
  if (IsRepeat(regex[1])) {
//matchrepetitionandregexathead（）调用matchRegexad（），因此
//这是一个间接递归。当正则表达式
//在每个递归中更短。
    return MatchRepetitionAndRegexAtHead(
        escaped, regex[0], regex[1], regex + 2, str);
  } else {
//regex不是空的，不是“$”，并且不是以
//重复。我们将regex的第一个原子与第一个匹配
//str和recurse的特征。
    return (*str != '\0') && AtomMatchesChar(escaped, *regex, *str) &&
        MatchRegexAtHead(regex + 1, str + 1);
  }
}

//返回true iff regex匹配str的任何子字符串。regex必须
//有效的简单正则表达式，或未定义结果。
//
//算法是递归的，但递归深度不超过
//Regex的长度，所以我们不需要担心用完
//堆栈空间正常。在极少数情况下，时间复杂性可以是
//关于regex长度的指数+字符串长度，
//但通常它必须更快（通常接近线性）。
bool MatchRegexAnywhere(const char* regex, const char* str) {
  if (regex == NULL || str == NULL)
    return false;

  if (*regex == '^')
    return MatchRegexAtHead(regex + 1, str);

//一个成功的匹配可以在str的任何地方。
  do {
    if (MatchRegexAtHead(regex, str))
      return true;
  } while (*str++ != '\0');
  return false;
}

//实现re类。

RE::~RE() {
  free(const_cast<char*>(pattern_));
  free(const_cast<char*>(full_pattern_));
}

//返回与整个str重新匹配的真iff正则表达式。
bool RE::FullMatch(const char* str, const RE& re) {
  return re.is_valid_ && MatchRegexAnywhere(re.full_pattern_, str);
}

//返回true iff正则表达式，重新匹配str的子字符串
//（包括str本身）。
bool RE::PartialMatch(const char* str, const RE& re) {
  return re.is_valid_ && MatchRegexAnywhere(re.pattern_, str);
}

//从其字符串表示形式初始化RE。
void RE::Init(const char* regex) {
  pattern_ = full_pattern_ = NULL;
  if (regex != NULL) {
    pattern_ = posix::StrDup(regex);
  }

  is_valid_ = ValidateRegex(regex);
  if (!is_valid_) {
//当regex无效时，不需要计算完整模式。
    return;
  }

  const size_t len = strlen(regex);
//保留足够的字节以保存用于
//完全匹配：我们需要空间来预加“^”，追加“$”，以及
//用'\0'终止字符串。
  char* buffer = static_cast<char*>(malloc(len + 3));
  full_pattern_ = buffer;

  if (*regex != '^')
*buffer++ = '^';  //确保完整的“模式”以“^”开头。

//我们不使用snprintf或strncpy，因为它们在
//用VC++8.0编译。
  memcpy(buffer, regex, len);
  buffer += len;

  if (len == 0 || regex[len - 1] != '$')
*buffer++ = '$';  //确保完整的_模式以“$”结尾。

  *buffer = '\0';
}

#endif  //gtest使用posix

const char kUnknownFile[] = "unknown file";

//格式化源文件路径和行号
//在编译此代码的编译器发出的错误消息中。
GTEST_API_ ::std::string FormatFileLocation(const char* file, int line) {
  const std::string file_name(file == NULL ? kUnknownFile : file);

  if (line < 0) {
    return file_name + ":";
  }
#ifdef _MSC_VER
  return file_name + "(" + StreamableToString(line) + "):";
#else
  return file_name + ":" + StreamableToString(line) + ":";
#endif  //三桅纵帆船
}

//为与编译器无关的XML输出格式化文件位置。
//尽管此函数不依赖于平台，但我们将其放在
//格式化文件位置以对比这两个函数。
//请注意，FormatCompilerIndependentFileLocation（）不追加冒号
//不同于formatfilelocation（）。
GTEST_API_ ::std::string FormatCompilerIndependentFileLocation(
    const char* file, int line) {
  const std::string file_name(file == NULL ? kUnknownFile : file);

  if (line < 0)
    return file_name;
  else
    return file_name + ":" + StreamableToString(line);
}


GTestLog::GTestLog(GTestLogSeverity severity, const char* file, int line)
    : severity_(severity) {
  const char* const marker =
      severity == GTEST_INFO ?    "[  INFO ]" :
      severity == GTEST_WARNING ? "[WARNING]" :
      severity == GTEST_ERROR ?   "[ ERROR ]" : "[ FATAL ]";
  GetStream() << ::std::endl << marker << " "
              << FormatFileLocation(file, line).c_str() << ": ";
}

//刷新缓冲区，如果严重性为gtest_致命，则中止程序。
GTestLog::~GTestLog() {
  GetStream() << ::std::endl;
  if (severity_ == GTEST_FATAL) {
    fflush(stderr);
    posix::Abort();
  }
}
//对从调用的POSIX函数禁用Microsoft拒绝警告
//此类（创建、复制、复制和关闭）
GTEST_DISABLE_MSC_WARNINGS_PUSH_(4996)

#if GTEST_HAS_STREAM_REDIRECTION

//捕获输出流的对象（stdout/stderr）。
class CapturedStream {
 public:
//ctor将流重定向到临时文件。
  explicit CapturedStream(int fd) : fd_(fd), uncaptured_fd_(dup(fd)) {
# if GTEST_OS_WINDOWS
char temp_dir_path[MAX_PATH + 1] = { '\0' };  //诺林
char temp_file_path[MAX_PATH + 1] = { '\0' };  //诺林

    ::GetTempPathA(sizeof(temp_dir_path), temp_dir_path);
    const UINT success = ::GetTempFileNameA(temp_dir_path,
                                            "gtest_redir",
0,  //生成唯一的文件名。
                                            temp_file_path);
    GTEST_CHECK_(success != 0)
        << "Unable to create a temporary file in " << temp_dir_path;
    const int captured_fd = creat(temp_file_path, _S_IREAD | _S_IWRITE);
    GTEST_CHECK_(captured_fd != -1) << "Unable to open temporary file "
                                    << temp_file_path;
    filename_ = temp_file_path;
# else
//无法保证测试具有对当前
//目录，因此我们在/tmp目录中创建临时文件
//相反。我们在大多数系统上使用/tmp，在Android上使用/sdcard。
//这是因为Android没有/tmp。
#  if GTEST_OS_LINUX_ANDROID
//注：Android应用程序需要调用框架的
//context.getExternalStorageDirectory（）方法，通过jni获取
//世界可写SD卡目录的位置。然而，
//这需要一个上下文句柄，无法检索该句柄
//从本机代码全局获取。这样做也排除了运行
//作为常规独立可执行文件的一部分的代码，而不是
//在dalvik进程中运行（例如，通过“adb shell”运行时）。
//
//位置/SD卡可从本机代码直接访问
//并且是Android唯一（非官方）支持的位置
//团队。它通常是到真实SD卡安装点的符号链接
//可以是/mnt/sdcard、/mnt/sdcard0、/system/media/sdcard，或者
//其他OEM定制地点。永远不要依赖这些
//使用/SD卡。
    char name_template[] = "/sdcard/gtest_captured_stream.XXXXXX";
#  else
    char name_template[] = "/tmp/captured_stream.XXXXXX";
#  endif  //GTEST操作系统Linux安卓
    const int captured_fd = mkstemp(name_template);
    filename_ = name_template;
# endif  //GTEST操作系统窗口
    fflush(NULL);
    dup2(captured_fd, fd_);
    close(captured_fd);
  }

  ~CapturedStream() {
    remove(filename_.c_str());
  }

  std::string GetCapturedString() {
    if (uncaptured_fd_ != -1) {
//恢复原始流。
      fflush(NULL);
      dup2(uncaptured_fd_, fd_);
      close(uncaptured_fd_);
      uncaptured_fd_ = -1;
    }

    FILE* const file = posix::FOpen(filename_.c_str(), "r");
    const std::string content = ReadEntireFile(file);
    posix::FClose(file);
    return content;
  }

 private:
//以std：：字符串形式读取文件的全部内容。
  static std::string ReadEntireFile(FILE* file);

//返回文件的大小（字节）。
  static size_t GetFileSize(FILE* file);

const int fd_;  //要捕获的流。
  int uncaptured_fd_;
//保存stderr输出的临时文件的名称。
  ::std::string filename_;

  GTEST_DISALLOW_COPY_AND_ASSIGN_(CapturedStream);
};

//返回文件的大小（字节）。
size_t CapturedStream::GetFileSize(FILE* file) {
  fseek(file, 0, SEEK_END);
  return static_cast<size_t>(ftell(file));
}

//以字符串形式读取文件的全部内容。
std::string CapturedStream::ReadEntireFile(FILE* file) {
  const size_t file_size = GetFileSize(file);
  char* const buffer = new char[file_size];

size_t bytes_last_read = 0;  //最后一个fread（）中读取的字节数
size_t bytes_read = 0;       //目前读取的字节数

  fseek(file, 0, SEEK_SET);

//继续读取文件，直到无法进一步读取或
//达到预先确定的文件大小。
  do {
    bytes_last_read = fread(buffer+bytes_read, 1, file_size-bytes_read, file);
    bytes_read += bytes_last_read;
  } while (bytes_last_read > 0 && bytes_read < file_size);

  const std::string content(buffer, bytes_read);
  delete[] buffer;

  return content;
}

GTEST_DISABLE_MSC_WARNINGS_POP_()

static CapturedStream* g_captured_stderr = NULL;
static CapturedStream* g_captured_stdout = NULL;

//开始捕获输出流（stdout/stderr）。
void CaptureStream(int fd, const char* stream_name, CapturedStream** stream) {
  if (*stream != NULL) {
    GTEST_LOG_(FATAL) << "Only one " << stream_name
                      << " capturer can exist at a time.";
  }
  *stream = new CapturedStream(fd);
}

//停止捕获输出流并返回捕获的字符串。
std::string GetCapturedStream(CapturedStream** captured_stream) {
  const std::string content = (*captured_stream)->GetCapturedString();

  delete *captured_stream;
  *captured_stream = NULL;

  return content;
}

//开始捕获stdout。
void CaptureStdout() {
  CaptureStream(kStdOutFileno, "stdout", &g_captured_stdout);
}

//开始捕获stderr。
void CaptureStderr() {
  CaptureStream(kStdErrFileno, "stderr", &g_captured_stderr);
}

//停止捕获stdout并返回捕获的字符串。
std::string GetCapturedStdout() {
  return GetCapturedStream(&g_captured_stdout);
}

//停止捕获stderr并返回捕获的字符串。
std::string GetCapturedStderr() {
  return GetCapturedStream(&g_captured_stderr);
}

#endif  //gtest_具有流重定向

#if GTEST_HAS_DEATH_TEST

//所有命令行参数的副本。由initGoogleTest（）设置。
::std::vector<testing::internal::string> g_argvs;

static const ::std::vector<testing::internal::string>* g_injected_test_argvs =
NULL;  //拥有。

void SetInjectableArgvs(const ::std::vector<testing::internal::string>* argvs) {
  if (g_injected_test_argvs != argvs)
    delete g_injected_test_argvs;
  g_injected_test_argvs = argvs;
}

const ::std::vector<testing::internal::string>& GetInjectableArgvs() {
  if (g_injected_test_argvs != NULL) {
    return *g_injected_test_argvs;
  }
  return g_argvs;
}
#endif  //GTEST有死亡测试

#if GTEST_OS_WINDOWS_MOBILE
namespace posix {
void Abort() {
  DebugBreak();
  TerminateProcess(GetCurrentProcess(), 1);
}
}  //命名空间POSIX
#endif  //GTEST操作系统Windows移动

//返回与
//给定标志。例如，flagtoenvar（“foo”）将返回
//开源版本中的“gtest foo”。
static std::string FlagToEnvVar(const char* flag) {
  const std::string full_flag =
      (Message() << GTEST_FLAG_PREFIX_ << flag).GetString();

  Message env_var;
  for (size_t i = 0; i != full_flag.length(); i++) {
    env_var << ToUpper(full_flag.c_str()[i]);
  }

  return env_var.GetString();
}

//分析32位有符号整数的“str”。如果成功，则写入
//结果为*value并返回true；否则返回*value
//不变并返回false。
bool ParseInt32(const Message& src_text, const char* str, Int32* value) {
//将环境变量解析为十进制整数。
  char* end = NULL;
const long long_value = strtol(str, &end, 10);  //诺林

//strtol（）是否已使用字符串中的所有字符？
  if (*end != '\0') {
//否-遇到无效字符。
    Message msg;
    msg << "WARNING: " << src_text
        << " is expected to be a 32-bit integer, but actually"
        << " has value \"" << str << "\".\n";
    printf("%s", msg.GetString().c_str());
    fflush(stdout);
    return false;
  }

//分析的值是否在Int32的范围内？
  const Int32 result = static_cast<Int32>(long_value);
  if (long_value == LONG_MAX || long_value == LONG_MIN ||
//解析的值溢出为长。（strtol（）返回
//当输入溢出时，long_max或long_min。）
      result != long_value
//解析的值作为Int32溢出。
      ) {
    Message msg;
    msg << "WARNING: " << src_text
        << " is expected to be a 32-bit integer, but actually"
        << " has value " << str << ", which overflows.\n";
    printf("%s", msg.GetString().c_str());
    fflush(stdout);
    return false;
  }

  *value = result;
  return true;
}

//读取并返回对应于
//给定的标志；如果未设置，则返回默认值。
//
//如果不是“0”，则该值被视为真。
bool BoolFromGTestEnv(const char* flag, bool default_value) {
  const std::string env_var = FlagToEnvVar(flag);
  const char* const string_value = posix::GetEnv(env_var.c_str());
  return string_value == NULL ?
      default_value : strcmp(string_value, "0") != 0;
}

//读取并返回存储在环境中的32位整数
//与给定标志对应的变量；如果未设置或
//不表示有效的32位整数，返回默认值。
Int32 Int32FromGTestEnv(const char* flag, Int32 default_value) {
  const std::string env_var = FlagToEnvVar(flag);
  const char* const string_value = posix::GetEnv(env_var.c_str());
  if (string_value == NULL) {
//未设置环境变量。
    return default_value;
  }

  Int32 result = default_value;
  if (!ParseInt32(Message() << "Environment variable " << env_var,
                  string_value, &result)) {
    printf("The default value %s is used.\n",
           (Message() << default_value).GetString().c_str());
    fflush(stdout);
    return default_value;
  }

  return result;
}

//读取并返回与
//给定的标志；如果未设置，则返回默认值。
const char* StringFromGTestEnv(const char* flag, const char* default_value) {
  const std::string env_var = FlagToEnvVar(flag);
  const char* const value = posix::GetEnv(env_var.c_str());
  return value == NULL ? default_value : value;
}

}  //命名空间内部
}  //命名空间测试
//版权所有2007，Google Inc.
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

//谷歌测试-谷歌C++测试框架
//
//此文件实现了一个通用值打印机，可以打印
//任何类型t的值：
//
//void:：testing:：internal:：universalprinter<t>：：print（value，ostream_ptr）；
//
//如果可能，它使用<<运算符，并打印
//否则拒绝。用户可以重写类的行为
//通过定义运算符<（：：std:：ostream&，const foo&）键入foo
//或在命名空间中void printto（const foo&，：：std：：ostream*）
//定义FoO。

#include <ctype.h>
#include <stdio.h>
#include <cwchar>
#include <ostream>  //诺林
#include <string>

namespace testing {

namespace {

using ::std::ostream;

//打印给定对象中的一段字节。
GTEST_ATTRIBUTE_NO_SANITIZE_MEMORY_
GTEST_ATTRIBUTE_NO_SANITIZE_ADDRESS_
GTEST_ATTRIBUTE_NO_SANITIZE_THREAD_
void PrintByteSegmentInObjectTo(const unsigned char* obj_bytes, size_t start,
                                size_t count, ostream* os) {
  char text[5] = "";
  for (size_t i = 0; i != count; i++) {
    const size_t j = start + i;
    if (i != 0) {
//将字节组织为2组，以便按
//人类。
      if ((j % 2) == 0)
        *os << ' ';
      else
        *os << '-';
    }
    GTEST_SNPRINTF_(text, sizeof(text), "%02X", obj_bytes[j]);
    *os << text;
  }
}

//将给定值中的字节打印到给定的Ostream。
void PrintBytesInObjectToImpl(const unsigned char* obj_bytes, size_t count,
                              ostream* os) {
//告诉用户对象有多大。
  *os << count << "-byte object <";

  const size_t kThreshold = 132;
  const size_t kChunkSize = 64;
//如果对象大小大于kthreshold，则必须省略
//只打印第一个和最后一个kChunkSize的一些详细信息
//字节。
//TODO（WAN）：让用户使用标志控制阈值。
  if (count < kThreshold) {
    PrintByteSegmentInObjectTo(obj_bytes, 0, count, os);
  } else {
    PrintByteSegmentInObjectTo(obj_bytes, 0, kChunkSize, os);
    *os << " ... ";
//四舍五入到2字节边界。
    const size_t resume_pos = (count - kChunkSize + 1)/2*2;
    PrintByteSegmentInObjectTo(obj_bytes, resume_pos, count - resume_pos, os);
  }
  *os << ">";
}

}  //命名空间

namespace internal2 {

//委托到printBytesInbjectToImpl（）以打印
//给定的对象。该代表团简化了实施过程，从而
//使用<<运算符，因此在
//：：测试：：内部命名空间，其中包含一个<<运算符，
//有时与STL中的冲突。
void PrintBytesInObjectTo(const unsigned char* obj_bytes, size_t count,
                          ostream* os) {
  PrintBytesInObjectToImpl(obj_bytes, count, os);
}

}  //命名空间内部2

namespace internal {

//根据字符（或wchar_t）的值，我们将它打印在一个字符中
//三种格式：
//-如果它是可打印的ASCII（例如“A”、“2”、“”），则为，
//-作为十六进制转义序列（例如“x7f”），或
//—作为特殊的转义序列（例如“r”、“n”）。
enum CharFormat {
  kAsIs,
  kHexEscape,
  kSpecialEscape
};

//如果c是可打印的ASCII字符，则返回true。我们测试
//C的值，而不是调用isprint（），它有问题
//Windows Mobile。
inline bool IsPrintableAscii(wchar_t c) {
  return 0x20 <= c && c <= 0x7E;
}

//将宽字符或窄字符C作为字符文本打印，而不使用
//引号，必要时转义；返回C的格式。
//模板参数unsigned char是char的无符号版本，
//这是C的类型。
template <typename UnsignedChar, typename Char>
static CharFormat PrintAsCharLiteralTo(Char c, ostream* os) {
  switch (static_cast<wchar_t>(c)) {
    case L'\0':
      *os << "\\0";
      break;
    case L'\'':
      *os << "\\'";
      break;
    case L'\\':
      *os << "\\\\";
      break;
    case L'\a':
      *os << "\\a";
      break;
    case L'\b':
      *os << "\\b";
      break;
    case L'\f':
      *os << "\\f";
      break;
    case L'\n':
      *os << "\\n";
      break;
    case L'\r':
      *os << "\\r";
      break;
    case L'\t':
      *os << "\\t";
      break;
    case L'\v':
      *os << "\\v";
      break;
    default:
      if (IsPrintableAscii(c)) {
        *os << static_cast<char>(c);
        return kAsIs;
      } else {
        *os << "\\x" + String::FormatHexInt(static_cast<UnsignedChar>(c));
        return kHexEscape;
      }
  }
  return kSpecialEscape;
}

//打印wchar_t c，就像它是字符串文字的一部分，在
//必要；返回C的格式。
static CharFormat PrintAsStringLiteralTo(wchar_t c, ostream* os) {
  switch (c) {
    case L'\'':
      *os << "'";
      return kAsIs;
    case L'"':
      *os << "\\\"";
      return kSpecialEscape;
    default:
      return PrintAsCharLiteralTo<wchar_t>(c, os);
  }
}

//打印一个字符c，就像它是字符串文字的一部分，当
//必要；返回C的格式。
static CharFormat PrintAsStringLiteralTo(char c, ostream* os) {
  return PrintAsStringLiteralTo(
      static_cast<wchar_t>(static_cast<unsigned char>(c)), os);
}

//打印宽字符或窄字符C及其代码。”0’是印刷的
//作为“\\0”，其他无法打印的字符也会正确转义。
//使用标准的C++转义序列。模板参数
//unsigned char是char的无符号版本，它是C类型。
template <typename UnsignedChar, typename Char>
void PrintCharAndCodeTo(Char c, ostream* os) {
//首先，以我们能找到的最可读的形式将C打印为文本。
  *os << ((sizeof(c) > 1) ? "L'" : "'");
  const CharFormat format = PrintAsCharLiteralTo<UnsignedChar>(c, os);
  *os << "'";

//为了帮助用户调试，我们还以十进制打印C代码，除非
//它是0（在这种情况下，C被打印为'\0'，生成代码
//显而易见）
  if (c == 0)
    return;
  *os << " (" << static_cast<int>(c);

//为了更方便起见，我们用十六进制再次打印C代码，
//除非C已经以\x的形式打印，或者代码在
//〔1, 9〕。
  if (format == kHexEscape || (1 <= c && c <= 9)) {
//什么也不做。
  } else {
    *os << ", 0x" << String::FormatHexInt(static_cast<UnsignedChar>(c));
  }
  *os << ")";
}

void PrintTo(unsigned char c, ::std::ostream* os) {
  PrintCharAndCodeTo<unsigned char>(c, os);
}
void PrintTo(signed char c, ::std::ostream* os) {
  PrintCharAndCodeTo<unsigned char>(c, os);
}

//将wchar打印为符号（如果可以打印或作为内部符号）
//以其他方式编码，也作为其代码。L'\0'打印为“L'\0''。
void PrintTo(wchar_t wc, ostream* os) {
  PrintCharAndCodeTo<wchar_t>(wc, os);
}

//将给定的字符数组打印到Ostream。CharType必须是
//char或wchar_t。
//数组从开始处开始，长度为len，可以包含'\0'字符
//不得终止。
template <typename CharType>
GTEST_ATTRIBUTE_NO_SANITIZE_MEMORY_
GTEST_ATTRIBUTE_NO_SANITIZE_ADDRESS_
GTEST_ATTRIBUTE_NO_SANITIZE_THREAD_
static void PrintCharsAsStringTo(
    const CharType* begin, size_t len, ostream* os) {
  const char* const kQuoteBegin = sizeof(CharType) == 1 ? "\"" : "L\"";
  *os << kQuoteBegin;
  bool is_previous_hex = false;
  for (size_t index = 0; index < len; ++index) {
    const CharType cur = begin[index];
    if (is_previous_hex && IsXDigit(cur)) {
//前一个字符是'.x.'形式，此字符可以是
//在其数字中解释为另一个十六进制数字。将字符串断开
//消除歧义。
      *os << "\" " << kQuoteBegin;
    }
    is_previous_hex = PrintAsStringLiteralTo(cur, os) == kHexEscape;
  }
  *os << "\"";
}

//打印“len”元素的（const）char/wchar_t数组，从地址开始
//“开始”。CharType必须是char或wchar_t。
template <typename CharType>
GTEST_ATTRIBUTE_NO_SANITIZE_MEMORY_
GTEST_ATTRIBUTE_NO_SANITIZE_ADDRESS_
GTEST_ATTRIBUTE_NO_SANITIZE_THREAD_
static void UniversalPrintCharArray(
    const CharType* begin, size_t len, ostream* os) {
//代码
//const char kfoo[]=“foo”；
//生成一个由4个元素而不是3个元素组成的数组，最后一个元素是'\0'。
//
//因此，在打印char数组时，如果
//它是'\0'，这样输出与字符串文字匹配
//写在源代码中。
  if (len > 0 && begin[len - 1] == '\0') {
    PrintCharsAsStringTo(begin, len - 1, os);
    return;
  }

//但是，如果数组中的最后一个元素不是'\0'，例如
//常量char kfoo[]='f'、'o'、'o'
//我们必须打印整个数组。我们还打印了一条消息来表明
//数组未被nul终止。
  PrintCharsAsStringTo(begin, len, os);
  *os << " (no terminating NUL)";
}

//打印“len”元素的（const）char数组，从地址“begin”开始。
void UniversalPrintArray(const char* begin, size_t len, ostream* os) {
  UniversalPrintCharArray(begin, len, os);
}

//打印“len”元素的（const）wchar_t数组，从地址开始
//“开始”。
void UniversalPrintArray(const wchar_t* begin, size_t len, ostream* os) {
  UniversalPrintCharArray(begin, len, os);
}

//将给定的C字符串打印到Ostream。
void PrintTo(const char* s, ostream* os) {
  if (s == NULL) {
    *os << "NULL";
  } else {
    *os << ImplicitCast_<const void*>(s) << " pointing to ";
    PrintCharsAsStringTo(s, strlen(s), os);
  }
}

//可以将MSVC编译器配置为将what定义为typedef
//无符号短。在这种情况下为const wchar_t*定义重载
//会导致指向无符号短裤的指针打印为宽字符串，
//可能访问超过预期的内存并导致无效
//内存访问。当
//wchar_t作为本机类型实现。
#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
//将给定的宽C字符串打印到Ostream。
void PrintTo(const wchar_t* s, ostream* os) {
  if (s == NULL) {
    *os << "NULL";
  } else {
    *os << ImplicitCast_<const void*>(s) << " pointing to ";
    PrintCharsAsStringTo(s, std::wcslen(s), os);
  }
}
#endif  //wchar_t是本地的

//打印一个：：字符串对象。
#if GTEST_HAS_GLOBAL_STRING
void PrintStringTo(const ::string& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}
#endif  //gtest_有\u全局\u字符串

void PrintStringTo(const ::std::string& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}

//打印：wstring对象。
#if GTEST_HAS_GLOBAL_WSTRING
void PrintWideStringTo(const ::wstring& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}
#endif  //GTEST拥有全球性的

#if GTEST_HAS_STD_WSTRING
void PrintWideStringTo(const ::std::wstring& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}
#endif  //GTEST_有_-std_-wstring

}  //命名空间内部

}  //命名空间测试
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
//谷歌C++测试框架（谷歌测试）


//指示此翻译单元是Google测试的一部分
//实施。它必须在gtest内部inl.h之前
//包含，否则将出现编译器错误。这个把戏是为了
//防止在
//用户代码。
#define GTEST_IMPLEMENTATION_ 1
#undef GTEST_IMPLEMENTATION_

namespace testing {

using internal::GetUnitTestImpl;

//通过省略堆栈跟踪获取失败消息的摘要
//在里面。
std::string TestPartResult::ExtractSummary(const char* message) {
  const char* const stack_trace = strstr(message, internal::kStackTraceMarker);
  return stack_trace == NULL ? message :
      std::string(message, stack_trace);
}

//打印testpartresult对象。
std::ostream& operator<<(std::ostream& os, const TestPartResult& result) {
  return os
      << result.file_name() << ":" << result.line_number() << ": "
      << (result.type() == TestPartResult::kSuccess ? "Success" :
          result.type() == TestPartResult::kFatalFailure ? "Fatal failure" :
          "Non-fatal failure") << ":\n"
      << result.message() << std::endl;
}

//向数组追加testpartresult。
void TestPartResultArray::Append(const TestPartResult& result) {
  array_.push_back(result);
}

//返回给定索引处的testpartresult（从0开始）。
const TestPartResult& TestPartResultArray::GetTestPartResult(int index) const {
  if (index < 0 || index >= size()) {
    printf("\nInvalid index (%d) into TestPartResultArray.\n", index);
    internal::posix::Abort();
  }

  return array_[index];
}

//返回数组中的testpartresult对象数。
int TestPartResultArray::size() const {
  return static_cast<int>(array_.size());
}

namespace internal {

HasNewFatalFailureHelper::HasNewFatalFailureHelper()
    : has_new_fatal_failure_(false),
      original_reporter_(GetUnitTestImpl()->
                         GetTestPartResultReporterForCurrentThread()) {
  GetUnitTestImpl()->SetTestPartResultReporterForCurrentThread(this);
}

HasNewFatalFailureHelper::~HasNewFatalFailureHelper() {
  GetUnitTestImpl()->SetTestPartResultReporterForCurrentThread(
      original_reporter_);
}

void HasNewFatalFailureHelper::ReportTestPartResult(
    const TestPartResult& result) {
  if (result.fatally_failed())
    has_new_fatal_failure_ = true;
  original_reporter_->ReportTestPartResult(result);
}

}  //命名空间内部

}  //命名空间测试
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


namespace testing {
namespace internal {

#if GTEST_HAS_TYPED_TEST_P

//跳过str中的第一个非空格字符。如果str
//仅包含空白字符。
static const char* SkipSpaces(const char* str) {
  while (IsSpace(*str))
    str++;
  return str;
}

static std::vector<std::string> SplitIntoTestNames(const char* src) {
  std::vector<std::string> name_vec;
  src = SkipSpaces(src);
  for (; src != NULL; src = SkipComma(src)) {
    name_vec.push_back(StripTrailingSpaces(GetPrefixUntilComma(src)));
  }
  return name_vec;
}

//验证已注册的测试是否与中的测试名称匹配
//defined_test_names_uu；如果成功，返回已注册的_test，或者
//否则中止程序。
const char* TypedTestCasePState::VerifyRegisteredTestNames(
    const char* file, int line, const char* registered_tests) {
  typedef ::std::set<const char*>::const_iterator DefinedTestIter;
  registered_ = true;

  std::vector<std::string> name_vec = SplitIntoTestNames(registered_tests);

  Message errors;

  std::set<std::string> tests;
  for (std::vector<std::string>::const_iterator name_it = name_vec.begin();
       name_it != name_vec.end(); ++name_it) {
    const std::string& name = *name_it;
    if (tests.count(name) != 0) {
      errors << "Test " << name << " is listed more than once.\n";
      continue;
    }

    bool found = false;
    for (DefinedTestIter it = defined_test_names_.begin();
         it != defined_test_names_.end();
         ++it) {
      if (name == *it) {
        found = true;
        break;
      }
    }

    if (found) {
      tests.insert(name);
    } else {
      errors << "No test named " << name
             << " can be found in this test case.\n";
    }
  }

  for (DefinedTestIter it = defined_test_names_.begin();
       it != defined_test_names_.end();
       ++it) {
    if (tests.count(*it) == 0) {
      errors << "You forgot to list test " << *it << ".\n";
    }
  }

  const std::string& errors_str = errors.GetString();
  if (errors_str != "") {
    fprintf(stderr, "%s %s", FormatFileLocation(file, line).c_str(),
            errors_str.c_str());
    fflush(stderr);
    posix::Abort();
  }

  return registered_tests;
}

#endif  //GTEST_有_型_测试_p

}  //命名空间内部
}  //命名空间测试

#endif  //_uuu-clang_分析仪
