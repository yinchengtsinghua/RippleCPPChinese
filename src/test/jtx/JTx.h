
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

#ifndef RIPPLE_TEST_JTX_JTX_H_INCLUDED
#define RIPPLE_TEST_JTX_JTX_H_INCLUDED

#include <test/jtx/requires.h>
#include <test/jtx/basic_prop.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/TER.h>
#include <functional>
#include <memory>
#include <vector>

namespace ripple {
namespace test {
namespace jtx {

class Env;

/*用于应用JSON事务的执行上下文。
    这将使用各种设置增加事务。
**/

struct JTx
{
    Json::Value jv;
    requires_t requires;
    boost::optional<TER> ter = TER {tesSUCCESS};
    bool fill_fee = true;
    bool fill_seq = true;
    bool fill_sig = true;
    std::shared_ptr<STTx const> stx;
    std::function<void(Env&, JTx&)> signer;

    JTx() = default;
    JTx (JTx const&) = default;
    JTx& operator=(JTx const&) = default;
    JTx(JTx&&) = default;
    JTx& operator=(JTx&&) = default;

    JTx (Json::Value&& jv_)
        : jv(std::move(jv_))
    {
    }

    JTx (Json::Value const& jv_)
        : jv(jv_)
    {
    }

    template <class Key>
    Json::Value&
    operator[](Key const& key)
    {
        return jv[key];
    }

    /*返回属性（如果存在）

        @如果道具不存在，返回nullptr
    **/

    /*@ {*/
    template <class Prop>
    Prop*
    get()
    {
        for (auto& prop : props_.list)
        {
            if (auto test = dynamic_cast<
                    prop_type<Prop>*>(
                        prop.get()))
                return &test->t;
        }
        return nullptr;
    }

    template <class Prop>
    Prop const*
    get() const
    {
        for (auto& prop : props_.list)
        {
            if (auto test = dynamic_cast<
                    prop_type<Prop> const*>(
                        prop.get()))
                return &test->t;
        }
        return nullptr;
    }
    /*@ }*/

    /*设置属性
        如果该属性已经存在，
        它被替换了。
    **/

    /*@ {*/
    void
    set(std::unique_ptr<basic_prop> p)
    {
        for (auto& prop : props_.list)
        {
            if (prop->assignable(p.get()))
            {
                prop = std::move(p);
                return;
            }
        }
        props_.list.emplace_back(std::move(p));
    }

    template <class Prop, class... Args>
    void
    set(Args&&... args)
    {
        set(std::make_unique<
            prop_type<Prop>>(
                std::forward <Args> (
                    args)...));
    }
    /*@ }*/

private:
    struct prop_list
    {
        prop_list() = default;

        prop_list(prop_list const& other)
        {
            for (auto const& prop : other.list)
                list.emplace_back(prop->clone());
        }

        prop_list& operator=(prop_list const& other)
        {
            if (this != &other)
            {
                list.clear();
                for (auto const& prop : other.list)
                    list.emplace_back(prop->clone());
            }
            return *this;
        }

        prop_list(prop_list&& src) = default;
        prop_list& operator=(prop_list&& src) = default;

        std::vector<std::unique_ptr<
            basic_prop>> list;
    };

    prop_list props_;
};

} //JTX
} //测试
} //涟漪

#endif
