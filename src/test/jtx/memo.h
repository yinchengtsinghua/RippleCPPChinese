
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

#ifndef RIPPLE_TEST_JTX_MEMO_H_INCLUDED
#define RIPPLE_TEST_JTX_MEMO_H_INCLUDED

#include <test/jtx/Env.h>
#include <boost/optional.hpp>

namespace ripple {
namespace test {
namespace jtx {

/*向JTX添加备忘录。

    如果备忘录已经存在，新的
    备忘录被附加到数组中。
**/

class memo
{
private:
    std::string data_;
    std::string format_;
    std::string type_;

public:
    memo (std::string const& data,
        std::string const& format,
            std::string const& type)
        : data_(data)
        , format_(format)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memodata
{
private:
    std::string s_;

public:
    memodata (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memoformat
{
private:
    std::string s_;

public:
    memoformat (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memotype
{
private:
    std::string s_;

public:
    memotype (std::string const& s)
        : s_(s)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memondata
{
private:
    std::string format_;
    std::string type_;

public:
    memondata (std::string const& format,
            std::string const& type)
        : format_(format)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memonformat
{
private:
    std::string data_;
    std::string type_;

public:
    memonformat (std::string const& data,
            std::string const& type)
        : data_(data)
        , type_(type)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

class memontype
{
private:
    std::string data_;
    std::string format_;

public:
    memontype (std::string const& data,
            std::string const& format)
        : data_(data)
        , format_(format)
    {
    }

    void
    operator()(Env&, JTx& jt) const;
};

} //JTX
} //测试
} //涟漪

#endif
