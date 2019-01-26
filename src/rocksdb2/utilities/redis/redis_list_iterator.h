
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有2013 Facebook
/*
 *转发器：
 *对“列表”概念的抽象（例如，对于redis列表）。
 *提供读取、遍历、编辑和写入这些列表的功能。
 *
 *构造完成后，会给Redisliterator一组列表数据。
 *在内部，它存储一个指向数据的指针和一个指向当前项的指针。
 *它还存储了一个“结果”列表，该列表将随着时间变化。
 *
 *通过“正向迭代”进行遍历和变异。
 *push（）和skip（）方法将使迭代器前进到下一项。
 *但是push（）也将“将当前项写入结果”。
 *skip（）只会移动到下一个项目，导致当前项目被删除。
 *
 *完成后，将保存结果（可通过writeResult（）访问）。
 *所有“跳过”的项目都将消失；所有“推送”的项目将保留。
 *
 *@throw如果无效，任何操作都可能引发redisListException
 *执行操作或发现数据损坏。
 *
 *@notes默认情况下，如果writeresult（）通过迭代部分调用，
 *它将自动将迭代器推进到末尾，并保留（）。
 *所有尚未遍历的项。这可能是主题
 *回顾。
 *
 *@notes可以通过getcurrent（）和其他
 *列出特定信息，如length（）。
 *
 *目前，
 *列表如下：
 *-32位整数头：列表中的项数
 *对于每个项目：
 *-32位int（n）：表示此项的字节数
 *-n字节的数据：实际数据。
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 **/


#pragma once
#ifndef ROCKSDB_LITE

#include <string>

#include "redis_list_exception.h"
#include "rocksdb/slice.h"
#include "util/coding.h"

namespace rocksdb {

///对“列表”概念的抽象。
///all操作可能引发redisListException
class RedisListIterator {
 public:
///基于数据构造redis列表迭代器。
///如果数据非空，则必须按照上面的@notes进行格式化。
///
///如果数据有效，我们可以假定以下不变量：
///a）长度_u，num_bytes_u设置正确。
///b）cur_byte_u总是指当前元素的开头，
///在指定元素长度的字节之前。
///c）cur elem始终是当前元素的索引。
///d）cur elem length始终是当前元素中的字节数，
///排除4字节头本身。
///e）结果_u将始终包含数据_[0..cur_byte_u）和标题。
///f）每当遇到损坏的数据或无效操作时，
///已尝试，将立即引发redisListException。
  explicit RedisListIterator(const std::string& list_data)
      : data_(list_data.data()),
        num_bytes_(static_cast<uint32_t>(list_data.size())),
        cur_byte_(0),
        cur_elem_(0),
        cur_elem_length_(0),
        length_(0),
        result_() {
//初始化结果（为头保留足够的空间）
    InitializeResult();

//仅当数据不为空时才分析数据。
    if (num_bytes_ == 0) {
      return;
    }

//如果非空但小于4字节，则数据必须损坏
    if (num_bytes_ < sizeof(length_)) {
ThrowError("Corrupt header.");    //将中断控制流
    }

//很好。第一个字节指定元素的数目
    length_ = DecodeFixed32(data_);
    cur_byte_ = sizeof(length_);

//如果我们至少有一个元素，请指向该元素。
//另外，读取元素的第一个整数（指定大小），
//如果可能的话。
    if (length_ > 0) {
      if (cur_byte_ + sizeof(cur_elem_length_) <= num_bytes_) {
        cur_elem_length_ = DecodeFixed32(data_+cur_byte_);
      } else {
        ThrowError("Corrupt data for first element.");
      }
    }

//在这一点上，我们已经完全准备好了。
//头中描述的不变量现在应该是真的。
  }

///为结果保留一些空间。
///相当于结果保留（字节）。
  void Reserve(int bytes) {
    result_.reserve(bytes);
  }

///转到数据文件中的下一个元素。
///同时将当前元素写入结果\。
  RedisListIterator& Push() {
    WriteCurrentElement();
    MoveNext();
    return *this;
  }

///转到数据文件中的下一个元素。
///drops/skip当前元素。它不会被写入结果。
  RedisListIterator& Skip() {
    MoveNext();
--length_;          //少一项
--cur_elem_;        //我们前进了一步，但指数没有改变。
    return *this;
  }

///insert elem into the result_uu（刚好在当前元素/字节之前）
///note:if done（）（即：迭代器指向末尾），这将附加elem。
  void InsertElement(const Slice& elem) {
//确保我们处于有效状态
    CheckErrors();

    const int kOrigSize = static_cast<int>(result_.size());
    result_.resize(kOrigSize + SizeOf(elem));
    EncodeFixed32(result_.data() + kOrigSize,
                  static_cast<uint32_t>(elem.size()));
    memcpy(result_.data() + kOrigSize + sizeof(uint32_t), elem.data(),
           elem.size());
    ++length_;
    ++cur_elem_;
  }

///访问当前元素，并将结果保存到*curelem中
  void GetCurrent(Slice* curElem) {
//确保我们处于有效状态
    CheckErrors();

//确保我们没有超过最后一个元素。
    if (Done()) {
      ThrowError("Invalid dereferencing.");
    }

//取消引用元素
    *curElem = Slice(data_+cur_byte_+sizeof(cur_elem_length_),
                     cur_elem_length_);
  }

//元素数量
  int Length() const {
    return length_;
  }

//最终表示形式中的字节数（即：writeResult（）.size（））
  int Size() const {
//结果保存当前写入的数据
//data_u[cur_byte..num_bytes-1]是数据的剩余部分
    return static_cast<int>(result_.size() + (num_bytes_ - cur_byte_));
  }

//到达终点了吗？
  bool Done() const {
    return cur_byte_ >= num_bytes_ || cur_elem_ >= length_;
  }

///返回表示已编辑的最终数据的字符串。
///假定已读取范围[0，cur_byte_u]内的所有数据字节。
///结果中包含了这些数据。
///其余数据仍必须写入。
///so，此方法在写入之前将迭代器推进到末尾。
  Slice WriteResult() {
    CheckErrors();

//标题当前应填充虚拟数据（0）
//正确更新标题。
//注意，这是安全的，因为结果_u是一个向量（保证连续）
    EncodeFixed32(&result_[0],length_);

//将其余数据追加到结果中。
    result_.insert(result_.end(),data_+cur_byte_, data_ +num_bytes_);

//查找文件结尾
    cur_byte_ = num_bytes_;
    cur_elem_ = length_;
    cur_elem_length_ = 0;

//返回结果
    return Slice(result_.data(),result_.size());
  }

public: //静态公共功能

///存储此元素所需的字节数的上限。
///用于对客户端隐藏表示信息。
///e.g.这可用于计算要保留的字节（）。
  static uint32_t SizeOf(const Slice& elem) {
//[整数长度。数据]
    return static_cast<uint32_t>(sizeof(uint32_t) + elem.size());
  }

private: //私人职能

///初始化结果字符串。
///它将用0填充前几个字节，以便
///当我们稍后需要写入时，为头信息留出足够的空间。
///目前，“标题信息”是指：长度（元素数）
///假设结果_u为空
  void InitializeResult() {
assert(result_.empty());            //应该永远是真的。
result_.resize(sizeof(uint32_t),0); //将0的块作为标题
  }

///转到下一个元素（在push（）和skip（）中使用）
  void MoveNext() {
    CheckErrors();

//检查以确保我们尚未处于完成状态
    if (Done()) {
      ThrowError("Attempting to iterate past end of list.");
    }

//向前移动一个元素。
    cur_byte_ += sizeof(cur_elem_length_) + cur_elem_length_;
    ++cur_elem_;

//如果我们在最后，完成
    if (Done()) {
      cur_elem_length_ = 0;
      return;
    }

//否则，我们应该能够读取新元素的长度
    if (cur_byte_ + sizeof(cur_elem_length_) > num_bytes_) {
      ThrowError("Corrupt element data.");
    }

//设置新元素的长度
    cur_elem_length_ = DecodeFixed32(data_+cur_byte_);

    return;
  }

///append当前元素（由cur_byte_u指向）to result_
///假设结果_u已适当保留。
  void WriteCurrentElement() {
//首先验证迭代器是否仍然有效。
    CheckErrors();
    if (Done()) {
      ThrowError("Attempting to write invalid element.");
    }

//附加cur元素。
    result_.insert(result_.end(),
                   data_+cur_byte_,
                   data_+cur_byte_+ sizeof(uint32_t) + cur_elem_length_);
  }

///will throwerror（）如有必要。
///检查大多数操作后可能出现的常见/普遍错误。
///任何读取操作之前都应调用此方法。
///如果此函数成功，则保证我们处于有效状态。
///other成员函数还应检查错误和throwerror（）。
///如果发生特定于它的错误，即使处于有效状态。
  void CheckErrors() {
//检查一下最近有没有发生什么疯狂的事情
if ((cur_elem_ > length_) ||                              //不良指标
(cur_byte_ > num_bytes_) ||                           //不再字节
(cur_byte_ + cur_elem_length_ > num_bytes_) ||        //项目太大
(cur_byte_ == num_bytes_ && cur_elem_ != length_) ||  //项目太多
(cur_elem_ == length_ && cur_byte_ != num_bytes_)) {  //字节太多
      ThrowError("Corrupt data.");
    }
  }

///将根据传入的消息引发异常。
///此功能保证停止控制流。
///（即：调用throwerror后，不必调用“return”）。
  void ThrowError(const char* const msg = NULL) {
//TODO:现在我们忽略msg参数。这可以稍后扩展。
    throw RedisListException();
  }

 private:
const char* const data_;      //指向数据的指针（第一个字节）
const uint32_t num_bytes_;    //此列表中的字节数

uint32_t cur_byte_;           //正在读取的当前字节
uint32_t cur_elem_;           //正在读取的当前元素
uint32_t cur_elem_length_;    //当前元素中的字节数

uint32_t length_;             //此列表中的元素数
std::vector<char> result_;    //输出数据
};

} //命名空间rocksdb
#endif  //摇滚乐
