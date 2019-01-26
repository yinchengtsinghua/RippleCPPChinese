
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

#ifndef RIPPLE_TEST_JTX_PATHS_H_INCLUDED
#define RIPPLE_TEST_JTX_PATHS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/protocol/Issue.h>
#include <type_traits>

namespace ripple {
namespace test {
namespace jtx {

/*在jtx上设置路径sendmax。*/
class paths
{
private:
    Issue in_;
    int depth_;
    unsigned int limit_;

public:
    paths (Issue const& in,
            int depth = 7, unsigned int limit = 4)
        : in_(in)
        , depth_(depth)
        , limit_(limit)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*添加路径。

    如果没有路径存在，则创建一个新路径。
**/

class path
{
private:
    Json::Value jv_;

public:
    path();

    template <class T, class... Args>
    explicit
    path (T const& t, Args const&... args);

    void
    operator()(Env&, JTx& jt) const;

private:
    Json::Value&
    create();

    void
    append_one(Account const& account);

    template <class T>
    std::enable_if_t<
        std::is_constructible<Account, T>::value>
    append_one(T const& t)
    {
        append_one(Account{ t });
    }

    void
    append_one(IOU const& iou);

    void
    append_one(BookSpec const& book);

    inline
    void
    append()
    {
    }

    template <class T, class... Args>
    void
    append (T const& t, Args const&... args);
};

template <class T, class... Args>
path::path (T const& t, Args const&... args)
    : jv_(Json::arrayValue)
{
    append(t, args...);
}

template <class T, class... Args>
void
path::append (T const& t, Args const&... args)
{
    append_one(t);
    append(args...);
}

} //JTX
} //测试
} //涟漪

#endif
