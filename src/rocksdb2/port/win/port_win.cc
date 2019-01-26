
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

#if !defined(OS_WIN) && !defined(WIN32) && !defined(_WIN32)
#error Windows Specific Code
#endif

#include "port/win/port_win.h"

#include <io.h>
#include "port/dirent.h"
#include "port/sys_time.h"

#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <memory>
#include <exception>
#include <chrono>

#include "util/logging.h"

namespace rocksdb {
namespace port {

/*D GetTimeOfDay（结构时间值*电视，结构时区*/*TZ*/）
  使用命名空间std:：chrono；

  微秒usnow（
      持续时间_cast<microseconds>（system_clock:：now（）.time_since_epoch（））；

  seconds secnow（duration_cast<seconds>（usnow））；

  tv->tv_sec=static_cast<long>（secnow.count（））；
  tv->tv_usec=static_cast<long>（usnow.count（）-
      持续时间_cast<microseconds>（secnow.count（））；
}

互斥体：：~mutex（）

condvar:：~condvar（）

void condvar:：wait（）
  //调用方必须确保在调用此方法之前保持互斥体
  std:：unique_lock<std:：mutex>lk（mu_->getlock（），std:：adopt_lock）；
nIFUDEF NDEUG
  mu_uu->locked_uu=false；
第二节
  CVI.等待（LK）；
nIFUDEF NDEUG
  mu_uu->locked_uu=true；
第二节
  //释放锁的所有权，因为我们不希望在
  //超出了范围（因为我们采用了锁，而不是自己锁定它）
  LK.Relasee（）；
}

bool condvar:：timedwait（uint64_t abs_time_us）

  使用命名空间std:：chrono；

  //msvc++库实现wait_until，根据wait_for so
  //我们需要将绝对等待转换为相对等待。
  微秒us abs time（abs_time_us）；

  微秒usnow（
    持续时间_cast<microseconds>（system_clock:：now（）.time_since_epoch（））；
  微秒reltimeus=
    （usabtime>usnow）？（usabtime-usnow）：微秒：：零（）；

  //调用方必须确保在调用此方法之前保持互斥体
  std:：unique_lock<std:：mutex>lk（mu_->getlock（），std:：adopt_lock）；
nIFUDEF NDEUG
  mu_uu->locked_uu=false；
第二节
  std：：cv_status cv status=cv_u等待（Lk，reltimeus）；
nIFUDEF NDEUG
  mu_uu->locked_uu=true；
第二节
  //释放锁的所有权，因为我们不希望在
  //超出了范围（因为我们采用了锁，而不是自己锁定它）
  LK.Relasee（）；

  如果（cv status==std:：cv_status:：timeout）
    回归真实；
  }

  
}

void condvar:：signal（）cv_u.notify_one（）；

void condvar:：signalAll（）cv_u.notify_all（）；

int physicalReid（）返回getcurrentProcessorNumber（）；

void initonce（onceType*一次，void（*初始值设定项）（）；
  std:：call_once（once->flag_u，initializer）；
}

//私有结构，仅由指针公开
结构DIR
  Intptr_t手柄
  bool firstread_uu；
  结构查找数据
  动态输入；

  dir（）：句柄_（-1），firstread_u（true）

  dir（const dir&）=删除；
  dir&operator=（const dir&）=删除；

  ~（）{
    如果（1）！{把手）{
      _findclose（手柄_u）；
    }
  }
}；

dir*opendir（const char*名称）
  如果（！）姓名*姓名==0）
    埃诺；
    返回null pTR；
  }

  std：：字符串模式（名称）；
  pattern.append（“\\”）.append（“*”）；

  std:：unique_ptr<dir>dir（new dir）；

  dir->handle_u findfirst64（pattern.c_str（），&dir->data_u）；

  if（dir->handle_=-1）
    返回null pTR；
  }

  strcpy_s（dir->entry_u name，sizeof（dir->entry_u name），dir->data_u name）；

  返回dir.release（）；
}

结构目录*readdir（dir*dirp）
  如果（！）dirp dirp->handle_=-1）
    ErnO＝EBADF；
    返回null pTR；
  }

  if（dirp->firstread_
    dirp->firstread_u=false；
    返回&dirp->entry；
  }

  auto ret=_findnext64（dirp->handle_，&dirp->data_u）；

  如果（RET）！= 0）{
    返回null pTR；
  }

  strcpy_s（dirp->entry_u name，sizeof（dirp->entry_u name），dirp->data_u name）；

  返回&dirp->entry；
}

int closedir（dir*dirp）
  删除DIRP；
  返回0；
}

int truncate（const char*path，int64_t len）
  if（path==nullptr）
    ErnO＝E过失；
    返回- 1；
  }

  如果（Le＜0）{
    ErnNO＝EnValAl；
    返回- 1；
  }

  处理HFLASH =
      创建文件（路径，通用通用写，
                 文件共享读取文件共享写入文件共享删除，
                 空，//安全属性
                 打开现有文件，//仅截断现有文件
                 文件\属性\正常，空）；

  if（无效的_handle_value==hfile）
    Auto LastError=GetLastError（）；
    if（lasterror==error_file_not_found）
      埃诺；
    else if（lasterror==error_access_denied）
      ErnO＝EcAES；
    }否则{
      ErNO＝EIO；
    }
    返回- 1；
  }

  INT结果＝0；
  文件\结束\文件\信息结束\文件；
  end_of_file.end of file.quadpart=len；

  如果（！）setfileinformationbyhandle（hfile，fileendoffileinfo，&结束文件，
                                  sizeof（file_end_of_file_info））
    ErNO＝EIO；
    结果＝- 1；
  }

  闭合手柄（hfile）；
  返回结果；
}

void crash（const std:：string&srcfile，int srcline）
  fprintf（stdout，“在%s发生崩溃：%d\n”，srcFile.c_str（），srcLine）；
  FFLUH（STDUT）；
  中止（）；
}

int getmaxopenfiles（）返回-1；

//命名空间端口
//命名空间rocksdb
