
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
//有关以下类型/功能的文档，请参阅port_example.h。

#ifndef STORAGE_LEVELDB_PORT_PORT_WIN_H_
#define STORAGE_LEVELDB_PORT_PORT_WIN_H_

//始终需要最小邮件头
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

//假设在任何地方
#undef PLATFORM_IS_LITTLE_ENDIAN
#define PLATFORM_IS_LITTLE_ENDIAN true

#include <windows.h>
#include <string>
#include <string.h>
#include <mutex>
#include <limits>
#include <condition_variable>
#include <malloc.h>

#include <stdint.h>

#include "port/win/win_thread.h"

#include "rocksdb/options.h"

#undef min
#undef max
#undef DeleteFile
#undef GetCurrentTime


#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#undef GetCurrentTime
#undef DeleteFile

#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

//以c99标准格式命名的大小打印格式
//字符串，如priu64
//事实上，我们可以用那个
#ifndef ROCKSDB_PRIszt
#define ROCKSDB_PRIszt "Iu"
#endif

#ifdef _MSC_VER
#define __attribute__(A)

//Linux上的线程本地存储
//C++中有TyRead本地11
#ifndef __thread
#define __thread __declspec(thread)
#endif

#endif

#ifndef PLATFORM_IS_LITTLE_ENDIAN
#define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

namespace rocksdb {

#define PREFETCH(addr, rw, locality)

namespace port {

//VS 15
#if (defined _MSC_VER) && (_MSC_VER >= 1900)

#define ROCKSDB_NOEXCEPT noexcept

//用于db/file_indexer.h klevelmaxindex
const int kMaxInt32 = std::numeric_limits<int>::max();
const uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();
const int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();

const size_t kMaxSizet = std::numeric_limits<size_t>::max();

#else //三桅纵帆船

//vs 15有snprintf
#define snprintf _snprintf

#define ROCKSDB_NOEXCEPT
//std:：numeric_limits<size_t>：：max（）还不是constexpr
//因此，使用相同的限制

//用于db/file_indexer.h klevelmaxindex
const int kMaxInt32 = INT32_MAX;
const int64_t kMaxInt64 = INT64_MAX;
const uint64_t kMaxUint64 = UINT64_MAX;

#ifdef _WIN64
const size_t kMaxSizet = UINT64_MAX;
#else
const size_t kMaxSizet = UINT_MAX;
#endif

#endif //三桅纵帆船

const bool kLittleEndian = true;

class CondVar;

class Mutex {
 public:

   /*隐式*/mutex（bool adaptive=false）
nIFUDEF NDEUG
     ：锁定（假）
第二节
   {}

  ~ MutExter（）；

  空锁（）
    MutExff.LoC（）；
nIFUDEF NDEUG
    洛克特=真；
第二节
  }

  空隙解锁（）
nIFUDEF NDEUG
    已锁定=false；
第二节
    mutex？unlock（）；
  }

  //如果互斥体没有被锁定，这将断言
  //它不验证调用线程是否持有互斥体
  void asserthold（）
nIFUDEF NDEUG
    断言（锁定）；
第二节
  }

  //mutex仅在锁所有权转移时移动
  mutex（const mutex&）=删除；
  void运算符=（const mutex&）=删除；

 私人：

  朋友阶级康德瓦；

  std:：mutex&getlock（）
    返回互斥；
  }

  std：：互斥互斥
nIFUDEF NDEUG
  波洛克洛奇；
第二节
}；

类rWutMutX {
 公众：
  rwmutex（）初始化srwlock（&srwlock）；

  void readlock（）acquiresrwlockshared（&srwlock）

  void writeLock（）AcquiresRwLockExclusive（&srwLock）

  void readunlock（）释放srwlockshared（&srwlock）

  void writeUnlock（）releasesrwlockexclusive（&srwlock）

  //与posix中一样为空
  void asserthold（）

 私人：
  srwlock srwlock_u；
  //不允许复制
  rwmutex（const rwmutex&）；
  void运算符=（const rwmutex&）；
}；

康德瓦尔级
 公众：
  显式condvar（mutex*mu）：mu_uu（mu）
  }

  ~·CONDVARY（）；
  空WAITE（）；
  bool timedwait（uint64过期时间）；
  空信号（）；
  void signalAll（）；

  //条件变量不可复制/移动构造
  condvar（const condvar&）=删除；
  condvar&operator=（const condvar&）=删除；

  condvar（condvar&&）=删除；
  condvar&operator=（condvar&&）=删除；

 私人：
  std：：条件变量cv；
  互斥；
}；

//高效地包装平台
//或其他更可取的实现
使用线程=windowsthread；

//OnceInit类型有助于模拟
//带有初始化的POSIX语义
//项目采用
结构onceType

    init {}；

    OnthyYyPE.（）{}
    onceType（const init&）
    onceType（const onceType&）=删除；
    onceType&operator=（const onceType&）=删除；

    std：：一次_flag flag_uuu；
}；

定义leveldb_一次_init port:：onceType:：init（）。
extern void initonce（onceType*一次，void（*initializer）（）；

ifndef缓存线大小
定义缓存线\大小64U
第二节

ifdef rocksdb诳jemalloc
包括“jemalloc/jemalloc.h”
//分离入口线，以便在需要时替换它们
inline void*jemalloc_-alloc（大小、大小、对齐）
  返回je对齐的alloc（对齐，大小）；
}
内联空Jemalloc_-Aligned_-Free（void*p）
  JeFielp（P）；
}
第二节

inline void*cacheline_-alloc（size_t-size）
ifdef rocksdb诳jemalloc
  返回jemalloc_-alloc（大小，缓存线大小）；
否则
  返回对齐的\u malloc（大小，缓存线\u大小）；
第二节
}

内联void cacheline_-aligned_-free（void*memblock）
ifdef rocksdb诳jemalloc
  Jemalloc_-Aligned_-Free（Memblock）；
否则
  _对齐的_-free（memblock）；
第二节
}

//https://gcc.gnu.org/bugzilla/show_bug.cgi？MingW32的ID=52991
//无法与by-mno-ms位域一起使用
ifndef_uuu mingw32_uu
将“对齐”定义为（n）uu declspec（align（n））。
否则
将对齐定义为（n）
第二节

static inline void asmvolatilepause（）
如果定义（35u mix86）定义（35u mix64）
  yieldprocessor（）；
第二节
  //在这里武装“wfe”会很好
}

extern int physicalReid（）；

//用于线程本地存储抽象
typedef dword pthread_key_t；

inline int pthread_key_create（pthread_key_t*key，void（*析构函数）（void*）
  /不使用
  （无效）毁灭者；

  pthread_key_t k=tlsalloc（）；
  if（tls_out_of_indexes==k）
    返回EMEOM；
  }

  *密钥＝K；
  返回0；
}

inline int pthread_key_delete（pthread_key_t key）
  如果（！）TlsFree（Key））{
    返回；
  }
  返回0；
}

inline int pthread_setspecific（pthread_key_t key，const void*值）
  如果（！）tlssetValue（key，const_cast<void*>（value）））
    返回EMEOM；
  }
  返回0；
}

inline void*pthread_getspecific（pthread_key_t key）
  void*结果=tlsgetvalue（key）；
  如果（！）结果）{
    如果（getLastError（）！=错误成功
      ErnNO＝EnValAl；
    }否则{
      errno=无错误；
    }
  }
  返回结果；
}

//unix equiv尽管errno编号将关闭
//使用c-runtime实现。注意，这不是
//如果文件被扩展，则使用零感觉空间。
int truncate（const char*path，int64_t length）；
void crash（const std:：string&srcfile，int srcline）；
extern int getmaxopenfiles（）；

//命名空间端口

使用端口：：pthread_key_t；
使用端口：：pthread_key_create；
使用端口：：pthread_key_delete；
使用端口：：pthread_setspecific；
使用端口：：pthread_getspecific；
使用端口：：truncate；

//命名空间rocksdb

endif//存储级别b_port_port_win_h_
