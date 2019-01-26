
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
#ifndef ROCKSDB_LITE

#include "tools/sst_dump_tool_imp.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include "db/memtable.h"
#include "db/write_batch_internal.h"
#include "options/cf_options.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/status.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/utilities/ldb_cmd.h"
#include "table/block.h"
#include "table/block_based_table_builder.h"
#include "table/block_based_table_factory.h"
#include "table/block_builder.h"
#include "table/format.h"
#include "table/meta_blocks.h"
#include "table/plain_table_factory.h"
#include "table/table_reader.h"
#include "util/compression.h"
#include "util/random.h"

#include "port/port.h"

namespace rocksdb {

SstFileReader::SstFileReader(const std::string& file_path,
                             bool verify_checksum,
                             bool output_hex)
    :file_name_(file_path), read_num_(0), verify_checksum_(verify_checksum),
    output_hex_(output_hex), ioptions_(options_),
    internal_comparator_(BytewiseComparator()) {
  fprintf(stdout, "Process %s\n", file_path.c_str());
  init_result_ = GetTableReader(file_name_);
}

extern const uint64_t kBlockBasedTableMagicNumber;
extern const uint64_t kLegacyBlockBasedTableMagicNumber;
extern const uint64_t kPlainTableMagicNumber;
extern const uint64_t kLegacyPlainTableMagicNumber;

const char* testFileName = "test_file_name";

static const std::vector<std::pair<CompressionType, const char*>>
    kCompressions = {
        {CompressionType::kNoCompression, "kNoCompression"},
        {CompressionType::kSnappyCompression, "kSnappyCompression"},
        {CompressionType::kZlibCompression, "kZlibCompression"},
        {CompressionType::kBZip2Compression, "kBZip2Compression"},
        {CompressionType::kLZ4Compression, "kLZ4Compression"},
        {CompressionType::kLZ4HCCompression, "kLZ4HCCompression"},
        {CompressionType::kXpressCompression, "kXpressCompression"},
        {CompressionType::kZSTD, "kZSTD"}};

Status SstFileReader::GetTableReader(const std::string& file_path) {
//“magic_number”未初始化的警告仅在ubsan中显示
//构建。虽然访问由“s.ok（）”检查保护，但将问题修复为
//避免任何警告。
  uint64_t magic_number = Footer::kInvalidTableMagicNumber;

//读取表格幻数
  Footer footer;

  unique_ptr<RandomAccessFile> file;
  uint64_t file_size;
  Status s = options_.env->NewRandomAccessFile(file_path, &file, soptions_);
  if (s.ok()) {
    s = options_.env->GetFileSize(file_path, &file_size);
  }

  file_.reset(new RandomAccessFileReader(std::move(file), file_path));

  if (s.ok()) {
    /*readfooterfromfile（file_.get（），nullptr/*预取缓冲区*/，
                           文件大小和页脚）；
  }
  如果（S.O.（））{
    magic_number=footer.table_magic_number（）；
  }

  如果（S.O.（））{
    if（magic_number==kPlainTablemagicNumber_
        magic_number==klegacycplainTablemagicNumber）
      soptions_uu.use_mmap_reads=true；
      选项“env->newrandomaccessfile”（文件路径，&file，选项）
      文件重置（new randomaccessfilereader（std:：move（file），file_path））；
    }
    options_.comparator=&internal_comparator_u；
    //对于旧的sst格式，readTableProperties可能会失败，但可以读取文件
    if（readTableProperties（magic_number，file_.get（），file_size）.ok（））
      可设置选项YmagicNumber（幻数）；
    }否则{
      setOldTableOptions（）；
    }
  }

  如果（S.O.（））{
    S=NewTableReader（IOptions_uu，Soptions_u，Internal_Comparator_u，文件大小，
                       表阅读器
  }
  返回S；
}

状态sstfileReader:：NewTableReader（
    常量不变的选项和操作、常量环境选项和操作，
    const InternalKeyComparator&Internal_Comparator，uint64_t文件大小，
    唯一的读卡器
  //我们需要关闭索引和筛选节点的预取
  //BlockBasedTable
  if（blockBasedTableFactory:：kname==options_u.table_Factory->name（））
    返回选项_uuTable_Factory->NewTableReader（
        TableReaderOptions（IOPTIONS_u、SoPTIONS_u、Internal_Comparator_u、Internal_x、Internal_x、Internal_x等）
                           /*SkiPad滤波器*/false),

        /*：：move（file_u），file_size，&table_reader_u，/*启用_prefetch=*/false）；
  }

  //对于所有其他工厂实现
  返回选项_uuTable_Factory->NewTableReader（
      TableReaderOptions（IOPTIONS_u，SoPTIONS_u，Internal_Comparator_u），表读取选项（IOPTIONS_u，SoPTIONS_u，Internal_Comparator_u）
      std：：移动（file_u）、文件大小和表读卡器；
}

状态sstfileReader:：verifychecksum（）
  返回表_reader_u->verifychecksum（）；
}

status sstfilereader:：dumptable（const std:：string&out？文件名）
  唯一的_ptr<writablefile>out_file；
  env*env=env:：default（）；
  env->newwritablefile（out_文件名，&out_文件，soptions_u）；
  状态S=table_reader_u->dumptable（out_file.get（））；
  out_file->close（）；
  返回S；
}

uint64_t sstfileReader:：CalculateCompressedTableSize（）。
    const tablebuilderoptions&tb_options，size_t block_size）
  唯一的_ptr<writablefile>out_file；
  唯一_ptr<env>env（newmemenv（env:：default（））；
  env->newwritablefile（testfilename，&out_file，soptions_u）；
  unique_ptr<writablefilewriter>dest_writer；
  dest_writer.reset（new writablefilewriter（std:：move（out_file），soptions_uu））；
  BlockBasedTableOptions表\u选项；
  table_options.block_size=block_size；
  BlockBasedTableFactory Block_-based_-tf（表_选项）；
  unique_ptr<table builder>table_builder；
  table_builder.reset（基于block_的_tf.newTableBuilder（
      TBY选项，
      TablePropertiesCollectorFactory:：Context:：KunknownColumnFamily，
      dest_writer.get（））；
  unique_ptr<internalIterator>iter（table_reader_->newIterator（readoptions（））；
  for（iter->seektofirst（）；iter->valid（）；iter->next（））
    如果（！）iter->status（）.ok（））
      fputs（iter->status（）.toString（）.c_str（），stderr）；
      退出（1）；
    }
    tableeu builder->add（iter->key（），iter->value（））；
  }
  状态S=表格生成器->完成（）；
  如果（！）S.O.（））{
    fputs（s.toString（）.c_str（），stderr）；
    退出（1）；
  }
  uint64_t size=table_builder->fileSize（）；
  env->deletefile（测试文件名）；
  返回大小；
}

int sstfileReader：：显示所有压缩大小（
    大小块大小，
    const std:：vector<std:：pair<compressionType，const char*>>&
        压缩类型）
  读取选项读取选项；
  期权选择；
  构造不变的概念（opts）；
  RocksDB:：InternalKeyComparator ikc（opts.Comparator）；
  std:：vector<std:：unique_ptr<inttblpropcollectorFactory>>
      基于块的表工厂；

  fprintf（stdout，“块大小：”%“rocksdb-priszt”，“n”，“块大小”）；

  用于（auto&i:compression_types）
    if（compressiontypesupported（i.first））
      压缩选项压缩选项；
      std：：字符串列_family_name；
      int未知_级=-1；
      TableBuilderOptions tb_opts（iOptions、iKC和基于Block_的_Table_工厂，
                                  首先，压缩选项，
                                  nullptr/*压缩\u dict*/,

                                  /*SE/*跳过\过滤器*/，列\族\名称，
                                  未知级别）；
      uint64_t file_size=计算压缩表大小（tb_opts，block_size）；
      fprintf（stdout，“压缩：%s”，i.second）；
      fprintf（stdout，“大小：%”priu64“\n”，文件大小）；
    }否则{
      fprintf（stdout，“不支持的压缩类型：%s.\n”，i.second）；
    }
  }
  返回0；
}

状态sstfileReader:：readTableProperties（uint64_t table_magic_number，
                                          RandomAccessFileReader*文件，
                                          uint64_t文件大小）
  table properties*table_properties=nullptr；
  状态S=rocksdb:：readTableProperties（文件、文件大小、表幻数，
                                          IOptions_uu，&表_属性）；
  如果（S.O.（））{
    表\属性\重置（表\属性）；
  }否则{
    fprintf（stdout，“无法读取表属性”）；
  }
  返回S；
}

状态sstfileReader：：可设置选项bymagicNumber（
    uint64_t table_magic_number）_
  断言（表属性）；
  if（table_magic_number==kblockbasedTablemagicNumber_
      table_magic_number==klegacycblockbasedTablemagicNumber）
    选项“table_factory=std:：make_shared<blockbasedTableFactory>（）；
    fprintf（stdout，“sst文件格式：基于块\n”）；
    auto&props=table_properties_uu->user_collected_properties；
    auto pos=props.find（blockBasedTablePropertyNames:：kindExtype）；
    如果（POS）！=props.end（））
      auto index_type_on_file=static_cast<blockbasedTableOptions:：indexType>（）
          decodeFixed32（pos->second.c_str（））；
      if（index_type_on_file=
          blockBasedTableOptions:：indexType:：khashsearch）
        选项前缀提取程序重置（newNoopTransform（））；
      }
    }
  else if（table_magic_number==kPlainTablemagicNumber_
             table_magic_number==klegacycplainTablemagicNumber）
    选项允许读取=真；

    普通表选项普通表选项；
    plain_table_options.user_key_len=kPlainTableVariableLength；
    plain_table_options.bloom_bits_per_key=0；
    plain_table_options.hash_table_ratio=0；
    plain_table_options.index_sparseness=1；
    plain_table_options.large_page_tlb_size=0；
    plain_table_options.encoding_type=kplain；
    plain_table_options.full_scan_mode=true；

    选项“table_factory.reset”（newPlainTableFactory（plain_table_options））；
    fprintf（stdout，“sst文件格式：普通表\n”）；
  }否则{
    char错误_msg_buffer[80]；
    snprintf（错误_msg_buffer，sizeof（错误_msg_buffer）-1，
             “不支持的表幻数---%lx”，
             （长）桌上的魔法号码；
    返回状态：：invalidArgument（错误_msg_buffer）；
  }

  返回状态：：OK（）；
}

状态sstfileReader:：setOldTableOptions（）
  断言（表属性==nullptr）；
  选项“table_factory=std:：make_shared<blockbasedTableFactory>（）；
  fprintf（stdout，“sst文件格式：基于块（旧版本）”）；

  返回状态：：OK（）；
}

状态sstfileReader:：readSequential（bool print_kv，uint64_t read_num，
                                     bool有\u from，const std:：string&from \u key，
                                     bool有to，const std:：string&to_键，
                                     bool使用_-from_作为_前缀）
  如果（！）表读卡器
    返回初始结果
  }

  InternalIterator*Iter=
      表_reader_->newiterator（readoptions（verify_checksum_u，false））；
  UIT64 64 T I＝0；
  如果（HasyOf）{
    Internalkey伊基；
    ikey.setmaxpossibleforuserkey（从_key）；
    iter->seek（ikey.encode（））；
  }否则{
    iter->seektofirst（）；
  }
  for（；iter->valid（）；iter->next（））
    slice key=iter->key（）；
    slice value=iter->value（）；
    +i；
    if（读取数值>0&&i>读取数值）
      断裂；

    Parsedinteralkey伊基；
    如果（！）parseInternalKey（key，&ikey））
      std:：cerr<“内部密钥[”
                <<key.tostring（true/*in he*/)

                << "] parse error!\n";
      continue;
    }

//返回的密钥没有前缀out'from'密钥
    if (use_from_as_prefix && !ikey.user_key.starts_with(from_key)) {
      break;
    }

//如果指定了结束标记，我们将在它之前停止
    if (has_to && BytewiseComparator()->Compare(ikey.user_key, to_key) >= 0) {
      break;
    }

    if (print_kv) {
      fprintf(stdout, "%s => %s\n",
          ikey.DebugString(output_hex_).c_str(),
          value.ToString(output_hex_).c_str());
    }
  }

  read_num_ += i;

  Status ret = iter->status();
  delete iter;
  return ret;
}

Status SstFileReader::ReadTableProperties(
    std::shared_ptr<const TableProperties>* table_properties) {
  if (!table_reader_) {
    return init_result_;
  }

  *table_properties = table_reader_->GetTableProperties();
  return init_result_;
}

namespace {

void print_help() {
  fprintf(stderr,
          R"(sst_dump --file=<data_dir_OR_sst_file> [--command=check|scan|raw]
    --file=<data_dir_OR_sst_file>
      Path to SST file or directory containing SST files

    --command=check|scan|raw|verify
        check: Iterate over entries in files but dont print anything except if an error is encounterd (default command)
        scan: Iterate over entries in files and print them to screen
        raw: Dump all the table contents to <file_name>_dump.txt
        verify: Iterate all the blocks in files verifying checksum to detect possible coruption but dont print anything except if a corruption is encountered
        recompress: reports the SST file size if recompressed with different
                    compression types

    --output_hex
      Can be combined with scan command to print the keys and values in Hex

    --from=<user_key>
      Key to start reading from when executing check|scan

    --to=<user_key>
      Key to stop reading at when executing check|scan

    --prefix=<user_key>
      Returns all keys with this prefix when executing check|scan
      Cannot be used in conjunction with --from

    --read_num=<num>
      Maximum number of entries to read when executing check|scan

    --verify_checksum
      Verify file checksum when executing check|scan

    --input_key_hex
      Can be combined with --from and --to to indicate that these values are encoded in Hex

    --show_properties
      Print table properties after iterating over the file when executing
      check|scan|raw

    --set_block_size=<block_size>
      Can be combined with --command=recompress to set the block size that will
      be used when trying different compression algorithms

    --compression_types=<comma-separated list of CompressionType members, e.g.,
      kSnappyCompression>
      Can be combined with --command=recompress to run recompression for this
      list of compression types

    --parse_internal_key=<0xKEY>
      Convenience option to parse an internal key on the command line. Dumps the
      internal key in hex format {'key' @ SN: type}
)");
}

}  //命名空间

int SSTDumpTool::Run(int argc, char** argv) {
  const char* dir_or_file = nullptr;
  uint64_t read_num = -1;
  std::string command;

  char junk;
  uint64_t n;
  bool verify_checksum = false;
  bool output_hex = false;
  bool input_key_hex = false;
  bool has_from = false;
  bool has_to = false;
  bool use_from_as_prefix = false;
  bool show_properties = false;
  bool show_summary = false;
  bool set_block_size = false;
  std::string from_key;
  std::string to_key;
  std::string block_size_str;
  size_t block_size;
  std::vector<std::pair<CompressionType, const char*>> compression_types;
  uint64_t total_num_files = 0;
  uint64_t total_num_data_blocks = 0;
  uint64_t total_data_block_size = 0;
  uint64_t total_index_block_size = 0;
  uint64_t total_filter_block_size = 0;
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--file=", 7) == 0) {
      dir_or_file = argv[i] + 7;
    } else if (strcmp(argv[i], "--output_hex") == 0) {
      output_hex = true;
    } else if (strcmp(argv[i], "--input_key_hex") == 0) {
      input_key_hex = true;
    } else if (sscanf(argv[i],
               "--read_num=%lu%c",
               (unsigned long*)&n, &junk) == 1) {
      read_num = n;
    } else if (strcmp(argv[i], "--verify_checksum") == 0) {
      verify_checksum = true;
    } else if (strncmp(argv[i], "--command=", 10) == 0) {
      command = argv[i] + 10;
    } else if (strncmp(argv[i], "--from=", 7) == 0) {
      from_key = argv[i] + 7;
      has_from = true;
    } else if (strncmp(argv[i], "--to=", 5) == 0) {
      to_key = argv[i] + 5;
      has_to = true;
    } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
      from_key = argv[i] + 9;
      use_from_as_prefix = true;
    } else if (strcmp(argv[i], "--show_properties") == 0) {
      show_properties = true;
    } else if (strcmp(argv[i], "--show_summary") == 0) {
      show_summary = true;
    } else if (strncmp(argv[i], "--set_block_size=", 17) == 0) {
      set_block_size = true;
      block_size_str = argv[i] + 17;
      std::istringstream iss(block_size_str);
      iss >> block_size;
      if (iss.fail()) {
        fprintf(stderr, "block size must be numeric\n");
        exit(1);
      }
    } else if (strncmp(argv[i], "--compression_types=", 20) == 0) {
      std::string compression_types_csv = argv[i] + 20;
      std::istringstream iss(compression_types_csv);
      std::string compression_type;
      while (std::getline(iss, compression_type, ',')) {
        auto iter = std::find_if(
            kCompressions.begin(), kCompressions.end(),
            [&compression_type](std::pair<CompressionType, const char*> curr) {
              return curr.second == compression_type;
            });
        if (iter == kCompressions.end()) {
          fprintf(stderr, "%s is not a valid CompressionType\n",
                  compression_type.c_str());
          exit(1);
        }
        compression_types.emplace_back(*iter);
      }
    } else if (strncmp(argv[i], "--parse_internal_key=", 21) == 0) {
      std::string in_key(argv[i] + 21);
      try {
        in_key = rocksdb::LDBCommand::HexToString(in_key);
      } catch (...) {
        std::cerr << "ERROR: Invalid key input '"
          << in_key
          << "' Use 0x{hex representation of internal rocksdb key}" << std::endl;
        return -1;
      }
      Slice sl_key = rocksdb::Slice(in_key);
      ParsedInternalKey ikey;
      int retc = 0;
      if (!ParseInternalKey(sl_key, &ikey)) {
        /*：：cerr<“internal key[”<<sl_key.toString（true/*in hex*/）
                  <<“]分析错误！\n；
        RTEC＝- 1；
      }
      fprintf（stdout，“key=%s\n”，ikey.debugString（true）.c_str（））；
      返回返回；
    }否则{
      fprintf（stderr，“无法识别的参数'%s'\n\n”，argv[i]）；
      PrimtBelp（）；
      退出（1）；
    }
  }

  if（使用_-from_作为_前缀&&has_from）
    fprintf（stderr，“不能指定--prefix和--from \n\n”）；
    退出（1）；
  }

  if（输入_key_hex）
    如果（从开始使用作为前缀）
      from_key=rocksdb:：ldb命令：：hextostring（from_key）；
    }
    如果（Hasyto）{
      to_key=rocksdb:：ldbcommand:：hextostring（to_key）；
    }
  }

  if（dir_or_file==nullptr）
    fprintf（stderr，“必须指定文件或目录。\n\n”）；
    PrimtBelp（）；
    退出（1）；
  }

  std:：vector<std:：string>文件名；
  rocksdb:：env*env=rocksdb:：env:：default（）；
  rocksdb:：status st=env->getchildren（dir_or_file，&filename）；
  bool dir=真；
  如果（！）圣奥克（））{
    文件名.clear（）；
    filename.push_back（dir_或_file）；
    dir＝false；
  }

  fprintf（stdout，“从[%s]到[%s]\n”，
      rocksdb:：slice（from_key）.toString（true）.c_str（），
      rocksdb：：slice（to_key）.toString（true）.c_str（））；

  uint64_t total_read=0；
  对于（size_t i=0；i<filenames.size（）；i++）
    std:：string filename=filename.at（i）；
    if（filename.length（）<=4_
        文件名.rfind（“.sst”）！=文件名.length（）-4）
      /忽略
      继续；
    }
    如果（DIR）{
      文件名=std：：string（dir_或_file）+“/”+文件名；
    }

    rocksdb:：sstfilereader reader（文件名，验证校验和，
                                  OutPuthHEX）；
    如果（！）reader.getstatus（）.ok（））
      fprintf（stderr，“%s:%s\n”，文件名.c_str（），
              reader.getstatus（）.toString（）.c_str（））；
      继续；
    }

    if（command=“重新压缩”）；
      reader.showAllCompressionSizes（
          设置块大小？区块大小：16384，
          compression_types.empty（）？K压缩：压缩类型；
      返回0；
    }

    if（command=“raw”）
      std：：string out_filename=filename.substr（0，filename.length（）-4）；
      out_filename.append（“_dump.txt”）；

      st=reader.dumptable（out_文件名）；
      如果（！）圣奥克（））{
        fprintf（stderr，“%s:%s\n”，filename.c_str（），st.toString（）.c_str（））；
        退出（1）；
      }否则{
        fprintf（stdout，“原始转储已写入文件%s\n”，&out\filename[0]）；
      }
      继续；
    }

    //扫描给定文件路径中的所有文件。
    if（command=“”command==“扫描”command==“检查”）
      st=reader.readSequential（
          command=“scan”，读取“num>0”？（read_num-total_read）：read_num，
          有使用从作为前缀，从键，从到键，
          使用\中的\作为\前缀）；
      如果（！）圣奥克（））{
        fprintf（stderr，“%s:%s\n”，文件名.c_str（），
            st.toString（）.c_str（））；
      }
      _read+=reader.getreadNumber（）总数；
      if（读_Num>0&&total_read>read_Num）
        断裂；
      }
    }

    if（command=“verify”）
      st=reader.verifychecksum（）；
      如果（！）圣奥克（））{
        fprintf（stderr，“%s已损坏：%s\n”，文件名.c_str（），
                st.toString（）.c_str（））；
      }否则{
        fprintf（stdout，“文件正常\n”）；
      }
      继续；
    }

    if（显示_属性_显示_摘要）
      const rocksdb:：table properties*表属性；

      std:：shared_ptr<const rocksdb:：tableproperties>
          表\属性\来自读卡器；
      st=reader.readTableProperties（&table_properties_from_reader）；
      如果（！）圣奥克（））{
        fprintf（stderr，“%s:%s\n”，filename.c_str（），st.toString（）.c_str（））；
        fprintf（stderr，“尝试使用初始表属性”）；
        table_properties=reader.getInitTableProperties（）；
      }否则{
        table_properties=table_properties_from_reader.get（）；
      }
      如果（表属性！= null pTr）{
        如果（显示属性）
          FPTCTF（STDUT）
                  “表属性：\n”
                  “--------------------------\n”
                  “%s”，
                  table_properties->toString（“\n”，“：”）.c_str（））；
          fprintf（stdout，“删除的密钥：%”priu64“\n”，
                  rocksdb：：获取删除键（
                      表_properties->user_collected_properties））；

          布尔财产存在；
          uint64_t merge_operands=rocksdb:：getmergeoperands（
              表_properties->user_collected_properties，&property_present）；
          如果（财产存在）
            fprintf（stdout，“合并操作数：%”priu64“\n”，
                    合并操作数）；
          }否则{
            fprintf（stdout，“合并操作数：未知\n”）；
          }
        }
        _num_文件总数+=1；
        _num_data_blocks+=表_properties->num_data_blocks；
        总数据块大小=表属性>数据大小；
        总索引块大小=表属性>索引大小；
        _filter_block_size+=表_properties->filter_size；
      }
      如果（显示属性）
        FPTCTF（STDUT）
                “原始用户收集的属性\n”
                “--------------------------\n”）；
        for（const auto&kv:table_properties->user_collected_properties）
          std:：string prop_name=kv.first；
          std：：string prop_val=slice（kv.second）.toString（true）；
          fprintf（stdout，“%s:0x%s\n”，prop_name.c_str（），
                  prop_val.c_str（））；
        }
      }
    }
  }
  如果（显示摘要）
    fprintf（stdout，“文件总数%”priu64“\n”，文件总数\u num）；
    fprintf（stdout，“数据块总数：%”priu64“\n”，
            数据块总数）；
    fprintf（stdout，“总数据块大小：%”priu64“\n”，
            总数据块大小）；
    fprintf（stdout，“索引块总大小：%”priu64“\n”，
            总索引块大小）；
    fprintf（stdout，“总滤块大小：%”priu64“\n”，
            总滤块大小）；
  }
  返回0；
}
//命名空间rocksdb

endif//rocksdb_lite
