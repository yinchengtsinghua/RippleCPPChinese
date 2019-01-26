
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
#ifndef ROCKSDB_UTILITIES_ENV_LIBRADOS_H
#define ROCKSDB_UTILITIES_ENV_LIBRADOS_H

#include <memory>
#include <string>

#include "rocksdb/status.h"
#include "rocksdb/utilities/env_mirror.h"

#include <rados/librados.hpp>

namespace rocksdb {
class LibradosWritableFile;

class EnvLibrados : public EnvWrapper {
 public:
//用指定的名称创建一个全新的顺序可读文件。
//成功后，将指向新文件的指针存储在*result中，并返回OK。
//失败时将nullptr存储在*result中并返回non ok。如果文件有
//不存在，返回非OK状态。
//
//返回的文件一次只能由一个线程访问。
  Status NewSequentialFile(const std::string& fname,
                           std::unique_ptr<SequentialFile>* result,
                           const EnvOptions& options) override;

//创建一个全新的随机访问只读文件
//指定的名称。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。如果文件不存在，则返回一个非OK
//状态。
//
//多个线程可以同时访问返回的文件。
  Status NewRandomAccessFile(const std::string& fname,
                             std::unique_ptr<RandomAccessFile>* result,
                             const EnvOptions& options) override;

//创建一个对象，该对象使用指定的
//姓名。删除具有相同名称的任何现有文件并创建
//新文件。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
//
//返回的文件一次只能由一个线程访问。
  Status NewWritableFile(const std::string& fname,
                         std::unique_ptr<WritableFile>* result,
                         const EnvOptions& options) override;

//通过重命名现有文件并将其打开为可写文件来重用该文件。
  Status ReuseWritableFile(const std::string& fname,
                           const std::string& old_fname,
                           std::unique_ptr<WritableFile>* result,
                           const EnvOptions& options) override;

//创建表示目录的对象。如果目录
//不存在。如果目录存在，它将打开目录
//并创建一个新的目录对象。
//
//成功后，将指向新目录的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
  Status NewDirectory(const std::string& name,
                      std::unique_ptr<Directory>* result) override;

//如果命名文件存在，则返回OK。
//如果命名文件不存在，则找不到，
//调用进程没有权限确定
//此文件是否存在，或者路径是否无效。
//如果遇到IO错误，则为IOERROR
  Status FileExists(const std::string& fname) override;

//将指定目录的子目录名存储在*result中。
//这些名称是相对于“dir”的。
//*结果的原始内容将被删除。
  Status GetChildren(const std::string& dir, std::vector<std::string>* result);

//删除命名文件。
  Status DeleteFile(const std::string& fname) override;

//创建指定的目录。如果目录存在，则返回错误。
  Status CreateDir(const std::string& dirname) override;

//如果缺少，则创建目录。如果存在或成功
//创建。
  Status CreateDirIfMissing(const std::string& dirname) override;

//删除指定的目录。
  Status DeleteDir(const std::string& dirname) override;

//将fname的大小存储在*文件\大小中。
  Status GetFileSize(const std::string& fname, uint64_t* file_size) override;

//将fname的最后修改时间存储在*文件时间中。
  Status GetFileModificationTime(const std::string& fname,
                                 uint64_t* file_mtime) override;
//将文件src重命名为target。
  Status RenameFile(const std::string& src, const std::string& target) override;
//硬链接文件SRC到目标。
  Status LinkFile(const std::string& src, const std::string& target) override;

//锁定指定的文件。用于防止同时访问
//通过多个进程访问同一数据库。失败时，将nullptr存储在
//*锁定并返回非OK。
//
//成功时，存储指向表示
//获得锁定*锁定并返回OK。打电话的人应该打电话
//解锁文件（*lock）以释放锁。如果进程退出，
//锁将自动释放。
//
//如果有人已经锁上了锁，立即结束
//失败了。即，此调用不等待现有锁
//走开。
//
//如果命名文件不存在，则可以创建它。
  Status LockFile(const std::string& fname, FileLock** lock);

//释放先前成功调用lockfile获得的锁。
//要求：成功的lockfile（）调用返回了lock
//要求：锁尚未解锁。
  Status UnlockFile(FileLock* lock);

//获取此数据库的完整目录名。
  Status GetAbsolutePath(const std::string& db_path, std::string* output_path);

//生成唯一ID
  std::string GenerateUniqueId();

//获取默认envlibrados
  static EnvLibrados* Default();

  explicit EnvLibrados(const std::string& db_name,
                       const std::string& config_path,
                       const std::string& db_pool);

  explicit EnvLibrados(
const std::string& client_name,  //前3个参数是
//Rados客户端初始化
      const std::string& cluster_name, const uint64_t flags,
      const std::string& db_name, const std::string& config_path,
      const std::string& db_pool, const std::string& wal_dir,
      const std::string& wal_pool, const uint64_t write_buffer_size);
  ~EnvLibrados() { _rados.shutdown(); }

 private:
  std::string _client_name;
  std::string _cluster_name;
  uint64_t _flags;
std::string _db_name;  //从用户获取，可读字符串；也用作db_id
//对于DB元数据
  std::string _config_path;
librados::Rados _rados;  //雷达客户端
  std::string _db_pool_name;
librados::IoCtx _db_pool_ioctx;  //用于连接数据库池的ioctx
std::string _wal_dir;            //瓦尔迪尔路径
  std::string _wal_pool_name;
librados::IoCtx _wal_pool_ioctx;  //用于连接Wal_Pool的ioctx
uint64_t _write_buffer_size;      //可写入文件缓冲区最大大小

  /*与氡通信的专用功能*/
  std::string _CreateFid();
  Status _GetFid(const std::string& fname, std::string& fid);
  Status _GetFid(const std::string& fname, std::string& fid, int fid_len);
  Status _RenameFid(const std::string& old_fname, const std::string& new_fname);
  Status _AddFid(const std::string& fname, const std::string& fid);
  Status _DelFid(const std::string& fname);
  Status _GetSubFnames(const std::string& dirname,
                       std::vector<std::string>* result);
  librados::IoCtx* _GetIoctx(const std::string& prefix);
  friend class LibradosWritableFile;
};
}
#endif
