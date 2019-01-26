
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有2013 Facebook
/*
 *使用RockSDB后端构建的（持久）Redis API。
 *实现redis列表，如：http://redis.io/commands list所述。
 *
 *@throw all函数可能在错误/损坏时引发redisListException。
 *
 *@notes内部，列表集存储在rocksdb数据库中，
 *将键映射到值。每个“值”都是列表本身，存储
 *数据的某种内部表示。所有的
 *表示细节由Redisisterator类处理。
 *当前文件应忽略表示细节，
 *仅处理客户端（Redis）API和对RockSDB的调用。
 *
 *@todo目前，所有操作至少需要o（nv）时间，其中
 *n是列表中的元素数，v是平均数。
 *列表中每个值的字节数。所以可能，使用合并运算符
 *我们可以将其改进为最佳O（V）摊销时间，因为我们
 *不必读写整个列表。
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 **/


#ifndef ROCKSDB_LITE
#include "redis_lists.h"

#include <iostream>
#include <memory>
#include <cmath>

#include "rocksdb/slice.h"
#include "util/coding.h"

namespace rocksdb
{

///构造函数

RedisLists::RedisLists(const std::string& db_path,
                       Options options, bool destructive)
    : put_option_(),
      get_option_() {

//存储数据库的名称
  db_name_ = db_path;

//如果是破坏性的，在重新打开数据库之前将其销毁。
  if (destructive) {
    DestroyDB(db_name_, Options());
  }

//现在打开并处理数据库
  DB* db;
  Status s = DB::Open(options, db_name_, &db);
  if (!s.ok()) {
    std::cerr << "ERROR " << s.ToString() << std::endl;
    assert(false);
  }

  db_ = std::unique_ptr<DB>(db);
}


//访问器

//与键关联的列表中的元素数
//：引发RedisListException
int RedisLists::Length(const std::string& key) {
//提取表示列表的字符串数据。
  std::string data;
  db_->Get(get_option_, key, &data);

//返回长度
  RedisListIterator it(data);
  return it.Length();
}

//在中的指定索引处获取元素（list:key）
//在越界时返回<empty>（“”）
//：引发RedisListException
bool RedisLists::Index(const std::string& key, int32_t index,
                       std::string* result) {
//提取表示列表的字符串数据。
  std::string data;
  db_->Get(get_option_, key, &data);

//处理redis负索引（从结尾）；fast iff length（）接受o（1）
  if (index < 0) {
index = Length(key) - (-index);  //将-i替换为（n-i）。
  }

//遍历列表，直到找到所需的索引。
  int curIndex = 0;
  RedisListIterator it(data);
  while(curIndex < index && !it.Done()) {
    ++curIndex;
    it.Skip();
  }

//如果我们真的找到了索引
  if (curIndex == index && !it.Done()) {
    Slice elem;
    it.GetCurrent(&elem);
    if (result != NULL) {
      *result = elem.ToString();
    }

    return true;
  } else {
    return false;
  }
}

//返回列表的截断版本。
//首先，第一个/最后一个的负值被解释为“列表末尾”。
//所以，如果首先=-1，那么它将被重新设置为index:（length（key）-1）
//然后，精确地返回那些指数i，这样第一个<=i<=last。
//：引发RedisListException
std::vector<std::string> RedisLists::Range(const std::string& key,
                                           int32_t first, int32_t last) {
//提取表示列表的字符串数据。
  std::string data;
  db_->Get(get_option_, key, &data);

//处理负边界（-1表示最后一个元素等）
  int listLen = Length(key);
  if (first < 0) {
first = listLen - (-first);           //用（n-x）替换（-x）
  }
  if (last < 0) {
    last = listLen - (-last);
  }

//验证边界（并截断该范围以使其有效）
  first = std::max(first, 0);
  last = std::min(last, listLen-1);
  int len = std::max(last-first+1, 0);

//初始化结果列表
  std::vector<std::string> result(len);

//遍历列表并更新矢量
  int curIdx = 0;
  Slice elem;
  for (RedisListIterator it(data); !it.Done() && curIdx<=last; it.Skip()) {
    if (first <= curIdx && curIdx <= last) {
      it.GetCurrent(&elem);
      result[curIdx-first].assign(elem.data(),elem.size());
    }

    ++curIdx;
  }

//返回结果。可能是空的
  return result;
}

//将（list:key）输出到stdout。主要用于调试。公众现在。
void RedisLists::Print(const std::string& key) {
//提取表示列表的字符串数据。
  std::string data;
  db_->Get(get_option_, key, &data);

//遍历列表并打印项
  Slice elem;
  for (RedisListIterator it(data); !it.Done(); it.Skip()) {
    it.GetCurrent(&elem);
    std::cout << "ITEM " << elem.ToString() << std::endl;
  }

//现在打印字节数据
  RedisListIterator it(data);
  std::cout << "==Printing data==" << std::endl;
  std::cout << data.size() << std::endl;
  std::cout << it.Size() << " " << it.Length() << std::endl;
  Slice result = it.WriteResult();
  std::cout << result.data() << std::endl;
  if (true) {
    std::cout << "size: " << result.size() << std::endl;
    const char* val = result.data();
    for(int i=0; i<(int)result.size(); ++i) {
      std::cout << (int)val[i] << " " << (val[i]>=32?val[i]:' ') << std::endl;
    }
    std::cout << std::endl;
  }
}

///插入/更新函数
///注意：“real”插入函数是私有的。见下文。

//insertbefore和insertafter只是围绕insert函数的包装器。
int RedisLists::InsertBefore(const std::string& key, const std::string& pivot,
                             const std::string& value) {
  return Insert(key, pivot, value, false);
}

int RedisLists::InsertAfter(const std::string& key, const std::string& pivot,
                            const std::string& value) {
  return Insert(key, pivot, value, true);
}

//将值前置到的开头（列表：键）
//：引发RedisListException
int RedisLists::PushLeft(const std::string& key, const std::string& value) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//构造结果
  RedisListIterator it(data);
  it.Reserve(it.Size() + it.SizeOf(value));
  it.InsertElement(value);

//将数据推回到数据库并返回长度
  db_->Put(put_option_, key, it.WriteResult());
  return it.Length();
}

//将值附加到的结尾（列表：键）
//托多：做这个O（1）次。可能需要MergeOperator。
//：引发RedisListException
int RedisLists::PushRight(const std::string& key, const std::string& value) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//为数据创建一个迭代器并查找到末尾。
  RedisListIterator it(data);
  it.Reserve(it.Size() + it.SizeOf(value));
  while (!it.Done()) {
it.Push();    //边写边写
  }

//在当前位置插入新元素（结尾）
  it.InsertElement(value);

//将其推回数据库，并返回长度
  db_->Put(put_option_, key, it.WriteResult());
  return it.Length();
}

//set（list:key）[idx]=val。成功时返回true，失败时返回false。
//：引发RedisListException
bool RedisLists::Set(const std::string& key, int32_t index,
                     const std::string& value) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//为redis处理负索引（意思是-从列表末尾索引）
  if (index < 0) {
    index = Length(key) - (-index);
  }

//遍历列表，直到找到所需的元素
  int curIndex = 0;
  RedisListIterator it(data);
it.Reserve(it.Size() + it.SizeOf(value));  //估计过高很好
  while(curIndex < index && !it.Done()) {
    it.Push();
    ++curIndex;
  }

//如果找不到，返回false（当索引无效时会发生这种情况）
  if (it.Done() || curIndex != index) {
    return false;
  }

//写入新元素值，并删除上一个元素值
  it.InsertElement(value);
  it.Skip();

//将数据写入数据库
//检查状态，因为它需要返回真/假保证
  Status s = db_->Put(put_option_, key, it.WriteResult());

//成功
  return s.ok();
}

///delete/remove/pop函数

//修剪（list:key），使其只包含从start..stop开始的索引
//无效的索引不会生成错误，只是空的，
//或列表中适合此间隔的部分
//：引发RedisListException
bool RedisLists::Trim(const std::string& key, int32_t start, int32_t stop) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//处理redis中的负索引
  int listLen = Length(key);
  if (start < 0) {
    start = listLen - (-start);
  }
  if (stop < 0) {
    stop = listLen - (-stop);
  }

//截断边界以仅适合列表
  start = std::max(start, 0);
  stop = std::min(stop, listLen-1);

//构造列表的迭代器。删除所有不需要的元素。
  int curIndex = 0;
  RedisListIterator it(data);
it.Reserve(it.Size());          //过高估计
  while(!it.Done()) {
//如果不在范围内，则跳过该项（将其删除）。
//否则，照常继续。
    if (start <= curIndex && curIndex <= stop) {
      it.Push();
    } else {
      it.Skip();
    }

//增加当前索引
    ++curIndex;
  }

//将（可能为空）结果写入数据库
  Status s = db_->Put(put_option_, key, it.WriteResult());

//只要写入成功，返回true
  return s.ok();
}

//返回并删除列表中的第一个元素（如果为空，则返回“”）
//：引发RedisListException
bool RedisLists::PopLeft(const std::string& key, std::string* result) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//指向列表中的第一个元素（如果存在），并获取其值/大小
  RedisListIterator it(data);
if (it.Length() > 0) {            //仅当列表非空时继续
    Slice elem;
it.GetCurrent(&elem);           //存储第一个元素的值
    it.Reserve(it.Size() - it.SizeOf(elem));
it.Skip();                      //删除第一项并移到下一项

//更新数据库
    db_->Put(put_option_, key, it.WriteResult());

//返回值
    if (result != NULL) {
      *result = elem.ToString();
    }
    return true;
  } else {
    return false;
  }
}

//删除并返回列表中的最后一个元素（如果为空，则返回“”）
//托多：做这个O（1）。可能需要MergeOperator。
//：引发RedisListException
bool RedisLists::PopRight(const std::string& key, std::string* result) {
//提取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//构造数据的迭代器并移动到最后一个元素
  RedisListIterator it(data);
  it.Reserve(it.Size());
  int len = it.Length();
  int curIndex = 0;
  while(curIndex < (len-1) && !it.Done()) {
    it.Push();
    ++curIndex;
  }

//提取并删除/跳过最后一个元素
  if (curIndex == len-1) {
assert(!it.Done());         //清醒检查。不应该在这里结束。

//提取并弹出元素
    Slice elem;
it.GetCurrent(&elem);       //保存元素的值。
it.Skip();                  //跳过元素

//将结果写入数据库
    db_->Put(put_option_, key, it.WriteResult());

//返回值
    if (result != NULL) {
      *result = elem.ToString();
    }
    return true;
  } else {
//必须是空列表
    assert(it.Done() && len==0 && curIndex == 0);
    return false;
  }
}

//删除（list:key）中值的（第一次或最后一次）“num”出现
//：引发RedisListException
int RedisLists::Remove(const std::string& key, int32_t num,
                       const std::string& value) {
//负num=>删除；正num=>先删除
  if (num < 0) {
    return RemoveLast(key, -num, value);
  } else if (num > 0) {
    return RemoveFirst(key, num, value);
  } else {
    return RemoveFirst(key, Length(key), value);
  }
}

//删除（list:key）中第一个出现的值“num”。
//：引发RedisListException
int RedisLists::RemoveFirst(const std::string& key, int32_t num,
                            const std::string& value) {
//确保数字为正数
  assert(num >= 0);

//提取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//遍历列表，追加除所需值之外的所有值
int numSkipped = 0;         //跟踪看到值的次数
  Slice elem;
  RedisListIterator it(data);
  it.Reserve(it.Size());
  while (!it.Done()) {
    it.GetCurrent(&elem);

    if (elem == value && numSkipped < num) {
//如果需要，请删除此项目
      it.Skip();
      ++numSkipped;
    } else {
//否则保持项目正常进行
      it.Push();
    }
  }

//将结果放回数据库
  db_->Put(put_option_, key, it.WriteResult());

//返回删除的元素数
  return numSkipped;
}


//删除（list:key）中最后出现的“num”值。
//托多：我遍历列表2倍。快点。可能需要MergeOperator。
//：引发RedisListException
int RedisLists::RemoveLast(const std::string& key, int32_t num,
                           const std::string& value) {
//确保数字为正数
  assert(num >= 0);

//提取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//在下面的块中保存“当前元素”的临时变量
  Slice elem;

//计算出现值的总数
  int totalOccs = 0;
  for (RedisListIterator it(data); !it.Done(); it.Skip()) {
    it.GetCurrent(&elem);
    if (elem == value) {
      ++totalOccs;
    }
  }

//构造数据的迭代器。为结果保留足够的空间。
  RedisListIterator it(data);
  int bytesRemoved = std::min(num,totalOccs)*it.SizeOf(value);
  it.Reserve(it.Size() - bytesRemoved);

//遍历列表，追加除所需值之外的所有值。
//注：“删除最后k次事件”等于
//“仅保留第一个n-k次出现”，其中n是总出现次数。
int numKept = 0;          //跟踪保持值的次数
  while(!it.Done()) {
    it.GetCurrent(&elem);

//如果我们在删除范围内并且等于value，则将其删除。
//否则，追加/保留/推送它。
    if (elem == value) {
      if (numKept < totalOccs - num) {
        it.Push();
        ++numKept;
      } else {
        it.Skip();
      }
    } else {
//总是附加其他的
      it.Push();
    }
  }

//将结果放回数据库
  db_->Put(put_option_, key, it.WriteResult());

//返回删除的元素数
  return totalOccs - numKept;
}

///private函数

//将元素值插入（列表：键），在前面/后面
//轴的第一次出现
//：引发RedisListException
int RedisLists::Insert(const std::string& key, const std::string& pivot,
                       const std::string& value, bool insert_after) {
//获取原始列表数据
  std::string data;
  db_->Get(get_option_, key, &data);

//构造数据的迭代器，并为结果保留足够的空间。
  RedisListIterator it(data);
  it.Reserve(it.Size() + it.SizeOf(value));

//遍历列表，直到找到所需的元素
  Slice elem;
  bool found = false;
  while(!it.Done() && !found) {
    it.GetCurrent(&elem);

//找到元素后，插入元素并标记找到的元素
if (elem == pivot) {                //找到它了！
      found = true;
if (insert_after == true) {       //如果在后面插入，则跳过一个
        it.Push();
      }
      it.InsertElement(value);
    } else {
      it.Push();
    }

  }

//将数据（字符串）放入数据库
  if (found) {
    db_->Put(put_option_, key, it.WriteResult());
  }

//返回列表的新（可能不变）长度
  return it.Length();
}

}  //命名空间rocksdb
#endif  //摇滚乐
