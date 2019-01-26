
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

#pragma once
#ifndef ROCKSDB_LITE

#include <string>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/stackable_db.h"

namespace rocksdb {
namespace spatial {

//注意：SPATIALDB是实验性的，我们可以在不发出警告的情况下更改其API。
//在开发SPATIALDB API之前，请与我们联系。
//
//SPATIALDB是对RocksDB顶部构建的空间索引的支持。
//创建新的SPATIALDB时，客户机将指定空间索引列表
//以他们的数据为基础。每个空间索引由区域和
//粒度。如果要存储地图数据，请使用不同的空间索引
//粒度可以用于不同的缩放级别。
//
//插入SPATIALDB的每个元素都具有：
//*一个边界框，用于确定如何为元素编制索引。
//*字符串blob，通常是多边形的wkb表示。
//（http://en.wikipedia.org/wiki/well-known\u文本）
//*功能集，是键值对的映射，其中值可以为空，
//int、double、bool、string
//*要插入元素的索引列表
//
//每个查询都在一个空间索引上执行。查询保证
//将返回与指定边界框相交的所有元素，但它
//还可能返回一些额外的非相交元素。

//variant是一个可以是很多东西的类：null、bool、int、double或string
//它用于在Featureset中存储不同的值类型（见下文）
struct Variant {
//不要更改此处的值，它们将保留在磁盘上
  enum Type {
    kNull = 0x0,
    kBool = 0x1,
    kInt = 0x2,
    kDouble = 0x3,
    kString = 0x4,
  };

  Variant() : type_(kNull) {}
  /*隐式*/variant（bool b）：type_（kbool）data_.b=b；
  /*隐式*/ Variant(uint64_t i) : type_(kInt) { data_.i = i; }

  /*隐式*/variant（double d）：类型（kdouble）数据.d=d；
  /*隐式*/ Variant(const std::string& s) : type_(kString) {

    new (&data_.s) std::string(s);
  }

  Variant(const Variant& v) : type_(v.type_) { Init(v, data_); }

  Variant& operator=(const Variant& v);

  Variant(Variant&& rhs) : type_(kNull) { *this = std::move(rhs); }

  Variant& operator=(Variant&& v);

  ~Variant() { Destroy(type_, data_); }

  Type type() const { return type_; }
  bool get_bool() const { return data_.b; }
  uint64_t get_int() const { return data_.i; }
  double get_double() const { return data_.d; }
  const std::string& get_string() const { return *GetStringPtr(data_); }

  bool operator==(const Variant& other) const;
  bool operator!=(const Variant& other) const { return !(*this == other); }

 private:
  Type type_;

  union Data {
    bool b;
    uint64_t i;
    double d;
//当前版本的MS编译器不兼容C++ 11所以不能放
//STD：：字符串
//但是，即使这样，我们仍然需要其余的维护。
    char s[sizeof(std::string)];
  } data_;

//避免类型混淆问题
  static std::string* GetStringPtr(Data& d) {
    void* p = d.s;
    return reinterpret_cast<std::string*>(p);
  }

  static const std::string* GetStringPtr(const Data& d) {
    const void* p = d.s;
    return reinterpret_cast<const std::string*>(p);
  }

  static void Init(const Variant&, Data&);

  static void Destroy(Type t, Data& d) {
    if (t == kString) {
      using std::string;
      GetStringPtr(d)->~string();
    }
  }
};

//Featureset是键-值对的映射。一个功能集与
//SPATIALDB中的每个元素。它可以用来添加元素的丰富数据。
class FeatureSet {
 private:
  typedef std::unordered_map<std::string, Variant> map;

 public:
  class iterator {
   public:
    /*隐式*/迭代器（const map:：const_迭代器itr）：itr_u（itr）
    迭代器和运算符++（）
      +ILTZ；
      返回这个；
    }
    布尔操作员！=（const iterator&other）返回itr_uu！=其他。
    bool operator==（const iterator&other）返回itr_==other.itr_
    map:：value_type operator*（）返回*itr_

   私人：
    map：：const_迭代器itr_uu；
  }；
  featureset（）=默认值；

  功能集*集（const std:：string&key，const variant&value）；
  bool包含（const std:：string&key）const；
  //需要：包含（键）
  const variant&get（const std:：string&key）const；
  迭代器查找（const std:：string&key）const；

  迭代器begin（）const返回map_.begin（）；
  迭代器end（）const返回map_.end（）；

  空隙清除（）；
  size_t size（）常量返回map_u.size（）；

  void serialize（std:：string*output）常量；
  //必需：空FeatureSet
  bool反序列化（const slice&input）；

  std:：string debugString（）常量；

 私人：
  地图地图；
}；

//boundingbox是用于定义表示
//空间元素的边界框。
模板<typename t>
结构边界框
  t最小值x，最小值y，最大值x，最大值y；
  boundingbox（）=默认值；
  分界框（t 58417; min_x，t min_y，t max_x，t max_y）
      ：最小值x（_最小值x），最小值y（_最小值y），最大值x（_最大值x），最大值y（_最大值y）

  bool intersects（const boundingbox<t>>&a）const_
    回来！（最小值x>a.max_x_最小值y>a.max_y_a.min_x>max_x_
             a.最小值y>最大值y）；
  }
}；

结构空间选项
  uint64缓存大小=1*1024*1024*1024ll；/1GB
  int num_threads=16；
  bool bulk_load=真；
}；

//光标用于将查询中的数据返回到客户端。得到所有
//来自查询的数据，只要在valid（）为真时调用next（）。
类光标{
 公众：
  cursor（）=默认值；
  虚拟~cursor（）

  虚拟bool valid（）const=0；
  //需要：valid（））
  virtual void next（）=0；

  //下一次调用next（）之前基础存储的生存期
  //需要：valid（））
  虚拟常量切片blob（）=0；
  //下一次调用next（）之前基础存储的生存期
  //需要：valid（））
  虚拟常量feature set&feature_set（）=0；

  虚拟状态状态（）常量=0；

 私人：
  //不允许复制
  光标（const cursor&）；
  void运算符=（const cursor&）；
}；

//SpatialIndexOptions定义将基于数据构建的空间索引
结构空间索引选项
  //空间索引由名称引用
  std：：字符串名称；
  //索引的区域。如果元素不与空间相交
  //索引的bbox，它不会插入到索引中
  boundingbox<double>bbox；
  //平铺位控制空间索引的粒度。的每个维度
  //bbox将被拆分为（1<<tile_bits）个tiles，因此
  //总共（1<<tile_bits）^2个tiles。建议配置的大小为
  //每个图块大约是该空间索引上查询的大小
  uint32_t平铺位；
  空间索引选项（）
  空间索引选项（const std:：string&\u name，
                      const boundingbox<double>&bbox，uint32_t_tile_bits）
      ：name（_name），bbox（_bbox），tile_bits（_tile_bits）
}；

类空间数据库：公共stackabledb
 公众：
  //使用指定的索引列表创建空间数据库。
  //必需：db不存在
  静态状态创建（const spatialdboptions&options，const std:：string&name，
                       const std：：vector<spaceialindexoptions>&spatial_indexes）；

  //打开现有的SPATIALDB。将返回生成的db对象
  //通过db参数。
  //必需：数据库是使用SPATIALDB:：CREATE创建的
  静态状态打开（const spatialdboptions&options，const std:：string&name，
                     空间数据库**db，bool read_only=false）；

  显式空间数据库（db*db）：stackabledb（db）

  //将元素插入数据库。元素将插入指定的
  //空间索引，基于指定的bbox。
  //需要：space_indexes.size（）>0
  虚拟状态插入（const write options&write_options，
                        const boundingbox<double>&bbox，const slice&blob，
                        构造特征集和特征集，
                        const std:：vector<std:：string>空间索引=0；

  //在插入一组元素后调用compact（）应该加快
  /阅读。如果您使用SPATIALDBOPTIONS：：BULK_LOAD，这尤其有用
  //num threads决定我们将使用多少线程进行压缩。设置
  //这个数目越大，IO和CPU的使用量就越大，但完成的速度就越快
  虚拟状态压缩（int num_threads=1）=0；

  //查询指定的空间索引。查询将返回
  //与bbox相交，但它也可能返回一些额外的元素。
  虚拟光标*查询（const read options&read_options，
                        const boundingbox<double>和bbox，
                        const std:：string&spatial_index=0；
}；

//命名空间空间
//命名空间rocksdb
endif//rocksdb_lite
