
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//

#ifndef ROCKSDB_LITE

#include "util/auto_roll_logger.h"
#include <errno.h>
#include <sys/stat.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>
#include "port/port.h"
#include "rocksdb/db.h"
#include "util/logging.h"
#include "util/sync_point.h"
#include "util/testharness.h"

namespace rocksdb {
namespace {
class NoSleepEnv : public EnvWrapper {
 public:
  NoSleepEnv(Env* base) : EnvWrapper(base) {}
  virtual void SleepForMicroseconds(int micros) override {
    fake_time_ += static_cast<uint64_t>(micros);
  }

  virtual uint64_t NowNanos() override { return fake_time_ * 1000; }

  virtual uint64_t NowMicros() override { return fake_time_; }

 private:
  uint64_t fake_time_ = 6666666666;
};
}  //命名空间

class AutoRollLoggerTest : public testing::Test {
 public:
  static void InitTestDb() {
#ifdef OS_WIN
//替换路径中的所有斜线，这样Windows Compaspec就不会
//变得困惑
    std::string testDir(kTestDir);
    std::replace_if(testDir.begin(), testDir.end(),
                    [](char ch) { return ch == '/'; }, '\\');
    std::string deleteCmd = "if exist " + testDir + " rd /s /q " + testDir;
#else
    std::string deleteCmd = "rm -rf " + kTestDir;
#endif
    ASSERT_TRUE(system(deleteCmd.c_str()) == 0);
    Env::Default()->CreateDir(kTestDir);
  }

  void RollLogFileBySizeTest(AutoRollLogger* logger, size_t log_max_size,
                             const std::string& log_message);
  void RollLogFileByTimeTest(Env*, AutoRollLogger* logger, size_t time,
                             const std::string& log_message);

  static const std::string kSampleMessage;
  static const std::string kTestDir;
  static const std::string kLogFile;
  static Env* default_env;
};

const std::string AutoRollLoggerTest::kSampleMessage(
    "this is the message to be written to the log file!!");
const std::string AutoRollLoggerTest::kTestDir(test::TmpDir() + "/db_log_test");
const std::string AutoRollLoggerTest::kLogFile(test::TmpDir() +
                                               "/db_log_test/LOG");
Env* AutoRollLoggerTest::default_env = Env::Default();

//在这个测试中，我们只想记录一些简单的日志消息
//没有格式。logmessage（）提供了如此简单的接口和
//避免在您
//直接呼叫rocks_log_info（logger，log_message）。
namespace {
void LogMessage(Logger* logger, const char* message) {
  ROCKS_LOG_INFO(logger, "%s", message);
}

void LogMessage(const InfoLogLevel log_level, Logger* logger,
                const char* message) {
  Log(log_level, logger, "%s", message);
}
}  //命名空间

void AutoRollLoggerTest::RollLogFileBySizeTest(AutoRollLogger* logger,
                                               size_t log_max_size,
                                               const std::string& log_message) {
  logger->SetInfoLogLevel(InfoLogLevel::INFO_LEVEL);
//测量每个消息的大小，假设
//等于或大于log message.size（）。
  LogMessage(logger, log_message.c_str());
  size_t message_size = logger->GetLogFileSize();
  size_t current_log_size = message_size;

//测试将不滚动日志文件的情况。
  while (current_log_size + message_size < log_max_size) {
    LogMessage(logger, log_message.c_str());
    current_log_size += message_size;
    ASSERT_EQ(current_log_size, logger->GetLogFileSize());
  }

//现在将滚动日志文件
  LogMessage(logger, log_message.c_str());
//由于在实际测井前检查了旋转，因此我们需要
//通过记录另一条消息触发旋转。
  LogMessage(logger, log_message.c_str());

  ASSERT_TRUE(message_size == logger->GetLogFileSize());
}

void AutoRollLoggerTest::RollLogFileByTimeTest(Env* env, AutoRollLogger* logger,
                                               size_t time,
                                               const std::string& log_message) {
  uint64_t expected_ctime;
  uint64_t actual_ctime;

  uint64_t total_log_size;
  EXPECT_OK(env->GetFileSize(kLogFile, &total_log_size));
  expected_ctime = logger->TEST_ctime();
  logger->SetCallNowMicrosEveryNRecords(0);

//--多次写入日志，假定
//在时间之前完成。
  for (int i = 0; i < 10; ++i) {
    env->SleepForMicroseconds(50000);
    LogMessage(logger, log_message.c_str());
    EXPECT_OK(logger->GetStatus());
//确保始终写入同一日志文件（通过
//检查创建时间）；

    actual_ctime = logger->TEST_ctime();

//还要确保日志大小在增加。
    EXPECT_EQ(expected_ctime, actual_ctime);
    EXPECT_GT(logger->GetLogFileSize(), total_log_size);
    total_log_size = logger->GetLogFileSize();
  }

//--使日志文件过期
  env->SleepForMicroseconds(static_cast<int>(time * 1000000));
  LogMessage(logger, log_message.c_str());

//此时，应该创建新的日志文件。
  actual_ctime = logger->TEST_ctime();
  EXPECT_LT(expected_ctime, actual_ctime);
  EXPECT_LT(logger->GetLogFileSize(), total_log_size);
}

TEST_F(AutoRollLoggerTest, RollLogFileBySize) {
    InitTestDb();
    size_t log_max_size = 1024 * 5;

    AutoRollLogger logger(Env::Default(), kTestDir, "", log_max_size, 0);

    RollLogFileBySizeTest(&logger, log_max_size,
                          kSampleMessage + ":RollLogFileBySize");
}

TEST_F(AutoRollLoggerTest, RollLogFileByTime) {
  NoSleepEnv nse(Env::Default());

  size_t time = 2;
  size_t log_size = 1024 * 5;

  InitTestDb();
//--在服务器重新启动期间测试文件是否存在。
  ASSERT_EQ(Status::NotFound(), default_env->FileExists(kLogFile));
  AutoRollLogger logger(&nse, kTestDir, "", log_size, time);
  ASSERT_OK(default_env->FileExists(kLogFile));

  RollLogFileByTimeTest(&nse, &logger, time,
                        kSampleMessage + ":RollLogFileByTime");
}

TEST_F(AutoRollLoggerTest, OpenLogFilesMultipleTimesWithOptionLog_max_size) {
//如果只指定了“日志最大值”选项，则每次
//当rocksdb重新启动时，将创建一个新的空日志文件。
  InitTestDb();
//解决办法：
//避免编者抱怨“签字比较”
//和无符号整数表达式“，因为文本0是
//视为“烧焦”。
  size_t kZero = 0;
  size_t log_size = 1024;

  AutoRollLogger* logger = new AutoRollLogger(
    Env::Default(), kTestDir, "", log_size, 0);

  LogMessage(logger, kSampleMessage.c_str());
  ASSERT_GT(logger->GetLogFileSize(), kZero);
  delete logger;

//重新打开日志文件，将创建一个空日志文件。
  logger = new AutoRollLogger(
    Env::Default(), kTestDir, "", log_size, 0);
  ASSERT_EQ(logger->GetLogFileSize(), kZero);
  delete logger;
}

TEST_F(AutoRollLoggerTest, CompositeRollByTimeAndSizeLogger) {
  size_t time = 2, log_max_size = 1024 * 5;

  InitTestDb();

  NoSleepEnv nse(Env::Default());
  AutoRollLogger logger(&nse, kTestDir, "", log_max_size, time);

//测试按大小滚动的能力
  RollLogFileBySizeTest(&logger, log_max_size,
                        kSampleMessage + ":CompositeRollByTimeAndSizeLogger");

//测试按时间滚动的能力
  RollLogFileByTimeTest(&nse, &logger, time,
                        kSampleMessage + ":CompositeRollByTimeAndSizeLogger");
}

#ifndef OS_WIN
//TODO:不为Windows生成，因为下面使用了posixlogger。需要
//港口
TEST_F(AutoRollLoggerTest, CreateLoggerFromOptions) {
  DBOptions options;
  NoSleepEnv nse(Env::Default());
  shared_ptr<Logger> logger;

//正常记录器
  ASSERT_OK(CreateLoggerFromOptions(kTestDir, options, &logger));
  ASSERT_TRUE(dynamic_cast<PosixLogger*>(logger.get()));

//仅按大小滚动
  InitTestDb();
  options.max_log_file_size = 1024;
  ASSERT_OK(CreateLoggerFromOptions(kTestDir, options, &logger));
  AutoRollLogger* auto_roll_logger =
    dynamic_cast<AutoRollLogger*>(logger.get());
  ASSERT_TRUE(auto_roll_logger);
  RollLogFileBySizeTest(
      auto_roll_logger, options.max_log_file_size,
      kSampleMessage + ":CreateLoggerFromOptions - size");

//只按时间滚动
  options.env = &nse;
  InitTestDb();
  options.max_log_file_size = 0;
  options.log_file_time_to_roll = 2;
  ASSERT_OK(CreateLoggerFromOptions(kTestDir, options, &logger));
  auto_roll_logger =
    dynamic_cast<AutoRollLogger*>(logger.get());
  RollLogFileByTimeTest(&nse, auto_roll_logger, options.log_file_time_to_roll,
                        kSampleMessage + ":CreateLoggerFromOptions - time");

//按时间和大小滚动
  InitTestDb();
  options.max_log_file_size = 1024 * 5;
  options.log_file_time_to_roll = 2;
  ASSERT_OK(CreateLoggerFromOptions(kTestDir, options, &logger));
  auto_roll_logger =
    dynamic_cast<AutoRollLogger*>(logger.get());
  RollLogFileBySizeTest(auto_roll_logger, options.max_log_file_size,
                        kSampleMessage + ":CreateLoggerFromOptions - both");
  RollLogFileByTimeTest(&nse, auto_roll_logger, options.log_file_time_to_roll,
                        kSampleMessage + ":CreateLoggerFromOptions - both");
}

TEST_F(AutoRollLoggerTest, LogFlushWhileRolling) {
  DBOptions options;
  shared_ptr<Logger> logger;

  InitTestDb();
  options.max_log_file_size = 1024 * 5;
  ASSERT_OK(CreateLoggerFromOptions(kTestDir, options, &logger));
  AutoRollLogger* auto_roll_logger =
      dynamic_cast<AutoRollLogger*>(logger.get());
  ASSERT_TRUE(auto_roll_logger);
  rocksdb::port::Thread flush_thread;

//笔记：
//（1）开始滚动前需要固定旧的记录器，因为滚动抓取
//互斥，这将阻止我们访问旧的记录器。这个
//还使用autorolllogger:：flush:pinnedlogger标记flush_线程。
//（2）需要在posiXLogger:：flush（）期间重置记录器以进行比赛
//条件案例，执行与固定（旧）的刷新
//自动侧倾记录器切换到新的记录器后的记录器。
//（3）posiXLogger:：flush（）在两个线程中都发生，但仅在其同步点中发生。
//在Flush_线程（固定旧记录器的线程）中启用。
  rocksdb::SyncPoint::GetInstance()->LoadDependencyAndMarkers(
      {{"AutoRollLogger::Flush:PinnedLogger",
        "AutoRollLoggerTest::LogFlushWhileRolling:PreRollAndPostThreadInit"},
       {"PosixLogger::Flush:Begin1",
        "AutoRollLogger::ResetLogger:BeforeNewLogger"},
       {"AutoRollLogger::ResetLogger:AfterNewLogger",
        "PosixLogger::Flush:Begin2"}},
      {{"AutoRollLogger::Flush:PinnedLogger", "PosixLogger::Flush:Begin1"},
       {"AutoRollLogger::Flush:PinnedLogger", "PosixLogger::Flush:Begin2"}});
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

  flush_thread = port::Thread ([&]() { auto_roll_logger->Flush(); });
  TEST_SYNC_POINT(
      "AutoRollLoggerTest::LogFlushWhileRolling:PreRollAndPostThreadInit");
  RollLogFileBySizeTest(auto_roll_logger, options.max_log_file_size,
                        kSampleMessage + ":LogFlushWhileRolling");
  flush_thread.join();
  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

#endif  //奥斯温

TEST_F(AutoRollLoggerTest, InfoLogLevel) {
  InitTestDb();

  size_t log_size = 8192;
  size_t log_lines = 0;
//一个额外的作用域，用于强制AutoRollLogger在日志文件刷新时
//超出范围。
  {
    AutoRollLogger logger(Env::Default(), kTestDir, "", log_size, 0);
    for (int log_level = InfoLogLevel::HEADER_LEVEL;
         log_level >= InfoLogLevel::DEBUG_LEVEL; log_level--) {
      logger.SetInfoLogLevel((InfoLogLevel)log_level);
      for (int log_type = InfoLogLevel::DEBUG_LEVEL;
           log_type <= InfoLogLevel::HEADER_LEVEL; log_type++) {
//日志级别小于日志级别的日志消息将不会
//记录的。
        LogMessage((InfoLogLevel)log_type, &logger, kSampleMessage.c_str());
      }
      log_lines += InfoLogLevel::HEADER_LEVEL - log_level + 1;
    }
    for (int log_level = InfoLogLevel::HEADER_LEVEL;
         log_level >= InfoLogLevel::DEBUG_LEVEL; log_level--) {
      logger.SetInfoLogLevel((InfoLogLevel)log_level);

//同样，级别小于Log_级别的消息将不会被记录。
      ROCKS_LOG_HEADER(&logger, "%s", kSampleMessage.c_str());
      ROCKS_LOG_DEBUG(&logger, "%s", kSampleMessage.c_str());
      ROCKS_LOG_INFO(&logger, "%s", kSampleMessage.c_str());
      ROCKS_LOG_WARN(&logger, "%s", kSampleMessage.c_str());
      ROCKS_LOG_ERROR(&logger, "%s", kSampleMessage.c_str());
      ROCKS_LOG_FATAL(&logger, "%s", kSampleMessage.c_str());
      log_lines += InfoLogLevel::HEADER_LEVEL - log_level + 1;
    }
  }
  std::ifstream inFile(AutoRollLoggerTest::kLogFile.c_str());
  size_t lines = std::count(std::istreambuf_iterator<char>(inFile),
                         std::istreambuf_iterator<char>(), '\n');
  ASSERT_EQ(log_lines, lines);
  inFile.close();
}

//测试记录器头功能是否有防滚翻日志
//我们期望新的日志作为滚动来承载指定的头文件。
static std::vector<std::string> GetOldFileNames(const std::string& path) {
  std::vector<std::string> ret;

  /*st std：：string dirname=path.substr（/*start=*/0，path.find_last_of（“/”）；
  const std：：string fname=path.substr（path.find_last_of（“/”）+1）；

  std:：vector<std:：string>children；
  env:：default（）->getchildren（dirname，&children）；

  //我们知道旧的日志文件名为[path]<something>
  //返回与模式匹配的所有实体
  用于（自动和儿童：儿童）
    如果（FNEAR）！=子级&&child.find（fname）==0）
      ret.push_back（dirname+“/”+child）；
    }
  }

  返回RET；
}

//返回在文件中找到给定模式的行数
静态大小“getLinesCount”（const std:：string&fname，
                            const std：：字符串和模式）
  std:：stringstream ssbuf；
  std：：字符串行；
  尺寸t计数=0；

  std:：ifstream infile（fname.c_str（））；
  ssbuf<<infile.rdbuf（）；

  while（getline（ssbuf，line））
    如果（线。查找（模式）！=std：：字符串：：npos）
      计数+；
    }
  }

  返回计数；
}

测试F（autorollloggertest，logheadertest）
  静态常量大小_t max_headers=10；
  静态常量大小日志最大大小=1024*5；
  static const std:：string header_str=“日志头行”；

  //test_num==0->对header（）的标准调用
  //test_num==1->infologlevel:：header_level调用log（）。
  for（int test_num=0；test_num<2；test_num++）

    InestTestBar（）；

    AutoRollLogger记录器（env:：default（），ktestdir，/*db_log_dir*/ "",

                          /*_max_-size，/*日志文件_-time_-to_-roll=*/0）；

    如果（test_num==0）
      //使用header（）显式记录一些头
      对于（尺寸_t i=0；i<max_headers；i++）
        头文件（&logger，“%s%d”，header_str.c_str（），i）；
      }
    否则，如果（test_num==1）
      //头级别应使其行为类似于调用头（）。
      对于（尺寸_t i=0；i<max_headers；i++）
        rocks_log_header（&logger，“%s%d”，header_str.c_str（），i）；
      }
    }

    const std:：string newfname=logger.test_log_fname（）；

    //记录足够的数据以导致翻滚
    int i＝0；
    对于（尺寸t iter=0；iter<2；iter++）
      while（logger.getlogfilesize（）<log_max_size）
        信息（&logger，（ksampleMessage+“：logHeaderTest行%d”）.c_str（），i）；
        +i；
      }

      信息（&logger，“滚动”）；
    }

    //刷新最新文件的日志
    日志刷新（&logger）；

    const auto oldfiles=getoldfilename（newfname）；

    断言eq（oldfiles.size（），（size_t）2）；

    对于（auto&oldfname:oldfiles）
      //验证文件是否已回滚
      断言（oldfname，newfname）；
      //验证旧日志是否包含所有头日志
      断言eq（getlinescount（oldfname，header_str），max_headers）；
    }
  }
}

测试_f（autorollloggertest，logfileexistence）
  RockSDB：：分贝*分贝；
  rocksdb：：选项；
奥菲德
  //替换路径中的所有斜线，这样Windows Compaspec就不会
  //困惑
  std：：字符串testdir（ktestdir）；
  std:：replace_if（testdir.begin（），testdir.end（），
    []（char ch）返回ch='/'；，'\\'）；
  std:：string deletecmd=“如果存在”+testdir+“rd/s/q”+testdir；
否则
  std:：string deletecmd=“rm-rf”+ktestdir；
第二节
  assert_eq（system（deletecmd.c_str（）），0）；
  options.max_log_file_size=100*1024*1024；
  options.create_if_missing=true；
  断言ou OK（rocksdb:：db:：open（options，ktestdir，&db））；
  断言_OK（默认_env->fileexists（klogfile））；
  删除数据库；
}

//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}

否则
include<stdio.h>

int main（int argc，char**argv）
  FPrTNF（STDER）
          “由于RocksDB-Lite不支持AutoRollLogger，所以跳过此项操作\n”）；
  返回0；
}

我很喜欢你！摇滚乐
