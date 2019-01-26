
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

#include "rocksdb/sst_file_writer.h"

#include <vector>
#include "db/dbformat.h"
#include "rocksdb/table.h"
#include "table/block_based_table_builder.h"
#include "table/sst_file_writer_collectors.h"
#include "util/file_reader_writer.h"
#include "util/sync_point.h"

namespace rocksdb {

const std::string ExternalSstFilePropertyNames::kVersion =
    "rocksdb.external_sst_file.version";
const std::string ExternalSstFilePropertyNames::kGlobalSeqno =
    "rocksdb.external_sst_file.global_seqno";

#ifndef ROCKSDB_LITE

const size_t kFadviseTrigger = 1024 * 1024; //1MB

struct SstFileWriter::Rep {
  Rep(const EnvOptions& _env_options, const Options& options,
      Env::IOPriority _io_priority, const Comparator* _user_comparator,
      ColumnFamilyHandle* _cfh, bool _invalidate_page_cache)
      : env_options(_env_options),
        ioptions(options),
        mutable_cf_options(options),
        io_priority(_io_priority),
        internal_comparator(_user_comparator),
        cfh(_cfh),
        invalidate_page_cache(_invalidate_page_cache),
        last_fadvise_size(0) {}

  std::unique_ptr<WritableFileWriter> file_writer;
  std::unique_ptr<TableBuilder> builder;
  EnvOptions env_options;
  ImmutableCFOptions ioptions;
  MutableCFOptions mutable_cf_options;
  Env::IOPriority io_priority;
  InternalKeyComparator internal_comparator;
  ExternalSstFileInfo file_info;
  InternalKey ikey;
  std::string column_family_name;
  ColumnFamilyHandle* cfh;
//如果为真，我们将向操作系统提示不需要此文件页
//每次我们写1MB到文件。
  bool invalidate_page_cache;
//上次调用fadvise删除时文件的大小
//从页面缓存缓存缓存的页面。
  uint64_t last_fadvise_size;
  Status Add(const Slice& user_key, const Slice& value,
             const ValueType value_type) {
    if (!builder) {
      return Status::InvalidArgument("File is not opened");
    }

    if (file_info.num_entries == 0) {
      file_info.smallest_key.assign(user_key.data(), user_key.size());
    } else {
      if (internal_comparator.user_comparator()->Compare(
              user_key, file_info.largest_key) <= 0) {
//确保按顺序添加密钥
        return Status::InvalidArgument("Keys must be added in order");
      }
    }

//TODO（tec）：对于外部SST文件，可以省略seqno和type。
    switch (value_type) {
      case ValueType::kTypeValue:
        /*y.set（用户键，0/*序列号*/，
                 valuetype:：ktypeValue/*输入*/);

        break;
      case ValueType::kTypeMerge:
        /*y.set（用户键，0/*序列号*/，
                 值类型：：ktypeMerge/*合并*/);

        break;
      case ValueType::kTypeDeletion:
        /*y.set（用户键，0/*序列号*/，
                 valuetype:：ktypedelection/*删除*/);

        break;
      default:
        return Status::InvalidArgument("Value type is not supported");
    }
    builder->Add(ikey.Encode(), value);

//更新文件信息
    file_info.num_entries++;
    file_info.largest_key.assign(user_key.data(), user_key.size());
    file_info.file_size = builder->FileSize();

    /*alidatepagecache（false/*正在关闭*/）；

    返回状态：：OK（）；
  }

  void invalidpagecache（bool关闭）
    if（使页面缓存失效==false）
      //Fadvise已禁用
      返回；
    }
    uint64_t字节\u自上次使用后的\u fadvise=
      builder->fileSize（）-最后一个“fadvise”大小；
    if（自上次关闭以来的字节数）
      测试同步点回调（“sstfileWriter:：rep:：invalidpagecache”，
                               &（字节\u自上次使用后的\u fadvise））；
      //告诉操作系统在页面缓存中不需要此文件
      文件写入程序->无效缓存（0，0）；
      last_fadvise_size=builder->fileSize（）；
    }
  }

}；

sstfileWriter:：sstfileWriter（const envOptions和env_选项，
                             常量选项和选项，
                             常量比较器*用户比较器，
                             columnFamilyhandle*column_系列，
                             bool使页面缓存无效，
                             env:：io priority io_优先级）
    ：rep_u（新rep（env_选项、选项、IO_优先级、用户_比较器，
                   列_family，使_page_cache失效）
  rep_u->file_info.file_size=0；
}

sstfileWriter:：~sstfileWriter（）
  如果（rep_->builder）
    //用户没有调用finish（）或finish（）失败，我们需要
    //放弃生成器。
    rep_->builder->discarge（）；
  }
}

状态sstfilewriter:：open（const std:：string&file_path）
  rep*r=rep_.get（）；
  状态S；
  std:：unique_ptr<writablefile>sst_file；
  s=r->ioptions.env->newwritablefile（file_path，&sst_file，r->env_options）；
  如果（！）S.O.（））{
    返回S；
  }

  sst_file->setiopriority（r->io_priority）；

  压缩式压缩
  如果（r->ioptions.bottommost_compression！=kdisablecompressionoption）
    compression_type=r->ioptions.bottomst_compression；
  否则，（如果）！r->ioptions.compression_per_level.empty（））
    //如果我们有每级压缩，则使用最后一级的压缩
    compression_type=*（r->ioptions.compression_per_level.rbegin（））；
  }否则{
    compression_type=r->mutable_cf_options.compression；
  }

  std:：vector<std:：unique_ptr<inttblpropcollectorFactory>>
      内特布尔道具收藏家工厂；

  //sstfilewriter属性收集器以添加sstfilewriter版本。
  内特堡道具收藏家工厂。安置后（
      新的sstfileWriterPropertiesCollectorFactory（2/*版本*/,

                                                  /**全球电话号码*/）；

  //用户收集器工厂
  自动用户收集工厂=
      r->ioptions.table_properties_collector_factures；
  对于（size_t i=0；i<user_collector_factures.size（）；i++）
    内特堡道具收藏家工厂。安置后（
        新的用户键表属性CollectorFactory（
            用户收集工厂[i]）；
  }
  int未知_级=-1；
  UIT32 32 CFX ID；

  如果（R）> CFH！= null pTr）{
    //用户明确指定此文件将被摄取到CFH中，
    //我们可以将此信息保存在文件中。
    cf_id=r->cfh->getid（）；
    r->column_family_name=r->cfh->getname（）；
  }否则{
    r->column_-family_-name=“”；
    cf_id=TablePropertiesCollectorFactory:：Context:：KunKnownColumnFamily；
  }

  tablebuilderOptions表构建器选项（
      r->ioptions，r->internal_comparator，&int_tbl_prop_collector_工厂，
      压缩类型，r->ioptions.compression选项，
      nullptr/*压缩\u dict*/, false /* skip_filters */,

      r->column_family_name, unknown_level);
  r->file_writer.reset(
      new WritableFileWriter(std::move(sst_file), r->env_options));

//TODO（tec）：如果表工厂使用压缩块缓存，我们将
//在其中添加外部的sst文件块，这是浪费。
  r->builder.reset(r->ioptions.table_factory->NewTableBuilder(
      table_builder_options, cf_id, r->file_writer.get()));

  r->file_info.file_path = file_path;
  r->file_info.file_size = 0;
  r->file_info.num_entries = 0;
  r->file_info.sequence_number = 0;
  r->file_info.version = 2;
  return s;
}

Status SstFileWriter::Add(const Slice& user_key, const Slice& value) {
  return rep_->Add(user_key, value, ValueType::kTypeValue);
}

Status SstFileWriter::Put(const Slice& user_key, const Slice& value) {
  return rep_->Add(user_key, value, ValueType::kTypeValue);
}

Status SstFileWriter::Merge(const Slice& user_key, const Slice& value) {
  return rep_->Add(user_key, value, ValueType::kTypeMerge);
}

Status SstFileWriter::Delete(const Slice& user_key) {
  return rep_->Add(user_key, Slice(), ValueType::kTypeDeletion);
}

Status SstFileWriter::Finish(ExternalSstFileInfo* file_info) {
  Rep* r = rep_.get();
  if (!r->builder) {
    return Status::InvalidArgument("File is not opened");
  }
  if (r->file_info.num_entries == 0) {
    return Status::InvalidArgument("Cannot create sst file with no entries");
  }

  Status s = r->builder->Finish();
  r->file_info.file_size = r->builder->FileSize();

  if (s.ok()) {
    s = r->file_writer->Sync(r->ioptions.use_fsync);
    /*invalidpagecache（true/*正在关闭*/）；
    如果（S.O.（））{
      s=r->file_writer->close（）；
    }
  }
  如果（！）S.O.（））{
    r->ioptions.env->deletefile（r->file_info.file_path）；
  }

  如果（文件）信息！= null pTr）{
    *file_info=r->file_info；
  }

  r->builder.reset（）；
  返回S；
}

uint64_t sstfileWriter:：fileSize（）
  返回rep_u->file_info.file_size；
}
我很喜欢你！摇滚乐

//命名空间rocksdb
