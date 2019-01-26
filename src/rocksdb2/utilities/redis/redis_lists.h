
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *使用RockSDB后端构建的（持久）Redis API。
 *实现redis列表，如：http://redis.io/commands list所述。
 *
 *@throw所有函数都可能引发redisListException
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 *版权所有2013 Facebook
 **/


#ifndef ROCKSDB_LITE
#pragma once

#include <string>
#include "rocksdb/db.h"
#include "redis_list_iterator.h"
#include "redis_list_exception.h"

namespace rocksdb {

///redis功能（请参阅http://redis.io/commands list）
///all函数可能引发redisListException
class RedisLists {
public: //构造器/析构函数
///新建redislist数据库，名称/路径为db。
///will clear the database on open iff destructive is true（默认为false）。
///否则，它将还原保存的更改。
///可能引发redisListException
  RedisLists(const std::string& db_path,
             Options options, bool destructive = false);

public:  //访问器
///中的项目数（list:key）
  int Length(const std::string& key);

///Search the list for the（index）'th item（0-based）in（list:key）中的（索引）'th item（0-based）
///a负索引表示：“从列表结尾”
///if index在范围内：返回true，并返回*result中的值。
///if（index<-length或index>=length），则index超出范围：
///return false（且*结果保持不变）
///可能引发redisListException
  bool Index(const std::string& key, int32_t index,
             std::string* result);

///RETURN（list:key）[第一个..最后一个]（包含首尾）
///可能引发redisListException
  std::vector<std::string> Range(const std::string& key,
                                 int32_t first, int32_t last);

///打印整个（list:key），以便调试。
  void Print(const std::string& key);

public: //插入/更新
///insert value before/after pivot in（list:key）。返回长度。
///可能引发redisListException
  int InsertBefore(const std::string& key, const std::string& pivot,
                   const std::string& value);
  int InsertAfter(const std::string& key, const std::string& pivot,
                  const std::string& value);

///push/在列表的开头/结尾插入值。返回长度。
///可能引发redisListException
  int PushLeft(const std::string& key, const std::string& value);
  int PushRight(const std::string& key, const std::string& value);

///set（list:key）[idx]=val.成功时返回true，失败时返回false
///可能引发redisListException
  bool Set(const std::string& key, int32_t index, const std::string& value);

public: //删除/删除/弹出/修剪
///trim（list:key）使其只包含从start..stop开始的索引
///成功时返回true
///可能引发redisListException
  bool Trim(const std::string& key, int32_t start, int32_t stop);

///如果list为空，则返回false并保持*结果不变。
///else，删除第一个/最后一个元素，将其存储在*result中，并返回true
bool PopLeft(const std::string& key, std::string* result);  //弗斯特
bool PopRight(const std::string& key, std::string* result); //最后

///从列表（键）中删除第一次（或最后一次）出现的数值
///返回删除的元素数。
///可能引发redisListException
  int Remove(const std::string& key, int32_t num,
             const std::string& value);
  int RemoveFirst(const std::string& key, int32_t num,
                  const std::string& value);
  int RemoveLast(const std::string& key, int32_t num,
                 const std::string& value);

private: //私人职能
///calls insertbefore或insertafter
  int Insert(const std::string& key, const std::string& pivot,
             const std::string& value, bool insert_after);
 private:
std::string db_name_;       //实际数据库名称/路径
  WriteOptions put_option_;
  ReadOptions get_option_;

///后端rocksdb数据库。
///map:key->列表
///其中列表是元素序列
///元素是一个4字节的整数（n），后跟n字节的数据
  std::unique_ptr<DB> db_;
};

} //命名空间rocksdb
#endif  //摇滚乐
