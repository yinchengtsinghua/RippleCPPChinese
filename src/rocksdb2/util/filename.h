
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
//
//数据库代码使用的文件名

#pragma once
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <vector>

#include "options/db_options.h"
#include "port/port.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/transaction_log.h"

namespace rocksdb {

class Env;
class Directory;
class WritableFileWriter;

enum FileType {
  kLogFile,
  kDBLockFile,
  kTableFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
kInfoLogFile,  //要么是现在的，要么是旧的
  kMetaDatabase,
  kIdentityFile,
  kOptionsFile,
  kBlobFile
};

//用指定的数字返回日志文件的名称
//在以“dbname”命名的数据库中。结果将以前缀
//“dBNEX”。
extern std::string LogFileName(const std::string& dbname, uint64_t number);

extern std::string BlobFileName(const std::string& bdirname, uint64_t number);

static const std::string ARCHIVAL_DIR = "archive";

extern std::string ArchivalDirectory(const std::string& dbname);

//用指定的数字返回存档日志文件的名称
//在以“dbname”命名的数据库中。结果将以“dbname”作为前缀。
extern std::string ArchivedLogFileName(const std::string& dbname,
                                       uint64_t num);

extern std::string MakeTableFileName(const std::string& name, uint64_t number);

//返回带有leveldb后缀的sstable的名称
//从rocksdb sstable后缀名创建
extern std::string Rocks2LevelTableFileName(const std::string& fullname);

//makeTableFileName的反向函数
//todo（yhching）：无法将此函数与parseFileName（）合并
extern uint64_t TableFileNameToNumber(const std::string& name);

//用指定的数字返回sstable的名称
//在以“dbname”命名的数据库中。结果将以前缀
//“dBNEX”。
extern std::string TableFileName(const std::vector<DbPath>& db_paths,
                                 uint64_t number, uint32_t path_id);

//足够的缓冲区大小用于格式化文件号。
const size_t kFormatFileNumberBufSize = 38;

extern void FormatFileNumber(uint64_t number, uint32_t path_id, char* out_buf,
                             size_t out_buf_size);

//返回由命名的数据库的描述符文件名
//“dbname”和指定的化身号。结果是
//前缀为“dbname”。
extern std::string DescriptorFileName(const std::string& dbname,
                                      uint64_t number);

//返回当前文件的名称。此文件包含名称
//当前清单文件的。结果将以前缀
//“dBNEX”。
extern std::string CurrentFileName(const std::string& dbname);

//返回由命名的数据库的锁文件名
//“dBNEX”。结果将以“dbname”作为前缀。
extern std::string LockFileName(const std::string& dbname);

//返回名为“db name”的数据库所拥有的临时文件的名称。
//结果将以“dbname”作为前缀。
extern std::string TempFileName(const std::string& dbname, uint64_t number);

//信息日志名称前缀的助手结构。
struct InfoLogPrefix {
  char buf[260];
  Slice prefix;
//带DB绝对路径编码的前缀
  explicit InfoLogPrefix(bool has_log_dir, const std::string& db_absolute_path);
//默认前缀
  explicit InfoLogPrefix();
};

//返回“dbname”的信息日志文件名。
extern std::string InfoLogFileName(const std::string& dbname,
                                   const std::string& db_path = "",
                                   const std::string& log_dir = "");

//返回“dbname”的旧信息日志文件的名称。
extern std::string OldInfoLogFileName(const std::string& dbname, uint64_t ts,
                                      const std::string& db_path = "",
                                      const std::string& log_dir = "");

static const std::string kOptionsFileNamePrefix = "OPTIONS-";
static const std::string kTempFileNameSuffix = "dbtmp";

//返回给定“dbname”和文件号的选项文件名。
//格式：选项-[数字].dbtmp
extern std::string OptionsFileName(const std::string& dbname,
                                   uint64_t file_num);

//返回给定“dbname”和文件号的临时选项文件名。
//格式：选项-[数字]
extern std::string TempOptionsFileName(const std::string& dbname,
                                       uint64_t file_num);

//返回用于元数据库的名称。结果将以前缀
//“dBNEX”。
extern std::string MetaDatabaseName(const std::string& dbname,
                                    uint64_t number);

//返回为数据库存储唯一编号的标识文件的名称
//如果数据库丢失了所有数据并重新创建为新的，将重新生成。
//从备份映像或空映像
extern std::string IdentityFileName(const std::string& dbname);

//如果文件名是rocksdb文件，请将文件类型存储在*类型中。
//文件名中编码的数字存储在*数字中。如果
//已成功分析文件名，返回true。否则返回false。
//info_log_name_prefix是信息日志的路径。
extern bool ParseFileName(const std::string& filename, uint64_t* number,
                          const Slice& info_log_name_prefix, FileType* type,
                          WalFileType* log_type = nullptr);
//与上一个函数相同，但跳过信息日志文件。
extern bool ParseFileName(const std::string& filename, uint64_t* number,
                          FileType* type, WalFileType* log_type = nullptr);

//使当前文件指向描述符文件
//指定的数字。
extern Status SetCurrentFile(Env* env, const std::string& dbname,
                             uint64_t descriptor_number,
                             Directory* directory_to_fsync);

//生成数据库的标识文件
extern Status SetIdentityFile(Env* env, const std::string& dbname);

//同步清单文件“file”。
extern Status SyncManifest(Env* env, const ImmutableDBOptions* db_options,
                           WritableFileWriter* file);

}  //命名空间rocksdb
