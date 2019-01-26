
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

#include "table/plain_table_reader.h"

#include <string>
#include <vector>

#include "db/dbformat.h"

#include "rocksdb/cache.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"

#include "table/block.h"
#include "table/bloom_block.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "table/internal_iterator.h"
#include "table/meta_blocks.h"
#include "table/two_level_iterator.h"
#include "table/plain_table_factory.h"
#include "table/plain_table_key_coding.h"
#include "table/get_context.h"

#include "monitoring/histogram.h"
#include "monitoring/perf_context_imp.h"
#include "util/arena.h"
#include "util/coding.h"
#include "util/dynamic_bloom.h"
#include "util/hash.h"
#include "util/murmurhash.h"
#include "util/stop_watch.h"
#include "util/string_util.h"

namespace rocksdb {

namespace {

//从char数组安全地获取uint32_t元素，其中，从
//‘base`，每4个字节被视为一个固定的32位整数。
inline uint32_t GetFixed32Element(const char* base, size_t offset) {
  return DecodeFixed32(base + offset * sizeof(uint32_t));
}
}  //命名空间

//迭代索引表的迭代器
class PlainTableIterator : public InternalIterator {
 public:
  explicit PlainTableIterator(PlainTableReader* table, bool use_prefix_seek);
  ~PlainTableIterator();

  bool Valid() const override;

  void SeekToFirst() override;

  void SeekToLast() override;

  void Seek(const Slice& target) override;

  void SeekForPrev(const Slice& target) override;

  void Next() override;

  void Prev() override;

  Slice key() const override;

  Slice value() const override;

  Status status() const override;

 private:
  PlainTableReader* table_;
  PlainTableKeyDecoder decoder_;
  bool use_prefix_seek_;
  uint32_t offset_;
  uint32_t next_offset_;
  Slice key_;
  Slice value_;
  Status status_;
//不允许复制
  PlainTableIterator(const PlainTableIterator&) = delete;
  void operator=(const Iterator&) = delete;
};

extern const uint64_t kPlainTableMagicNumber;
PlainTableReader::PlainTableReader(const ImmutableCFOptions& ioptions,
                                   unique_ptr<RandomAccessFileReader>&& file,
                                   const EnvOptions& storage_options,
                                   const InternalKeyComparator& icomparator,
                                   EncodingType encoding_type,
                                   uint64_t file_size,
                                   const TableProperties* table_properties)
    : internal_comparator_(icomparator),
      encoding_type_(encoding_type),
      full_scan_mode_(false),
      user_key_len_(static_cast<uint32_t>(table_properties->fixed_key_len)),
      prefix_extractor_(ioptions.prefix_extractor),
      enable_bloom_(false),
      bloom_(6, nullptr),
      file_info_(std::move(file), storage_options,
                 static_cast<uint32_t>(table_properties->data_size)),
      ioptions_(ioptions),
      file_size_(file_size),
      table_properties_(nullptr) {}

PlainTableReader::~PlainTableReader() {
}

Status PlainTableReader::Open(const ImmutableCFOptions& ioptions,
                              const EnvOptions& env_options,
                              const InternalKeyComparator& internal_comparator,
                              unique_ptr<RandomAccessFileReader>&& file,
                              uint64_t file_size,
                              unique_ptr<TableReader>* table_reader,
                              const int bloom_bits_per_key,
                              double hash_table_ratio, size_t index_sparseness,
                              size_t huge_page_tlb_size, bool full_scan_mode) {
  if (file_size > PlainTableIndex::kMaxFileSize) {
    return Status::NotSupported("File is too large for PlainTableReader!");
  }

  TableProperties* props = nullptr;
  auto s = ReadTableProperties(file.get(), file_size, kPlainTableMagicNumber,
                               ioptions, &props);
  if (!s.ok()) {
    return s;
  }

  assert(hash_table_ratio >= 0.0);
  auto& user_props = props->user_collected_properties;
  auto prefix_extractor_in_file = props->prefix_extractor_name;

  if (!full_scan_mode &&
      /*efix_提取器_in_file.empty（）/*旧版sst文件*/
      &prefix提取程序在文件中！=“null pTr”）{
    如果（！）ioptions.prefix_提取器）
      返回状态：：invalidArgument（
          “打开生成的普通表时缺少前缀提取程序”
          “使用前缀提取程序”）；
    else if（前缀_extractor_in_file.compare（
                   ioptions.prefix_extractor->name（））！= 0）{
      返回状态：：invalidArgument（
          “给定的前缀提取程序与用于生成的前缀提取程序不匹配”
          “明码表”；
    }
  }

  编码类型编码类型=kplain；
  自动编码\类型\属性=
      用户属性查找（plainTablePropertyNames:：kencodingType）；
  如果（编码\类型\属性！=user_props.end（））
    encoding_type=static_cast<encoding type>
        decodeFixed32（encoding_type_prop->second.c_str（））；
  }

  std:：unique_ptr<plaintablereader>new_reader（new plaintablereader（
      ioptions，std：：move（file），env_选项，internal_comparator，
      编码类型、文件大小、属性）；

  s=new_reader->mmapdationFeeded（）；
  如果（！）S.O.（））{
    返回S；
  }

  如果（！）全扫描模式）
    s=new_reader->populateindex（props，bloom_bits_per_key，hash_table_ratio，
                                  索引稀疏度，巨大的页面大小）；
    如果（！）S.O.（））{
      返回S；
    }
  }否则{
    //指示它是完全扫描模式的标志，以便没有任何索引
    /可以使用。
    新的读卡器->全扫描模式uu=true；
  }

  *表读卡器=标准：：移动（新读卡器）；
  返回S；
}

void plainTableReader:：SetupForCompaction（）
}

InternalIterator*PlainTableReader:：NewIterator（const readoptions&options，
                                                竞技场*竞技场，
                                                bool skip_filters）_
  bool use_prefix_seek=！istotalOrderMode（）&&！选项。总订单搜索；
  如果（竞技场==nullptr）
    返回新的PlainTableIterator（这将使用前缀\u seek）；
  }否则{
    auto mem=arena->allocateAligned（sizeof（plainTableIterator））；
    返回new（mem）plainTableIterator（这个，使用前缀\u seek）；
  }
}

状态PlainTableReader:：PopulateIndexRecordList（
    plainTableIndexBuilder*索引生成器，vector<uint32_t>*前缀_hashes）
  切片前一个键前缀切片；
  std：：字符串prev_key_prefix_buf；
  uint32_t pos=数据\开始\偏移量\；

  bool isou firstou record=true；
  切片键\前缀\切片；
  PlainTableKeyDecoder解码器（&file_info_uuu，encoding_type_u，user_key_len_uuuu，&file_info_uu，encoding_type_u，user_key_u len_uuu，&file
                               ioptions前缀提取程序）；
  同时（pos<file_info_u.data_end_offset）
    uint32_t key_offset=pos；
    ParsedinInternalkey键；
    切片值_slice；
    bool seecable=false；
    状态S=下一个（&decoder，&pos，&key，nullptr，&value_slice，&seecable）；
    如果（！）S.O.（））{
      返回S；
    }

    key_prefix_slice=getPrefix（key）；
    如果（启用ou bloom_
      Bloom_u.AddHash（GetSliceHash（key.user_key））；
    }否则{
      如果（是“第一个记录”上一个键_前缀_切片！=key_prefix_slice）
        如果（！）是第一个记录吗？
          前缀_hashes->push_back（getsliehash（prev_key_prefix_slice））；
        }
        如果（文件信息为“命令”模式）
          prev_key_prefix_slice=key_prefix_slice；
        }否则{
          prev_key_prefix_buf=key_prefix_slice.toString（）；
          prev_key_prefix_slice=prev_key_prefix_buf；
        }
      }
    }

    index_builder->addkeyprefix（getprefix（key），key_offset）；

    如果（！）可查找&&是第一条记录）
      返回状态：：损坏（“前缀的键不可查找”）；
    }

    第一条记录为假；
  }

  前缀_hashes->push_back（getslicehash（key_prefix_slice））；
  auto s=index_.initfromrawdata（index_builder->finish（））；
  返回S；
}

void plainTableReader:：allocateAndFillBloom（int bloom_bits_per_key，
                                            int num_前缀，
                                            大小\u t超大\u页\u tlb \u大小，
                                            vector<uint32_t>*前缀_hashes）
  如果（！）istotalOrderMode（））
    uint32_t bloom_total_bits=num_prefixes*bloom_bits_per_key；
    如果（bloom_total_bits>0）
      启用“Bloom”=true；
      布卢姆·塞托阿尔比斯（&arena_uuu，布卢姆o total_u bits，ioptions uu.bloom u地区，
                          巨大的页面大小，ioptions，信息日志；
      FillBloom（前缀_hashes）；
    }
  }
}

void plainTableReader:：fillBloom（vector<uint32_t>>*prefix_hashes）
  断言（bloom_uu.isInitialized（））；
  for（自动前缀_hash:*前缀_hashes）
    bloom_u.addhash（前缀_hash）；
  }
}

status plainTableReader:：mmapdationFeeded（）
  如果（文件信息为“命令”模式）
    //获取mmap内存。
    返回文件“信息文件”->“读取”（0，文件“大小”，文件“信息文件”，“数据，空指针”）；
  }
  返回状态：：OK（）；
}

状态PlainTableReader:：PopulateIndex（TableProperties*Props，
                                       Int Bloom_Bits_per_键，
                                       双哈希表比率，
                                       尺寸指数稀疏度，
                                       尺寸特大页面TLB尺寸
  断言（道具）！= null pTr）；
  表“属性”重置（props）；

  块内容索引块内容；
  status s=readmetablock（file_info_.file.get（），nullptr/*预取缓冲区*/,

                           file_size_, kPlainTableMagicNumber, ioptions_,
                           PlainTableIndexBuilder::kPlainTableIndexBlock,
                           &index_block_contents);

  bool index_in_file = s.ok();

  BlockContents bloom_block_contents;
  bool bloom_in_file = false;
//如果索引块在文件中，我们只需要读取bloom块。
  if (index_in_file) {
    /*readmetablock（file_info_.file.get（），nullptr/*预取缓冲区*/，
                      文件大小，kPlainTableMagicNumber，ioptions，
                      BloomBlockBuilder:：KbloomBlock，&BloomBlock_内容）；
    bloom_in_file=s.ok（）&&bloom_block_contents.data.size（）>0；
  }

  切片*花块；
  如果（bloom_in_file）
    //如果bloom块的contents.allocation不是空的（这种情况下
    //对于非mmap模式），它保存bloom块的分配内存。
    //它需要保持活动才能使'bloom_block'保持有效。
    bloom_block_alloc_u=std:：move（bloom_block_contents.allocation）；
    bloom_block=&bloom_block_contents.data；
  }否则{
    bloom_block=nullptr；
  }

  切片*索引块；
  if（index_in_file）
    //如果index_block_contents.allocation不为空（即
    //对于非mmap模式），它保存索引块的分配内存。
    //需要保持活动状态才能使'index_block'保持有效。
    index_block_alloc_u=std:：move（index_block_contents.allocation）；
    index_block=&index_block_contents.data；
  }否则{
    索引块=nullptr；
  }

  if（（ioptions前缀提取程序=nullptr）&&
      （哈希表比率！= 0）{
    //对于基于哈希的查找，需要ioptons.prefix_提取器。
    返回状态：：不支持（
        “PlainTable需要前缀提取程序启用前缀哈希模式。”）；
  }

  //首先，读取整个文件，每个kindexintervalforameprefixkeys行
  //对于前缀（从第一个开始），生成（hash，
  //offset）并将其附加到indexrecordlist，这是一个创建的数据结构
  //存储它们。

  如果（！）_文件中的索引_
    //在此处为总订单模式分配Bloom筛选器。
    if（istotalOrderMode（））
      uint32_t num_bloom_位=
          static_cast<uint32_t>（table_properties_u->num_entries）*
          布卢姆每把钥匙都有一点；
      如果（num_bloom_位>0）
        启用“Bloom”=true；
        布卢姆·塞托阿尔比斯（&arena_uuu，num_o Bloom_Bits，IOptions_u.Bloom_地区，
                            巨大的页面大小，ioptions，信息日志；
      }
    }
  否则，如果（bloom_在_文件中）
    启用“Bloom”=true；
    auto num_blocks_property=props->user_collected_properties.find（
        普通表属性名称：：knumbloomblocks）；

    uint32_t num_块=0；
    如果（num_块_属性！=props->user_collected_properties.end（））
      切片温度切片（num_blocks_property->second）；
      如果（！）getvarint32（&temp_slice，&num_blocks））
        NUMIX块＝0；
      }
    }
    //取消常量限定符，因为不会更改bloom_u
    布卢姆·塞特拉瓦达（
        const_cast<unsigned char*>（）
            reinterpret_cast<const unsigned char*>（bloom_block->data（）），
        static_cast<uint32_t>（bloom_block->size（））*8，num_blocks）；
  }否则{
    //文件中有索引，但文件中没有bloom。在这种情况下禁用Bloom过滤器。
    启用“Bloom”=false；
    Bloom_Bits_per_键=0；
  }

  plaintableindexbuilder index_builder（&arena_uuu，ioptions_u，index_稀疏度，
                                       hash_table_比率，巨大的_page_tlb_大小）；

  std:：vector<uint32_t>前缀_hashes；
  如果（！）_文件中的索引_
    s=popultiendexrecordlist（&index_builder，&prefix_hashes）；
    如果（！）S.O.（））{
      返回S；
    }
  }否则{
    s=index_.initfromrawdata（*index_block）；
    如果（！）S.O.（））{
      返回S；
    }
  }

  如果（！）_文件中的索引_
    //计算的Bloom筛选器大小并为分配内存
    //根据前缀数进行bloom过滤，然后填充。
    allocateAndFillBloom（bloom_bits_per_key，index_.getNumPrefixes（），
                         巨大的_page_tlb_大小，&prefix_hashes）；
  }

  //填充两个表属性。
  如果（！）_文件中的索引_
    props->user_collected_properties[“plain_table_hash_table_size”]=
        ToString（index_.getIndexSize（）*PlainTableIndex:：KoffsetLen）；
    props->user_collected_properties[“plain_table_sub_index_size”]=
        toString（index_.getSubIndexSize（））；
  }否则{
    props->user_collected_properties[“plain_table_hash_table_size”]=
        ToStand（0）；
    props->user_collected_properties[“plain_table_sub_index_size”]=
        ToStand（0）；
  }

  返回状态：：OK（）；
}

状态PlainTableReader:：GetOffset（PlainTableKeyDecoder*解码器，
                                   常量切片和目标，常量切片和前缀，
                                   uint32_t prefix_hash，bool&prefix_matched，
                                   uint32_t*偏移）常量
  前缀匹配=假；
  uint32_t前缀_index_offset；
  auto res=index_.getoffset（前缀_hash，&prefix_index_offset）；
  if（res==plainTableIndex:：knoprefixForBucket）
    *offset=文件\信息\数据\结束\偏移；
    返回状态：：OK（）；
  else if（res==plaintableindex:：kdirecttofile）
    *offset=前缀_index_offset；
    返回状态：：OK（）；
  }

  //指向子索引，需要进行二进制搜索
  uint32_t上限；
  常量字符*基指针=
      index_u.getSubindexBaseptrandUpperbound（前缀_index_offset，&upper_bound）；
  uint32_t低=0；
  uint32_t high=上限；
  ParsedinInternalkey Mid_键；
  parsedinteralkey parsed_目标；
  如果（！）parseInternalKey（target，&parsed_target））
    返回状态：：损坏（slice（））；
  }

  //键在[低，高]之间。在两者之间进行二进制搜索。
  同时（高-低>1）
    uint32_t mid=（高+低）/2；
    uint32_t file_offset=getFixed32元素（base_ptr，mid）；
    UIT32 32 TMP；
    状态S=decoder->nextkenovalue（文件偏移、&mid-key、nullptr和tmp）；
    如果（！）S.O.（））{
      返回S；
    }
    int cmp_result=internal_comparator_u.compare（mid_key，parsed_target）；
    if（cmp_result<0）
      低=中；
    }否则{
      如果（cmp_result==0）
        //碰巧发现精确的键或目标小于
        //基极偏移后的第一个键。
        前缀匹配=真；
        *offset=文件偏移量；
        返回状态：：OK（）；
      }否则{
        高=中；
      }
    }
  }
  //低位或低位的两个键+1可以相同
  //前缀作为目标。我们得排除其中一个以免走
  //到错误的前缀。
  ParsedinInternalkey低_键；
  UIT32 32 TMP；
  uint32_t low_key_offset=getFixed32element（base_ptr，low）；
  状态S=解码器->NextKeyNovalue（低\u键偏移量，&low \u键，空指针，&tmp）；
  如果（！）S.O.（））{
    返回S；
  }

  if（getPrefix（low_key）==前缀）
    前缀匹配=真；
    *偏移量=低\键\偏移量；
  否则，如果（低+1<上限）
    //可能有下一个前缀，返回它
    前缀匹配=假；
    *offset=getFixed32element（基极指针，低+1）；
  }否则{
    //目标大于此bucket中最后一个前缀的键
    //但前缀不同。密钥不存在。
    *offset=文件\信息\数据\结束\偏移；
  }
  返回状态：：OK（）；
}

bool plainTableReader:：matchBloom（uint32_t hash）const_
  如果（！）启用“Bloom”
    回归真实；
  }

  如果（布卢姆可能含有烟灰（哈希））
    性能计数器添加（bloom-sst-hit-count，1）；
    回归真实；
  }否则{
    性能计数器添加（Bloom_sst_Miss_Count，1）；
    返回错误；
  }
}

状态：PlainTableReader:：Next（PlainTableKeyDecoder*解码器，uint32_t*偏移量，
                              parsedinteralkey*已分析的_键，
                              slice*内部\键，slice*值，
                              bool*seecable）const_
  if（*offset==file_info_u.data_end_offset）
    *offset=文件\信息\数据\结束\偏移；
    返回状态：：OK（）；
  }

  如果（*offset>file_info_u.data_end_offset）
    返回状态：：损坏（“偏移量超出文件大小”）；
  }

  uint32字节读取；
  状态S=解码器->下一个键（*偏移量，已解析的键，内部键，值，
                              &bytes_read，可查找）；
  如果（！）S.O.（））{
    返回S；
  }
  *offset=*offset+bytes_read；
  返回状态：：OK（）；
}

void plainTableReader：：准备（const slice&target）
  如果（启用ou bloom_
    uint32_t prefix_hash=getsliehash（getprefix（target））；
    布卢姆预取（前缀_hash）；
  }
}

status plaintablereader：：get（const readoptions&ro，const slice&target，
                             getContext*获取_context，bool skip_filters）
  //首先检查Bloom过滤器。
  切片前缀\切片；
  uint32_t前缀_hash；
  if（istotalOrderMode（））
    如果（全扫描模式）
      StutsUs=
          status:：invalidArgument（“get（）在完全扫描模式下不允许使用”）；
    }
    //匹配Bloom筛选器检查的整个用户密钥。
    如果（！）matchbloom（getsliehash（getuserkey（target）））
      返回状态：：OK（）；
    }
    //在total order模式下，只有一个bucket 0，我们总是使用空的
    / /前缀。
    前缀_slice=slice（）；
    前缀_hash=0；
  }否则{
    prefix_slice=getprefix（目标）；
    前缀_hash=getslicehash（前缀_slice）；
    如果（！）matchbloom（前缀_hash））
      返回状态：：OK（）；
    }
  }
  uint32_t偏移；
  bool前缀匹配；
  PlainTableKeyDecoder解码器（&file_info_uuu，encoding_type_u，user_key_len_uuuu，&file_info_uu，encoding_type_u，user_key_u len_uuu，&file
                               ioptions前缀提取程序）；
  状态s=getoffset（&decoder，target，prefix_slice，prefix_hash，
                       前缀“匹配与偏移”）；

  如果（！）S.O.（））{
    返回S；
  }
  ParsedinInternalkey找到了\u键；
  parsedinteralkey parsed_目标；
  如果（！）parseInternalKey（target，&parsed_target））
    返回状态：：损坏（slice（））；
  }
  切片找到值；
  while（offset<file_info_u.data_end_offset）
    s=下一个（&decoder，&offset，&found_key，nullptr，&found_value）；
    如果（！）S.O.（））{
      返回S；
    }
    如果（！）前缀匹配（{）
      //如果尚未找到第一个键，则需要验证其前缀
      /检查。
      如果（getprefix（found_key）！=前缀_slice）
        返回状态：：OK（）；
      }
      前缀_match=true；
    }
    //todo（ljin）：因为我们知道这里的关键比较结果，
    //我们能启用快速路径吗？
    if（内部_comparator_u.compare（found_key，parsed_target）>=0）
      如果（！）get_context->savevalue（found_key，found_value））
        断裂；
      }
    }
  }
  返回状态：：OK（）；
}

uint64_t plainTableReader:：approceoffsetof（const slice&key）
  返回0；
}

PlainTableIterator:：PlainTableIterator（PlainTableReader*表，
                                       bool使用前缀查找）
    ：表u（表），
      解码器uux（&table）->文件u信息u，表->编码u类型uu，
               table_u->user_key_len_u，table_->prefix_extractor_u，
      使用_前缀_seek_u（使用_前缀_seek）
  下一个偏移量
}

PlainTableIterator:：~PlainTableIterator（）
}

bool plainTableIterator:：valid（）常量
  返回偏移量表->文件信息数据结束偏移量和
         offset_>=table_->data_start_offset_；
}

void PlainTableIterator:：seektofFirst（）
  下一个_offset_u=table_u->data_start_offset_u；
  if（next_offset_>=table_->file_info_u.data_end_offset）
    下一个偏移量
  }否则{
    （下）；
  }
}

void PlainTableIterator:：seektolast（）
  断言（假）；
  status_u=status:：not supported（“seektolast（）在plaintable中不受支持”）；
}

void plainTableIterator:：seek（const slice&target）
  如果（使用前缀搜索）=！表_->istotalOrderMode（））
    //在此处执行此检查而不是newIterator（），以允许创建
    //total_order_seek=true的迭代器，即使我们无法seek（）。
    //它。压缩时需要这样做：它使用
    //totalou orderou seek=true，但通常不会对其执行seek（），
    //仅SeekToFirst（）。
    StutsUs=
        状态：：无效参数（
          “未为PlainTable实现总订单寻道。”）；
    offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
    返回；
  }

  //如果用户没有设置前缀查找选项，并且我们无法执行
  //total seek（）。断言失败。
  if（table_->istotalOrderMode（））
    if（表_->full_scan_mode_123;
      StutsUs=
          status:：invalidArgument（“seek（）在完全扫描模式下不允许使用”）；
      offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
      返回；
    else if（table_->getIndexSize（）>1）
      断言（假）；
      状态=状态：：不支持（
          “PlainTable不能发出非前缀查找，除非按总顺序”
          “模式”；
      offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
      返回；
    }
  }

  slice prefix_slice=table_uu->getprefix（目标）；
  uint32_t prefix_hash=0；
  //在TOTAL ORDER模式下，Bloom过滤器被忽略。
  如果（！）表_->istotalOrderMode（））
    前缀_hash=getslicehash（前缀_slice）；
    如果（！）表_->matchbloom（前缀_hash））
      offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
      返回；
    }
  }
  bool前缀匹配；
  状态_u=table_u->getoffset（&decoder_uux，target，prefix_slice，prefix_hash，
                              前缀“匹配”，下一个“偏移”；
  如果（！）StasuS.O.（））{
    offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
    返回；
  }

  if（下一个_o偏移量_<table_u->file_信息_u.data_结束_偏移量）
    for（next（）；status_.ok（）&&valid（）；next（））
      如果（！）前缀匹配（{）
        //需要验证第一个键的前缀
        如果（table_->getPrefix（key（））！=前缀_slice）
          offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
          断裂；
        }
        前缀_match=true；
      }
      if（table_->internal_comparator_.compare（key（），target）>=0）
        断裂；
      }
    }
  }否则{
    offset_=table_u->file_info_u.data_end_offset；
  }
}

void PlainTableIterator:：seekForRev（const slice&target）
  断言（假）；
  StutsUs=
      状态：：NotSupported（“SeekForRev（）在PlainTable中不受支持”）；
}

void PlainTableIterator:：Next（）
  offset_u=下一个_offset_uu；
  if（offset<table_u->file_info_u.data_end_offset）
    切片TMP_切片；
    parsedinteralkey parsed_键；
    StutsUs=
        表->next（&decoder_uu，&next_offset_uu，&parsed_key，&key_uu，&value_u）；
    如果（！）StasuS.O.（））{
      offset_u=下一个_u offset_u=表u->文件u信息u.data _结束_offset；
    }
  }
}

void PlainTableIterator:：prev（）
  断言（假）；
}

Slice PlainTableIterator:：key（）常量
  断言（valid（））；
  返回键；
}

Slice PlainTableIterator:：Value（）常量
  断言（valid（））；
  返回值；
}

status plainTableIterator:：status（）常量
  返回状态；
}

//命名空间rocksdb
endif//rocksdb_lite
