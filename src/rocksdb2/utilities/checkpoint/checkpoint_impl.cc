
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
//版权所有（c）2012 Facebook。
//此源代码的使用受可以
//在许可证文件中找到。

#ifndef ROCKSDB_LITE

#include "utilities/checkpoint/checkpoint_impl.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <algorithm>
#include <string>
#include <vector>

#include "db/wal_manager.h"
#include "port/port.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/transaction_log.h"
#include "rocksdb/utilities/checkpoint.h"
#include "util/file_util.h"
#include "util/filename.h"
#include "util/sync_point.h"

namespace rocksdb {

Status Checkpoint::Create(DB* db, Checkpoint** checkpoint_ptr) {
  *checkpoint_ptr = new CheckpointImpl(db);
  return Status::OK();
}

Status Checkpoint::CreateCheckpoint(const std::string& checkpoint_dir,
                                    uint64_t log_size_for_flush) {
  return Status::NotSupported("");
}

//构建RockSDB的可打开快照
Status CheckpointImpl::CreateCheckpoint(const std::string& checkpoint_dir,
                                        uint64_t log_size_for_flush) {
  DBOptions db_options = db_->GetDBOptions();

  Status s = db_->GetEnv()->FileExists(checkpoint_dir);
  if (s.ok()) {
    return Status::InvalidArgument("Directory exists");
  } else if (!s.IsNotFound()) {
    assert(s.IsIOError());
    return s;
  }

  ROCKS_LOG_INFO(
      db_options.info_log,
      "Started the snapshot process -- creating snapshot in directory %s",
      checkpoint_dir.c_str());
  std::string full_private_path = checkpoint_dir + ".tmp";
//创建快照目录
  s = db_->GetEnv()->CreateDir(full_private_path);
  uint64_t sequence_number = 0;
  if (s.ok()) {
    db_->DisableFileDeletions();
    s = CreateCustomCheckpoint(
        db_options,
        [&](const std::string& src_dirname, const std::string& fname,
            FileType) {
          ROCKS_LOG_INFO(db_options.info_log, "Hard Linking %s", fname.c_str());
          return db_->GetEnv()->LinkFile(src_dirname + fname,
                                         full_private_path + fname);
        /**链接文件\u cb*/，
        [&]（const std:：string&src_dirname，const std:：string&fname，
            uint64_t size_limit_bytes，filetype）
          rocks_log_info（db_options.info_log，“复制%s”，fname.c_str（））；
          返回copyfile（db_->getenv（），src_dirname+fname，
                          完整的私有路径+fname，大小限制字节，
                          数据库选项。使用同步）；
        /*复制_文件_cb*/,

        [&](const std::string& fname, const std::string& contents, FileType) {
          ROCKS_LOG_INFO(db_options.info_log, "Creating %s", fname.c_str());
          return CreateFile(db_->GetEnv(), full_private_path + fname, contents);
        /**创建\u文件\u cb*/，
        序列号，日志大小，用于冲洗；
    //我们复制了所有文件，启用文件删除
    db_u->启用文件删除（false）；
  }

  如果（S.O.（））{
    //将tmp private backup移到real snapshot目录
    s=db_->getenv（）->renamefile（完整的私有路径，checkpoint_dir）；
  }
  如果（S.O.（））{
    唯一的检查点目录；
    db_u->getenv（）->newdirectory（checkpoint_dir，&checkpoint_directory）；
    如果（检查点目录！= null pTr）{
      s=checkpoint_directory->fsync（）；
    }
  }

  如果（S.O.（））{
    //这里我们知道我们成功安装了新的快照
    rocks_log_info（db_options.info_log），快照完成。“一切都好”；
    rocks_log_info（db_options.info_log，“快照序列号：%”priu64，
                   序列号）；
  }否则{
    //清除我们可能创建的所有文件
    rocks_log_info（db_options.info_log，“快照失败--%s”，
                   s.toString（）.c_str（））；
    //我们必须删除dir及其所有子目录
    std:：vector<std:：string>subchildren；
    db_u->getenv（）->getchildren（完整的私有路径，&subchildren）；
    用于（自动和子类别：子类别）
      std：：string subchild_path=full_private_path+“/”+subchild；
      状态s1=db_->getenv（）->deletefile（subchild_path）；
      rocks_log_info（db_options.info_log，“删除文件%s--%s”，
                     子child_path.c_str（），s1.toString（）.c_str（））；
    }
    //最后删除private dir
    状态s1=db_->getenv（）->deletedir（完整的_private_路径）；
    rocks_log_info（db_options.info_log，“删除目录%s--%s”，
                   full_private_path.c_str（），s1.toString（）.c_str（））；
  }
  返回S；
}

状态检查点IMPL:：CreateCustomCheckpoint（
    const db options和db_选项，
    std:：function<status（const std:：string&src_dirname，
                         const std：：string&src_fname，filetype type）>
        林克菲
    std:：function<status（const std:：string&src_dirname，
                         const std:：string&srcfu fname，
                         uint64_t size_limit_bytes，filetype type）>
        CopyFielFieleCB，
    std:：function<status（const std:：string&fname，const std:：string&contents，
                         文件类型）>
        CuraTyFieleCB，
    uint64_t*序列号，uint64_t log_size_for_flush）
  状态S；
  std:：vector<std:：string>live_files；
  uint64清单文件大小=0；
  uint64_t min_log_num=端口：：kmaxuint64；
  *序列号=db_->GetLatestSequenceNumber（）；
  bool same_fs=真；
  vectorlogptr实时文件；

  bool flush_memtable=真；
  如果（S.O.（））{
    如果（！）db_options.allow2pc）
      if（log_size_for_flush==port:：kmaxuint64）
        flush_memtable=false；
      else if（log_size_for_flush>0）
        //如果现有日志文件很小，则跳过刷新。
        s=db_u->getSortedWalfiles（实时文件）；

        如果（！）S.O.（））{
          返回S；
        }

        //如果总日志大小小于，则不刷新列族
        //记录\大小\用于\刷新。我们复制日志文件。
        //我们也可以覆盖2PC的情况。
        uint64总尺寸=0；
        用于（auto&wal:live_-wal_文件）
          总_wal_size+=wal->sizeFileBytes（）；
        }
        if（total_wal_size<log_size_for_flush）
          flush_memtable=false；
        }
        live_wal_files.clear（）；
      }
    }

    //这将返回以“/”为前缀的活动\ u文件
    s=db_->getlivefiles（live_file s，&manifest_file_size，flush_memtable）；

    if（s.ok（）&&db_options.allow2pc）
      //如果启用2PC，我们需要在刷新后得到最小的日志号。
      //需要重新获取实时文件以重新获取快照。
      如果（！）db_u->getIntProperty（db:：properties:：kminLogNumberToKeep，
                               &min_log_num））
        返回状态：：invalidArgument（
            “2PC已启用，但无法确定要保留的最小日志号。”）；
      }
      //我们需要使用flush重新提取活动文件来处理这种情况：
      //previous 000001.log包含事务tnx1的准备记录。
      //当前日志文件为000002.log，序列号指向
      //文件。
      //调用getLiveFiles（）后，会创建000003.log。
      //然后提交tnx1。提交记录将写入000003.log。
      //现在取min_log_num，即3。
      //则只复制000002.log和000003.log，而000001.log将
      /跳过。000003.log包含tnx1的提交消息，但我们没有
      //有相应的准备记录。
      //为了避免这种情况，我们需要强制刷新以确保
      //将刷新在获取min_log_num之前提交的所有事务
      //到sst文件。
      //在调用getLiveFiles（）之前，无法获取min_log_num。
      //这是第一次，因为如果我们这样做，所有的日志文件都将包含在内，
      //远远超出了需要。
      s=db_->getlivefiles（live_file s，&manifest_file_size，flush_memtable）；
    }

    测试同步点（“checkpointimpl:：createCheckpoint:savedLivefiles1”）；
    测试同步点（“checkpointimpl:：createCheckpoint:savedLivefiles2”）；
    db_u->flushwal（假/*同步*/);

  }
//如果我们有多个列族，我们还需要获取wal文件
  if (s.ok()) {
    s = db_->GetSortedWalFiles(live_wal_files);
  }
  if (!s.ok()) {
    return s;
  }

  size_t wal_size = live_wal_files.size();

//复制/硬链接实时文件
  std::string manifest_fname, current_fname;
  for (size_t i = 0; s.ok() && i < live_files.size(); ++i) {
    uint64_t number;
    FileType type;
    bool ok = ParseFileName(live_files[i], &number, &type);
    if (!ok) {
      s = Status::Corruption("Can't parse file name. This is very bad");
      break;
    }
//我们只能在这里得到sst、选项、清单和当前文件
    assert(type == kTableFile || type == kDescriptorFile ||
           type == kCurrentFile || type == kOptionsFile);
    assert(live_files[i].size() > 0 && live_files[i][0] == '/');
    if (type == kCurrentFile) {
//我们将手动创建当前文件，以确保它与
//清单编号。这是必要的，因为当前的文件内容
//可以在创建检查点期间更改。
      current_fname = live_files[i];
      continue;
    } else if (type == kDescriptorFile) {
      manifest_fname = live_files[i];
    }
    std::string src_fname = live_files[i];

//规则：
//*如果是ktablefile，则共享
//*如果是kdescriptorfile，则将大小限制为manifest文件大小
//*如果跨设备链接，则始终复制
    if ((type == kTableFile) && same_fs) {
      s = link_file_cb(db_->GetName(), src_fname, type);
      if (s.IsNotSupported()) {
        same_fs = false;
        s = Status::OK();
      }
    }
    if ((type != kTableFile) || (!same_fs)) {
      s = copy_file_cb(db_->GetName(), src_fname,
                       (type == kDescriptorFile) ? manifest_file_size : 0,
                       type);
    }
  }
  if (s.ok() && !current_fname.empty() && !manifest_fname.empty()) {
    create_file_cb(current_fname, manifest_fname.substr(1) + "\n",
                   kCurrentFile);
  }
  ROCKS_LOG_INFO(db_options.info_log, "Number of log files %" ROCKSDB_PRIszt,
                 live_wal_files.size());

//链接Wal文件。复制上一个的精确大小，因为它是唯一的
//在最后一次刷新之后发生了变化。
  for (size_t i = 0; s.ok() && i < wal_size; ++i) {
    if ((live_wal_files[i]->Type() == kAliveLogFile) &&
        (!flush_memtable ||
         live_wal_files[i]->StartSequence() >= *sequence_number ||
         live_wal_files[i]->LogNumber() >= min_log_num)) {
      if (i + 1 == wal_size) {
        s = copy_file_cb(db_options.wal_dir, live_wal_files[i]->PathName(),
                         live_wal_files[i]->SizeFileBytes(), kLogFile);
        break;
      }
      if (same_fs) {
//我们只关心实时日志文件
        s = link_file_cb(db_options.wal_dir, live_wal_files[i]->PathName(),
                         kLogFile);
        if (s.IsNotSupported()) {
          same_fs = false;
          s = Status::OK();
        }
      }
      if (!same_fs) {
        s = copy_file_cb(db_options.wal_dir, live_wal_files[i]->PathName(), 0,
                         kLogFile);
      }
    }
  }

  return s;
}

}  //命名空间rocksdb

#endif  //摇滚乐
