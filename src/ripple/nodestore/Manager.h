
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

#ifndef RIPPLE_NODESTORE_MANAGER_H_INCLUDED
#define RIPPLE_NODESTORE_MANAGER_H_INCLUDED

#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/DatabaseRotating.h>
#include <ripple/nodestore/DatabaseShard.h>

namespace ripple {
namespace NodeStore {

/*用于管理nodestore工厂和后端的singleton。*/
class Manager
{
public:
    virtual ~Manager () = default;
    Manager() = default;
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;

    /*返回Manager Singleton的实例。*/
    static
    Manager&
    instance();

    /*添加工厂。*/
    virtual
    void
    insert (Factory& factory) = 0;

    /*拆除工厂。*/
    virtual
    void
    erase (Factory& factory) = 0;

    /*返回指向匹配工厂的指针（如果存在）。
        @param name要匹配的名称，不区分大小写。
        @return`nullptr`如果找不到匹配项。
    **/

    virtual
    Factory*
    find(std::string const& name) = 0;

    /*创建后端。*/
    virtual
    std::unique_ptr <Backend>
    make_Backend (Section const& parameters,
        Scheduler& scheduler, beast::Journal journal) = 0;

    /*构造一个nodestore数据库。

        这些参数是传递到后端的键值对。这个
        “type”键必须存在，它定义后端的选择。大多数
        后端还需要“path”字段。

        “类型”的一些选项包括：
            超水平数据库，水平工厂，sqlite，mdb

        如果fastbackendparameter被省略或为空，则没有临时数据库
        使用。如果省略或未指定scheduler参数，则
        使用同步调度程序立即执行所有任务
        调用方的线程。

        @注意：如果数据库无法打开或创建，将引发异常。

        @param name数据库的诊断标签。
        @param scheduler用于执行异步任务的调度程序。
        @param read threads要创建的异步读取线程数
        @param backendparameters持久后端的参数字符串。
        @param fastbackendparameters[可选]临时后端的参数字符串。

        @返回打开的数据库。
    **/

    virtual
    std::unique_ptr <Database>
    make_Database (std::string const& name, Scheduler& scheduler,
        int readThreads, Stoppable& parent,
            Section const& backendParameters,
                beast::Journal journal) = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*创建后端。*/
std::unique_ptr <Backend>
make_Backend (Section const& config,
    Scheduler& scheduler, beast::Journal journal);

}
}

#endif
