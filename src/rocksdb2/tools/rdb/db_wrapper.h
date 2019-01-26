
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
#ifndef DBWRAPPER_H
#define DBWRAPPER_H

#include <map>
#include <node.h>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

using namespace v8;

//用于封装已打开数据库的特定实例。
//
//此对象不应直接用于C++；它仅用于提供。
//从JavaScript对象到可以使用ROCKSDB的C++代码的映射
//应用程序编程接口。
class DBWrapper : public node::ObjectWrap {
  public:
    static void Init(Handle<Object> exports);

  private:
    explicit DBWrapper();
    ~DBWrapper();

//辅助方法
    static bool HasFamilyNamed(std::string& name, DBWrapper* db);
    static bool AddToBatch(rocksdb::WriteBatch& batch, bool del,
        Handle<Array> array);
    static bool AddToBatch(rocksdb::WriteBatch& batch, bool del,
        Handle<Array> array, DBWrapper* db_wrapper, std::string cf);
    static Handle<Value> CompactRangeDefault(const v8::Arguments& args);
    static Handle<Value> CompactColumnFamily(const Arguments& args);
    static Handle<Value> CompactOptions(const Arguments& args);
    static Handle<Value> CompactAll(const Arguments& args);

//API方法的C++映射
    static Persistent<v8::Function> constructor;
    static Handle<Value> Open(const Arguments& args);
    static Handle<Value> New(const Arguments& args);
    static Handle<Value> Get(const Arguments& args);
    static Handle<Value> Put(const Arguments& args);
    static Handle<Value> Delete(const Arguments& args);
    static Handle<Value> Dump(const Arguments& args);
    static Handle<Value> WriteBatch(const Arguments& args);
    static Handle<Value> CreateColumnFamily(const Arguments& args);
    static Handle<Value> CompactRange(const Arguments& args);
    static Handle<Value> Close(const Arguments& args);

//内部场
    rocksdb::Options options_;
    rocksdb::Status status_;
    rocksdb::DB* db_;
    std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*>
        columnFamilies_;
};

#endif
