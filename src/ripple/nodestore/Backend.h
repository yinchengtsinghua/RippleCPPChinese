
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2013 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#ifndef RIPPLE_NODESTORE_BACKEND_H_INCLUDED
#define RIPPLE_NODESTORE_BACKEND_H_INCLUDED

#include <ripple/nodestore/Types.h>

namespace ripple {
namespace NodeStore {

/*用于节点存储的后端。

    nodestore使用可交换的后端，以便其他数据库系统
    可以试一试。不同的数据库可能提供不同的功能，例如
    作为改进的性能、容错或分布式存储，或
    全内存操作。

    后端的给定实例固定为特定的密钥大小。
**/

class Backend
{
public:
    /*销毁后端。

        所有打开的文件都已关闭并刷新。如果有成批写入
        或计划的其他任务，将在此呼叫之前完成
        返回。
    **/

    virtual ~Backend() = default;

    /*获取此后端的可读名称。
        用于诊断输出。
    **/

    virtual std::string getName() = 0;

    /*打开后端。
        @param createifmissing根据需要创建数据库文件。
        这允许调用者捕获异常。
    **/

    virtual void open(bool createIfMissing = true) = 0;

    /*关闭后端。
        这允许调用者捕获异常。
    **/

    virtual void close() = 0;

    /*获取单个对象。
        如果找不到对象或遇到错误，则
        结果将指示状况。
        @注意，这将同时调用。
        @param key指向键数据的指针。
        @param pobject[out]创建的对象如果成功。
        @返回操作结果。
    **/

    virtual Status fetch (void const* key, std::shared_ptr<NodeObject>* pObject) = 0;

    /*如果批量提取已优化，则返回“true”。*/
    virtual
    bool
    canFetchBatch() = 0;

    /*同步获取批。*/
    virtual
    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) = 0;

    /*存储单个对象。
        根据实施情况，这可能会立即发生
        或延迟使用计划任务。
        @注意，这将同时调用。
        @param object要存储的对象。
    **/

    virtual void store (std::shared_ptr<NodeObject> const& object) = 0;

    /*存储一组对象。
        @注意，此函数不会与
              自身或@ref store。
    **/

    virtual void storeBatch (Batch const& batch) = 0;

    /*访问数据库中的每个对象
        这通常在导入期间调用。
        @注意，此例程不会与自身同时调用
              或其他方法。
        参见进口
    **/

    virtual void for_each (std::function <void (std::shared_ptr<NodeObject>)> f) = 0;

    /*估计挂起的写入操作数。*/
    virtual int getWriteLoad () = 0;

    /*销毁时删除磁盘上的内容。*/
    virtual void setDeletePath() = 0;

    /*对数据库执行一致性检查*/
    virtual void verify() = 0;

    /*返回后端需要的文件句柄数*/
    virtual int fdlimit() const = 0;
};

}
}

#endif
