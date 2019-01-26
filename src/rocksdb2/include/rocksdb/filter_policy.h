
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
//版权所有（c）2012 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//可以使用自定义的filterpolicy对象配置数据库。
//此对象负责从集合创建小筛选器
//钥匙。这些过滤器存储在RockSDB中，并进行咨询。
//由rocksdb自动决定是否阅读
//来自磁盘的信息。在许多情况下，过滤器可以减少
//从一把磁盘搜索到一个磁盘搜索的次数
//d::GET（）调用。
//
//大多数人都希望使用内置的Bloom过滤器支持（请参见
//下面是newbloomfilterpolicy（）。

#ifndef STORAGE_ROCKSDB_INCLUDE_FILTER_POLICY_H_
#define STORAGE_ROCKSDB_INCLUDE_FILTER_POLICY_H_

#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <vector>

namespace rocksdb {

class Slice;

//一个获取一堆键，然后生成过滤器的类
class FilterBitsBuilder {
 public:
  virtual ~FilterBitsBuilder() {}

//添加要筛选的密钥，您可以使用任何方法来存储密钥。
//例如：存储哈希或原始密钥
//键是按顺序排列的，可以使用重复的键。
  virtual void AddKey(const Slice& key) = 0;

//使用添加的键生成筛选器
//这个函数的返回值是过滤位，
//实际数据的所有权设置为buf
  virtual Slice Finish(std::unique_ptr<const char[]>* buf) = 0;

//计算适合空间的条目数。
  virtual int CalculateNumEntry(const uint32_t space) {
#ifndef ROCKSDB_LITE
    throw std::runtime_error("CalculateNumEntry not Implemented");
#else
    abort();
#endif
    return 0;
  }
};

//检查键是否在筛选器中的类
//它应该由bitsbuilder生成的切片初始化
class FilterBitsReader {
 public:
  virtual ~FilterBitsReader() {}

//检查条目是否与过滤器中的位匹配
  virtual bool MayMatch(const Slice& entry) = 0;
};

//我们添加了一种新的过滤块格式，称为完全过滤块
//这个新界面为您提供了更多的定制空间
//
//对于完整的筛选器块，可以通过实现插入您的版本
//filterbitsbuilder和filterbitsreader
//
//filterpolicy中有两组接口
//设置1:CreateFilter，KeyMayMatch:用于基于块的筛选器
//设置2:getfilterbitsbuilder、getfilterbitsreader，它们用于
//全过滤器。
//必须正确执行集合1，集合2是可选的
//RockSDB将首先尝试使用集合2中的函数。如果它们返回nullptr，
//它将使用set 1。
//可以在NewbloomFilterPolicy中选择过滤器类型
class FilterPolicy {
 public:
  virtual ~FilterPolicy();

//返回此策略的名称。注意如果过滤器编码
//以不兼容的方式更改，此方法返回的名称
//必须更改。否则，旧的不兼容筛选器可能是
//传递给此类型的方法。
  virtual const char* Name() const = 0;

//键[0，n-1]包含键列表（可能有重复项）
//根据用户提供的比较器订购。
//附加一个将键[0，n-1]汇总到*dst的过滤器。
//
//警告：不要更改*dst的初始内容。相反，
//将新构造的过滤器附加到*dst。
  virtual void CreateFilter(const Slice* keys, int n, std::string* dst)
      const = 0;

//“filter”包含前面调用附加到
//类上的CreateFilter（）。如果
//密钥在传递给CreateFilter（）的密钥列表中。
//如果密钥不在
//列表，但它的目标应该是以很高的概率返回false。
  virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const = 0;

//获取filterbitsbuilder，它只用于完整的筛选器块
//它包含获取单个键，然后生成筛选器的接口
  virtual FilterBitsBuilder* GetFilterBitsBuilder() const {
    return nullptr;
  }

//获取filterbitsreader，它只用于完整的筛选器块
//它包含一个接口，用于判断密钥是否在筛选器中
//filterpolicy不应删除输入切片
  virtual FilterBitsReader* GetFilterBitsReader(const Slice& contents) const {
    return nullptr;
  }
};

//返回一个新的筛选器策略，该策略使用大约
//每个键的指定位数。
//
//Bits_per_键：布卢姆滤波器中的Bits per键。一个很好的每键位值
//是10，这会产生一个假阳性率为1%的过滤器。
//使用基于块的编译器：使用基于块的过滤器，而不是完全过滤器。
//如果要构建完整的过滤器，需要将其设置为false。
//
//调用方必须在使用
//结果已关闭。
//
//注意：如果您使用的是忽略某些部分的自定义比较器
//在要比较的密钥中，不能使用newbloomfilterpolicy（）。
//并且必须提供自己的filterpolicy，它还忽略
//键的相应部分。例如，如果比较器
//忽略尾随空格，使用
//不忽略的filterpolicy（如newbloomfilterpolicy）
//键中的尾随空格。
extern const FilterPolicy* NewBloomFilterPolicy(int bits_per_key,
    bool use_block_based_builder = true);
}

#endif  //存储\u rocksdb_包括\u filter_policy_h_
