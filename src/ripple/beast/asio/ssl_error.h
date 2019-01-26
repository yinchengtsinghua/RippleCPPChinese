
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_ASIO_SSL_ERROR_H_INCLUDED
#define BEAST_ASIO_SSL_ERROR_H_INCLUDED

//前两个包含必须首先解决Boost1.62 ASIO头错误
#ifdef _MSC_VER
#  include <winsock2.h>
#endif
#include <boost/asio/ssl/detail/openssl_types.hpp>
#include <boost/asio.hpp>
//include<boost/asio/error.hpp>//导致winsock.h出错
#include <boost/asio/ssl/error.hpp>
#include <boost/lexical_cast.hpp>

namespace beast {

/*如果错误代码与SSL相关，则返回可读消息。*/
template<class = void>
std::string
error_message_with_ssl(boost::system::error_code const& ec)
{
    std::string error;

    if (ec.category() == boost::asio::error::get_ssl_category())
    {
        error = " ("
            + boost::lexical_cast<std::string>(ERR_GET_LIB (ec.value ()))
            + ","
            + boost::lexical_cast<std::string>(ERR_GET_FUNC (ec.value ()))
            + ","
            + boost::lexical_cast<std::string>(ERR_GET_REASON (ec.value ()))
            + ") ";

//此缓冲区必须至少为120字节，大多数示例使用256字节。
//https://www.openssl.org/docs/crypto/err_错误_string.html
        char buf[256];
        ::ERR_error_string_n(ec.value (), buf, sizeof(buf));
        error += buf;
    }
    else
    {
        error = ec.message();
    }

    return error;
}

/*如果错误代码为ssl“短读”，则返回“true”。*/
inline
bool
is_short_read(boost::system::error_code const& ec)
{
#ifdef SSL_R_SHORT_READ
    return (ec.category() == boost::asio::error::get_ssl_category())
        && (ERR_GET_REASON(ec.value()) == SSL_R_SHORT_READ);
#else
    return false;
#endif
}

} //野兽

#endif
