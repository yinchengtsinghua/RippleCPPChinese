
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

#include <ripple/basics/contract.h>
#include <ripple/nodestore/Factory.h>
#include <ripple/nodestore/Manager.h>
#include <memory>

namespace ripple {
namespace NodeStore {

class NullBackend : public Backend
{
public:
    NullBackend() = default;

    ~NullBackend() = default;

    std::string
    getName() override
    {
        return std::string ();
    }

    void
    open(bool createIfMissing) override
    {
    }

    void
    close() override
    {
    }

    Status
    fetch (void const*, std::shared_ptr<NodeObject>*) override
    {
        return notFound;
    }

    bool
    canFetchBatch() override
    {
        return false;
    }

    std::vector<std::shared_ptr<NodeObject>>
    fetchBatch (std::size_t n, void const* const* keys) override
    {
        Throw<std::runtime_error> ("pure virtual called");
        return {};
    }

    void
    store (std::shared_ptr<NodeObject> const& object) override
    {
    }

    void
    storeBatch (Batch const& batch) override
    {
    }

    void
    for_each (std::function <void(std::shared_ptr<NodeObject>)> f) override
    {
    }

    int
    getWriteLoad () override
    {
        return 0;
    }

    void
    setDeletePath() override
    {
    }

    void
    verify() override
    {
    }

    /*返回后端需要的文件句柄数*/
    int
    fdlimit() const override
    {
        return 0;
    }

private:
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class NullFactory : public Factory
{
public:
    NullFactory()
    {
        Manager::instance().insert(*this);
    }

    ~NullFactory() override
    {
        Manager::instance().erase(*this);
    }

    std::string
    getName () const override
    {
        return "none";
    }

    std::unique_ptr <Backend>
    createInstance (
        size_t,
        Section const&,
        Scheduler&, beast::Journal) override
    {
        return std::make_unique <NullBackend> ();
    }
};

static NullFactory nullFactory;

}
}
