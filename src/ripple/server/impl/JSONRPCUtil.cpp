
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

#include <ripple/basics/Log.h>
#include <ripple/server/impl/JSONRPCUtil.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/BuildInfo.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/json/to_string.h>
#include <boost/algorithm/string.hpp>

namespace ripple {

std::string getHTTPHeaderTimestamp ()
{
//检查这可能经常被称为优化它使
//感觉。如果这个函数
//每秒被多次调用。
    char buffer[96];
    time_t now;
    time (&now);
    struct tm now_gmt{};
#ifndef _MSC_VER
    gmtime_r(&now, &now_gmt);
#else
    gmtime_s(&now_gmt, &now);
#endif
    strftime (buffer, sizeof (buffer),
        "Date: %a, %d %b %Y %H:%M:%S +0000\r\n",
        &now_gmt);
    return std::string (buffer);
}

void HTTPReply (
    int nStatus, std::string const& content, Json::Output const& output, beast::Journal j)
{
    JLOG (j.trace())
        << "HTTP Reply " << nStatus << " " << content;

    if (nStatus == 401)
    {
        output ("HTTP/1.0 401 Authorization Required\r\n");
        output (getHTTPHeaderTimestamp ());

//选中此选项将返回与以下答复不同的版本。是
//这是由于设计或意外，或者应该使用
//buildinfo：：getfullversionString（）也是吗？
        output ("Server: " + systemName () + "-json-rpc/v1");
        output ("\r\n");

//小心修改！如果你改变内容，你必须
//更新content-length头，以指示正确的
//数据的大小。
        output ("WWW-Authenticate: Basic realm=\"jsonrpc\"\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: 296\r\n"
                    "\r\n"
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01英寸
"Transitional//E'\r\n”
"\"http://www.w3.org/tr/1999/rec-html401-19991224/loose.dtd“
                    "\">\r\n"
                    "<HTML>\r\n"
                    "<HEAD>\r\n"
                    "<TITLE>Error</TITLE>\r\n"
                    "<META HTTP-EQUIV='Content-Type' "
                    "CONTENT='text/html; charset=ISO-8859-1'>\r\n"
                    "</HEAD>\r\n"
                    "<BODY><H1>401 Unauthorized.</H1></BODY>\r\n");

        return;
    }

    switch (nStatus)
    {
    case 200: output ("HTTP/1.1 200 OK\r\n"); break;
    case 400: output ("HTTP/1.1 400 Bad Request\r\n"); break;
    case 403: output ("HTTP/1.1 403 Forbidden\r\n"); break;
    case 404: output ("HTTP/1.1 404 Not Found\r\n"); break;
    case 500: output ("HTTP/1.1 500 Internal Server Error\r\n"); break;
    case 503: output ("HTTP/1.1 503 Server is overloaded\r\n"); break;
    }

    output (getHTTPHeaderTimestamp ());

    output ("Connection: Keep-Alive\r\n"
            "Content-Length: ");

//vvalco todo确定是否/何时应添加此标题
//if（context.app.config（）.rpc允许远程）
//输出（“访问控制允许来源：*\r\n”）；

    output (std::to_string(content.size () + 2));
    output ("\r\n"
            "Content-Type: application/json; charset=UTF-8\r\n");

    output ("Server: " + systemName () + "-json-rpc/");
    output (BuildInfo::getFullVersionString ());
    output ("\r\n"
            "\r\n");
    output (content);
    output ("\r\n");
}

} //涟漪
