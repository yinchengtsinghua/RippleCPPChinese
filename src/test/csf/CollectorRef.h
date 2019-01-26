
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_COLLECTOREF_H_INCLUDED
#define RIPPLE_TEST_CSF_COLLECTOREF_H_INCLUDED

#include <test/csf/events.h>
#include <test/csf/SimTime.h>

namespace ripple {
namespace test {
namespace csf {

/*保存对仲裁收集器的类型擦除引用。

    收集器是实现

        打开（nodeid、simtime、event）

    对于对等机发出的所有事件。

    此类用于键入擦除中每个对等方使用的实际收集器。
    模拟。其思想是用
    collectors.h中的助手，然后只在最高级键入erase
    添加到模拟时。

    下面的示例代码演示了存储收集器的原因
    作为参考。收藏家的寿命一般比
    模拟；可能对单个收集器运行多个模拟
    实例。收集器还可能存储大量数据，因此
    模拟需要指向单个实例，而不是要求
    收集器可以在其设计中有效地管理复制数据。

    @代码
        //初始化可能写入文件的特定收集器。
        someFancyCollector收集器“out.file”

        //设置模拟
        SIM SIM（信任图、拓扑结构、收集器）；

        //运行模拟
        SIM。运行（100）；

        //是否有任何与收集器相关的报告
        collector.report（）；

    @端码

    @注意：如果添加了新的事件类型，则需要将其添加到接口中
    下面。

**/

class CollectorRef
{
    using tp = SimTime;

//类型已擦除收集器实例的接口
    struct ICollector
    {
        virtual ~ICollector() = default;

        virtual void
        on(PeerID node, tp when, Share<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Share<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Receive<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Tx> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<TxSet> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Validation> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Ledger> const&) = 0;

        virtual void
        on(PeerID node, tp when, Relay<Proposal> const&) = 0;

        virtual void
        on(PeerID node, tp when, SubmitTx const&) = 0;

        virtual void
        on(PeerID node, tp when, StartRound const&) = 0;

        virtual void
        on(PeerID node, tp when, CloseLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, AcceptLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, WrongPrevLedger const&) = 0;

        virtual void
        on(PeerID node, tp when, FullyValidateLedger const&) = 0;
    };

//类型ful收集器t和类型擦除实例之间的桥接
    template <class T>
    class Any final : public ICollector
    {
        T & t_;

    public:
        Any(T & t) : t_{t}
        {
        }

//不能复制
        Any(Any const & ) = delete;
        Any& operator=(Any const & ) = delete;

        Any(Any && ) = default;
        Any& operator=(Any && ) = default;

        virtual void
        on(PeerID node, tp when, Share<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Share<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        void
        on(PeerID node, tp when, Receive<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Receive<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        void
        on(PeerID node, tp when, Relay<Tx> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<TxSet> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Validation> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Ledger> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, Relay<Proposal> const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, SubmitTx const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, StartRound const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, CloseLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, AcceptLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, WrongPrevLedger const& e) override
        {
            t_.on(node, when, e);
        }

        virtual void
        on(PeerID node, tp when, FullyValidateLedger const& e) override
        {
            t_.on(node, when, e);
        }
    };

    std::unique_ptr<ICollector> impl_;

public:
    template <class T>
    CollectorRef(T& t) : impl_{new Any<T>(t)}
    {
    }

//不可复制的
    CollectorRef(CollectorRef const& c) = delete;
    CollectorRef& operator=(CollectorRef& c) = delete;

    CollectorRef(CollectorRef&&) = default;
    CollectorRef& operator=(CollectorRef&&) = default;

    template <class E>
    void
    on(PeerID node, tp when, E const& e)
    {
        impl_->on(node, when, e);
    }
};

/*集尘器容器

    处理相同事件的CollectorRef实例集。事件是
    由收集器按添加收集器的顺序进行处理。

    此类类型将擦除收集器实例。根据合同，
    collectors.h中的collectors/collectors class/helper不会被类型删除，并且
    提供类型转换和组合的机会
    改进的编译器优化。
**/

class CollectorRefs
{
    std::vector<CollectorRef> collectors_;
public:

    template <class Collector>
    void add(Collector & collector)
    {
        collectors_.emplace_back(collector);
    }

    template <class E>
    void
    on(PeerID node, SimTime when, E const& e)
    {
        for (auto & c : collectors_)
        {
            c.on(node, when, e);
        }
    }

};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
