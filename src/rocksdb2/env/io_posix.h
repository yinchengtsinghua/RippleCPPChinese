
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
#pragma once
#include <errno.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include "rocksdb/env.h"

//对于非Linux平台，以下宏仅用作
//持有人。
#if !(defined OS_LINUX) && !(defined CYGWIN) && !(defined OS_AIX)
/*精细的Posix_Fadv_Normal 0/*[MC1]无需进一步特殊治疗*/
定义posix_fadv_random 1/*[mc1]期望随机页引用*/

/*精细的posix_fadv_sequential 2/*[mc1]预期顺序页引用*/
define posix_fadv_will need 3/*[mc1]will need these pages*/

/*很好的posix-fadv-dontned 4/*[mc1]不需要这些页面*/
第二节

名称空间RocksDB
静态std:：string ioerrormsg（const std:：string&context，
                              const std：：字符串和文件名）
  if（file_name.empty（））
    返回上下文；
  }
  返回context+“：”+文件名；
}

//如果文件名不是未知的，则可以保留为空。
静态状态ioerror（const std:：string&context，const std:：string&file_name，
                      内部错误编号）
  开关（错误编号）
  ENOSPC案：
    返回状态：：nospace（ioerrormsg（上下文，文件名），
                           strerrror（err_number））；
  案例：
    返回状态：：ioerror（状态：：kstalefile）；
  违约：
    返回状态：：IOERROR（IOERRORMSG（上下文，文件名），
                           strerrror（err_number））；
  }
}

类posixhelper
 公众：
  静态大小\u t getuniqueidfromfile（int fd，char*id，大小\u t max\u size）；
}；

类posixSequentialFile:公共SequentialFile_
 私人：
  std：：字符串文件名\；
  文件*文件；
  INT FDY；
  bool使用\u direct \u io \；
  大小\逻辑\扇区\大小\；

 公众：
  posixSequentialFile（const std:：string&fname，file*文件，int fd，
                      const env选项和选项）；
  虚拟~posixSequentialFile（）；

  虚拟状态读取（大小、切片*结果、字符*划痕）覆盖；
  虚拟状态位置数据（uint64_t offset，size_t n，slice*result，
                                char*scratch）覆盖；
  虚拟状态跳过（uint64_t n）覆盖；
  虚拟状态无效缓存（大小偏移、大小长度）覆盖；
  虚拟bool use_direct_io（）const override_return use_direct_io_
  虚拟大小“getRequiredBufferAlignment（）const override”
    返回逻辑“扇区大小”；
  }
}；

类posixrandomaccessfile:public randomaccessfile_
 受保护的：
  std：：字符串文件名\；
  INT FDY；
  bool使用\u direct \u io \；
  大小\逻辑\扇区\大小\；

 公众：
  posixrandomaaccessfile（const std:：string&fname，int fd，
                        const env选项和选项）；
  虚拟~posixrandomaaccessfile（）；

  虚拟状态读取（uint64_t offset，size_t n，slice*result，
                      char*scratch）const override；

  虚拟状态预取（uint64_t offset，size_t n）覆盖；

如果定义（OS_Linux）定义（OS_Macosx）定义（OS_AIX）
  虚拟大小\u t getuniqueid（char*id，大小\u t max\u size）常量覆盖；
第二节
  虚拟void提示（accesspattern pattern）覆盖；
  虚拟状态无效缓存（大小偏移、大小长度）覆盖；
  虚拟bool use_direct_io（）const override_return use_direct_io_
  虚拟大小“getRequiredBufferAlignment（）const override”
    返回逻辑“扇区大小”；
  }
}；

类posixwritablefile:public writablefile_
 受保护的：
  const std：：字符串文件名u；
  const bool使用\u direct \u io \；
  INT FDY；
  uint64文件大小
  大小\逻辑\扇区\大小\；
ifdef rocksdb_fallocate_存在
  胸部允许放松；
  用“保持大小”使胸部放松；
第二节

 公众：
  显式posixwritablefile（const std:：string&fname，int fd，
                             const env选项和选项）；
  虚拟~posixwritablefile（）；

  //需要实现此函数，以便正确截断文件
  //直接输入/输出
  虚拟状态截断（uint64_t size）覆盖；
  虚拟状态close（）覆盖；
  虚拟状态附加（const slice&data）覆盖；
  虚拟状态位置暂停（const slice&data，uint64_t offset）覆盖；
  虚拟状态flush（）覆盖；
  虚拟状态同步（）覆盖；
  虚拟状态fsync（）覆盖；
  虚拟bool issyncthreadsafe（）const override；
  虚拟bool use_direct_io（）const override_return use_direct_io_
  虚拟uint64_t getfilesize（）重写；
  虚拟状态无效缓存（大小偏移、大小长度）覆盖；
  虚拟大小“getRequiredBufferAlignment（）const override”
    返回逻辑“扇区大小”；
  }
ifdef rocksdb_fallocate_存在
  虚拟状态分配（uint64_t offset，uint64_t len）覆盖；
第二节
存在ifdef rocksdb诳rangesync诳
  虚拟状态范围同步（uint64_t offset，uint64_t nbytes）覆盖；
第二节
iFIFF OSLinux
  虚拟大小\u t getuniqueid（char*id，大小\u t max\u size）常量覆盖；
第二节
}；

//基于mmap（）的随机访问
类posixmmapreadablefile:public randomaaccessfile_
 私人：
  INT FDY；
  std：：字符串文件名\；
  空区*M映射区
  尺寸长度；

 公众：
  posixmmapreadablefile（const int fd，const std:：string&fname，void*基础，
                        尺寸、长度、常量和选项）；
  虚拟~posixmmapreadablefile（）；
  虚拟状态读取（uint64_t offset，size_t n，slice*result，
                      char*scratch）const override；
  虚拟状态无效缓存（大小偏移、大小长度）覆盖；
}；

类posixmmapfile:公共可写文件
 私人：
  std：：字符串文件名\；
  INT FDY；
  大小\页面\大小\；
  size_t map_size_；//一次要映射多少额外内存
  char*base_；//映射区域
  char*limit_；//映射区域的限制
  char*dst_；//下一个写入位置（在范围[base_u，limit_u]内）
  char*last_sync_；//我们在哪里同步到
  uint64_t file_offset_；//文件中基_u的偏移量
ifdef rocksdb_fallocate_存在
  bool allow_fallocate_uu；//如果为false，则忽略fallocate调用
  用“保持大小”使胸部放松；
第二节

  //将X向上取整为Y的倍数
  静态大小返回（（x+y-1）/y）*y；

  大小截断页面边界（大小）
    s-=（s&（page_size_u1））；
    断言（（s%页面大小=0）；
    返回S；
  }

  状态mapNewRegion（）；
  状态unmapcurrentRegion（）；
  状态MsCyc（）；

 公众：
  posixmmapfile（const std:：string&fname，int fd，size_t page_size，
                const env选项和选项）；
  ~posixmmapfile（）；

  //表示close（）将正确处理truncate
  //不需要任何附加信息
  虚拟状态截断（uint64_t size）覆盖返回状态：：OK（）；
  虚拟状态close（）覆盖；
  虚拟状态附加（const slice&data）覆盖；
  虚拟状态flush（）覆盖；
  虚拟状态同步（）覆盖；
  虚拟状态fsync（）覆盖；
  虚拟uint64_t getfilesize（）重写；
  虚拟状态无效缓存（大小偏移、大小长度）覆盖；
ifdef rocksdb_fallocate_存在
  虚拟状态分配（uint64_t offset，uint64_t len）覆盖；
第二节
}；

类posixrandomrwfile:public randomrwfile_
 公众：
  显式posixrandomrwfile（const std:：string&fname，int fd，
                             const env选项和选项）；
  虚拟~posixrandomrwfile（）；

  虚拟状态写入（uint64_t offset，const slice&data）覆盖；

  虚拟状态读取（uint64_t offset，size_t n，slice*result，
                      char*scratch）const override；

  虚拟状态flush（）覆盖；
  虚拟状态同步（）覆盖；
  虚拟状态fsync（）覆盖；
  虚拟状态close（）覆盖；

 私人：
  const std：：字符串文件名u；
  INT FDY；
}；

类posixdirectory:公共目录
 公众：
  显式posixdirectory（int fd）：fd（fd）
  ~posixdirectory（）；
  虚拟状态fsync（）覆盖；

 私人：
  INT FDY；
}；

//命名空间rocksdb
