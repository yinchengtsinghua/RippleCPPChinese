
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

#ifndef RIPPLE_PROTOCOL_STPARSEDJSON_H_INCLUDED
#define RIPPLE_PROTOCOL_STPARSEDJSON_H_INCLUDED

#include <ripple/protocol/STArray.h>
#include <boost/optional.hpp>

namespace ripple {

/*保存分析输入JSON对象的序列化结果。
    这将对提供的JSON进行验证和检查。
**/

class STParsedJSONObject
{
public:
    /*分析并创建StParsedJSON对象。
        解析的结果存储在对象和错误中。
        例外情况：
            不投掷。
        @param name诊断中使用的json字段的名称。
        要分析的json-rpc中的@param json。
    **/

    STParsedJSONObject (std::string const& name, Json::Value const& json);

    STParsedJSONObject () = delete;
    STParsedJSONObject (STParsedJSONObject const&) = delete;
    STParsedJSONObject& operator= (STParsedJSONObject const&) = delete;
    ~STParsedJSONObject () = default;

    /*如果分析成功，则返回stobject。*/
    boost::optional <STObject> object;

    /*失败时，一组适当的错误值。*/
    Json::Value error;
};

/*保存分析输入JSON数组的序列化结果。
    这将对提供的JSON进行验证和检查。
**/

class STParsedJSONArray
{
public:
    /*分析并创建一个stparsedjson数组。
        解析的结果存储在数组和错误中。
        例外情况：
            不投掷。
        @param name诊断中使用的json字段的名称。
        要分析的json-rpc中的@param json。
    **/

    STParsedJSONArray (std::string const& name, Json::Value const& json);

    STParsedJSONArray () = delete;
    STParsedJSONArray (STParsedJSONArray const&) = delete;
    STParsedJSONArray& operator= (STParsedJSONArray const&) = delete;
    ~STParsedJSONArray () = default;

    /*如果分析成功，则返回starray。*/
    boost::optional <STArray> array;

    /*失败时，一组适当的错误值。*/
    Json::Value error;
};



} //涟漪

#endif
