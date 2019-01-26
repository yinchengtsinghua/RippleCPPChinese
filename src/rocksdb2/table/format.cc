
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

#include "table/format.h"

#include <string>
#include <inttypes.h>

#include "monitoring/perf_context_imp.h"
#include "monitoring/statistics.h"
#include "rocksdb/env.h"
#include "table/block.h"
#include "table/block_based_table_reader.h"
#include "table/persistent_cache_helper.h"
#include "util/coding.h"
#include "util/compression.h"
#include "util/crc32c.h"
#include "util/file_reader_writer.h"
#include "util/logging.h"
#include "util/stop_watch.h"
#include "util/string_util.h"
#include "util/xxhash.h"

namespace rocksdb {

extern const uint64_t kLegacyBlockBasedTableMagicNumber;
extern const uint64_t kBlockBasedTableMagicNumber;

#ifndef ROCKSDB_LITE
extern const uint64_t kLegacyPlainTableMagicNumber;
extern const uint64_t kPlainTableMagicNumber;
#else
//RocksDB-Lite没有普通的桌子
const uint64_t kLegacyPlainTableMagicNumber = 0;
const uint64_t kPlainTableMagicNumber = 0;
#endif
const uint32_t DefaultStackBufferSize = 5000;

bool ShouldReportDetailedTime(Env* env, Statistics* stats) {
  return env != nullptr && stats != nullptr &&
         stats->stats_level_ > kExceptDetailedTimers;
}

void BlockHandle::EncodeTo(std::string* dst) const {
//健全性检查所有字段是否已设置
  assert(offset_ != ~static_cast<uint64_t>(0));
  assert(size_ != ~static_cast<uint64_t>(0));
  PutVarint64Varint64(dst, offset_, size_);
}

Status BlockHandle::DecodeFrom(Slice* input) {
  if (GetVarint64(input, &offset_) &&
      GetVarint64(input, &size_)) {
    return Status::OK();
  } else {
//部分解码后故障复位
    offset_ = 0;
    size_ = 0;
    return Status::Corruption("bad block handle");
  }
}

//返回包含句柄副本的字符串。
std::string BlockHandle::ToString(bool hex) const {
  std::string handle_str;
  EncodeTo(&handle_str);
  if (hex) {
    return Slice(handle_str).ToString(true);
  } else {
    return handle_str;
  }
}

const BlockHandle BlockHandle::kNullBlockHandle(0, 0);

namespace {
inline bool IsLegacyFooterFormat(uint64_t magic_number) {
  return magic_number == kLegacyBlockBasedTableMagicNumber ||
         magic_number == kLegacyPlainTableMagicNumber;
}
inline uint64_t UpconvertLegacyFooterFormat(uint64_t magic_number) {
  if (magic_number == kLegacyBlockBasedTableMagicNumber) {
    return kBlockBasedTableMagicNumber;
  }
  if (magic_number == kLegacyPlainTableMagicNumber) {
    return kPlainTableMagicNumber;
  }
  assert(false);
  return 0;
}
}  //命名空间

//旧页脚格式：
//元索引句柄（varint64偏移量，varint64大小）
//索引句柄（varint64偏移量，varint64大小）
//<padding>使总大小为2*blockhandle:：kmaxEncodedLength
//表\u幻数（8字节）
//新页脚格式：
//校验和类型（字符，1字节）
//元索引句柄（varint64偏移量，varint64大小）
//索引句柄（varint64偏移量，varint64大小）
//<padding>使总大小为2*blockhandle:：kmaxEncodedLength+1
//页脚版本（4字节）
//表\u幻数（8字节）
void Footer::EncodeTo(std::string* dst) const {
  assert(HasInitializedTableMagicNumber());
  if (IsLegacyFooterFormat(table_magic_number())) {
//必须是具有旧页脚的默认校验和
    assert(checksum_ == kCRC32c);
    const size_t original_size = dst->size();
    metaindex_handle_.EncodeTo(dst);
    index_handle_.EncodeTo(dst);
dst->resize(original_size + 2 * BlockHandle::kMaxEncodedLength);  //衬垫
    PutFixed32(dst, static_cast<uint32_t>(table_magic_number() & 0xffffffffu));
    PutFixed32(dst, static_cast<uint32_t>(table_magic_number() >> 32));
    assert(dst->size() == original_size + kVersion0EncodedLength);
  } else {
    const size_t original_size = dst->size();
    dst->push_back(static_cast<char>(checksum_));
    metaindex_handle_.EncodeTo(dst);
    index_handle_.EncodeTo(dst);
dst->resize(original_size + kNewVersionsEncodedLength - 12);  //衬垫
    PutFixed32(dst, version());
    PutFixed32(dst, static_cast<uint32_t>(table_magic_number() & 0xffffffffu));
    PutFixed32(dst, static_cast<uint32_t>(table_magic_number() >> 32));
    assert(dst->size() == original_size + kNewVersionsEncodedLength);
  }
}

Footer::Footer(uint64_t _table_magic_number, uint32_t _version)
    : version_(_version),
      checksum_(kCRC32c),
      table_magic_number_(_table_magic_number) {
//这应该由构造函数调用方保证
  assert(!IsLegacyFooterFormat(_table_magic_number) || version_ == 0);
}

Status Footer::DecodeFrom(Slice* input) {
  assert(!HasInitializedTableMagicNumber());
  assert(input != nullptr);
  assert(input->size() >= kMinEncodedLength);

  const char *magic_ptr =
      input->data() + input->size() - kMagicNumberLengthByte;
  const uint32_t magic_lo = DecodeFixed32(magic_ptr);
  const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
  uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) |
                    (static_cast<uint64_t>(magic_lo)));

//我们在这里检查旧格式，然后静默地向上转换它们
  bool legacy = IsLegacyFooterFormat(magic);
  if (legacy) {
    magic = UpconvertLegacyFooterFormat(magic);
  }
  set_table_magic_number(magic);

  if (legacy) {
//该尺寸已被认定为至少为kminencodedlength。
//在函数的开头
    input->remove_prefix(input->size() - kVersion0EncodedLength);
    /*Sion_=0/*旧版*/；
    校验和=KCRC32C；
  }否则{
    version_uu=decodefixed32（magic_ptr-4）；
    //页脚版本1及更高版本将始终占用这么多字节。
    //它由校验和类型、两个块句柄、填充、
    //版本号和幻数
    if（input->size（）<knewversionsEncodedLength）
      返回状态：：损坏（“输入太短，不能是sstable”）；
    }否则{
      input->remove_prefix（input->size（）-knewversionsEncodedLength）；
    }
    uint32英寸；
    如果（！）getvarint32（输入，&chksum））
      返回状态：：损坏（“错误校验和类型”）；
    }
    checksum_uux static_cast<checksumtype>（chksum）；
  }

  状态结果=metaindex_handle_.decodeFrom（输入）；
  if（result.ok（））
    结果=索引句柄解码（输入）；
  }
  if（result.ok（））
    //跳过“input”中的所有剩余数据（现在只填充）
    const char*end=magic_ptr+kmagicNumberLengthByte；
    *输入=切片（end，input->data（）+input->size（）-end）；
  }
  返回结果；
}

std:：string footer:：toString（）常量
  std：：字符串结果，句柄_uu；
  结果.准备金（1024）；

  bool legacyfooterformat（table_magic_number_uu）；
  如果（遗产）{
    result.append（“metaindex句柄：”+metaindex_handle_.toString（）+“\n”）；
    result.append（“index handle：”+index_handle_.toString（）+“\n”）；
    result.append（“表\u幻数：”+
                  rocksdb:：tostring（table_magic_number_ux）+\n”）；
  }否则{
    result.append（“校验和：”+rocksdb:：toString（checksum_x）+“\n”）；
    result.append（“metaindex句柄：”+metaindex_handle_.toString（）+“\n”）；
    result.append（“index handle：”+index_handle_.toString（）+“\n”）；
    result.append（“页脚版本：”+rocksdb:：toString（version_ux+“\n”）；
    result.append（“表\u幻数：”+
                  rocksdb:：tostring（table_magic_number_ux）+\n”）；
  }
  返回结果；
}

状态readfooterfromfile（randomaaccessfilereader*文件，
                          filePrefetchBuffer*预取缓冲区，
                          uint64_t文件大小，页脚*页脚，
                          uint64_t强制_table_magic_number）
  if（文件大小<页脚：：kminencodedlength）
    返回状态：：损坏（
      “文件太短（”+ToString（file_size）+“bytes”）不能是”
      “sstable：”+file->file_name（））；
  }

  字符页脚\空格[页脚：：kmaxEncodedLength]；
  切片页脚输入；
  大小读取偏移=
      （文件大小>页脚：：kmaxEncodedLength）
          ？静态强制转换（文件大小-页脚：：kmaxEncodedLength）
          ：0；
  状态S；
  if（预取缓冲区==nullptr
      ！预取缓冲区->TryReadFromCache（读取偏移量，页脚：：kmaxEncodedLength，
                                         页脚输入）
    S=文件->读取（读取偏移量，页脚：：kmaxEncodedLength，&页脚输入，
                   脚注空间；
    如果（！）s.ok（））返回s；
  }

  //检查是否从文件中读取整个页脚。它可能是
  //这个尺寸不正确。
  if（footer_input.size（）<footer:：kminencodedlength）
    返回状态：：损坏（
      “文件太短（”+ToString（file_size）+“bytes”）不能是”
      “sstable”+file->file_name（））；
  }

  S=footer->decodefrom（&footer_input）；
  如果（！）S.O.（））{
    返回S；
  }
  如果（强制执行表格魔法编号！= 0 & &
      强制执行表格魔法编号！=footer->table_magic_number（））
    返回状态：：损坏（
      “错误的表幻数：应为”
      +ToString（强制执行表格\u幻数）+“，找到”
      +toString（footer->table_magic_number（））
      +“in”+file->file_name（））；
  }
  返回状态：：OK（）；
}

//如果此处没有匿名命名空间，则警告-wMissing原型将失败
命名空间{
状态检查块校验和（const readoptions&options，const footer&footer，
                          常量切片和内容，大小块大小，
                          RandomAccessFileReader*文件，
                          const blockhandle和handle）
  状态S；
  //检查类型和块内容的CRC
  if（选项。验证_校验和）
    const char*data=contents.data（）；//指向读取位置的指针
    性能计时器保护（块校验和时间）；
    uint32_t value=decodefixed32（数据+块大小+1）；
    uint32_t实际值=0；
    开关（footer.checksum（））
      案例Knochecksum：
        断裂；
      案例KCRC32 C：
        value=crc32c:：unmask（value）；
        实际值=crc32c：：值（数据，块大小+1）；
        断裂；
      案例：
        实际值=xxh32（数据，静态\u cast<int>（块\大小）+1，0）；
        断裂；
      违约：
        S=状态：：损坏（
            “未知的校验和类型”+ToString（footer.checksum（））+“in”+
            file->file_name（）+“offset”+toString（handle.offset（））+
            “尺寸”+ToString（块尺寸））；
    }
    如果（s.ok（）&&actual！=值）{
      S=状态：：损坏（
          “块校验和不匹配：应为”+ToString（实际）+“，得到”+
          toString（value）+“in”+file->file_name（）+“offset”+
          ToString（handle.offset（））+“大小”+ToString（block_size））；
    }
    如果（！）S.O.（））{
      返回S；
    }
  }
  返回S；
}

//读取块并检查其CRC
//内容是读取的结果。
//根据file->read的实现，内容不能指向buf
状态读块（randomaaccessfilereader*文件，常量页脚和页脚，
                 const readoptions&options、const blockhandle&handle、const readoptions&options、const blockhandle&handle、
                 切片*内容，/*读取结果*/ char* buf) {

  size_t n = static_cast<size_t>(handle.size());
  Status s;

  {
    PERF_TIMER_GUARD(block_read_time);
    s = file->Read(handle.offset(), n + kBlockTrailerSize, contents, buf);
  }

  PERF_COUNTER_ADD(block_read_count, 1);
  PERF_COUNTER_ADD(block_read_byte, n + kBlockTrailerSize);

  if (!s.ok()) {
    return s;
  }
  if (contents->size() != n + kBlockTrailerSize) {
    return Status::Corruption("truncated block read from " + file->file_name() +
                              " offset " + ToString(handle.offset()) +
                              ", expected " + ToString(n + kBlockTrailerSize) +
                              " bytes, got " + ToString(contents->size()));
  }
  return CheckBlockChecksum(options, footer, *contents, n, file, handle);
}

}  //命名空间

Status ReadBlockContents(RandomAccessFileReader* file,
                         FilePrefetchBuffer* prefetch_buffer,
                         const Footer& footer, const ReadOptions& read_options,
                         const BlockHandle& handle, BlockContents* contents,
                         const ImmutableCFOptions& ioptions,
                         bool decompression_requested,
                         const Slice& compression_dict,
                         const PersistentCacheOptions& cache_options) {
  Status status;
  Slice slice;
  size_t n = static_cast<size_t>(handle.size());
  std::unique_ptr<char[]> heap_buf;
  char stack_buf[DefaultStackBufferSize];
  char* used_buf = nullptr;
  rocksdb::CompressionType compression_type;

  if (cache_options.persistent_cache &&
      !cache_options.persistent_cache->IsCompressed()) {
    status = PersistentCacheHelper::LookupUncompressedPage(cache_options,
                                                           handle, contents);
    if (status.ok()) {
//找到块句柄的未压缩页
      return status;
    } else {
//找不到未压缩的页
      if (ioptions.info_log && !status.IsNotFound()) {
        assert(!status.ok());
        ROCKS_LOG_INFO(ioptions.info_log,
                       "Error reading from persistent cache. %s",
                       status.ToString().c_str());
      }
    }
  }

  bool got_from_prefetch_buffer = false;
  if (prefetch_buffer != nullptr &&
      prefetch_buffer->TryReadFromCache(
          handle.offset(),
          static_cast<size_t>(handle.size()) + kBlockTrailerSize, &slice)) {
    status =
        CheckBlockChecksum(read_options, footer, slice,
                           static_cast<size_t>(handle.size()), file, handle);
    if (!status.ok()) {
      return status;
    }
    got_from_prefetch_buffer = true;
    used_buf = const_cast<char*>(slice.data());
  } else if (cache_options.persistent_cache &&
             cache_options.persistent_cache->IsCompressed()) {
//查找未压缩缓存模式p-cache
    status = PersistentCacheHelper::LookupRawPage(
        cache_options, handle, &heap_buf, n + kBlockTrailerSize);
  } else {
    status = Status::NotFound();
  }

  if (!got_from_prefetch_buffer) {
    if (status.ok()) {
//缓存命中
      used_buf = heap_buf.get();
      slice = Slice(heap_buf.get(), n);
    } else {
      if (ioptions.info_log && !status.IsNotFound()) {
        assert(!status.ok());
        ROCKS_LOG_INFO(ioptions.info_log,
                       "Error reading from persistent cache. %s",
                       status.ToString().c_str());
      }
//缓存未读设备
      if (decompression_requested &&
          n + kBlockTrailerSize < DefaultStackBufferSize) {
//如果我们有足够小的数据块，把它读到
//无需完整的malloc（），只需简单地分配堆栈缓冲区
        used_buf = &stack_buf[0];
      } else {
        heap_buf = std::unique_ptr<char[]>(new char[n + kBlockTrailerSize]);
        used_buf = heap_buf.get();
      }

      status = ReadBlock(file, footer, read_options, handle, &slice, used_buf);
      if (status.ok() && read_options.fill_cache &&
          cache_options.persistent_cache &&
          cache_options.persistent_cache->IsCompressed()) {
//插入到原始缓存
        PersistentCacheHelper::InsertRawPage(cache_options, handle, used_buf,
                                             n + kBlockTrailerSize);
      }
    }

    if (!status.ok()) {
      return status;
    }
  }

  PERF_TIMER_GUARD(block_decompress_time);

  compression_type = static_cast<rocksdb::CompressionType>(slice.data()[n]);

  if (decompression_requested && compression_type != kNoCompression) {
//压缩页面、解压缩、更新缓存
    status = UncompressBlockContents(slice.data(), n, contents,
                                     footer.version(), compression_dict,
                                     ioptions);
  } else if (slice.data() != used_buf) {
//切片内容不是提供的缓冲区
    *contents = BlockContents(Slice(slice.data(), n), false, compression_type);
  } else {
//页面未压缩，提供了堆栈或堆的缓冲区
    if (got_from_prefetch_buffer || used_buf == &stack_buf[0]) {
      heap_buf = std::unique_ptr<char[]>(new char[n]);
      memcpy(heap_buf.get(), used_buf, n);
    }
    *contents = BlockContents(std::move(heap_buf), n, true, compression_type);
  }

  if (status.ok() && !got_from_prefetch_buffer && read_options.fill_cache &&
      cache_options.persistent_cache &&
      !cache_options.persistent_cache->IsCompressed()) {
//插入到未压缩缓存
    PersistentCacheHelper::InsertUncompressedPage(cache_options, handle,
                                                  *contents);
  }

  return status;
}

Status UncompressBlockContentsForCompressionType(
    const char* data, size_t n, BlockContents* contents,
    uint32_t format_version, const Slice& compression_dict,
    CompressionType compression_type, const ImmutableCFOptions &ioptions) {
  std::unique_ptr<char[]> ubuf;

  assert(compression_type != kNoCompression && "Invalid compression type");

  StopWatchNano timer(ioptions.env,
    ShouldReportDetailedTime(ioptions.env, ioptions.statistics));
  int decompress_size = 0;
  switch (compression_type) {
    case kSnappyCompression: {
      size_t ulength = 0;
      static char snappy_corrupt_msg[] =
        "Snappy not supported or corrupted Snappy compressed block contents";
      if (!Snappy_GetUncompressedLength(data, n, &ulength)) {
        return Status::Corruption(snappy_corrupt_msg);
      }
      ubuf.reset(new char[ulength]);
      if (!Snappy_Uncompress(data, n, ubuf.get())) {
        return Status::Corruption(snappy_corrupt_msg);
      }
      *contents = BlockContents(std::move(ubuf), ulength, true, kNoCompression);
      break;
    }
    case kZlibCompression:
      ubuf.reset(Zlib_Uncompress(
          data, n, &decompress_size,
          GetCompressFormatForVersion(kZlibCompression, format_version),
          compression_dict));
      if (!ubuf) {
        static char zlib_corrupt_msg[] =
          "Zlib not supported or corrupted Zlib compressed block contents";
        return Status::Corruption(zlib_corrupt_msg);
      }
      *contents =
          BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    case kBZip2Compression:
      ubuf.reset(BZip2_Uncompress(
          data, n, &decompress_size,
          GetCompressFormatForVersion(kBZip2Compression, format_version)));
      if (!ubuf) {
        static char bzip2_corrupt_msg[] =
          "Bzip2 not supported or corrupted Bzip2 compressed block contents";
        return Status::Corruption(bzip2_corrupt_msg);
      }
      *contents =
          BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    case kLZ4Compression:
      ubuf.reset(LZ4_Uncompress(
          data, n, &decompress_size,
          GetCompressFormatForVersion(kLZ4Compression, format_version),
          compression_dict));
      if (!ubuf) {
        static char lz4_corrupt_msg[] =
          "LZ4 not supported or corrupted LZ4 compressed block contents";
        return Status::Corruption(lz4_corrupt_msg);
      }
      *contents =
          BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    case kLZ4HCCompression:
      ubuf.reset(LZ4_Uncompress(
          data, n, &decompress_size,
          GetCompressFormatForVersion(kLZ4HCCompression, format_version),
          compression_dict));
      if (!ubuf) {
        static char lz4hc_corrupt_msg[] =
          "LZ4HC not supported or corrupted LZ4HC compressed block contents";
        return Status::Corruption(lz4hc_corrupt_msg);
      }
      *contents =
          BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    case kXpressCompression:
      ubuf.reset(XPRESS_Uncompress(data, n, &decompress_size));
      if (!ubuf) {
        static char xpress_corrupt_msg[] =
          "XPRESS not supported or corrupted XPRESS compressed block contents";
        return Status::Corruption(xpress_corrupt_msg);
      }
      *contents =
        BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    case kZSTD:
    case kZSTDNotFinalCompression:
      ubuf.reset(ZSTD_Uncompress(data, n, &decompress_size, compression_dict));
      if (!ubuf) {
        static char zstd_corrupt_msg[] =
            "ZSTD not supported or corrupted ZSTD compressed block contents";
        return Status::Corruption(zstd_corrupt_msg);
      }
      *contents =
          BlockContents(std::move(ubuf), decompress_size, true, kNoCompression);
      break;
    default:
      return Status::Corruption("bad block type");
  }

  if(ShouldReportDetailedTime(ioptions.env, ioptions.statistics)){
    MeasureTime(ioptions.statistics, DECOMPRESSION_TIMES_NANOS,
      timer.ElapsedNanos());
    MeasureTime(ioptions.statistics, BYTES_DECOMPRESSED, contents->data.size());
    RecordTick(ioptions.statistics, NUMBER_BLOCK_DECOMPRESSED);
  }

  return Status::OK();
}

//
//“data”指向从文件中读取的原始块内容。
//此方法分配新的堆缓冲区和原始块
//内容被解压缩到这个缓冲区。这个
//缓冲区通过“result”返回，由调用方
//释放这个缓冲区。
//format_version是include/rocksdb/table.h中定义的块格式。
Status UncompressBlockContents(const char* data, size_t n,
                               BlockContents* contents, uint32_t format_version,
                               const Slice& compression_dict,
                               const ImmutableCFOptions &ioptions) {
  assert(data[n] != kNoCompression);
  return UncompressBlockContentsForCompressionType(
      data, n, contents, format_version, compression_dict,
      (CompressionType)data[n], ioptions);
}

}  //命名空间rocksdb
