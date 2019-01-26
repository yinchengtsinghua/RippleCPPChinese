
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

#ifndef ROCKSDB_LITE

#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run this test... Skipping...\n");
  return 0;
}
#else

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <gflags/gflags.h>
#include <vector>
#include <string>
#include <map>

#include "table/meta_blocks.h"
#include "table/cuckoo_table_builder.h"
#include "table/cuckoo_table_reader.h"
#include "table/cuckoo_table_factory.h"
#include "table/get_context.h"
#include "util/arena.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::SetUsageMessage;

DEFINE_string(file_dir, "", "Directory where the files will be created"
    " for benchmark. Added for using tmpfs.");
DEFINE_bool(enable_perf, false, "Run Benchmark Tests too.");
DEFINE_bool(write, false,
    "Should write new values to file in performance tests?");
DEFINE_bool(identity_as_first_hash, true, "use identity as first hash");

namespace rocksdb {

namespace {
const uint32_t kNumHashFunc = 10;
//方法，与哈希函数相关的变量。
std::unordered_map<std::string, std::vector<uint64_t>> hash_map;

void AddHashLookups(const std::string& s, uint64_t bucket_id,
        uint32_t num_hash_fun) {
  std::vector<uint64_t> v;
  for (uint32_t i = 0; i < num_hash_fun; i++) {
    v.push_back(bucket_id + i);
  }
  hash_map[s] = v;
}

uint64_t GetSliceHash(const Slice& s, uint32_t index,
    uint64_t max_num_buckets) {
  return hash_map[s.ToString()][index];
}
}  //命名空间

class CuckooReaderTest : public testing::Test {
 public:
  using testing::Test::SetUp;

  CuckooReaderTest() {
    options.allow_mmap_reads = true;
    env = options.env;
    env_options = EnvOptions(options);
  }

  void SetUp(int num) {
    num_items = num;
    hash_map.clear();
    keys.clear();
    keys.resize(num_items);
    user_keys.clear();
    user_keys.resize(num_items);
    values.clear();
    values.resize(num_items);
  }

  std::string NumToStr(int64_t i) {
    return std::string(reinterpret_cast<char*>(&i), sizeof(i));
  }

  void CreateCuckooFileAndCheckReader(
      const Comparator* ucomp = BytewiseComparator()) {
    std::unique_ptr<WritableFile> writable_file;
    ASSERT_OK(env->NewWritableFile(fname, &writable_file, env_options));
    unique_ptr<WritableFileWriter> file_writer(
        new WritableFileWriter(std::move(writable_file), env_options));

    CuckooTableBuilder builder(
        file_writer.get(), 0.9, kNumHashFunc, 100, ucomp, 2, false, false,
        /*slicehash，0/*column_family_id*/，kDefaultColumnFamilyName）；
    断言_OK（builder.status（））；
    对于（uint32_t key_idx=0；key_idx<num_items；++key_idx）
      builder.add（slice（keys[key_idx]），slice（values[key_idx]）；
      断言_OK（builder.status（））；
      断言_eq（builder.numEntries（），key_idx+1）；
    }
    断言ou ok（builder.finish（））；
    断言_Eq（num_items，builder.numEntries（））；
    文件大小=builder.fileSize（）；
    断言_OK（file_writer->close（））；

    //现在检查读卡器。
    std:：unique_ptr<randomaccessfile>read_file；
    断言“OK”（env->newrandomaccessfile（fname，&read_file，env_options））；
    唯一的读卡器
        新建RandomAccessFileReader（std:：move（read_file），fname））；
    常量不变的选项（选项）；
    布谷表读卡器读卡器（ioptions，std:：move（file_reader），文件大小，ucomp，
                             GETSLICHASH；
    断言ou OK（reader.status（））；
    //假设没有合并/删除
    对于（uint32_t i=0；i<num_items；+i）
      pinnableslice值；
      getContext获取上下文（ucomp、nullptr、nullptr、nullptr，
                             getContext:：knotfound，slice（用户键[i]），&value，
                             nullptr、nullptr、nullptr、nullptr）；
      断言ou ok（reader.get（readoptions（），slice（keys[i]），&get_context））；
      断言_streq（values[i].c_str（），value.data（））；
    }
  }
  void updatekeys（bool with_zero_seqno）
    对于（uint32_t i=0；i<num_items；i++）
      parsedinteralkey ikey（用户键[i]，
          带着“零”号？0:i+1000，k型值）；
      键[i].clear（）；
      附录内部键（&keys[i]，ikey）；
    }
  }

  void checkIterator（const comparator*ucomp=bytewiseComparator（））
    std:：unique_ptr<randomaccessfile>read_file；
    断言“OK”（env->newrandomaccessfile（fname，&read_file，env_options））；
    唯一的读卡器
        新建RandomAccessFileReader（std:：move（read_file），fname））；
    常量不变的选项（选项）；
    布谷表读卡器读卡器（ioptions，std:：move（file_reader），文件大小，ucomp，
                             GETSLICHASH；
    断言ou OK（reader.status（））；
    InternalIterator*它=reader.newIterator（readOptions（），nullptr）；
    断言_OK（it->status（））；
    断言是真的（！）IT->有效（））；
    it->seektofirst（）；
    INTN＝0；
    while（it->valid（））
      断言_OK（it->status（））；
      assert_true（slice（keys[cnt]）==it->key（））；
      assert_true（slice（values[cnt]）==it->value（））；
      +CNT；
      IT-> NEXT（）；
    }
    assert_eq（static_cast<uint32_t>（cnt），num_items）；

    it->seektolast（）；
    cnt=static_cast<int>（num_items）-1；
    断言_true（it->valid（））；
    while（it->valid（））
      断言_OK（it->status（））；
      assert_true（slice（keys[cnt]）==it->key（））；
      assert_true（slice（values[cnt]）==it->value（））；
      CNT；
      IT->（）；
    }
    断言eq（cnt，-1）；

    cnt=static_cast<int>（num_items）/2；
    IT->SEEK（键[CNT]）；
    while（it->valid（））
      断言_OK（it->status（））；
      assert_true（slice（keys[cnt]）==it->key（））；
      assert_true（slice（values[cnt]）==it->value（））；
      +CNT；
      IT-> NEXT（）；
    }
    assert_eq（static_cast<uint32_t>（cnt），num_items）；
    删除它；

    竞技场竞技场；
    它=reader.newIterator（readoptions（），&arena）；
    断言_OK（it->status（））；
    断言是真的（！）IT->有效（））；
    it->seek（键[num_items/2]）；
    断言_true（it->valid（））；
    断言_OK（it->status（））；
    断言_true（keys[num_items/2]==it->key（））；
    assert_true（values[num_items/2]==it->value（））；
    断言_OK（it->status（））；
    它->~InternalIterator（）；
  }

  std:：vector<std:：string>keys；
  std:：vector<std:：string>用户键；
  std:：vector<std:：string>值；
  uint64_t num_项；
  std：：字符串fname；
  uint64_t文件大小；
  期权期权；
  Env＊Env；
  环境选项环境选项；
}；

测试_f
  设置（knumhashfunc）；
  fname=test:：tmpdir（）+“/cuckooreader_whenkeyexists”；
  对于（uint64_t i=0；i<num_items；i++）
    user_keys[i]=“键”+numtostr（i）；
    parsedinteralkey ikey（用户键[i]，i+1000，ktypevalue）；
    附录内部键（&keys[i]，ikey）；
    values[i]=“值”+numtostr（i）；
    //给出不相交的哈希值。
    addhashlookups（用户键[i]，i，knumhashfunc）；
  }
  CreateBuckooFileAndCheckReader（）；
  //最后一级文件。
  updateKeys（真）；
  CreateBuckooFileAndCheckReader（）；
  //碰撞测试。使所有哈希值冲突。
  hash_map.clear（）；
  对于（uint32_t i=0；i<num_items；i++）
    addhashlookups（用户_键[i]，0，knumhashfunc）；
  }
  updateKeys（错误）；
  CreateBuckooFileAndCheckReader（）；
  //最后一级文件。
  updateKeys（真）；
  CreateBuckooFileAndCheckReader（）；
}

测试_f（杜鹃读写器测试，使用uint64复合器时眼睛存在）
  设置（knumhashfunc）；
  fname=test:：tmpdir（）+“/cuckooreaderuint64_whenkeyexists”；
  对于（uint64_t i=0；i<num_items；i++）
    用户键[I].调整大小（8）；
    memcpy（&user_keys[i][0]，static_cast<void*>（&i），8）；
    parsedinteralkey ikey（用户键[i]，i+1000，ktypevalue）；
    附录内部键（&keys[i]，ikey）；
    values[i]=“值”+numtostr（i）；
    //给出不相交的哈希值。
    addhashlookups（用户键[i]，i，knumhashfunc）；
  }
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
  //最后一级文件。
  updateKeys（真）；
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
  //碰撞测试。使所有哈希值冲突。
  hash_map.clear（）；
  对于（uint32_t i=0；i<num_items；i++）
    addhashlookups（用户_键[i]，0，knumhashfunc）；
  }
  updateKeys（错误）；
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
  //最后一级文件。
  updateKeys（真）；
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
}

测试_f（布谷鸟读写器测试、检查迭代器）
  设置（2*knumhashfunc）；
  fname=test:：tmpdir（）+“/cuckooreader_checkiterator”；
  对于（uint64_t i=0；i<num_items；i++）
    user_keys[i]=“键”+numtostr（i）；
    parsedinteralkey ikey（用户键[i]，1000，ktypeValue）；
    附录内部键（&keys[i]，ikey）；
    values[i]=“值”+numtostr（i）；
    //以相反的顺序给出不相交的哈希值。
    addhashlookups（用户_键[i]，num_items-i-1，knumhashfunc）；
  }
  CreateBuckooFileAndCheckReader（）；
  checkIterator（）；
  //最后一级文件。
  updateKeys（真）；
  CreateBuckooFileAndCheckReader（）；
  checkIterator（）；
}

测试_f（布谷鸟读写器测试，检查迭代器或Int64）
  设置（2*knumhashfunc）；
  fname=test:：tmpdir（）+“/cuckooreader_checkiterator”；
  对于（uint64_t i=0；i<num_items；i++）
    用户键[I].调整大小（8）；
    memcpy（&user_keys[i][0]，static_cast<void*>（&i），8）；
    parsedinteralkey ikey（用户键[i]，1000，ktypeValue）；
    附录内部键（&keys[i]，ikey）；
    values[i]=“值”+numtostr（i）；
    //以相反的顺序给出不相交的哈希值。
    addhashlookups（用户_键[i]，num_items-i-1，knumhashfunc）；
  }
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
  checkIterator（test:：uint64 comparator（））；
  //最后一级文件。
  updateKeys（真）；
  创建BuckooFileAndCheckReader（test:：uint64 comparator（））；
  checkIterator（test:：uint64 comparator（））；
}

测试（布谷鸟读写器测试，未找到时）
  //添加具有冲突哈希值的键。
  设置（knumhashfunc）；
  fname=test:：tmpdir（）+“/cuckooreader_whenkeynotfound”；
  对于（uint64_t i=0；i<num_items；i++）
    user_keys[i]=“键”+numtostr（i）；
    parsedinteralkey ikey（用户键[i]，i+1000，ktypevalue）；
    附录内部键（&keys[i]，ikey）；
    values[i]=“值”+numtostr（i）；
    //使所有哈希值冲突。
    addhashlookups（用户_键[i]，0，knumhashfunc）；
  }
  auto*ucmp=bytewiseComparator（）；
  CreateBuckooFileAndCheckReader（）；
  std:：unique_ptr<randomaccessfile>read_file；
  断言“OK”（env->newrandomaccessfile（fname，&read_file，env_options））；
  唯一的读卡器
      新建RandomAccessFileReader（std:：move（read_file），fname））；
  常量不变的选项（选项）；
  布谷表读卡器读卡器（IOptions，std:：move（file_reader），文件大小，UCMP，
                           GETSLICHASH；
  断言ou OK（reader.status（））；
  //搜索具有冲突哈希值的键。
  std:：string not_found_user_key=“key”+numtostr（num_items）；
  std：：字符串未找到\u键；
  addhashlookups（未找到用户密钥，0，knumhashfunc）；
  parsedinteralkey ikey（未找到用户密钥，1000，ktypeValue）；
  appendInternalKey（&not_found_key，ikey）；
  pinnableslice值；
  getContext获取上下文（ucmp、nullptr、nullptr、nullptr、getContext:：knotfound，
                         slice（未找到\u key），&value，nullptr，nullptr，
                         nullptr，nullptr）；
  断言“OK”（reader.get（readoptions（），slice（not_found_key），&get_context））；
  断言_true（value.empty（））；
  断言ou OK（reader.status（））；
  //搜索具有独立哈希值的键。
  std:：string not_found_user_key2=“key”+numtostr（num_items+1）；
  addhashlookups（未找到用户key2、knumhashfunc、knumhashfunc）；
  parsedinternalkey ikey2（未找到用户密钥2，1000，ktypeValue）；
  std:：string not_found_key2；
  appendInternalKey（&not_found_key2，ikey2）；
  REST（）；
  获取上下文2（ucmp，nullptr，nullptr，nullptr，nullptr，
                          getContext:：knotfound，slice（not_found_key2），&value，
                          nullptr、nullptr、nullptr、nullptr）；
  断言“OK”（reader.get（readoptions（），slice（not_found_key2），&get_context2））；
  断言_true（value.empty（））；
  断言ou OK（reader.status（））；

  //当键是未使用的键时测试读取。
  std：：字符串未使用的_键=
    reader.getTableProperties（）->用户收集的属性。
    布谷鸟表属性名称：：kempykey）；
  //添加映射到空存储桶的哈希值。
  addHashLookups（extractUserKey（unused_key）.toString（），
      knumhashfunc、knumhashfunc）；
  REST（）；
  getContext获取_context3（ucmp，nullptr，nullptr，nullptr，
                          getContext:：knotfound，slice（未使用的_键），&value，
                          nullptr、nullptr、nullptr、nullptr）；
  断言ou ok（reader.get（readoptions（），slice（unused_key），&get_context3））；
  断言_true（value.empty（））；
  断言ou OK（reader.status（））；
}

//性能测试
命名空间{
void getkeys（uint64_t num，std:：vector<std:：string>*keys）
  键->清除（）；
  IterKey k；
  k.setInternalKey（“”，0，ktypeValue）；
  std:：string内部_key_suffix=k.getInternalKey（）.toString（）；
  assert_eq（static_cast<size_t>（8），internal_key_suffix.size（））；
  对于（uint64_t key_idx=0；key_idx<num；++key_idx）
    uint64_t value=2*键_idx；
    std:：string new_key（reinterpret_cast<char*>（&value），sizeof（value））；
    new_key+=内部_key_后缀；
    键->后推（新键）；
  }
}

std:：string getfilename（uint64_t num）
  if（flags_file_dir.empty（））
    标志_file_dir=test:：tmpdir（）；
  }
  返回标志_file_dir+“/cuckoo_read_benchmark”+
    ToString（num/1000000）+“mkeys”；
}

//创建最新级别的文件，因为我们有兴趣测量
//仅限最后一级文件。
void writefile（const std:：vector<std:：string>&keys，
    const uint64_t num，double hash_ratio）
  期权期权；
  选项。允许mmap读取=真；
  env*env=options.env；
  envOptions env_options=envOptions（选项）；
  std:：string fname=getfilename（num）；

  std:：unique_ptr<writable file>writable_file；
  断言_OK（env->newwritablefile（fname，&writable_file，env_options））；
  唯一的文件写入程序（
      新的可写文件编写器（std:：move（可写文件），env_选项））；
  布谷台建筑商（
      file_writer.get（），hash_ratio，64，1000，test:：uint64 Comparator（），5，
      false，将_identity_标记为_first_hash，nullptr，0/*列_family_id*/,

      kDefaultColumnFamilyName);
  ASSERT_OK(builder.status());
  for (uint64_t key_idx = 0; key_idx < num; ++key_idx) {
//值只是键的一部分。
    builder.Add(Slice(keys[key_idx]), Slice(&keys[key_idx][0], 4));
    ASSERT_EQ(builder.NumEntries(), key_idx + 1);
    ASSERT_OK(builder.status());
  }
  ASSERT_OK(builder.Finish());
  ASSERT_EQ(num, builder.NumEntries());
  ASSERT_OK(file_writer->Close());

  uint64_t file_size;
  env->GetFileSize(fname, &file_size);
  std::unique_ptr<RandomAccessFile> read_file;
  ASSERT_OK(env->NewRandomAccessFile(fname, &read_file, env_options));
  unique_ptr<RandomAccessFileReader> file_reader(
      new RandomAccessFileReader(std::move(read_file), fname));

  const ImmutableCFOptions ioptions(options);
  CuckooTableReader reader(ioptions, std::move(file_reader), file_size,
                           test::Uint64Comparator(), nullptr);
  ASSERT_OK(reader.status());
  ReadOptions r_options;
  PinnableSlice value;
//假设只触发快速路径
  GetContext get_context(nullptr, nullptr, nullptr, nullptr,
                         GetContext::kNotFound, Slice(), &value, nullptr,
                         nullptr, nullptr, nullptr);
  for (uint64_t i = 0; i < num; ++i) {
    value.Reset();
    value.clear();
    ASSERT_OK(reader.Get(r_options, Slice(keys[i]), &get_context));
    ASSERT_TRUE(Slice(keys[i]) == Slice(&keys[i][0], 4));
  }
}

void ReadKeys(uint64_t num, uint32_t batch_size) {
  Options options;
  options.allow_mmap_reads = true;
  Env* env = options.env;
  EnvOptions env_options = EnvOptions(options);
  std::string fname = GetFileName(num);

  uint64_t file_size;
  env->GetFileSize(fname, &file_size);
  std::unique_ptr<RandomAccessFile> read_file;
  ASSERT_OK(env->NewRandomAccessFile(fname, &read_file, env_options));
  unique_ptr<RandomAccessFileReader> file_reader(
      new RandomAccessFileReader(std::move(read_file), fname));

  const ImmutableCFOptions ioptions(options);
  CuckooTableReader reader(ioptions, std::move(file_reader), file_size,
                           test::Uint64Comparator(), nullptr);
  ASSERT_OK(reader.status());
  const UserCollectedProperties user_props =
    reader.GetTableProperties()->user_collected_properties;
  const uint32_t num_hash_fun = *reinterpret_cast<const uint32_t*>(
      user_props.at(CuckooTablePropertyNames::kNumHashFunc).data());
  const uint64_t table_size = *reinterpret_cast<const uint64_t*>(
      user_props.at(CuckooTablePropertyNames::kHashTableSize).data());
  fprintf(stderr, "With %" PRIu64 " items, utilization is %.2f%%, number of"
      " hash functions: %u.\n", num, num * 100.0 / (table_size), num_hash_fun);
  ReadOptions r_options;

  std::vector<uint64_t> keys;
  keys.reserve(num);
  for (uint64_t i = 0; i < num; ++i) {
    keys.push_back(2 * i);
  }
  std::random_shuffle(keys.begin(), keys.end());

  PinnableSlice value;
//假设只触发快速路径
  GetContext get_context(nullptr, nullptr, nullptr, nullptr,
                         GetContext::kNotFound, Slice(), &value, nullptr,
                         nullptr, nullptr, nullptr);
  uint64_t start_time = env->NowMicros();
  if (batch_size > 0) {
    for (uint64_t i = 0; i < num; i += batch_size) {
      for (uint64_t j = i; j < i+batch_size && j < num; ++j) {
        reader.Prepare(Slice(reinterpret_cast<char*>(&keys[j]), 16));
      }
      for (uint64_t j = i; j < i+batch_size && j < num; ++j) {
        reader.Get(r_options, Slice(reinterpret_cast<char*>(&keys[j]), 16),
                   &get_context);
      }
    }
  } else {
    for (uint64_t i = 0; i < num; i++) {
      reader.Get(r_options, Slice(reinterpret_cast<char*>(&keys[i]), 16),
                 &get_context);
    }
  }
  float time_per_op = (env->NowMicros() - start_time) * 1.0f / num;
  fprintf(stderr,
      "Time taken per op is %.3fus (%.1f Mqps) with batch size of %u\n",
      time_per_op, 1.0 / time_per_op, batch_size);
}
}  //命名空间。

TEST_F(CuckooReaderTest, TestReadPerformance) {
  if (!FLAGS_enable_perf) {
    return;
  }
  double hash_ratio = 0.95;
//这些数字的哈希利用率接近
//分别为0.9、0.75、0.6和0.5。
//它们都创造了128米的水桶。
  std::vector<uint64_t> nums = {120*1024*1024, 100*1024*1024, 80*1024*1024,
    70*1024*1024};
#ifndef NDEBUG
  fprintf(stdout,
      "WARNING: Not compiled with DNDEBUG. Performance tests may be slow.\n");
#endif
  for (uint64_t num : nums) {
    if (FLAGS_write ||
        Env::Default()->FileExists(GetFileName(num)).IsNotFound()) {
      std::vector<std::string> all_keys;
      GetKeys(num, &all_keys);
      WriteFile(all_keys, num, hash_ratio);
    }
    ReadKeys(num, 0);
    ReadKeys(num, 10);
    ReadKeys(num, 25);
    ReadKeys(num, 50);
    ReadKeys(num, 100);
    fprintf(stderr, "\n");
  }
}
}  //命名空间rocksdb

int main(int argc, char** argv) {
  if (rocksdb::port::kLittleEndian) {
    ::testing::InitGoogleTest(&argc, argv);
    ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
  }
  else {
    fprintf(stderr, "SKIPPED as Cuckoo table doesn't support Big Endian\n");
    return 0;
  }
}

#endif  //GFLAGS

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as Cuckoo table is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //摇滚乐
