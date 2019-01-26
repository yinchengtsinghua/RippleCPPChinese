
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "rocksdb/c.h"

#include <unistd.h>  //sysconf（）-获取CPU计数

const char DBPath[] = "/tmp/rocksdb_simple_example";
const char DBBackupPath[] = "/tmp/rocksdb_simple_example_backup";

int main(int argc, char **argv) {
  rocksdb_t *db;
  rocksdb_backup_engine_t *be;
  rocksdb_options_t *options = rocksdb_options_create();
//优化RocksDB。这是最简单的方法
//让rocksdb表现出色
long cpus = sysconf(_SC_NPROCESSORS_ONLN);  //获取在线核心
  rocksdb_options_increase_parallelism(options, (int)(cpus));
  rocksdb_options_optimize_level_style_compaction(options, 0);
//如果数据库尚未存在，则创建它
  rocksdb_options_set_create_if_missing(options, 1);

//开放数据库
  char *err = NULL;
  db = rocksdb_open(options, DBPath, &err);
  assert(!err);

//打开将用于备份数据库的备份引擎
  be = rocksdb_backup_engine_open(options, DBBackupPath, &err);
  assert(!err);

//设置关键值
  rocksdb_writeoptions_t *writeoptions = rocksdb_writeoptions_create();
  const char key[] = "key";
  const char *value = "value";
  rocksdb_put(db, writeoptions, key, strlen(key), value, strlen(value) + 1,
              &err);
  assert(!err);
//获取价值
  rocksdb_readoptions_t *readoptions = rocksdb_readoptions_create();
  size_t len;
  char *returned_value =
      rocksdb_get(db, readoptions, key, strlen(key), &len, &err);
  assert(!err);
  assert(strcmp(returned_value, "value") == 0);
  free(returned_value);

//在dbbackuppath指定的目录中创建新备份
  rocksdb_backup_engine_create_new_backup(be, db, &err);
  assert(!err);

  rocksdb_close(db);

//如果有问题，您可能需要从上次备份中还原数据。
  rocksdb_restore_options_t *restore_options = rocksdb_restore_options_create();
  rocksdb_backup_engine_restore_db_from_latest_backup(be, DBPath, DBPath,
                                                      restore_options, &err);
  assert(!err);
  rocksdb_restore_options_destroy(restore_options);

  db = rocksdb_open(options, DBPath, &err);
  assert(!err);

//清理
  rocksdb_writeoptions_destroy(writeoptions);
  rocksdb_readoptions_destroy(readoptions);
  rocksdb_options_destroy(options);
  rocksdb_backup_engine_close(be);
  rocksdb_close(db);

  return 0;
}
