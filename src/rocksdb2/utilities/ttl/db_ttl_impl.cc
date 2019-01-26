
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#ifndef ROCKSDB_LITE

#include "utilities/ttl/db_ttl_impl.h"

#include "db/write_batch_internal.h"
#include "rocksdb/convenience.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/utilities/db_ttl.h"
#include "util/coding.h"
#include "util/filename.h"

namespace rocksdb {

void DBWithTTLImpl::SanitizeOptions(int32_t ttl, ColumnFamilyOptions* options,
                                    Env* env) {
  if (options->compaction_filter) {
    options->compaction_filter =
        new TtlCompactionFilter(ttl, env, options->compaction_filter);
  } else {
    options->compaction_filter_factory =
        std::shared_ptr<CompactionFilterFactory>(new TtlCompactionFilterFactory(
            ttl, env, options->compaction_filter_factory));
  }

  if (options->merge_operator) {
    options->merge_operator.reset(
        new TtlMergeOperator(options->merge_operator, env));
  }
}

//打开dbwithttlimpl内的db，因为选项需要指向其ttl的指针
DBWithTTLImpl::DBWithTTLImpl(DB* db) : DBWithTTL(db) {}

DBWithTTLImpl::~DBWithTTLImpl() {
//在清除过滤器之前需要停止背景压缩
  /*celallbackgroundwork（db_u，/*wait=*/true）；
  删除getOptions（）.compaction_filter；
}

状态实用程序db:：opentttldb（const选项和选项，const std:：string&dbname，
                            stackabledb**dbptr，int32_t ttl，bool read_
  DbWITTTL*DB；
  状态s=dbwithttl:：open（选项，dbname，&db，ttl，只读）；
  如果（S.O.（））{
    * dBPTR＝db；
  }否则{
    *dbptr=nullptr；
  }
  返回S；
}

状态dbwithttl:：open（const选项和选项，const std:：string&dbname，
                       dbwithttl**dbptr，int32_t ttl，bool read_

  数据库选项数据库选项（选项）；
  列系列选项cf_选项（选项）；
  std:：vector<columnFamilyDescriptor>column_families；
  列“家庭”。向后推（
      ColumnFamilyDescriptor（kDefaultColumnFamilyName，cf_选项））；
  std:：vector<columnFamilyhandle*>句柄；
  状态s=dbwithttl:：open（db_选项、dbname、column_系列和句柄，
                             dbptr，ttl，只读）；
  如果（S.O.（））{
    断言（handles.size（）==1）；
    //我可以删除句柄，因为dbimpl始终保持对
    //默认列族
    删除句柄[0]；
  }
  返回S；
}

状态dbwithttl:：open（
    const db options&db_options，const std:：string&dbname，
    const std:：vector<columnFamilyDescriptor>和column_families，
    std:：vector<columnFamilyhandle*>*句柄，dbwithttl**dbptr，
    std:：vector<int32_t>ttls，bool read_only）

  如果（ttls.size（）！=column_families.size（））
    返回状态：：invalidArgument（
        “TTLS大小必须与列族数相同”）；
  }

  std:：vector<columnFamilyDescriptor>column_Families_Sanitized=
      专栏_Families；
  for（size_t i=0；i<column_families_sanitized.size（）；++i）
    dbwithttlimp：：消毒剂（
        ttls[i]，&column_families_sanitized[i].选项，
        db_options.env==nullptr？env:：default（）：db_options.env）；
  }
  DB＊DB；

  状态ST；
  如果（只读）
    st=db：：openforreadonly（db_选项，dbname，column_系列清理，
                             句柄，和DB）；
  }否则{
    st=db：：open（db_选项，dbname，column_families_净化，handles，&db）；
  }
  如果（圣奥克（））{
    *dbptr=新dbwithttlimpl（db）；
  }否则{
    *dbptr=nullptr；
  }
  返回ST；
}

状态dbwithttliml:：createColumnFamilyWithTTL（
    const column系列选项和选项，const std:：string和column_系列名称，
    columnfamilyhandle**句柄，int ttl）
  columnFamilyOptions sanitized_options=选项；
  dbwithttlimpl:：SanitizeOptions（ttl，&Sanitized_options，getenv（））；

  返回dbwithttl:：createColumnFamily（已清理的_选项，列_Family_名称，
                                       句柄）；
}

状态dbwithttliml:：createColumnFamily（const columnFamilyOptions&Options，
                                         const std：：字符串和列\族\名称，
                                         columnfamilyhandle**句柄）
  返回createColumnFamilyWithTTL（选项，列_Family_名称，句柄，0）；
}

//将当前时间戳追加到字符串。
//如果无法获取当前时间，则返回false；如果append成功，则返回true。
状态dbwithttlimpl:：appendts（const slice&val，std:：string*val_with_ts，
                               {Env*Env）{
  val_with_ts->reserve（ktslength+val.size（））；
  字符字符串[ktslength]；
  Int64电流时间；
  状态st=env->getcurrenttime（&curtime）；
  如果（！）圣奥克（））{
    返回ST；
  }
  encodeFixed32（ts_string，（int32_t）curtime）；
  val_with_ts->append（val.data（），val.size（））；
  val_with_ts->append（ts_string，ktslength）；
  返回ST；
}

//如果字符串长度小于timestamp，则返回corruption，或者
//时间戳是指小于TTL功能释放时间的时间
状态dbwithttliml:：sanitychecktimestamp（const slice&str）
  if（str.size（）<ktslength）
    返回状态：：损坏（“错误：值的长度小于时间戳的\n”）；
  }
  //检查ts是否不小于kmInTimeStamp
  //在TTL模式下，Gaurds Anti Corruption&Normal数据库打开错误
  int32_t timestamp_value=decodeFixed32（str.data（）+str.size（）-ktslength）；
  if（timestamp_value<kmintimestamp）
    返回状态：：损坏（“错误：时间戳<TTL功能释放时间！”\n）；
  }
  返回状态：：OK（）；
}

//根据提供的TTL检查字符串是否过时
bool dbwithttlimpl:：isstale（const slice&value，int32_t ttl，env*env）
  if（ttl<=0）//如果ttl为非正数，则数据是新的
    返回错误；
  }
  Int64电流时间；
  如果（！）env->getcurrenttime（&curtime）.ok（））
    返回false；//如果无法获取当前时间，则将数据视为新数据
  }
  Int32_t时间戳_值=
      decodeFixed32（value.data（）+value.size（）-ktslength）；
  返回（timestamp_value+ttl）<curtime；
}

//从切片的末端剥离TS
状态dbwithttlimpl:：stripts（pinnableslice*pinnable_
  状态ST；
  if（pinnable_val->size（）<ktslength）
    返回状态：：损坏（“键值中的时间戳错误”）；
  }
  //删除保存TS的字符
  pinnable_val->remove_suffix（ktslength）；
  返回ST；
}

//从字符串的末尾剥离TS
状态dbwithttlimpl:：stripts（std:：string*str）
  状态ST；
  if（str->length（）<ktslength）
    返回状态：：损坏（“键值中的时间戳错误”）；
  }
  //删除保存TS的字符
  str->erase（str->length（）-ktslength，ktslength）；
  返回ST；
}

状态dbwithttlimpl:：put（const write选项和选项，
                          columnfamilyhandle*column_family，const slice&key，
                          常量切片和val）
  写入批处理；
  batch.put（columneu-family，key，val）；
  返回写入（选项和批处理）；
}

状态dbwithttlimpl:：get（const readoptions&options，
                          columnfamilyhandle*column_family，const slice&key，
                          pinnableslice*值）
  status st=db_u->get（选项，列_族，键，值）；
  如果（！）圣奥克（））{
    返回ST；
  }
  st=sanitychecktimestamp（*value）；
  如果（！）圣奥克（））{
    返回ST；
  }
  返回条带（值）；
}

std:：vector<status>dbwithttlimpl:：multiget（
    施工读数和选项，
    const std：：vector<columnFamilyhandle*>&column_family，
    const std:：vector<slice>&keys，std:：vector<std:：string>*值）
  自动状态=db_u->multiget（选项、列_Family、键、值）；
  对于（size_t i=0；i<keys.size（）；++i）
    如果（！）状态[I].OK（））
      继续；
    }
    状态[i]=sanitychecktimestamp（（*值[i]）；
    如果（！）状态[I].OK（））
      继续；
    }
    状态[i]=条带（&（*值[i]）；
  }
  返回状态；
}

bool dbwithttlimpl:：keymayexist（const readoptions&options，
                                columnFamilyhandle*column_系列，
                                常量切片和键，std:：string*值，
                                bool*找到值
  bool ret=db_u->keymayexist（选项、列_Family、键、值、找到的值）；
  如果（ret和value！=找到nullptr和value！=nullptr&&*找到值
    如果（！）sanitycheckTimestamp（*value）.ok（）！stripts（value）.ok（））
      返回错误；
    }
  }
  返回RET；
}

状态dbwithttliml:：merge（const write选项和选项，
                            columnfamilyhandle*column_family，const slice&key，
                            常量切片和值）
  写入批处理；
  batch.merge（列\族，键，值）；
  返回写入（选项和批处理）；
}

状态dbwithttliml:：write（const writeoptions&opts，writebatch*更新）
  类处理程序：public writebatch:：handler_
   公众：
    显式处理程序（env*env）：env_u（env）
    writebatch更新\ttl；
    状态批改写状态；
    虚拟状态putcf（uint32_t column_family_id，const slice&key，
                         常量切片和值）重写
      std：：字符串值_with_ts；
      status st=appendts（值，&value_with_ts，env_u）；
      如果（！）圣奥克（））{
        批量重写状态=st；
      }否则{
        writebatchInternal:：Put（&updates_ttl，column_family_id，key，
                                价值观；
      }
      返回状态：：OK（）；
    }
    虚拟状态合并cf（uint32_t column_family_id，const slice&key，
                           常量切片和值）重写
      std：：字符串值_with_ts；
      status st=appendts（值，&value_with_ts，env_u）；
      如果（！）圣奥克（））{
        批量重写状态=st；
      }否则{
        writebatchInternal:：Merge（&updates_ttl，column_family_id，key，
                                  价值观；
      }
      返回状态：：OK（）；
    }
    虚拟状态删除cf（uint32_t列_family_id，
                            const slice&key）覆盖
      writebatchInternal：：删除（&updates_ttl，column_family_id，key）；
      返回状态：：OK（）；
    }
    虚拟void logdata（const slice&blob）覆盖
      更新ttl.putlogdata（blob）；
    }

   私人：
    Env.Envi.；
  }；
  处理程序处理程序（getenv（））；
  更新->迭代（&handler）；
  如果（！）handler.batch_rewrite_status.ok（））
    返回handler.batch_rewrite_status；
  }否则{
    返回db_u->write（opts，&（handler.updates_ttl））；
  }
}

迭代器*dbwithttlimpl:：new迭代器（const readoptions&opts，
                                     ColumnFamilyhandle*Column_Family）
  返回新的ttliterator（db_u->newiterator（opts，column_family））；
}

//命名空间rocksdb
endif//rocksdb_lite
