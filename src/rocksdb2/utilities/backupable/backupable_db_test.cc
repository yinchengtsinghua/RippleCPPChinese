
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#if !defined(ROCKSDB_LITE) && !defined(OS_WIN)

#include <algorithm>
#include <string>

#include "db/db_impl.h"
#include "env/env_chroot.h"
#include "port/port.h"
#include "port/stack_trace.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/transaction_log.h"
#include "rocksdb/types.h"
#include "rocksdb/utilities/backupable_db.h"
#include "rocksdb/utilities/options_util.h"
#include "util/file_reader_writer.h"
#include "util/filename.h"
#include "util/mutexlock.h"
#include "util/random.h"
#include "util/stderr_logger.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

namespace {

using std::unique_ptr;

class DummyDB : public StackableDB {
 public:
  /*隐性的*/
  DummyDB(const Options& options, const std::string& dbname)
     : StackableDB(nullptr), options_(options), dbname_(dbname),
       deletions_enabled_(true), sequence_number_(0) {}

  virtual SequenceNumber GetLatestSequenceNumber() const override {
    return ++sequence_number_;
  }

  virtual const std::string& GetName() const override {
    return dbname_;
  }

  virtual Env* GetEnv() const override {
    return options_.env;
  }

  using DB::GetOptions;
  virtual Options GetOptions(ColumnFamilyHandle* column_family) const override {
    return options_;
  }

  virtual DBOptions GetDBOptions() const override {
    return DBOptions(options_);
  }

  virtual Status EnableFileDeletions(bool force) override {
    EXPECT_TRUE(!deletions_enabled_);
    deletions_enabled_ = true;
    return Status::OK();
  }

  virtual Status DisableFileDeletions() override {
    EXPECT_TRUE(deletions_enabled_);
    deletions_enabled_ = false;
    return Status::OK();
  }

  virtual Status GetLiveFiles(std::vector<std::string>& vec, uint64_t* mfs,
                              bool flush_memtable = true) override {
    EXPECT_TRUE(!deletions_enabled_);
    vec = live_files_;
    *mfs = 100;
    return Status::OK();
  }

  virtual ColumnFamilyHandle* DefaultColumnFamily() const override {
    return nullptr;
  }

  class DummyLogFile : public LogFile {
   public:
    /*隐性的*/
     DummyLogFile(const std::string& path, bool alive = true)
         : path_(path), alive_(alive) {}

    virtual std::string PathName() const override {
      return path_;
    }

    virtual uint64_t LogNumber() const override {
//你叫这个方法是干什么的？
      ADD_FAILURE();
      return 0;
    }

    virtual WalFileType Type() const override {
      return alive_ ? kAliveLogFile : kArchivedLogFile;
    }

    virtual SequenceNumber StartSequence() const override {
//此seqnum保证虚拟文件将包含在备份中。
//只要它还活着。
      return kMaxSequenceNumber;
    }

    virtual uint64_t SizeFileBytes() const override {
      return 0;
    }

   private:
    std::string path_;
    bool alive_;
}; //虚拟日志文件

  virtual Status GetSortedWalFiles(VectorLogPtr& files) override {
    EXPECT_TRUE(!deletions_enabled_);
    files.resize(wal_files_.size());
    for (size_t i = 0; i < files.size(); ++i) {
      files[i].reset(
          new DummyLogFile(wal_files_[i].first, wal_files_[i].second));
    }
    return Status::OK();
  }

//避免对堆栈db调用Flushwal，该db为nullptr
  virtual Status FlushWAL(bool sync) override { return Status::OK(); }

  std::vector<std::string> live_files_;
//配对<文件名，活动？>
  std::vector<std::pair<std::string, bool>> wal_files_;
 private:
  Options options_;
  std::string dbname_;
  bool deletions_enabled_;
  mutable SequenceNumber sequence_number_;
}; //DimyDB

class TestEnv : public EnvWrapper {
 public:
  explicit TestEnv(Env* t) : EnvWrapper(t) {}

  class DummySequentialFile : public SequentialFile {
   public:
    explicit DummySequentialFile(bool fail_reads)
        : SequentialFile(), rnd_(5), fail_reads_(fail_reads) {}
    virtual Status Read(size_t n, Slice* result, char* scratch) override {
      if (fail_reads_) {
        return Status::IOError();
      }
      size_t read_size = (n > size_left) ? size_left : n;
      for (size_t i = 0; i < read_size; ++i) {
        scratch[i] = rnd_.Next() & 255;
      }
      *result = Slice(scratch, read_size);
      size_left -= read_size;
      return Status::OK();
    }

    virtual Status Skip(uint64_t n) override {
      size_left = (n > size_left) ? size_left - n : 0;
      return Status::OK();
    }
   private:
    size_t size_left = 200;
    Random rnd_;
    bool fail_reads_;
  };

  Status NewSequentialFile(const std::string& f, unique_ptr<SequentialFile>* r,
                           const EnvOptions& options) override {
    MutexLock l(&mutex_);
    if (dummy_sequential_file_) {
      r->reset(
          new TestEnv::DummySequentialFile(dummy_sequential_file_fail_reads_));
      return Status::OK();
    } else {
      return EnvWrapper::NewSequentialFile(f, r, options);
    }
  }

  Status NewWritableFile(const std::string& f, unique_ptr<WritableFile>* r,
                         const EnvOptions& options) override {
    MutexLock l(&mutex_);
    written_files_.push_back(f);
    if (limit_written_files_ <= 0) {
      return Status::NotSupported("Sorry, can't do this");
    }
    limit_written_files_--;
    return EnvWrapper::NewWritableFile(f, r, options);
  }

  virtual Status DeleteFile(const std::string& fname) override {
    MutexLock l(&mutex_);
    if (fail_delete_files_) {
      return Status::IOError();
    }
    EXPECT_GT(limit_delete_files_, 0U);
    limit_delete_files_--;
    return EnvWrapper::DeleteFile(fname);
  }

  virtual Status DeleteDir(const std::string& dirname) override {
    MutexLock l(&mutex_);
    if (fail_delete_files_) {
      return Status::IOError();
    }
    return EnvWrapper::DeleteDir(dirname);
  }

  void AssertWrittenFiles(std::vector<std::string>& should_have_written) {
    MutexLock l(&mutex_);
    std::sort(should_have_written.begin(), should_have_written.end());
    std::sort(written_files_.begin(), written_files_.end());

    ASSERT_EQ(should_have_written, written_files_);
  }

  void ClearWrittenFiles() {
    MutexLock l(&mutex_);
    written_files_.clear();
  }

  void SetLimitWrittenFiles(uint64_t limit) {
    MutexLock l(&mutex_);
    limit_written_files_ = limit;
  }

  void SetLimitDeleteFiles(uint64_t limit) {
    MutexLock l(&mutex_);
    limit_delete_files_ = limit;
  }

  void SetDeleteFileFailure(bool fail) {
    MutexLock l(&mutex_);
    fail_delete_files_ = fail;
  }

  void SetDummySequentialFile(bool dummy_sequential_file) {
    MutexLock l(&mutex_);
    dummy_sequential_file_ = dummy_sequential_file;
  }
  void SetDummySequentialFileFailReads(bool dummy_sequential_file_fail_reads) {
    MutexLock l(&mutex_);
    dummy_sequential_file_fail_reads_ = dummy_sequential_file_fail_reads;
  }

  void SetGetChildrenFailure(bool fail) { get_children_failure_ = fail; }
  Status GetChildren(const std::string& dir,
                     std::vector<std::string>* r) override {
    if (get_children_failure_) {
      return Status::IOError("SimulatedFailure");
    }
    return EnvWrapper::GetChildren(dir, r);
  }

//有些测试用例实际上并不创建测试文件（例如，请参见
//dummydb:：live_files——对于这些情况，我们模拟这些文件的属性。
//因此createNewbackup（）可以获取它们的属性。
  void SetFilenamesForMockedAttrs(const std::vector<std::string>& filenames) {
    filenames_for_mocked_attrs_ = filenames;
  }
  Status GetChildrenFileAttributes(
      const std::string& dir, std::vector<Env::FileAttributes>* r) override {
    if (filenames_for_mocked_attrs_.size() > 0) {
      for (const auto& filename : filenames_for_mocked_attrs_) {
        /*向后推（dir+filename，10/*大小字节*/）；
      }
      返回状态：：OK（）；
    }
    返回envwrapper:：getchildrenfileattributes（dir，r）；
  }
  status getfilesize（const std:：string&path，uint64_t*size_bytes）override_
    if（文件名_for_mocked_attrs_.size（）>0）
      auto-fname=path.substr（path.find_last_of（'/'）；
      auto filename_iter=std：：find（文件名_表示模拟的_attrs_.begin（），
                                     文件名_for_mocked_attrs_.end（），fname）；
      如果（文件名为ITER！=文件名_for_mocked_attrs_.end（））
        *大小字节=10；
        返回状态：：OK（）；
      }
      返回状态：未找到（fname）；
    }
    返回envwrapper:：getfilesize（path，size_bytes）；
  }

  void setcreatedirifmissingfailure（bool fail）
    如果缺少故障，创建目录；
  }
  状态createdirifmissing（const std:：string&d）override
    if（创建_dir_if_missing_failure_
      返回状态：：ioerror（“SimulatedFailure”）；
    }
    返回envwrapper:：createdirifmissing（d）；
  }

  void setnewdirectoryfailure（bool fail）new_directory_failure_=fail；
  虚拟状态newdirectory（const std:：string&name，
                              唯一_ptr<directory>>*result）override_
    if（new_directory_failure_
      返回状态：：ioerror（“SimulatedFailure”）；
    }
    返回envwrapper:：newdirectory（name，result）；
  }

 私人：
  端口：：mutex mutex_uu；
  bool dummy_sequential_file_uu=false；
  bool dummy_sequential_file_fail_reads_u=false；
  std:：vector<std:：string>written_u files；
  std:：vector<std:：string>文件名\u表示模拟的\u属性\；
  uint64_t limit_written_files_uu1000000；
  uint64_t limit_delete_files_uu1000000；
  bool fail_delete_files_uu=false；

  bool get_children_failure_uu=false；
  bool create_dir_if_missing_failure_u=false；
  bool new_directory_failure_uu=false；
}；// TESTNOV

类文件管理器：public envwrapper_
 公众：
  显式文件管理器（env*t）：envwrapper（t），rnd_

  status deleterandomfileindir（const std:：string&dir）
    std:：vector<std:：string>children；
    getchildren（dir和children）；
    if（children.size（）<=2）//。并且…
      返回状态：：未找到（“”）；
    }
    当（真）{
      int i=rnd_u.next（）%children.size（）；
      如果（孩子们[我]！=“”&&children[i]！=“…”）{
        返回deletefile（dir+“/”+children[i]）；
      }
    }
    //不应该到这里
    断言（假）；
    返回状态：：未找到（“”）；
  }

  状态appendtorandomfileindir（const std:：string&dir，
                                 const std：：字符串和数据）
    std:：vector<std:：string>children；
    getchildren（dir和children）；
    if（children.size（）<=2）
      返回状态：：未找到（“”）；
    }
    当（真）{
      int i=rnd_u.next（）%children.size（）；
      如果（孩子们[我]！=“”&&children[i]！=“…”）{
        返回writetofile（dir+“/”+children[i]，data）；
      }
    }
    //不应该到这里
    断言（假）；
    返回状态：：未找到（“”）；
  }

  状态损坏文件（const std:：string&fname，uint64_t bytes_to_corrupt）
    std：：字符串文件目录；
    状态S=readfiletoString（this、fname和file_contents）；
    如果（！）S.O.（））{
      返回S；
    }
    s=删除文件（fname）；
    如果（！）S.O.（））{
      返回S；
    }

    for（uint64_t i=0；i<bytes_to_corrupt；++i）
      std：：字符串tmp；
      测试：：randomstring（&rnd_u1，&tmp）；
      file_contents[rnd_.next（）%file_contents.size（）]=tmp[0]；
    }
    返回writetofile（fname，文件目录）；
  }

  状态corruptchecksum（const std:：string&fname，bool显示为“有效”）；
    std：：字符串元数据；
    状态s=readfiletoString（this、fname和metadata）；
    如果（！）S.O.（））{
      返回S；
    }
    s=删除文件（fname）；
    如果（！）S.O.（））{
      返回S；
    }

    auto pos=metadata.find（“private”）；
    if（pos==std:：string:：npos）
      返回状态：：损坏（“需要专用文件”）；
    }
    pos=metadata.find（“crc32”，pos+6）；
    if（pos==std:：string:：npos）
      返回状态：：损坏（“未找到校验和”）；
    }

    if（metadata.size（）<pos+7）
      返回状态：：损坏（“CRC32校验和值错误”）；
    }

    如果（似乎有效）
      if（元数据[pos+8]='\n'）
        //单个数字值，可以安全地再插入一个数字
        元数据。插入（pos+8，1，'0'）；
      }否则{
        元数据。擦除（pos+8，1）；
      }
    }否则{
      元数据[pos+7]='a'；
    }

    返回writetofile（fname，元数据）；
  }

  状态写入文件（const std:：string&fname，const std:：string&data）
    唯一的文件；
    环境选项环境选项；
    env？options.use？mmap？writes=false；
    状态s=envwrapper:：newwritablefile（fname，&file，env_选项）；
    如果（！）S.O.（））{
      返回S；
    }
    返回文件->追加（切片（数据））；
  }

 私人：
  随机RNDI；
//文件管理器

//实用程序函数
静态大小_t filldb（db*db，int-from，int-to）
  大小T字节写入=0；
  对于（int i=from；i<to；++i）
    std：：string key=“testkey”+to字符串（i）；
    std：：string value=“testvalue”+to字符串（i）；
    bytes_written+=key.size（）+value.size（）；

    期望_OK（db->put（writeOptions（），slice（key），slice（value）））；
  }
  返回写入的字节数；
}

静态void断言存在（db*db，int-from，int-to）
  对于（int i=from；i<to；++i）
    std：：string key=“testkey”+to字符串（i）；
    std：：字符串值；
    状态s=db->get（readoptions（），slice（key），&value）；
    断言“eq（value，”“testvalue”+to字符串（i））；
  }
}

静态void断言空（db*db，int-from，int-to）
  对于（int i=from；i<to；++i）
    std：：string key=“testkey”+to字符串（i）；
    std：：string value=“testvalue”+to字符串（i）；

    状态s=db->get（readoptions（），slice（key），&value）；
    断言“真”（s.isNotFound（））；
  }
}

类backupabledbtest:公共测试：：test
 公众：
  backupabledbtest（）
    /设置文件
    std:：string db_chroot=test:：tmpdir（）+“/backupable_db”；
    std:：string backup_chroot=test:：tmpdir（）+“/backupable_db_backup”；
    env:：default（）->createdir（db_chroot）；
    env：：default（）->createdir（备份目录）；
    dbname_u=“/tempdb”；
    backupdir_u=“/tempbk”；

    /设置EVS
    db_chroot_env_u.reset（newchrootenv（env:：default（），db_chroot））；
    backup_chroot_env_u.reset（newchrootenv（env:：default（），backup_chroot））；
    test_db_env_u.reset（new test env（db_chroot_env_u.get（））；
    test_backup_env_u.reset（new test env（backup_chroot_env_u.get（））；
    文件管理器重置（new file manager（backup_chroot_env_u.get（））；

    //设置数据库选项
    选项u.create_if_missing=true；
    选项u.paranoid_checks=true；
    选项写入缓冲区大小=1<<17；//128kb
    options_u.env=test_db_env_u.get（）；
    选项u.wal_dir=dbname_u；

    //创建记录器
    dboptions logger_选项；
    logger_options.env=db_chroot_env_u.get（）；
    创建loggerfromoptions（dbname_uu，logger_options，&logger_u）；

    //设置备份数据库选项
    可备份选项重置（新的可备份选项（
        backupdir_u，test_backup_env_u.get（），true，logger_u.get（），true））；

    //大多数测试将使用多线程备份
    可备份的_Options_u->Max_background_operations=7；

    //删除数据库中的旧文件
    DestroyDB（dbname_uu，选项_u）；
  }

  db*opdB（）{
    DB＊DB；
    期望_OK（db:：open（选项_uu，dbname_u，&db））；
    返回数据库；
  }

  void opendbandbackupengineeshareewithchecksum（
      bool destroy_old_data=false，bool dummy=false，
      bool share_table_files=true，bool share_with_checksums=false）
    可备份的_选项_->share_files_with_checksum=share_with_checksums；
    opendbandbackupengine（销毁旧数据，虚拟，与校验和共享）；
  }

  void opendbandbackupengine（bool destroy_old_data=false，bool dummy=false，
                             bool share_table_files=true）
    //重置所有默认值
    测试备份文件（1000000）；
    测试_db_env_uu->setlimitwrittenfiles（1000000）；
    test_db_env_u->setdummy序列文件（dummy）；

    DB＊DB；
    如果（哑）{
      dummy_db_u=new dummy db（选项_uu，dbname_u）；
      DB＝DuMyMyBuz；
    }否则{
      断言ou ok（db:：open（options_u，dbname_uu，&db））；
    }
    重置（DB）；
    可备份的\选项->销毁旧的\数据=销毁旧的\数据；
    backupable_options_u->share_table_files=share_table_files；
    备份引擎*备份引擎；
    断言\u OK（backupengine:：open（test_db_env_u.get（），*backupable_options_，
                                 &backup_engine））；
    备份引擎重置（备份引擎）；
  }

  void closedbandbackupengine（）
    dB.ReSET（）；
    备份引擎重置（）；
  }

  void openbackupengine（）
    backupable_options_uu->destroy_old_data=false；
    备份引擎*备份引擎；
    断言\u OK（backupengine:：open（test_db_env_u.get（），*backupable_options_，
                                 &backup_engine））；
    备份引擎重置（备份引擎）；
  }

  void closebackupengine（）backup_engine_u.reset（nullptr）；

  //还原备份备份\id并断言存在
  //[开始存在，结束存在>且不存在
  //[结束存在，结束>
  / /
  //如果backup_id==0，则表示从最新版本还原
  //如果end==0，则不检查assertEmpty
  void assertbackupconsistency（backup id backup_id，uint32_t start_exist，
                               uint32_t end_exist，uint32_t end=0，
                               bool keep_log_files=false）
    恢复选项恢复选项（保留日志文件）；
    bool opened_backup_engine=false；
    if（备份引擎get（）==nullptr）
      打开的备份引擎=真；
      openbackupengine（）；
    }
    如果（backup_id>0）
      断言“OK”（备份引擎）->restoredbfrombackup（备份ID、dbname、dbname）
                                                    恢复选项）；
    }否则{
      断言“OK”（备份引擎）->restoredbfromlatestbackup（dbname_uu，dbname_uu，
                                                          恢复选项）；
    }
    db*db=opendb（）；
    断言存在（db，start_exist，end_exist）；
    如果（结束）！= 0）{
      断言空（db，end_exist，end）；
    }
    删除数据库；
    如果（打开的备用引擎）
      closebackupengine（）；
    }
  }

  void deletelogfiles（）
    std:：vector<std:：string>删除日志；
    db_chroot_env_u->getchildren（dbname_uu，&delete_logs）；
    for（auto f:删除日志）
      uint64_t编号；
      文件类型；
      bool ok=parseFileName（f，&number，&type）；
      if（确定&&type==klogfile）
        db_chroot_env_u->deletefile（dbname_+“/”+f）；
      }
    }
  }

  //文件
  std：：字符串dbname_u；
  std：：string backupdir_u；

  //记录器_u必须位于备用_u引擎_u之上，这样引擎的析构函数，
  //使用记录器的原始指针，首先执行。
  std:：shared&ptr<logger>logger；

  //eNVS
  唯一_ptr<env>db_chroot_env；
  唯一_ptr<env>backup_chroot_env；
  唯一的测试
  唯一的_ptr<test env>test_backup_env；
  唯一的文件管理器

  /所有DBS！
  dummy db*dummy_db_；//backupabledb拥有dummy_db_
  唯一的指针<db>db；
  唯一的引擎

  //选项
  选项选项

 受保护的：
  唯一的备份选项
//备份表测试

void appendpath（const std:：string&path，std:：vector<std:：string>>&v）
  对于（auto&f:v）
    f＝路径+f；
  }
}

类backupabledbtestwithparam:public backupabledbtest，
                                  公共测试：：withParamInterface<bool>
 公众：
  backupableDBTestWithParam（）
    可备份的_options_u->share_files_with_checksum=getparam（）；
  }
}；

//此测试验证verifybackup方法是否正确标识
//无效备份
测试_p（backupabledbtestwitparam，verifybackup）
  const int keys_迭代=5000；
  随机RND（6）；
  状态S；
  OpenDBandBackupEngine（真）；
  //创建五个备份
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  }
  closedbandbackupengine（）；

  opendbandbackupengine（）；
  //——————案例1。-有效备份-----------
  断言_true（backup_engine_uu->verifybackup（1）.ok（））；

  //——————案例2。-删除文件-----------i
  文件\u manager_u->deleterandomfileindir（backupdir_+“/private/1”）；
  断言_true（backup_engine_uu->verifybackup（1）.isNotFound（））；

  //——————第3种情况。-损坏文件-----------
  std:：string append_data=“损坏随机文件”；
  文件\u manager_u->appendtorandomfileindir（backupdir_+“/private/2”，
                                         附录数据；
  断言“真”（backup_engine_uu->verifybackup（2）.iscorrution（））；

  //——————第4种情况。-无效备份-----------
  断言_true（backup_engine_uu->verifybackup（6）.isNotFound（））；
  closedbandbackupengine（）；
}

//打开db，写入，关闭db，备份，还原，重复
测试_p（backupabledbtestwitparam，离线集成测试）
  //必须是一个大数字，以便触发memtable刷新
  const int keys_迭代=5000；
  const int max_key=keys_迭代*4+10；
  //第一个iter--备份前刷新
  //第二个iter——备份前不要刷新
  对于（int iter=0；iter<2；+iter）
    //删除旧数据
    DestroyDB（dbname_uu，选项_u）；
    bool destroy_data=true；

    //每次迭代--
    / / 1。在数据库中插入新数据
    / / 2。备份数据库
    / / 3。销毁数据库
    / / 4。恢复数据库，检查所有内容是否仍然存在
    对于（int i=0；i<5；+i）
      //在上一次迭代中，放入较少的数据，
      int fill_up_to=std:：min（keys_iteration*（i+1），max_key）；
      //---插入新数据并备份----
      opendbandbackupengine（销毁数据）；
      销毁数据=假；
      fillDB（db_u.get（），keys_iteration*i，fill_up_to）；
      断言_OK（backup_engine_u->createNewbackup（db_.get（），iter==0））；
      closedbandbackupengine（）；
      DestroyDB（dbname_uu，选项_u）；

      //---确保它是空的----
      db*db=opendb（）；
      断言空（db，0，填充到）；
      删除数据库；

      //---恢复数据库----
      openbackupengine（）；
      如果（i>=3）//测试清除旧备份
        //当i==4时，清除到只有1个备份
        //当i==3时，清除到2个备份
        断言“OK（备份引擎）”->purgeoldbackups（5-i））；
      }
      //---确保数据存在---
      断言备份一致性（0，0，填充到，最大值键）；
      closebackupengine（）；
    }
  }
}

//打开db，write，backup，write，backup，close，restore
测试_p（backupabledbtestwitparam，onlineintegrationtest）
  //必须是一个大数字，以便触发memtable刷新
  const int keys_迭代=5000；
  const int max_key=keys_迭代*4+10；
  随机RND（7）；
  //删除旧数据
  DestroyDB（dbname_uu，选项_u）；

  OpenDBandBackupEngine（真）；
  //写一些数据，备份，重复
  对于（int i=0；i<5；+i）
    如果（i＝4）{
      //删除备份号2，在线删除！
      断言“OK（备份引擎）”->DeleteBackup（2））；
    }
    //在上一次迭代中，放入较少的数据，
    //以便备份可以共享sst文件
    int fill_up_to=std:：min（keys_iteration*（i+1），max_key）；
    fillDB（db_u.get（），keys_iteration*i，fill_up_to）；
    //我们应该在备份之前得到与flush一致的结果
    //同时设置为true和false
    断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  }
  //关闭并销毁
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；

  //---确保它是空的----
  db*db=opendb（）；
  断言空（db，0，max_key）；
  删除数据库；

  //---还原每个备份并验证所有数据是否存在----
  openbackupengine（）；
  对于（int i=1；i<=5；++i）
    如果（i＝2）{
      //我们删除了备份2
      状态S=backup_engine_u->restoredbfrombackup（2，dbname_uu，dbname_u）；
      断言是真的（！）；
    }否则{
      int fill_up_to=std:：min（keys_iteration*i，max_key）；
      断言备份一致性（i，0，fill-up-to，max-u键）；
    }
  }

  //删除一些备份--这应该只保留备份3和5的活动状态
  断言“OK（备份引擎）”删除备份（4））；
  断言_OK（备份_引擎_u->purgeoldbackups（2））；

  std:：vector<backup info>backup_info；
  backup_engine_u->getbackupinfo（&backup_info）；
  断言_eq（2ul，backup_info.size（））；

  //检查备份3
  断言备份一致性（3，0，3*键迭代，最大键）；
  //检查备份5
  断言备份一致性（5，0，max_key）；

  closebackupengine（）；
}

实例化_test_case_p（backupabledbtestwitparam，backupabledbtestwitparam，
                        ：：测试：：bool（））；

//这将确保备份不会复制同一文件两次
测试（backupabledbtest，nodoubleCopy）
  OpenDBandBackupEngine（真，真）；

  //应该写入5 db文件+一个元文件
  测试备份文件（7）；
  test_backup_env_u->clearwrittenfiles（）；
  测试_db_env_uu->setlimitwrittenfiles（0）；
  虚拟文件u db_u->live_files_u=“/00010.sst”，“/00011.sst”，“/current”，
                            “/清单-01”
  虚拟_db_->wal_文件_123;_“/00011.log”，真，“/00012.log”，假_
  测试_db_env->setfilenamesformockedattrs（虚拟_db_uu->live_文件）；
  断言_OK（backup_engine_u->createNewbackup（db_.get（），false））；
  std:：vector<std:：string>应该有
      “/shared/00010.sst.tmp”，“/shared/00011.sst.tmp”，
      “/private/1.tmp/current”，“/private/1.tmp/manifest-01”，
      “/private/1.tmp/00011.log”，“/meta/1.tmp”
  appendpath（backupdir_u，应该已经写过了）；
  测试_backup_env_u->assertwrittenfiles（应_已写入）；

  //应该写入4个新的DB文件+一个元文件
  //不应该写/复制00010.sst，因为它已经存在了！
  测试备份文件（6）；
  test_backup_env_u->clearwrittenfiles（）；

  虚拟文件u db_u->live_files_u=“/00010.sst”，“/00015.sst”，“/current”，
                            “/清单-01”
  虚拟_db_->wal_文件_123;_“/00011.log”，真，“/00012.log”，假_
  测试_db_env->setfilenamesformockedattrs（虚拟_db_uu->live_文件）；
  断言_OK（backup_engine_u->createNewbackup（db_.get（），false））；
  //不应该打开00010.sst-它已经在那里了

  应该有“/shared/00015.sst.tmp”，“/private/2.tmp/current”，
                         “/private/2.tmp/manifest-01”，
                         “/private/2.tmp/00011.log”，“/meta/2.tmp”
  appendpath（backupdir_u，应该已经写过了）；
  测试_backup_env_u->assertwrittenfiles（应_已写入）；

  断言“OK（备份引擎）”->DeleteBackup（1））；
  断言_OK（test_backup_env_u->fileexists（backupdir_+“/shared/00010.sst”）；

  //00011.sst仅在备份1中，应删除
  断言eq（状态：：NotFound（），
            test_backup_env_u->fileexists（backupdir_+“/shared/00011.sst”）；
  断言_OK（test_backup_env_u->fileexists（backupdir_+“/shared/00015.sst”）；

  //清单文件大小应仅为100
  UTIN 64 t尺寸；
  测试备份环境获取文件大小（backupdir“/private/2/manifest-01”，&size）；
  断言eq（100ul，大小）；
  测试备份环境获取文件大小（backupdir_+“/shared/00015.sst”，&size）；
  断言eq（200ul，大小）；

  closedbandbackupengine（）；
}

//测试可能发生的各种损坏：
/ / 1。无法写入备份文件-该备份应失败，
//其他一切都能正常工作
/ / 2。备份元文件损坏或缺少备份文件-我们应该
//无法打开该备份，但所有其他备份都应
/ /罚款
/ / 3。校验和值损坏-如果校验和不是有效的uint32_t，
//db open应该失败，否则在还原过程中会中止。
测试（backupabledbtest，corruptiontest）
  const int keys_迭代=5000；
  随机RND（6）；
  状态S；

  OpenDBandBackupEngine（真）；
  //创建五个备份
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  }

  //——————案例1。-写入失败-----------
  //尝试创建备份6，但写入失败
  fillDB（db_u.get（），keys_iteration*5，keys_iteration*6）；
  测试备份文件（2）；
  /应该失败
  s=backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  断言是真的（！）；
  测试备份文件（1000000）；
  //最新备份应具有所有密钥
  closedbandbackupengine（）；
  断言备份一致性（0，0，keys_iteration*5，keys_iteration*6）；

  //——————案例2。备份元数据损坏或缺少备份文件----
  断言_OK（文件_Manager_->corruptfile（backupdir_+“/meta/5”，3））；
  //由于5元现在已损坏，最新备份应为4
  断言备份一致性（0，0，keys_iteration*4，keys_iteration*5）；
  openbackupengine（）；
  S=backup_engine_u->restoredbfrombackup（5，dbname_uu，dbname_u）；
  断言是真的（！）；
  closebackupengine（）；
  断言_OK（文件_Manager_u->DeleteRandomFileIndir（backupdir_+“/private/4”））；
  //4已损坏，3是最新的备份
  断言备份一致性（0，0，键迭代*3，键迭代*5）；
  openbackupengine（）；
  S=backup_engine_u->restoredbfrombackup（4，dbname_uu，dbname_u）；
  closebackupengine（）；
  断言是真的（！）；

  //——————案例3。损坏的校验和值----
  断言_OK（文件_manager_u->corruptchecksum（backupdir_+“/meta/3”，false））；
  //备份3的校验和是无效值，可以在
  //数据库打开时间，自动恢复到上一次备份
  断言备份一致性（0，0，keys_迭代*2，keys_迭代*5）；
  //备份2的校验和似乎有效，这可能导致校验和
  //不匹配并中止还原过程
  断言_OK（文件_Manager_u->corruptchecksum（backupdir_+“/meta/2”，true））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/2”））；
  openbackupengine（）；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/2”））；
  S=backup_engine_u->restoredbfrombackup（2，dbname_uu，dbname_u）；
  断言是真的（！）；

  //确保没有实际删除损坏的备份！
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/1”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/2”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/3”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/4”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/5”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/1”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/2”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/3”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/4”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/5”））；

  //删除损坏的备份，然后确保它们确实已被删除
  断言“OK（备份引擎）”->DeleteBackup（5））；
  断言“OK（备份引擎）”删除备份（4））；
  断言“OK（备份引擎）”->DeleteBackup（3））；
  断言“OK（备份引擎）”->DeleteBackup（2））；
  （void）备份引擎_u->garbageCollect（）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/meta/5”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/private/5”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/meta/4”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/private/4”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/meta/3”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/private/3”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/meta/2”）；
  断言eq（状态：：NotFound（），
            文件_Manager_->fileexists（backupdir_+“/private/2”）；

  closebackupengine（）；
  断言备份一致性（0，0，keys_iteration*1，keys_iteration*5）；

  //新备份应为2！
  opendbandbackupengine（）；
  fillDB（db_u.get（），keys_iteration*1，keys_iteration*2）；
  断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  closedbandbackupengine（）；
  断言备份一致性（2，0，keys_迭代*2，keys_迭代*5）；
}

测试（backupabledbtest，interruptcreationest）
  //通过失败的新写入和清除
  //部分状态。然后验证后续备份是否仍然可以成功。
  const int keys_迭代=5000；
  随机RND（6）；

  opendbandbackupengine（真/*销毁旧数据*/);

  FillDB(db_.get(), 0, keys_iteration);
  test_backup_env_->SetLimitWrittenFiles(2);
  test_backup_env_->SetDeleteFileFailure(true);
//创建失败
  ASSERT_FALSE(
      backup_engine_->CreateNewBackup(db_.get(), !!(rnd.Next() % 2)).ok());
  CloseDBAndBackupEngine();
//也应清除失败，以便tmp目录保持在后面
  ASSERT_OK(backup_chroot_env_->FileExists(backupdir_ + "/private/1.tmp/"));

  /*ndbandbackupengine（错误/*销毁旧的数据*/）；
  测试备份文件（1000000）；
  测试_backup_env_uu->setDeleteFileFailure（假）；
  断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  //最新备份应具有所有密钥
  closedbandbackupengine（）；
  断言备份一致性（0，0，键迭代）；
}

inline std:：string optionspath（std:：string ret，int backupid）
  ret+=“/private/”；
  ret+=std：：to_string（backupid）；
  RET+=“/”；
  返回RET；
}

//将最新的选项文件备份到
/“<backup_dir>/private/<backup_id>/options<number>”

测试（backupabledbtest，backupoptions）
  OpenDBandBackupEngine（真）；
  对于（int i=1；i<5；i++）
    std：：字符串名称；
    std:：vector<std:：string>文件名；
    //必须重置（）才能再次重置（opendb（））。
    //当存在*db_u时调用opendb（）将导致锁定问题
    dB.ReSET（）；
    数据库重置（opendb（））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
    rocksdb:：getlatestoptionsfilename（db_u->getname（），options_.env，&name）；
    断言_OK（文件_Manager_u->fileexists（optionspath（backupdir_u，i）+name））；
    backup_chroot_env_u->getchildren（选项空间（backupdir_uu，i），&filename）；
    for（auto-fn:文件名）
      if（fn.比较（0，7，“选项”）==0）
        断言eq（名称，fn）；
      }
    }
  }

  closedbandbackupengine（）；
}

//此测试验证在只读选项为
//集
test_f（backupabledbtest，nodeletewithreadonly）
  const int keys_迭代=5000；
  随机RND（6）；
  状态S；

  OpenDBandBackupEngine（真）；
  //创建五个备份
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（rnd.next（）%2））；
  }
  closedbandbackupengine（）；
  断言_OK（文件_Manager_u->writetofile（backupdir_+“/latest_backup”，“4”））；

  backupable_options_uu->destroy_old_data=false；
  备份引擎只读*只读\备份\引擎；
  断言“确定”（backupengineradonly:：open（backup_chroot_env_u.get（），
                                       *可备份选项
                                       &read_only_backup_engine））；

  //断言备份5中的数据仍然存在（即使最新的备份
  //说4是最新的）
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/meta/5”））；
  断言_OK（文件_Manager_u->fileexists（backupdir_+“/private/5”））；

  //行为更改：我们现在忽略最新的备份内容。这意味着
  //我们应该有5个备份，即使最新的备份为4。
  std:：vector<backup info>backup_info；
  只读\u backup\u engine->getbackupinfo（&backup\u info）；
  断言_eq（5ul，backup_info.size（））；
  删除只读备份引擎；
}

测试（backupabledbtest，failoverwritingbackups）
  选项写入缓冲区大小=1024*1024*1024；//1GB
  选项_u.disable_auto_compactions=true；

  //创建备份1、2、3、4、5
  OpenDBandBackupEngine（真）；
  对于（int i=0；i<5；+i）
    closedbandbackupengine（）；
    删除日志文件（）；
    opendbandbackupengine（错误）；
    fillDB（db_.get（），100*i，100*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  }
  closedbandbackupengine（）；

  //恢复3
  openbackupengine（）；
  断言_OK（备份引擎_u->restoredbfrombackup（3，dbname_u，dbname_u））；
  closebackupengine（）；

  opendbandbackupengine（错误）；
  FillDB（db_u.get（），0，300）；
  状态S=backup_engine_u->createNewbackup（db_.get（），true）；
  //新备份失败，因为新表文件
  //与备份4和5中的旧表文件冲突
  /（由于写缓冲区的大小很大，我们可以确定
  //每个备份只生成一个sst文件，
  //新备份生成的文件与
  //由备份4生成的sst文件）
  断言“真”（s.iscorrution（））；
  断言“OK（备份引擎）”删除备份（4））；
  断言“OK（备份引擎）”->DeleteBackup（5））；
  //现在，备份可以成功
  断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  closedbandbackupengine（）；
}

测试（backupabledbtest，nosharetablefiles）
  const int keys_迭代=5000；
  OpenDBandBackupEngine（真、假、假）；
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（i % 2））；
  }
  closedbandbackupengine（）；

  对于（int i=0；i<5；+i）
    断言备份一致性（i+1，0，键迭代*（i+1）
                            键_迭代*6）；
  }
}

//验证您是否可以使用共享文件进行备份和还原，并启用校验和
测试（backupabledbtest，shareTablefilewithchecksums）
  const int keys_迭代=5000；
  OpenDBandBackupEngineShareWith校验和（真、假、真、真）；
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_u.get（），！！（i % 2））；
  }
  closedbandbackupengine（）；

  对于（int i=0；i<5；+i）
    断言备份一致性（i+1，0，键迭代*（i+1）
                            键_迭代*6）；
  }
}

//验证是否可以使用共享文件进行备份和还原，并将校验和设置为
//false，然后将此选项转换为true
test_f（backupabledbtest，shareTablefilewithchecksumstration）
  const int keys_迭代=5000；
  //将share_files_with_checksum设置为false
  OpenDBandBackupEngineShareWith校验和（真、假、真、假）；
  对于（int i=0；i<5；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  }
  closedbandbackupengine（）；

  对于（int i=0；i<5；+i）
    断言备份一致性（i+1，0，键迭代*（i+1）
                            键_迭代*6）；
  }

  //将share_files_with_checksum设置为true并执行更多备份
  OpenDBandBackupEngineShareWith校验和（真、假、真、真）；
  对于（int i=5；i<10；+i）
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  }
  closedbandbackupengine（）；

  对于（int i=0；i<5；+i）
    断言备份一致性（i+1，0，键迭代*（i+5+1）
                            按键重复*11）；
  }
}

测试（backupabledbtest，deletetmpfiles）
  for（bool shared_checksum:false，true）
    if（共享校验和）
      opendbandbackupengineeshareewithchecksum（
          错误/*销毁旧数据*/, false /* dummy */,

          /*e/*共享_table_files*/，true/*与_checksums*/）共享_；
    }否则{
      opendbandbackupengine（）；
    }
    closedbandbackupengine（）；
    std：：string shared_tmp=backupdir_u；
    if（共享校验和）
      shared_tmp+=“/shared_checksum”；
    }否则{
      shared_tmp+=“/shared”；
    }
    shared_tmp+=“/00006.sst.tmp”；
    std：：string private_tmp_dir=backupdir_+“/private/10.tmp”；
    std：：string private_tmp_file=private_tmp_dir+“/00003.sst”；
    文件管理器->writetofile（shared_tmp，“tmp”）；
    文件管理器创建目录（private目录）；
    file_manager_u->writetofile（private_tmp_file，“tmp”）；
    断言_OK（文件_manager_u->fileexists（private_tmp_dir））；
    if（共享校验和）
      opendbandbackupengineeshareewithchecksum（
          错误/*销毁旧数据*/, false /* dummy */,

          /*e/*共享_table_files*/，true/*与_checksums*/）共享_；
    }否则{
      opendbandbackupengine（）；
    }
    //需要显式调用此函数来删除tmp文件
    （void）备份引擎_u->garbageCollect（）；
    closedbandbackupengine（）；
    断言_eq（status:：notfound（），文件_Manager_u->fileexists（shared_tmp））；
    断言_eq（status:：notfound（），文件_Manager_u->fileexists（private_tmp_file））；
    断言_eq（status:：notfound（），file_manager_u->fileexists（private_tmp_dir））；
  }
}

test_f（backupabledbtest，keeploggfiles）
  backupable_options_u->backup_log_files=false；
  //基本上是无限的
  选项_u.wal_ttl_seconds=24*60*60；
  OpenDBandBackupEngine（真）；
  FillDB（db_u.get（），0，100）；
  断言_OK（db_->flush（flushoptings（））；
  FillDB（db_u.get（），100，200）；
  断言_OK（backup_engine_u->createNewbackup（db_.get（），false））；
  FillDB（db_u.get（），200，300）；
  断言_OK（db_->flush（flushoptings（））；
  FillDB（db_u.get（），300，400）；
  断言_OK（db_->flush（flushoptings（））；
  FillDB（db_u.get（），400，500）；
  断言_OK（db_->flush（flushoptings（））；
  closedbandbackupengine（）；

  //如果调用keep_log_files=true，则所有数据都应该存在
  断言备份一致性（0，0，500，600，真）；
}

测试_f（backupabledbtest，ratelimiting）
  尺寸常数kmicrospersec=1000*1000ll；
  uint64常数mb=1024*1024；

  const std:：vector<std:：pair<uint64_t，uint64_t>>限制（
      1*MB，5*MB，2*MB，3*MB）；

  std:：shared_ptr<ratelimiter>backupthrottler（newGenericRatelimiter（1））；
  std:：shared_ptr<ratelimiter>restorethrottler（newGenericRatelimiter（1））；

  for（bool makethrotler:假，真）
    如果（作节流器）
      backupable_options_u->backup_rate_limiter=backupthrottler；
      backupable_options_u->restore_rate_limiter=restorethrottler；
    }
    //iter 0——单线程
    //ITER 1——多线程
    对于（int iter=0；iter<2；+iter）
      for（const auto&limit:限制）
        //销毁旧数据
        destroyDB（dbname_u，options（））；
        如果（作节流器）
          backupthrottler->setbytespersecond（limit.first）；
          restorethrottler->setbytespersecond（limit.second）；
        }否则{
          backupable_options_uu->backup_rate_limit=limit.first；
          backupable_options_uu->restore_rate_limit=limit.second；
        }
        可备份的_Options_u->Max_background_operations=（iter==0）？1:10；
        选项.compression=knocompression；
        OpenDBandBackupEngine（真）；
        size_t bytes_written=filldb（db_u.get（），0，100000）；

        自动启动_backup=db_chroot_env_u->nowmicros（）；
        断言_OK（backup_engine_u->createNewbackup（db_.get（），false））；
        auto backup_time=db_chroot_env_u->nowmicros（）-启动备份；
        自动速率限制备份时间=
            （bytes-written*kmicrospersec）/limit.first；
        断言（备份时间，0.8*速率限制备份时间）；

        closedbandbackupengine（）；

        openbackupengine（）；
        auto start_restore=db_chroot_env_uu->nowmicros（）；
        断言“确定”（backup_engine_u->restoredbfromlatestbackup（dbname_uu，dbname_u））；
        auto restore_time=db_chroot_env_u->nowmicros（）-启动_restore；
        closebackupengine（）；
        自动速率限制恢复时间=
            （bytes-written*kmicrospersec）/limit.second；
        断言恢复时间，0.8*速率限制恢复时间；

        断言备份一致性（0，0，100000，100010）；
      }
    }
  }
}

测试（backupabledbtest，readonlybackupengine）
  DestroyDB（dbname_uu，选项_u）；
  OpenDBandBackupEngine（真）；
  FillDB（db_u.get（），0，100）；
  断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  FillDB（db_u.get（），100，200）；
  断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；

  backupable_options_uu->destroy_old_data=false；
  test_backup_env_u->clearwrittenfiles（）；
  测试备份文件（0）；
  备份引擎只读*只读\备份\引擎；
  断言“确定”（backupengineradonly:：open（
      db_chroot_env_u.get（），*backupable_options_u，&read_only_backup_engine））；
  std:：vector<backup info>backup_info；
  只读\u backup\u engine->getbackupinfo（&backup\u info）；
  断言eq（backup_info.size（），2u）；

  restore options恢复选项（false）；
  断言\u OK（只读\u备份\u引擎->restoredbfromlatestbackup（
      dbname_uu，dbname_u，restore_options））；
  删除只读备份引擎；
  std:：vector<std:：string>应该已经写过了；
  测试_backup_env_u->assertwrittenfiles（应_已写入）；

  db*db=opendb（）；
  断言存在（db，0，200）；
  删除数据库；
}

测试_f（backupabledbtest，progresscallbackduringbackup）
  DestroyDB（dbname_uu，选项_u）；
  OpenDBandBackupEngine（真）；
  FillDB（db_u.get（），0，100）；
  bool is_callback_invoked=false；
  断言“OK”（备份引擎）->CreateNewBackup（
      db_.get（），真，
      [&Is撏callback_invoked]（）Is撏callback_invoked=true；））；

  断言为“真”（调用了“回调”）；
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
}

测试_F（备份前可备份的DBTEST、GarbageCollection）
  DestroyDB（dbname_uu，选项_u）；
  OpenDBandBackupEngine（真）；

  backup_chroot_env_u->createddirifmissing（backupdir_+“/shared”）；
  std:：string file_five=backupdir_+“/shared/000007.sst”；
  std:：string file_five_contents=“我不是一个真正的sst文件”；
  //这取决于00007.sst是数据库创建的第一个文件
  断言“确定”（文件管理器->写入文件（文件五，文件五内容））；

  FillDB（db_u.get（），0，100）；
  //备份覆盖文件000007.sst
  断言_true（backup_engine_u->createNewbackup（db_.get（），true）.ok（））；

  std：：字符串新的_文件_五个_内容；
  断言“OK”（readfiletoString（backup_chroot_env_u.get（），file_five，
                             &new_file_five_contents）（&new_file_five_contents））；
  //文件000007.sst被覆盖
  断言“真”（新的“文件”内容）=文件目录；

  closedbandbackupengine（）；

  断言备份一致性（0，0，100）；
}

//测试我们是否正确传播env失败
测试_f（backupabledbtest，envfailures）
  备份引擎*备份引擎；

  //获取子级失败
  {
    测试_backup_env_u->setgetchildrenfailure（真）；
    断言\u nok（backupengine:：open（test_db_env_u.get（），*backupable_options）
                                  &backup_engine））；
    测试_backup_env_u->setgetchildrenfailure（false）；
  }

  //创建目录失败
  {
    测试_backup_env_uu->setcreateddirifmissingfailure（true）；
    断言\u nok（backupengine:：open（test_db_env_u.get（），*backupable_options）
                                  &backup_engine））；
    测试_backup_env_uu->setcreateddirifmissingfailure（false）；
  }

  //新建目录失败
  {
    测试备份目录失败（真）；
    断言\u nok（backupengine:：open（test_db_env_u.get（），*backupable_options）
                                  &backup_engine））；
    测试_backup_env_uu->setnewdirectoryfailure（false）；
  }

  //读取元文件失败
  {
    DestroyDB（dbname_uu，选项_u）；
    OpenDBandBackupEngine（真）；
    FillDB（db_u.get（），0，100）；
    断言_true（backup_engine_u->createNewbackup（db_.get（），true）.ok（））；
    closedbandbackupengine（）；
    测试备份文件（真）；
    测试备份文件故障读取（真）；
    backupable_options_uu->destroy_old_data=false；
    断言\u nok（backupengine:：open（test_db_env_u.get（），*backupable_options）
                                  &backup_engine））；
    测试备份文件（假）；
    测试备份文件故障读取（假）；
  }

  /无故障
  {
    断言\u OK（backupengine:：open（test_db_env_u.get（），*backupable_options_，
                                 &backup_engine））；
    删除备份引擎；
  }
}

//验证在使用旧的
/ /清单。
test_f（backupabledbtest，在备份创建期间更改清单）
  DestroyDB（dbname_uu，选项_u）；
  选项_u.max_manifest_file_size=0；//添加文件时始终滚动清单
  OpenDBandBackupEngine（真）；
  FillDB（db_u.get（），0，100）；

  rocksdb:：syncpoint:：getInstance（）->loadDependency（
      “checkpointimpl:：createChepoint:savedLiveFiles1”，
       “版本集：：LogandApply:WriteManifest”，
      “版本集：：日志和应用程序：WriteManifestDone”，
       “checkpointimpl:：createChepoint:savedLivefiles2”，
  （}）；
  rocksdb:：syncpoint:：getInstance（）->启用处理（）；

  rocksdb:：port:：thread flush_thread[此]（）assert_ok（db_->flush（flushoptions（））；

  断言_OK（backup_engine_u->createNewbackup（db_.get（），false））；

  flush_thread.join（）；
  rocksdb:：syncpoint:：getInstance（）->disableProcessing（）；

  //最后一个清单卷可能已经被完全扫描清除了
  //当CreateNewBackup调用EnableFileDeletions时会发生这种情况。我们需要
  //触发另一个滚动来验证非完全扫描清除过时的清单。
  db impl*db_impl=reinterpret_cast<db impl*>（db_.get（））；
  std:：string prev_manifest_path=
      描述符文件名（dbname_u，db_impl->test_current_manifest_fileno（））；
  FillDB（db_u.get（），0，100）；
  断言“OK”（db_chroot_env_uu->fileexists（prev_manifest_path））；
  断言_OK（db_->flush（flushoptings（））；
  assert_true（db_chroot_env_u->fileexists（prev_manifest_path）.isnotfound（））；

  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
  断言备份一致性（0，0，100）；
}

//参见https://github.com/facebook/rocksdb/issues/921
测试（backupabledbtest，issue921test）
  备份引擎*备份引擎；
  backupable_options_u->share_table_files=false；
  backup_chroot_env_uu->createdirifmissing（backupable_options_u->backup_dir）；
  backupable_options_u->backup_dir+=“/new_dir”；
  断言\u OK（backupengine:：open（backup_chroot_env_u.get（），*backupable_options_，
                               &backup_engine））；

  删除备份引擎；
}

测试（backupabledbtest，backupwithmetadata）
  const int keys_迭代=5000；
  OpenDBandBackupEngine（真）；
  //创建五个备份
  对于（int i=0；i<5；+i）
    const std:：string metadata=std:：to_string（i）；
    fillDB（db_u.get（），keys_迭代*i，keys_迭代*（i+1））；
    断言
        backup_engine_u->createnebackupwithmetadata（db_.get（），metadata，true））；
  }
  closedbandbackupengine（）；

  opendbandbackupengine（）；
  std:：vector<backupinfo>backup_infos；
  backup_engine_u->getbackupinfo（&backup_infos）；
  断言_eq（5，backup_infos.size（））；
  对于（int i=0；i<5；i++）
    断言eq（std:：to_string（i），backup_infos[i].app_metadata）；
  }
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
}

test_f（backupabledbtest，二进制元数据）
  OpenDBandBackupEngine（真）；
  std:：string binarymetadata=“abc\ndef”；
  binarymetadata.push_back（'\0'）；
  binarymetadata.append（“ghi”）；
  断言
      backup_engine_u->createnebackupwithmetadata（db_.get（），binarymetadata））；
  closedbandbackupengine（）；

  opendbandbackupengine（）；
  std:：vector<backupinfo>backup_infos；
  backup_engine_u->getbackupinfo（&backup_infos）；
  断言_eq（1，backup_infos.size（））；
  断言eq（二进制元数据，备份信息[0].应用程序元数据）；
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
}

测试（backupabledbtest，metadatatoolarge）
  OpenDBandBackupEngine（真）；
  std：：字符串LargeMetadata（1024*1024+1，0）；
  断言
      backup_engine_u->createnebackupwithmetadata（db_.get（），largemetadata））；
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
}

测试（backupabledbtest，limitbackupspopened）
  //验证指定的最大备份是否已打开，包括跳过
  //备份已损坏。
  / /
  //设置：
  //-备份1、2和4有效
  //-备份3已损坏
  //-最大有效备份数
  / /
  //期望值：引擎打开备份4和2，因为它们是最新的两个
  //未损坏的备份。
  const int knumkeys=5000；
  OpenDBandBackupEngine（真）；
  对于（int i=1；i<=4；++i）
    fillDB（db_.get（），knumkeys*i，knumkeys*（i+1））；
    断言_OK（backup_engine_u->createNewbackup（db_.get（），true））；
    如果（i＝3）{
      断言_OK（文件_Manager_u->corruptfile（backupdir_+“/meta/3”，3））；
    }
  }
  closedbandbackupengine（）；

  backupable_options_u->max_valid_backups_to_open=2；
  opendbandbackupengine（）；
  std:：vector<backupinfo>backup_infos；
  backup_engine_u->getbackupinfo（&backup_infos）；
  断言_eq（2，backup_infos.size（））；
  断言Eq（2，备份Infos[0].备份ID）；
  断言Eq（4，备份Infos[1].备份ID）；
  closedbandbackupengine（）；
  DestroyDB（dbname_uu，选项_u）；
}

//anon命名空间

//命名空间rocksdb

int main（int argc，char**argv）
  rocksdb:：port:：installstacktracehandler（）；
  ：：测试：：initgoogletest（&argc，argv）；
  返回run_all_tests（）；
}

否则
include<stdio.h>

int main（int argc，char**argv）
  fprintf（stderr，“rocksdb-lite中不支持跳过的backupabledb”）；
  返回0；
}

我很喜欢你！定义（RocksDB-Lite）&&！定义（OsWin）
