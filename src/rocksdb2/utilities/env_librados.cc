
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//-*-模式：C++；制表符宽度：8；C基偏移：2；缩进选项卡模式：t**
//VIM：ts=8 sw=2 smarttab

#include "rocksdb/utilities/env_librados.h"
#include "util/random.h"
#include <mutex>
#include <cstdlib>

namespace rocksdb {
/*全球第四纪*/
//定义调试
#ifdef DEBUG
#include <cstdio>
#include <sys/syscall.h>
#include <unistd.h>
#define LOG_DEBUG(...)  do{\
    printf("[%ld:%s:%i:%s]", syscall(SYS_gettid), __FILE__, __LINE__, __FUNCTION__);\
    printf(__VA_ARGS__);\
  }while(0)
#else
#define LOG_DEBUG(...)
#endif

/*全局常数*/
const char *default_db_name     = "default_envlibrados_db";
const char *default_pool_name   = "default_envlibrados_pool";
const char *default_config_path = "CEPH_CONFIG_PATH";           //CEph配置文件的env变量名
//可存储在fs中的最大dir/文件数
const int MAX_ITEMS_IN_FS = 1 << 30;
//根目录标签
const std::string ROOT_DIR_KEY = "/";
const std::string DIR_ID_VALUE = "<DIR>";

/*
 *@brief将错误代码转换为状态
 *@details将内部Linux错误代码转换为状态
 *
 *@param r[描述]
 *@返回[描述]
 **/

Status err_to_status(int r)
{
  switch (r) {
  case 0:
    return Status::OK();
  case -ENOENT:
    return Status::IOError();
  case -ENODATA:
  case -ENOTDIR:
    return Status::NotFound(Status::kNone);
  case -EINVAL:
    return Status::InvalidArgument(Status::kNone);
  case -EIO:
    return Status::IOError(Status::kNone);
  default:
//固定器：
    assert(0 == "unrecognized error code");
    return Status::NotSupported(Status::kNone);
  }
}

/*
 *@brief将文件路径拆分为dir路径和文件名
 *@细节
 *因为rocksdb只需要一个2级结构（dir/file），所以所有输入路径都将缩短为dir/file格式。
 *例如：
 *b/c=>dir'/b'，文件'c'
 */a/b/c=>dir'/b'，文件'c'
 *
 *@param fn[描述]
 *@param dir[说明]
 *@param文件[说明]
 **/

void split(const std::string &fn, std::string *dir, std::string *file) {
  LOG_DEBUG("[IN]%s\n", fn.c_str());
  int pos = fn.size() - 1;
  while ('/' == fn[pos]) --pos;
  size_t fstart = fn.rfind('/', pos);
  *file = fn.substr(fstart + 1, pos - fstart);

  pos = fstart;
  while (pos >= 0 && '/' == fn[pos]) --pos;

  if (pos < 0) {
    *dir = "/";
  } else {
    size_t dstart = fn.rfind('/', pos);
    *dir = fn.substr(dstart + 1, pos - dstart);
    *dir = std::string("/") + *dir;
  }

  LOG_DEBUG("[OUT]%s | %s\n", dir->c_str(), file->c_str());
}

//用于按顺序读取文件的文件抽象
class LibradosSequentialFile : public SequentialFile {
  librados::IoCtx * _io_ctx;
  std::string _fid;
  std::string _hint;
  int _offset;
public:
  LibradosSequentialFile(librados::IoCtx * io_ctx, std::string fid, std::string hint):
    _io_ctx(io_ctx), _fid(fid), _hint(hint), _offset(0) {}

  ~LibradosSequentialFile() {}

  /*
   *@brief读取文件
   *@细节
   *从文件中读取最多“n”个字节。划痕[0..n-1]“可能是
   *由这个程序编写。将“*result”设置为
   *读取（包括成功读取小于“n”字节）。
   *可以将“*result”设置为指向“scratch[0..n-1]”中的数据，因此
   *“scratch[0..n-1]”必须在使用“*result”时有效。
   *如果遇到错误，则返回非OK状态。
   *
   *需要：外部同步
   *
   *@param n[描述]
   *@param result[描述]
   *@param scratch[描述]
   *@返回[描述]
   **/

  Status Read(size_t n, Slice* result, char* scratch) {
    LOG_DEBUG("[IN]%i\n", (int)n);
    librados::bufferlist buffer;
    Status s;
    int r = _io_ctx->read(_fid, buffer, n, _offset);
    if (r >= 0) {
      buffer.copy(0, r, scratch);
      *result = Slice(scratch, r);
      _offset += r;
      s = Status::OK();
    } else {
      s = err_to_status(r);
      if (s == Status::IOError()) {
        *result = Slice();
        s = Status::OK();
      }
    }
    LOG_DEBUG("[OUT]%s, %i, %s\n", s.ToString().c_str(), (int)r, buffer.c_str());
    return s;
  }

  /*
   *@brief跳过文件中的“n”字节
   *@细节
   *跳过文件中的“n”字节。这肯定不是
   *读取相同数据的速度较慢，但可能更快。
   *
   *如果到达文件结尾，跳过将在
   *file，skip返回OK。
   *
   *需要：外部同步
   *
   *@param n[描述]
   *@返回[描述]
   **/

  Status Skip(uint64_t n) {
    _offset += n;
    return Status::OK();
  }

  /*
   ＊简报
   *@细节
   *RockSDB拥有我们应该能够使用的缓存功能，
   *这里不依赖缓存。这可能是一个安全的禁忌。
   *
   *@param offset[说明]
   *@参数长度[描述]
   *
   *@返回[描述]
   **/

  Status InvalidateCache(size_t offset, size_t length) {
    return Status::OK();
  }
};

//随机读取文件内容的文件抽象。
class LibradosRandomAccessFile : public RandomAccessFile {
  librados::IoCtx * _io_ctx;
  std::string _fid;
  std::string _hint;
public:
  LibradosRandomAccessFile(librados::IoCtx * io_ctx, std::string fid, std::string hint):
    _io_ctx(io_ctx), _fid(fid), _hint(hint) {}

  ~LibradosRandomAccessFile() {}

  /*
   *@brief读取文件
   *@与libradoSequentialFile类似的详细信息：：read
   *
   *@param offset[说明]
   *@param n[描述]
   *@param result[描述]
   *@param scratch[描述]
   *@返回[描述]
   **/

  Status Read(uint64_t offset, size_t n, Slice* result,
              char* scratch) const {
    LOG_DEBUG("[IN]%i\n", (int)n);
    librados::bufferlist buffer;
    Status s;
    int r = _io_ctx->read(_fid, buffer, n, offset);
    if (r >= 0) {
      buffer.copy(0, r, scratch);
      *result = Slice(scratch, r);
      s = Status::OK();
    } else {
      s = err_to_status(r);
      if (s == Status::IOError()) {
        *result = Slice();
        s = Status::OK();
      }
    }
    LOG_DEBUG("[OUT]%s, %i, %s\n", s.ToString().c_str(), (int)r, buffer.c_str());
    return s;
  }

  /*
   *@brief[简要说明]
   *@details获取每个文件的唯一ID，并保证每个文件的ID不同
   *
   *@param id[描述]
   *@param max_size id的最大大小，应该大于16
   *
   *@返回[描述]
   **/

  size_t GetUniqueId(char* id, size_t max_size) const {
//所有FID都有相同的db_id前缀，因此我们需要忽略db_id前缀
    size_t s = std::min(max_size, _fid.size());
    strncpy(id, _fid.c_str() + (_fid.size() - s), s);
    id[s - 1] = '\0';
    return s;
  };

//枚举访问模式普通、随机、顺序、将需要、已命名
  void Hint(AccessPattern pattern) {
    /*什么也不做*/
  }

  /*
   ＊简报
   *@详细信息[详细描述]
   *
   *@param offset[说明]
   *@参数长度[描述]
   *
   *@返回[描述]
   **/

  Status InvalidateCache(size_t offset, size_t length) {
    return Status::OK();
  }
};


//用于顺序写入的文件抽象。实施
//必须提供缓冲，因为调用方可能附加小片段
//一次到文件。
class LibradosWritableFile : public WritableFile {
  librados::IoCtx * _io_ctx;
  std::string _fid;
  std::string _hint;
  const EnvLibrados * const _env;

std::mutex _mutex;                 //用于保护以下所有变量的修改
librados::bufferlist _buffer;      //写入缓冲器
uint64_t _buffer_size;             //写入缓冲区大小
uint64_t _file_size;               //此文件大小不包括缓冲区大小

  /*
   *@brief假设调用方持有锁
   *@详细信息[详细描述]
   *@返回[描述]
   **/

  int _SyncLocked() {
//1。同步附加数据到Rados
    int r = _io_ctx->append(_fid, _buffer, _buffer_size);
    assert(r >= 0);

//2。更新局部变量
    if (0 == r) {
      _buffer.clear();
      _file_size += _buffer_size;
      _buffer_size = 0;
    }

    return r;
  }

public:
  LibradosWritableFile(librados::IoCtx * io_ctx,
                       std::string fid,
                       std::string hint,
                       const EnvLibrados * const env)
    : _io_ctx(io_ctx), _fid(fid), _hint(hint), _env(env), _buffer(), _buffer_size(0), _file_size(0) {
    int ret = _io_ctx->stat(_fid, &_file_size, nullptr);

//如果文件不存在
    if (ret < 0) {
      _file_size = 0;
    }
  }

  ~LibradosWritableFile() {
//关闭可写文件前同步
    Sync();
  }

  /*
   *@brief将数据附加到文件
   *@细节
   *append将所有写入的数据保存在buffer util buffer size中。
   *达到缓冲区最大尺寸。然后，它将缓冲区写入雷达罩中。
   *
   *@param data[描述]
   *@返回[描述]
   **/

  Status Append(const Slice& data) {
//附加缓冲器
    LOG_DEBUG("[IN] %i | %s\n", (int)data.size(), data.data());
    int r = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    _buffer.append(data.data(), data.size());
    _buffer_size += data.size();

    if (_buffer_size > _env->_write_buffer_size) {
      r = _SyncLocked();
    }

    LOG_DEBUG("[OUT] %i\n", r);
    return err_to_status(r);
  }

  /*
   不支持*@brief
   *@详细信息[详细描述]
   *@返回[描述]
   **/

  Status PositionedAppend(
    /*st slice&/*数据*/，
    uint64_t/*偏移量*/) {

    return Status::NotSupported();
  }

  /*
   *@brief将文件截断到指定的大小
   *@详细信息[详细描述]
   *
   *@参数大小[描述]
   *@返回[描述]
   **/

  Status Truncate(uint64_t size) {
    LOG_DEBUG("[IN]%lld|%lld|%lld\n", (long long)size, (long long)_file_size, (long long)_buffer_size);
    int r = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    if (_file_size > size) {
      r = _io_ctx->trunc(_fid, size);

      if (r == 0) {
        _buffer.clear();
        _buffer_size = 0;
        _file_size = size;
      }
    } else if (_file_size == size) {
      _buffer.clear();
      _buffer_size = 0;
    } else {
      librados::bufferlist tmp;
      tmp.claim(_buffer);
      _buffer.substr_of(tmp, 0, size - _file_size);
      _buffer_size = size - _file_size;
    }

    LOG_DEBUG("[OUT] %i\n", r);
    return err_to_status(r);
  }

  /*
   *@brief关闭文件
   *@详细信息[详细描述]
   *@返回[描述]
   **/

  Status Close() {
    LOG_DEBUG("%s | %lld | %lld\n", _hint.c_str(), (long long)_buffer_size, (long long)_file_size);
    return Sync();
  }

  /*
   *@brief flush文件，
   *@详细信息启动AIO写入而不是等待
   *
   *@返回[描述]
   **/

  Status Flush() {
    librados::AioCompletion *write_completion = librados::Rados::aio_create_completion();
    int r = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    r = _io_ctx->aio_append(_fid, write_completion, _buffer, _buffer_size);

    if (0 == r) {
      _file_size += _buffer_size;
      _buffer.clear();
      _buffer_size = 0;
    }

    write_completion->release();

    return err_to_status(r);
  }

  /*
   *@brief向rados写入缓冲区数据
   *@详细信息启动AIO写入并等待结果
   *@返回[描述]
   **/

Status Sync() { //同步数据
    int r = 0;

    std::lock_guard<std::mutex> lock(_mutex);
    if (_buffer_size > 0) {
      r = _SyncLocked();
    }

    return err_to_status(r);
  }

  /*
   *@brief[简要说明]
   *@详细信息[详细描述]
   如果sync（）和fsync（）可以安全地与append（）和flush（）同时调用，则@return true。
   **/

  bool IsSyncThreadSafe() const {
    return true;
  }

  /*
   *@brief表示当前可写文件实现是否使用直接IO的上层。
   *@详细信息[详细描述]
   *@返回[描述]
   **/

  bool use_direct_io() const {
    return false;
  }

  /*
   *@brief获取文件大小
   *@细节
   *此API将使用缓存文件\大小。
   *@返回[描述]
   **/

  uint64_t GetFileSize() {
    LOG_DEBUG("%lld|%lld\n", (long long)_buffer_size, (long long)_file_size);

    std::lock_guard<std::mutex> lock(_mutex);
    int file_size = _file_size + _buffer_size;

    return file_size;
  }

  /*
   *@brief有关文档，请参阅randomaccessfile:：getuniqueid（）。
   *@详细信息[详细描述]
   *
   *@param id[描述]
   *@参数最大值[描述]
   *
   *@返回[描述]
   **/

  size_t GetUniqueId(char* id, size_t max_size) const {
//所有FID都有相同的db_id前缀，因此我们需要忽略db_id前缀
    size_t s = std::min(max_size, _fid.size());
    strncpy(id, _fid.c_str() + (_fid.size() - s), s);
    id[s - 1] = '\0';
    return s;
  }

  /*
   ＊简报
   *@详细信息[详细描述]
   *
   *@param offset[说明]
   *@参数长度[描述]
   *
   *@返回[描述]
   **/

  Status InvalidateCache(size_t offset, size_t length) {
    return Status::OK();
  }

  using WritableFile::RangeSync;
  /*
   *@brief不支持rangesync，只需调用sync（）。
   *@详细信息[详细描述]
   *
   *@param offset[说明]
   *@param nbytes[描述]
   *
   *@返回[描述]
   **/

  Status RangeSync(off_t offset, off_t nbytes) {
    return Sync();
  }

protected:
  using WritableFile::Allocate;
  /*
   ＊简报
   *@详细信息[详细描述]
   *
   *@param offset[说明]
   *@param len[描述]
   *
   *@返回[描述]
   **/

  Status Allocate(off_t offset, off_t len) {
    return Status::OK();
  }
};


//Directory对象表示文件和实现的集合
//可以在目录上执行的文件系统操作。
class LibradosDirectory : public Directory {
  librados::IoCtx * _io_ctx;
  std::string _fid;
public:
  explicit LibradosDirectory(librados::IoCtx * io_ctx, std::string fid):
    _io_ctx(io_ctx), _fid(fid) {}

//fsync目录。可以从多个线程并发调用。
  Status Fsync() {
    return Status::OK();
  }
};

//标识锁定的文件。
//这是独占锁，不能由同一线程嵌套锁
class LibradosFileLock : public FileLock {
  librados::IoCtx * _io_ctx;
  const std::string _obj_name;
  const std::string _lock_name;
  const std::string _cookie;
  int lock_state;
public:
  LibradosFileLock(
    librados::IoCtx * io_ctx,
    const std::string obj_name):
    _io_ctx(io_ctx),
    _obj_name(obj_name),
    _lock_name("lock_name"),
    _cookie("cookie") {

//TODO:锁永远不会过期。如果进程崩溃或异常退出，可能会导致问题。
    while (!_io_ctx->lock_exclusive(
             _obj_name,
             _lock_name,
             _cookie,
             "description", nullptr, 0));
  }

  ~LibradosFileLock() {
    _io_ctx->unlock(_obj_name, _lock_name, _cookie);
  }
};


//-----------------
//---envlibrados----
//-----------------
/*
 *@brief envlibrados ctor
 *@详细信息[详细描述]
 *
 *@param db_name唯一数据库名称
 *@param config_path雷达罩的配置文件路径
 **/

EnvLibrados::EnvLibrados(const std::string& db_name,
                         const std::string& config_path,
                         const std::string& db_pool)
  : EnvLibrados("client.admin",
                "ceph",
                0,
                db_name,
                config_path,
                db_pool,
                "/wal",
                db_pool,
                1 << 20) {}

/*
 *@brief envlibrados ctor
 *@详细信息[详细描述]
 *
 *@param client_name前3个参数用于rados client init
 *@param cluster\u名称
 *@ PARAM标志
 *@param db_name唯一数据库名称，用作db_id键
 *@param config_path雷达罩的配置文件路径
 *@param db_为db数据池
 *@param wal_为wal数据收集池
 *@param write_buffer_size writablefile buffer最大大小
 **/

EnvLibrados::EnvLibrados(const std::string& client_name,
                         const std::string& cluster_name,
                         const uint64_t flags,
                         const std::string& db_name,
                         const std::string& config_path,
                         const std::string& db_pool,
                         const std::string& wal_dir,
                         const std::string& wal_pool,
                         const uint64_t write_buffer_size)
  : EnvWrapper(Env::Default()),
    _client_name(client_name),
    _cluster_name(cluster_name),
    _flags(flags),
    _db_name(db_name),
    _config_path(config_path),
    _db_pool_name(db_pool),
    _wal_dir(wal_dir),
    _wal_pool_name(wal_pool),
    _write_buffer_size(write_buffer_size) {
  int ret = 0;

//1。创建Rados对象并初始化它
ret = _rados.init2(_client_name.c_str(), _cluster_name.c_str(), _flags); //只需使用client.admin密钥环
if (ret < 0) { //我们来处理可能出现的任何错误
    std::cerr << "couldn't initialize rados! error " << ret << std::endl;
    ret = EXIT_FAILURE;
    goto out;
  }

//2。读取配置文件
  ret = _rados.conf_read_file(_config_path.c_str());
  if (ret < 0) {
//如果配置文件格式不正确，这可能会失败，但这很困难。
    std::cerr << "failed to parse config file " << _config_path
              << "! error" << ret << std::endl;
    ret = EXIT_FAILURE;
    goto out;
  }

//三。我们实际上连接到集群
  ret = _rados.connect();
  if (ret < 0) {
    std::cerr << "couldn't connect to cluster! error " << ret << std::endl;
    ret = EXIT_FAILURE;
    goto out;
  }

//4。如果不存在，创建数据库池
  ret = _rados.pool_create(_db_pool_name.c_str());
  if (ret < 0 && ret != -EEXIST && ret !=  -EPERM) {
    std::cerr << "couldn't create pool! error " << ret << std::endl;
    goto out;
  }

//5。创建数据库池ioctx
  ret = _rados.ioctx_create(_db_pool_name.c_str(), _db_pool_ioctx);
  if (ret < 0) {
    std::cerr << "couldn't set up ioctx! error " << ret << std::endl;
    ret = EXIT_FAILURE;
    goto out;
  }

//6。如果不存在，则创建Wal_池
  ret = _rados.pool_create(_wal_pool_name.c_str());
  if (ret < 0 && ret != -EEXIST && ret !=  -EPERM) {
    std::cerr << "couldn't create pool! error " << ret << std::endl;
    goto out;
  }

//7。创建Wal_Pool_ioctx
  ret = _rados.ioctx_create(_wal_pool_name.c_str(), _wal_pool_ioctx);
  if (ret < 0) {
    std::cerr << "couldn't set up ioctx! error " << ret << std::endl;
    ret = EXIT_FAILURE;
    goto out;
  }

//8。添加根目录
  _AddFid(ROOT_DIR_KEY, DIR_ID_VALUE);

out:
  LOG_DEBUG("rados connect result code : %i\n", ret);
}

/************************************************
  处理FID操作的私有函数。
  dir也有fid，但是值是dir_id_value
************************************************/


/*
 *@brief生成新的FID
 *@详细信息[详细描述]
 *@返回[描述]
 **/

std::string EnvLibrados::_CreateFid() {
  return _db_name + "." + GenerateUniqueId();
}

/*
 *@brief获取FID
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@param fid[描述]
 *
 *@返回
 *状态：：（）
 *状态：：未找到（））
 **/

Status EnvLibrados::_GetFid(
  const std::string &fname,
  std::string& fid) {
  std::set<std::string> keys;
  std::map<std::string, librados::bufferlist> kvs;
  keys.insert(fname);
  int r = _db_pool_ioctx.omap_get_vals_by_keys(_db_name, keys, &kvs);

  if (0 == r && 0 == kvs.size()) {
    return Status::NotFound();
  } else if (0 == r && 0 != kvs.size()) {
    fid.assign(kvs[fname].c_str(), kvs[fname].length());
    return Status::OK();
  } else {
    return err_to_status(r);
  }
}

/*
 *@brief重命名fid
 *@details只修改一次rados中的对象，
 *所以这个重命名操作是原子性的，就氡而言。
 *
 *@param old_fname[描述]
 *@param new_fname[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::_RenameFid(const std::string& old_fname,
                               const std::string& new_fname) {
  std::string fid;
  Status s = _GetFid(old_fname, fid);

  if (Status::OK() != s) {
    return s;
  }

  librados::bufferlist bl;
  std::set<std::string> keys;
  std::map<std::string, librados::bufferlist> kvs;
  librados::ObjectWriteOperation o;
  bl.append(fid);
  keys.insert(old_fname);
  kvs[new_fname] = bl;
  o.omap_rm_keys(keys);
  o.omap_set(kvs);
  int r = _db_pool_ioctx.operate(_db_name, &o);
  return err_to_status(r);
}

/*
 *@brief add<file path，fid>到元数据对象。它可能会覆盖现有密钥。
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@param fid[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::_AddFid(
  const std::string& fname,
  const std::string& fid) {
  std::map<std::string, librados::bufferlist> kvs;
  librados::bufferlist value;
  value.append(fid);
  kvs[fname] = value;
  int r = _db_pool_ioctx.omap_set(_db_name, kvs);
  return err_to_status(r);
}

/*
 *@brief返回dir的子文件名。
 *@细节
 *RockSDB有两层结构，因此所有键
 *以dir为前缀的是dir的子文件。
 *这样我们就可以返回这些文件的名称。
 *
 *@param dir[说明]
 *@param result[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::_GetSubFnames(
  const std::string& dir,
  std::vector<std::string> * result
) {
  std::string start_after(dir);
  std::string filter_prefix(dir);
  std::map<std::string, librados::bufferlist> kvs;
  _db_pool_ioctx.omap_get_vals(_db_name,
                               start_after, filter_prefix,
                               MAX_ITEMS_IN_FS, &kvs);

  result->clear();
  for (auto i = kvs.begin(); i != kvs.end(); i++) {
    result->push_back(i->first.substr(dir.size() + 1));
  }
  return Status::OK();
}

/*
 *@brief从元数据对象中删除键fname
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::_DelFid(
  const std::string& fname) {
  std::set<std::string> keys;
  keys.insert(fname);
  int r = _db_pool_ioctx.omap_rm_keys(_db_name, keys);
  return err_to_status(r);
}

/*
 *@brief从_prefix_pool_map获取匹配ioctx
 *@详细信息[详细描述]
 *
 *@ PARAM前缀[描述]
 *@返回[描述]
 *
 **/

librados::IoCtx* EnvLibrados::_GetIoctx(const std::string& fpath) {
  auto is_prefix = [](const std::string & s1, const std::string & s2) {
    auto it1 = s1.begin(), it2 = s2.begin();
    while (it1 != s1.end() && it2 != s2.end() && *it1 == *it2) ++it1, ++it2;
    return it1 == s1.end();
  };

  if (is_prefix(_wal_dir, fpath)) {
    return &_wal_pool_ioctx;
  } else {
    return &_db_pool_ioctx;
  }
}

/**********************************************************
                公共职能
**********************************************************/

/*
 *@brief生成唯一ID
 *details将系统时间和随机数结合起来。
 *@返回[描述]
 **/

std::string EnvLibrados::GenerateUniqueId() {
  Random64 r(time(nullptr));
  uint64_t random_uuid_portion =
    r.Uniform(std::numeric_limits<uint64_t>::max());
  uint64_t nanos_uuid_portion = NowNanos();
  char uuid2[200];
  snprintf(uuid2,
           200,
           "%16lx-%16lx",
           (unsigned long)nanos_uuid_portion,
           (unsigned long)random_uuid_portion);
  return uuid2;
}

/*
 *@brief创建新的顺序读取文件处理程序
 *@详细信息它将检查是否存在fname
 *
 *@param fname[说明]
 *@param result[描述]
 *@param选项[说明]
 *@返回[描述]
 **/

Status EnvLibrados::NewSequentialFile(
  const std::string& fname,
  std::unique_ptr<SequentialFile>* result,
  const EnvOptions& options)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string dir, file, fid;
  split(fname, &dir, &file);
  Status s;
  std::string fpath = dir + "/" + file;
  do {
    s = _GetFid(dir, fid);

    if (!s.ok() || fid != DIR_ID_VALUE) {
      if (fid != DIR_ID_VALUE) s = Status::IOError();
      break;
    }

    s = _GetFid(fpath, fid);

    if (Status::NotFound() == s) {
      s = Status::IOError();
      errno = ENOENT;
      break;
    }

    result->reset(new LibradosSequentialFile(_GetIoctx(fpath), fid, fpath));
    s = Status::OK();
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief创建新的随机访问文件处理程序
 *@详细信息它将检查是否存在fname
 *
 *@param fname[说明]
 *@param result[描述]
 *@param选项[说明]
 *@返回[描述]
 **/

Status EnvLibrados::NewRandomAccessFile(
  const std::string& fname,
  std::unique_ptr<RandomAccessFile>* result,
  const EnvOptions& options)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string dir, file, fid;
  split(fname, &dir, &file);
  Status s;
  std::string fpath = dir + "/" + file;
  do {
    s = _GetFid(dir, fid);

    if (!s.ok() || fid != DIR_ID_VALUE) {
      s = Status::IOError();
      break;
    }

    s = _GetFid(fpath, fid);

    if (Status::NotFound() == s) {
      s = Status::IOError();
      errno = ENOENT;
      break;
    }

    result->reset(new LibradosRandomAccessFile(_GetIoctx(fpath), fid, fpath));
    s = Status::OK();
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief创建新的写入文件处理程序
 *@详细信息它将检查是否存在fname
 *
 *@param fname[说明]
 *@param result[描述]
 *@param选项[说明]
 *@返回[描述]
 **/

Status EnvLibrados::NewWritableFile(
  const std::string& fname,
  std::unique_ptr<WritableFile>* result,
  const EnvOptions& options)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string dir, file, fid;
  split(fname, &dir, &file);
  Status s;
  std::string fpath = dir + "/" + file;

  do {
//1。检查目录是否存在
    s = _GetFid(dir, fid);
    if (!s.ok()) {
      break;
    }

    if (fid != DIR_ID_VALUE) {
      s = Status::IOError();
      break;
    }

//2。检查文件是否存在。
//2.1存在，使用它
//2.2不存在，创建
    s = _GetFid(fpath, fid);
    if (Status::NotFound() == s) {
      fid = _CreateFid();
      _AddFid(fpath, fid);
    }

    result->reset(new LibradosWritableFile(_GetIoctx(fpath), fid, fpath, this));
    s = Status::OK();
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief重用写文件处理程序
 *@细节
 *此函数将旧的\u fname重命名为新的\u fname，
 *然后返回新名称的处理程序
 *
 *@param new_fname[描述]
 *@param old_fname[描述]
 *@param result[描述]
 *@param选项[说明]
 *@返回[描述]
 **/

Status EnvLibrados::ReuseWritableFile(
  const std::string& new_fname,
  const std::string& old_fname,
  std::unique_ptr<WritableFile>* result,
  const EnvOptions& options)
{
  LOG_DEBUG("[IN]%s => %s\n", old_fname.c_str(), new_fname.c_str());
  std::string src_fid, tmp_fid, src_dir, src_file, dst_dir, dst_file;
  split(old_fname, &src_dir, &src_file);
  split(new_fname, &dst_dir, &dst_file);

  std::string src_fpath = src_dir + "/" + src_file;
  std::string dst_fpath = dst_dir + "/" + dst_file;
  Status r = Status::OK();
  do {
    r = _RenameFid(src_fpath,
                   dst_fpath);
    if (!r.ok()) {
      break;
    }

    result->reset(new LibradosWritableFile(_GetIoctx(dst_fpath), src_fid, dst_fpath, this));
  } while (0);

  LOG_DEBUG("[OUT]%s\n", r.ToString().c_str());
  return r;
}

/*
 *@brief新建目录处理程序
 *@详细信息[详细描述]
 *
 *@param name[说明]
 *@param result[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::NewDirectory(
  const std::string& name,
  std::unique_ptr<Directory>* result)
{
  LOG_DEBUG("[IN]%s\n", name.c_str());
  std::string fid, dir, file;
  /*只想得到目录名*/
  split(name + "/tmp", &dir, &file);
  Status s;

  do {
    s = _GetFid(dir, fid);

    if (!s.ok() || DIR_ID_VALUE != fid) {
      s = Status::IOError(name, strerror(-ENOENT));
      break;
    }

    if (Status::NotFound() == s) {
      s = _AddFid(dir, DIR_ID_VALUE);
      if (!s.ok()) break;
    } else if (!s.ok()) {
      break;
    }

    result->reset(new LibradosDirectory(_GetIoctx(dir), dir));
    s = Status::OK();
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief检查是否存在fname
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::FileExists(const std::string& fname)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string fid, dir, file;
  split(fname, &dir, &file);
  Status s = _GetFid(dir + "/" + file, fid);

  if (s.ok() && fid != DIR_ID_VALUE) {
    s = Status::OK();
  }

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief获取目录的子文件名
 *@详细信息[详细描述]
 *
 *@param dir_in[说明]
 *@param result[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::GetChildren(
  const std::string& dir_in,
  std::vector<std::string>* result)
{
  LOG_DEBUG("[IN]%s\n", dir_in.c_str());
  std::string fid, dir, file;
  split(dir_in + "/temp", &dir, &file);
  Status s;

  do {
    s = _GetFid(dir, fid);
    if (!s.ok()) {
      break;
    }

    if (fid != DIR_ID_VALUE) {
      s = Status::IOError();
      break;
    }

    s = _GetSubFnames(dir, result);
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief删除fname
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::DeleteFile(const std::string& fname)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string fid, dir, file;
  split(fname, &dir, &file);
  Status s = _GetFid(dir + "/" + file, fid);

  if (s.ok() && DIR_ID_VALUE != fid) {
    s = _DelFid(dir + "/" + file);
  } else {
    s = Status::NotFound();
  }
  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief创建新目录
 *@详细信息[详细描述]
 *
 *@param dirname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::CreateDir(const std::string& dirname)
{
  LOG_DEBUG("[IN]%s\n", dirname.c_str());
  std::string fid, dir, file;
  split(dirname + "/temp", &dir, &file);
  Status s = _GetFid(dir + "/" + file, fid);

  do {
    if (Status::NotFound() != s && fid != DIR_ID_VALUE) {
      break;
    } else if (Status::OK() == s && fid == DIR_ID_VALUE) {
      break;
    }

    s = _AddFid(dir, DIR_ID_VALUE);
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief如果缺少创建目录
 *@详细信息[详细描述]
 *
 *@param dirname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::CreateDirIfMissing(const std::string& dirname)
{
  LOG_DEBUG("[IN]%s\n", dirname.c_str());
  std::string fid, dir, file;
  split(dirname + "/temp", &dir, &file);
  Status s = Status::OK();

  do {
    s = _GetFid(dir, fid);
    if (Status::NotFound() != s) {
      break;
    }

    s = _AddFid(dir, DIR_ID_VALUE);
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief删除目录
 *@细节
 *
 *@param dirname[说明]
 *@返回[描述]
 **/

Status EnvLibrados::DeleteDir(const std::string& dirname)
{
  LOG_DEBUG("[IN]%s\n", dirname.c_str());
  std::string fid, dir, file;
  split(dirname + "/temp", &dir, &file);
  Status s = Status::OK();

  s = _GetFid(dir, fid);

  if (s.ok() && DIR_ID_VALUE == fid) {
    std::vector<std::string> subs;
    s = _GetSubFnames(dir, &subs);
//如果存在子文件，则无法删除目录
    if (subs.size() > 0) {
      s = Status::IOError();
    } else {
      s = _DelFid(dir);
    }
  } else {
    s = Status::NotFound();
  }

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief返回文件大小
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@param file_size[说明]
 *
 *@返回[描述]
 **/

Status EnvLibrados::GetFileSize(
  const std::string& fname,
  uint64_t* file_size)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string fid, dir, file;
  split(fname, &dir, &file);
  time_t mtime;
  Status s;

  do {
    std::string fpath = dir + "/" + file;
    s = _GetFid(fpath, fid);

    if (!s.ok()) {
      break;
    }

    int ret = _GetIoctx(fpath)->stat(fid, file_size, &mtime);
    if (ret < 0) {
      LOG_DEBUG("%i\n", ret);
      if (-ENOENT == ret) {
        *file_size = 0;
        s = Status::OK();
      } else {
        s = err_to_status(ret);
      }
    } else {
      s = Status::OK();
    }
  } while (0);

  LOG_DEBUG("[OUT]%s|%lld\n", s.ToString().c_str(), (long long)*file_size);
  return s;
}

/*
 *@brief获取文件修改时间
 *@详细信息[详细描述]
 *
 *@param fname[说明]
 *@param file_mtime[说明]
 *
 *@返回[描述]
 **/

Status EnvLibrados::GetFileModificationTime(const std::string& fname,
    uint64_t* file_mtime)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string fid, dir, file;
  split(fname, &dir, &file);
  time_t mtime;
  uint64_t file_size;
  Status s = Status::OK();
  do {
    std::string fpath = dir + "/" + file;
    s = _GetFid(dir + "/" + file, fid);

    if (!s.ok()) {
      break;
    }

    int ret = _GetIoctx(fpath)->stat(fid, &file_size, &mtime);
    if (ret < 0) {
      if (Status::NotFound() == err_to_status(ret)) {
        *file_mtime = static_cast<uint64_t>(mtime);
        s = Status::OK();
      } else {
        s = err_to_status(ret);
      }
    } else {
      s = Status::OK();
    }
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief重命名文件
 *@细节
 *
 *@param src[说明]
 *@param target_in[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::RenameFile(
  const std::string& src,
  const std::string& target_in)
{
  LOG_DEBUG("[IN]%s => %s\n", src.c_str(), target_in.c_str());
  std::string src_fid, tmp_fid, src_dir, src_file, dst_dir, dst_file;
  split(src, &src_dir, &src_file);
  split(target_in, &dst_dir, &dst_file);

  auto s = _RenameFid(src_dir + "/" + src_file,
                      dst_dir + "/" + dst_file);
  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief不支持
 *@详细信息[详细描述]
 *
 *@param src[说明]
 *@param target_in[描述]
 *
 *@返回[描述]
 **/

Status EnvLibrados::LinkFile(
  const std::string& src,
  const std::string& target_in)
{
  LOG_DEBUG("[IO]%s => %s\n", src.c_str(), target_in.c_str());
  return Status::NotSupported();
}

/*
 *@brief锁文件。如果缺少则创建。
 *@详细信息[详细描述]
 *
 *锁文件似乎用于防止RockSDB的其他实例
 *同时打开数据库。来自RocksDB源代码，
 *调用lockfile的位置如下：
 *
 *./db/db_impl.cc:1159:s=env_u->lockfile（lockfilename（dbname_u），&db_lock_）；//db impl:：recover
 *./db/db_impl.cc:5839:status result=env->lockfile（lockname，&lock）；//状态destroyDB
 *
 *当数据库恢复和数据库销毁时，rocksdb将调用lockfile
 *
 *@param fname[说明]
 *@param lock[说明]
 *
 *@返回[描述]
 **/

Status EnvLibrados::LockFile(
  const std::string& fname,
  FileLock** lock)
{
  LOG_DEBUG("[IN]%s\n", fname.c_str());
  std::string fid, dir, file;
  split(fname, &dir, &file);
  Status s = Status::OK();

  do {
    std::string fpath = dir + "/" + file;
    s = _GetFid(fpath, fid);

    if (Status::OK() != s &&
        Status::NotFound() != s) {
      break;
    } else if (Status::NotFound() == s) {
      s = _AddFid(fpath, _CreateFid());
      if (!s.ok()) {
        break;
      }
    } else if (Status::OK() == s && DIR_ID_VALUE == fid) {
      s = Status::IOError();
      break;
    }

    *lock = new LibradosFileLock(_GetIoctx(fpath), fpath);
  } while (0);

  LOG_DEBUG("[OUT]%s\n", s.ToString().c_str());
  return s;
}

/*
 *@brief解锁文件
 *@详细信息[详细描述]
 *
 *@param lock[说明]
 *@返回[描述]
 **/

Status EnvLibrados::UnlockFile(FileLock* lock)
{
  LOG_DEBUG("[IO]%p\n", lock);
  if (nullptr != lock) {
    delete lock;
  }
  return Status::OK();
}


/*
 *@brief不支持
 *@详细信息[详细描述]
 *
 *@param db_path[说明]
 *@param output_path[说明]
 *
 *@返回[描述]
 **/

Status EnvLibrados::GetAbsolutePath(
  const std::string& db_path,
  std::string* output_path)
{
  LOG_DEBUG("[IO]%s\n", db_path.c_str());
  return Status::NotSupported();
}

/*
 *@brief获取默认envlibrados
 *@详细信息[详细描述]
 *@返回[描述]
 **/

EnvLibrados* EnvLibrados::Default() {
  static EnvLibrados default_env(default_db_name,
                                 std::getenv(default_config_path),
                                 default_pool_name);
  return &default_env;
}
}