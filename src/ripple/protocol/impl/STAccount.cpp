
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

#include <ripple/protocol/STAccount.h>

namespace ripple {

STAccount::STAccount ()
    : STBase ()
    , value_ (beast::zero)
    , default_ (true)
{
}

STAccount::STAccount (SField const& n)
    : STBase (n)
    , value_ (beast::zero)
    , default_ (true)
{
}

STAccount::STAccount (SField const& n, Buffer&& v)
    : STAccount (n)
{
    if (v.empty())
return;  //零是默认StatCount的有效大小。

//从这个构造函数中抛出是否安全？今天（2015年11月）
//唯一调用此构造函数的地方是
//stvar:：stvar（serialiter&，sfield const&）
//投掷。如果stvar可以抛出其构造函数，那么也可以
//STAccount。
    if (v.size() != uint160::bytes)
        Throw<std::runtime_error> ("Invalid STAccount size");

    default_ = false;
    memcpy (value_.begin(), v.data (), uint160::bytes);
}

STAccount::STAccount (SerialIter& sit, SField const& name)
    : STAccount(name, sit.getVLBuffer())
{
}

STAccount::STAccount (SField const& n, AccountID const& v)
    : STBase (n)
    , default_ (false)
{
    value_.copyFrom (v);
}

std::string STAccount::getText () const
{
    if (isDefault())
        return "";
    return toBase58 (value());
}

} //涟漪
