
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <atomic>
#include <thread>
#include <vector>

#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "util/delete_scheduler.h"
#include "util/sst_file_manager_impl.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"

#ifndef ROCKSDB_LITE

namespace rocksdb {

class DeleteSchedulerTest : public testing::Test {
 public:
  DeleteSchedulerTest() : env_(Env::Default()) {
    dummy_files_dir_ = test::TmpDir(env_) + "/delete_scheduler_dummy_data_dir";
    DestroyAndCreateDir(dummy_files_dir_);
    trash_dir_ = test::TmpDir(env_) + "/delete_scheduler_trash";
    DestroyAndCreateDir(trash_dir_);
  }

  ~DeleteSchedulerTest() {
    rocksdb::SyncPoint::GetInstance()->DisableProcessing();
    rocksdb::SyncPoint::GetInstance()->LoadDependency({});
    rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();
    test::DestroyDir(env_, dummy_files_dir_);
  }

  void DestroyAndCreateDir(const std::string& dir) {
    ASSERT_OK(test::DestroyDir(env_, dir));
    EXPECT_OK(env_->CreateDir(dir));
  }

  int CountFilesInDir(const std::string& dir) {
    std::vector<std::string> files_in_dir;
    EXPECT_OK(env_->GetChildren(dir, &files_in_dir));
//忽略“.”和“..”
    return static_cast<int>(files_in_dir.size()) - 2;
  }

  std::string NewDummyFile(const std::string& file_name, uint64_t size = 1024) {
    std::string file_path = dummy_files_dir_ + "/" + file_name;
    std::unique_ptr<WritableFile> f;
    env_->NewWritableFile(file_path, &f, EnvOptions());
    std::string data(size, 'A');
    EXPECT_OK(f->Append(data));
    EXPECT_OK(f->Close());
    sst_file_mgr_->OnAddFile(file_path);
    return file_path;
  }

  void NewDeleteScheduler() {
    ASSERT_OK(env_->CreateDirIfMissing(trash_dir_));
    sst_file_mgr_.reset(
        new SstFileManagerImpl(env_, nullptr, trash_dir_, rate_bytes_per_sec_));
    delete_scheduler_ = sst_file_mgr_->delete_scheduler();
//此文件中的测试用于DeleteScheduler组件，不创建任何
//DBS，所以我们需要使用设置该值为100%（而不是默认的25%）。
    delete_scheduler_->TEST_SetMaxTrashDBRatio(1.1);
  }

  Env* env_;
  std::string dummy_files_dir_;
  std::string trash_dir_;
  int64_t rate_bytes_per_sec_;
  DeleteScheduler* delete_scheduler_;
  std::unique_ptr<SstFileManagerImpl> sst_file_mgr_;
};

//测试DeleteScheduler的基本功能（速率限制）。
//1-创建100个虚拟文件
//2-使用DeleteScheduler删除100个虚拟文件
//---按住DeleteScheduler:：BackgroundEmptyTrash---
//3-等待DeleteScheduler删除垃圾桶中的所有文件
//4-验证backgroundemptytrash用于更正文件的笔数
//5-确保已完全删除所有创建的文件
TEST_F(DeleteSchedulerTest, BasicRateLimiting) {
  rocksdb::SyncPoint::GetInstance()->LoadDependency({
      {"DeleteSchedulerTest::BasicRateLimiting:1",
       "DeleteScheduler::BackgroundEmptyTrash"},
  });

  std::vector<uint64_t> penalties;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::BackgroundEmptyTrash:Wait",
      [&](void* arg) { penalties.push_back(*(static_cast<uint64_t*>(arg))); });

int num_files = 100;  //100档
uint64_t file_size = 1024;  //每个文件都是1 KB
  std::vector<uint64_t> delete_kbs_per_sec = {512, 200, 100, 50, 25};

  for (size_t t = 0; t < delete_kbs_per_sec.size(); t++) {
    penalties.clear();
    rocksdb::SyncPoint::GetInstance()->ClearTrace();
    rocksdb::SyncPoint::GetInstance()->EnableProcessing();

    DestroyAndCreateDir(dummy_files_dir_);
    rate_bytes_per_sec_ = delete_kbs_per_sec[t] * 1024;
    NewDeleteScheduler();

//创建100个虚拟文件，每个文件为1 KB
    std::vector<std::string> generated_files;
    for (int i = 0; i < num_files; i++) {
      std::string file_name = "file" + ToString(i) + ".data";
      generated_files.push_back(NewDummyFile(file_name, file_size));
    }

//删除虚拟文件并测量清空垃圾桶所花费的时间
    for (int i = 0; i < num_files; i++) {
      ASSERT_OK(delete_scheduler_->DeleteFile(generated_files[i]));
    }
    ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);

    uint64_t delete_start_time = env_->NowMicros();
    TEST_SYNC_POINT("DeleteSchedulerTest::BasicRateLimiting:1");
    delete_scheduler_->WaitForEmptyTrash();
    uint64_t time_spent_deleting = env_->NowMicros() - delete_start_time;

    auto bg_errors = delete_scheduler_->GetBackgroundErrors();
    ASSERT_EQ(bg_errors.size(), 0);

    uint64_t total_files_size = 0;
    uint64_t expected_penlty = 0;
    ASSERT_EQ(penalties.size(), num_files);
    for (int i = 0; i < num_files; i++) {
      total_files_size += file_size;
      expected_penlty = ((total_files_size * 1000000) / rate_bytes_per_sec_);
      ASSERT_EQ(expected_penlty, penalties[i]);
    }
    ASSERT_GT(time_spent_deleting, expected_penlty * 0.9);

    ASSERT_EQ(CountFilesInDir(trash_dir_), 0);
    rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  }
}

//与基本限制测试相同，但删除多个线程中的文件。
//1-创建100个虚拟文件
//2-使用DeleteScheduler使用10个线程删除100个虚拟文件
//---按住DeleteScheduler:：BackgroundEmptyTrash---
//3-等待DeleteScheduler删除队列中的所有文件
//4-验证backgroundemptytrash用于更正文件的笔数
//5-确保已完全删除所有创建的文件
TEST_F(DeleteSchedulerTest, RateLimitingMultiThreaded) {
  rocksdb::SyncPoint::GetInstance()->LoadDependency({
      {"DeleteSchedulerTest::RateLimitingMultiThreaded:1",
       "DeleteScheduler::BackgroundEmptyTrash"},
  });

  std::vector<uint64_t> penalties;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::BackgroundEmptyTrash:Wait",
      [&](void* arg) { penalties.push_back(*(static_cast<uint64_t*>(arg))); });

  int thread_cnt = 10;
int num_files = 10;  //每个线程10个文件
uint64_t file_size = 1024;  //每个文件都是1 KB

  std::vector<uint64_t> delete_kbs_per_sec = {512, 200, 100, 50, 25};
  for (size_t t = 0; t < delete_kbs_per_sec.size(); t++) {
    penalties.clear();
    rocksdb::SyncPoint::GetInstance()->ClearTrace();
    rocksdb::SyncPoint::GetInstance()->EnableProcessing();

    DestroyAndCreateDir(dummy_files_dir_);
    rate_bytes_per_sec_ = delete_kbs_per_sec[t] * 1024;
    NewDeleteScheduler();

//创建100个虚拟文件，每个文件为1 KB
    std::vector<std::string> generated_files;
    for (int i = 0; i < num_files * thread_cnt; i++) {
      std::string file_name = "file" + ToString(i) + ".data";
      generated_files.push_back(NewDummyFile(file_name, file_size));
    }

//使用10个线程删除虚拟文件并测量清空垃圾桶所花费的时间
    std::atomic<int> thread_num(0);
    std::vector<port::Thread> threads;
    std::function<void()> delete_thread = [&]() {
      int idx = thread_num.fetch_add(1);
      int range_start = idx * num_files;
      int range_end = range_start + num_files;
      for (int j = range_start; j < range_end; j++) {
        ASSERT_OK(delete_scheduler_->DeleteFile(generated_files[j]));
      }
    };

    for (int i = 0; i < thread_cnt; i++) {
      threads.emplace_back(delete_thread);
    }

    for (size_t i = 0; i < threads.size(); i++) {
      threads[i].join();
    }

    uint64_t delete_start_time = env_->NowMicros();
    TEST_SYNC_POINT("DeleteSchedulerTest::RateLimitingMultiThreaded:1");
    delete_scheduler_->WaitForEmptyTrash();
    uint64_t time_spent_deleting = env_->NowMicros() - delete_start_time;

    auto bg_errors = delete_scheduler_->GetBackgroundErrors();
    ASSERT_EQ(bg_errors.size(), 0);

    uint64_t total_files_size = 0;
    uint64_t expected_penlty = 0;
    ASSERT_EQ(penalties.size(), num_files * thread_cnt);
    for (int i = 0; i < num_files * thread_cnt; i++) {
      total_files_size += file_size;
      expected_penlty = ((total_files_size * 1000000) / rate_bytes_per_sec_);
      ASSERT_EQ(expected_penlty, penalties[i]);
    }
    ASSERT_GT(time_spent_deleting, expected_penlty * 0.9);

    ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
    ASSERT_EQ(CountFilesInDir(trash_dir_), 0);
    rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  }
}

//通过将“速率字节数/秒”设置为0禁用速率限制，并确保
//当DeleteScheduler删除一个文件时，它会立即删除它，而不会
//把它移到垃圾桶里
TEST_F(DeleteSchedulerTest, DisableRateLimiting) {
  int bg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });

  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

  rate_bytes_per_sec_ = 0;
  NewDeleteScheduler();

  for (int i = 0; i < 10; i++) {
//我们删除的每个文件都将立即删除
    std::string dummy_file = NewDummyFile("dummy.data");
    ASSERT_OK(delete_scheduler_->DeleteFile(dummy_file));
    ASSERT_TRUE(env_->FileExists(dummy_file).IsNotFound());
    ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
    ASSERT_EQ(CountFilesInDir(trash_dir_), 0);
  }

  ASSERT_EQ(bg_delete_file, 0);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

//测试将同名文件移动到垃圾桶不是问题
//1-创建10个同名文件“conflict.data”
//2-使用DeleteScheduler删除10个文件
//3-确保垃圾目录包含10个文件（“conflict.data”x 10）
//---按住DeleteScheduler:：BackgroundEmptyTrash---
//4-确保从垃圾桶中删除文件
TEST_F(DeleteSchedulerTest, ConflictNames) {
  rocksdb::SyncPoint::GetInstance()->LoadDependency({
      {"DeleteSchedulerTest::ConflictNames:1",
       "DeleteScheduler::BackgroundEmptyTrash"},
  });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 1024 * 1024;  //1兆字节/秒
  NewDeleteScheduler();

//创建“conflict.data”并将其移到垃圾桶中10次
  for (int i = 0; i < 10; i++) {
    std::string dummy_file = NewDummyFile("conflict.data");
    ASSERT_OK(delete_scheduler_->DeleteFile(dummy_file));
  }
  ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
//垃圾桶中的10个文件（“conflict.data”x 10）
  ASSERT_EQ(CountFilesInDir(trash_dir_), 10);

//后场空虚皮疹
  TEST_SYNC_POINT("DeleteSchedulerTest::ConflictNames:1");
  delete_scheduler_->WaitForEmptyTrash();
  ASSERT_EQ(CountFilesInDir(trash_dir_), 0);

  auto bg_errors = delete_scheduler_->GetBackgroundErrors();
  ASSERT_EQ(bg_errors.size(), 0);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

//1-创建10个虚拟文件
//2-使用DeleteScheduler删除10个文件（将它们移到trsah）
//3-直接删除10个文件（使用env_uu->deletefile）
//---按住DeleteScheduler:：BackgroundEmptyTrash---
//4-确保DeleteScheduler未能删除10个文件，并且
//报告了10个后台错误
TEST_F(DeleteSchedulerTest, BackgroundError) {
  rocksdb::SyncPoint::GetInstance()->LoadDependency({
      {"DeleteSchedulerTest::BackgroundError:1",
       "DeleteScheduler::BackgroundEmptyTrash"},
  });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 1024 * 1024;  //1兆字节/秒
  NewDeleteScheduler();

//生成10个虚拟文件并将其移到垃圾桶中
  for (int i = 0; i < 10; i++) {
    std::string file_name = "data_" + ToString(i) + ".data";
    ASSERT_OK(delete_scheduler_->DeleteFile(NewDummyFile(file_name)));
  }
  ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
  ASSERT_EQ(CountFilesInDir(trash_dir_), 10);

//从垃圾桶中删除10个文件，这将导致
//因为我们已经删除了文件
//删除目标
  for (int i = 0; i < 10; i++) {
    std::string file_name = "data_" + ToString(i) + ".data";
    ASSERT_OK(env_->DeleteFile(trash_dir_ + "/" + file_name));
  }

//后场空虚皮疹
  TEST_SYNC_POINT("DeleteSchedulerTest::BackgroundError:1");
  delete_scheduler_->WaitForEmptyTrash();
  auto bg_errors = delete_scheduler_->GetBackgroundErrors();
  ASSERT_EQ(bg_errors.size(), 10);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

//1-创建10个虚拟文件
//2-使用DeleteScheduler删除10个虚拟文件
//3-等待DeleteScheduler删除队列中的所有文件
//4-确保已删除垃圾目录中的所有文件
//5-重复前面的步骤5次
TEST_F(DeleteSchedulerTest, StartBGEmptyTrashMultipleTimes) {
  int bg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 1024 * 1024;  //1兆字节/秒
  NewDeleteScheduler();

//将文件移到垃圾桶，等待清空垃圾桶，重新启动
  for (int run = 1; run <= 5; run++) {
//生成10个虚拟文件并将其移到垃圾桶中
    for (int i = 0; i < 10; i++) {
      std::string file_name = "data_" + ToString(i) + ".data";
      ASSERT_OK(delete_scheduler_->DeleteFile(NewDummyFile(file_name)));
    }
    ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
    delete_scheduler_->WaitForEmptyTrash();
    ASSERT_EQ(bg_delete_file, 10 * run);
    ASSERT_EQ(CountFilesInDir(trash_dir_), 0);

    auto bg_errors = delete_scheduler_->GetBackgroundErrors();
    ASSERT_EQ(bg_errors.size(), 0);
  }

  ASSERT_EQ(bg_delete_file, 50);
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();
}

//1-创建具有非常慢的速率限制（1字节/秒）的DeleteScheduler
//2-使用DeleteScheduler删除100个文件
//3-删除DeleteScheduler（当队列不为空时调用析构函数）
//4-确保不是所有文件都从垃圾桶中删除，并且
//DeleteScheduler后台线程未删除所有文件
TEST_F(DeleteSchedulerTest, DestructorWithNonEmptyQueue) {
  int bg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 1;  //1字节/秒
  NewDeleteScheduler();

  for (int i = 0; i < 100; i++) {
    std::string file_name = "data_" + ToString(i) + ".data";
    ASSERT_OK(delete_scheduler_->DeleteFile(NewDummyFile(file_name)));
  }

//删除100个文件需要超过28小时才能删除
//当删除队列不为空时，我们将删除DeleteScheduler
  sst_file_mgr_.reset();

  ASSERT_LT(bg_delete_file, 100);
  ASSERT_GT(CountFilesInDir(trash_dir_), 0);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

//1-删除垃圾目录
//2-使用DeleteScheduler删除10个文件
//3-确保在删除计划程序之后立即删除10个文件
//无法将它们移到垃圾目录
TEST_F(DeleteSchedulerTest, MoveToTrashError) {
  int bg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 1024;  //1千字节/秒
  NewDeleteScheduler();

//我们将删除垃圾目录，这意味着DeleteScheduler不会
//能够将文件移到垃圾箱，并将立即删除文件。
  ASSERT_OK(test::DestroyDir(env_, trash_dir_));
  for (int i = 0; i < 10; i++) {
    std::string file_name = "data_" + ToString(i) + ".data";
    ASSERT_OK(delete_scheduler_->DeleteFile(NewDummyFile(file_name)));
  }

  ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);
  ASSERT_EQ(bg_delete_file, 0);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

TEST_F(DeleteSchedulerTest, DISABLED_DynamicRateLimiting1) {
  std::vector<uint64_t> penalties;
  int bg_delete_file = 0;
  int fg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteFile",
      [&](void* arg) { fg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::BackgroundEmptyTrash:Wait",
      [&](void* arg) { penalties.push_back(*(static_cast<int*>(arg))); });

  rocksdb::SyncPoint::GetInstance()->LoadDependency({
      {"DeleteSchedulerTest::DynamicRateLimiting1:1",
       "DeleteScheduler::BackgroundEmptyTrash"},
  });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

rate_bytes_per_sec_ = 0;  //最初禁用速率限制
  NewDeleteScheduler();


int num_files = 10;  //10档
uint64_t file_size = 1024;  //每个文件都是1 KB

  std::vector<int64_t> delete_kbs_per_sec = {512, 200, 0, 100, 50, -2, 25};
  for (size_t t = 0; t < delete_kbs_per_sec.size(); t++) {
    penalties.clear();
    bg_delete_file = 0;
    fg_delete_file = 0;
    rocksdb::SyncPoint::GetInstance()->ClearTrace();
    rocksdb::SyncPoint::GetInstance()->EnableProcessing();

    DestroyAndCreateDir(dummy_files_dir_);
    rate_bytes_per_sec_ = delete_kbs_per_sec[t] * 1024;
    delete_scheduler_->SetRateBytesPerSecond(rate_bytes_per_sec_);

//创建100个虚拟文件，每个文件为1 KB
    std::vector<std::string> generated_files;
    for (int i = 0; i < num_files; i++) {
      std::string file_name = "file" + ToString(i) + ".data";
      generated_files.push_back(NewDummyFile(file_name, file_size));
    }

//删除虚拟文件并测量清空垃圾桶所花费的时间
    for (int i = 0; i < num_files; i++) {
      ASSERT_OK(delete_scheduler_->DeleteFile(generated_files[i]));
    }
    ASSERT_EQ(CountFilesInDir(dummy_files_dir_), 0);

    if (rate_bytes_per_sec_ > 0) {
      uint64_t delete_start_time = env_->NowMicros();
      TEST_SYNC_POINT("DeleteSchedulerTest::DynamicRateLimiting1:1");
      delete_scheduler_->WaitForEmptyTrash();
      uint64_t time_spent_deleting = env_->NowMicros() - delete_start_time;

      auto bg_errors = delete_scheduler_->GetBackgroundErrors();
      ASSERT_EQ(bg_errors.size(), 0);

      uint64_t total_files_size = 0;
      uint64_t expected_penlty = 0;
      ASSERT_EQ(penalties.size(), num_files);
      for (int i = 0; i < num_files; i++) {
        total_files_size += file_size;
        expected_penlty = ((total_files_size * 1000000) / rate_bytes_per_sec_);
        ASSERT_EQ(expected_penlty, penalties[i]);
      }
      ASSERT_GT(time_spent_deleting, expected_penlty * 0.9);
      ASSERT_EQ(bg_delete_file, num_files);
      ASSERT_EQ(fg_delete_file, 0);
    } else {
      ASSERT_EQ(penalties.size(), 0);
      ASSERT_EQ(bg_delete_file, 0);
      ASSERT_EQ(fg_delete_file, num_files);
    }

    ASSERT_EQ(CountFilesInDir(trash_dir_), 0);
    rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  }
}

TEST_F(DeleteSchedulerTest, ImmediateDeleteOn25PercDBSize) {
  int bg_delete_file = 0;
  int fg_delete_file = 0;
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteTrashFile:DeleteFile",
      [&](void* arg) { bg_delete_file++; });
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "DeleteScheduler::DeleteFile", [&](void* arg) { fg_delete_file++; });

  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

int num_files = 100;  //100档
uint64_t file_size = 1024 * 10; //文件大小为100 KB
rate_bytes_per_sec_ = 1;  //每秒1字节（垃圾删除非常慢）

  NewDeleteScheduler();
  delete_scheduler_->TEST_SetMaxTrashDBRatio(0.25);

  std::vector<std::string> generated_files;
  for (int i = 0; i < num_files; i++) {
    std::string file_name = "file" + ToString(i) + ".data";
    generated_files.push_back(NewDummyFile(file_name, file_size));
  }

  for (std::string& file_name : generated_files) {
    delete_scheduler_->DeleteFile(file_name);
  }

//当我们把26个文件放进垃圾箱后，我们就开始
//立即删除新文件
  ASSERT_EQ(fg_delete_file, 74);

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
int main(int argc, char** argv) {
  printf("DeleteScheduler is not supported in ROCKSDB_LITE\n");
  return 0;
}
#endif  //摇滚乐
