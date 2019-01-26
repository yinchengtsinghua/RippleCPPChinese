
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

#ifndef RIPPLE_APP_LEDGER_LEDGERCLEANER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERCLEANER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/json/json_value.h>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <ripple/beast/utility/Journal.h>
#include <memory>

namespace ripple {
namespace detail {

/*检查分类帐/交易数据库以确保它们具有连续性*/
class LedgerCleaner
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
    explicit LedgerCleaner (Stoppable& parent);

public:
    /*摧毁物体。*/
    virtual ~LedgerCleaner () = 0;

    /*开始一个长时间运行的任务来清理分类帐。
        在定义的实现上异步清除分类帐
        线程。此函数调用不会阻塞。长期运行的任务
        如果可停止，将停止。

        线程安全：
            随时可以从任何线程安全调用。

        @param参数具有可配置参数的JSON对象。
    **/

    virtual void doClean (Json::Value const& parameters) = 0;
};

std::unique_ptr<LedgerCleaner>
make_LedgerCleaner (Application& app,
    Stoppable& parent, beast::Journal journal);

} //细节
} //涟漪

#endif
