
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

#ifndef RIPPLE_PROTOCOL_ERRORCODES_H_INCLUDED
#define RIPPLE_PROTOCOL_ERRORCODES_H_INCLUDED

#include <ripple/protocol/JsonFields.h>
#include <ripple/json/json_value.h>

namespace ripple {

//vfalc注意这些在rpc名称空间之外

enum error_code_i
{
rpcUNKNOWN = -1,     //表示此枚举中未列出的代码

    rpcSUCCESS = 0,

rpcBAD_SYNTAX,  //必须为1才能将使用情况打印到命令行。
    rpcJSON_RPC,
    rpcFORBIDDEN,

//超出此行的错误号在版本之间不稳定。
//程序应该使用错误标记。

//误码率
    rpcGENERAL,
    rpcLOAD_FAILED,
    rpcNO_PERMISSION,
    rpcNO_EVENTS,
    rpcNOT_STANDALONE,
    rpcTOO_BUSY,
    rpcSLOW_DOWN,
    rpcHIGH_FEE,
    rpcNOT_ENABLED,
    rpcNOT_READY,
    rpcAMENDMENT_BLOCKED,

//网络
    rpcNO_CLOSED,
    rpcNO_CURRENT,
    rpcNO_NETWORK,

//Ledger状态
    rpcACT_EXISTS,
    rpcACT_NOT_FOUND,
    rpcINSUF_FUNDS,
    rpcLGR_NOT_FOUND,
    rpcLGR_NOT_VALIDATED,
    rpcMASTER_DISABLED,
    rpcNO_ACCOUNT,
    rpcNO_PATH,
    rpcPASSWD_CHANGED,
    rpcSRC_MISSING,
    rpcSRC_UNCLAIMED,
    rpcTXN_NOT_FOUND,
    rpcWRONG_SEED,

//命令格式错误
    rpcINVALID_PARAMS,
    rpcUNKNOWN_COMMAND,
    rpcNO_PF_REQUEST,

//不良参数
    rpcACT_BITCOIN,
    rpcACT_MALFORMED,
    rpcQUALITY_MALFORMED,
    rpcBAD_BLOB,
    rpcBAD_FEATURE,
    rpcBAD_ISSUER,
    rpcBAD_MARKET,
    rpcBAD_SECRET,
    rpcBAD_SEED,
    rpcCHANNEL_MALFORMED,
    rpcCHANNEL_AMT_MALFORMED,
    rpcCOMMAND_MISSING,
    rpcDST_ACT_MALFORMED,
    rpcDST_ACT_MISSING,
    rpcDST_ACT_NOT_FOUND,
    rpcDST_AMT_MALFORMED,
    rpcDST_AMT_MISSING,
    rpcDST_ISR_MALFORMED,
    rpcGETS_ACT_MALFORMED,
    rpcGETS_AMT_MALFORMED,
    rpcHOST_IP_MALFORMED,
    rpcLGR_IDXS_INVALID,
    rpcLGR_IDX_MALFORMED,
    rpcPAYS_ACT_MALFORMED,
    rpcPAYS_AMT_MALFORMED,
    rpcPORT_MALFORMED,
    rpcPUBLIC_MALFORMED,
    rpcSIGN_FOR_MALFORMED,
    rpcSENDMAX_MALFORMED,
    rpcSRC_ACT_MALFORMED,
    rpcSRC_ACT_MISSING,
    rpcSRC_ACT_NOT_FOUND,
    rpcSRC_AMT_MALFORMED,
    rpcSRC_CUR_MALFORMED,
    rpcSRC_ISR_MALFORMED,
    rpcSTREAM_MALFORMED,
    rpcATX_DEPRECATED,

//内部错误（不应发生）
rpcINTERNAL,        //一般内部错误。
    rpcNOT_IMPL,
    rpcNOT_SUPPORTED,
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//vfalc注意到这些可能不应该在rpc名称空间中。

namespace RPC {

/*将RPC错误代码映射到其令牌和默认消息。*/
struct ErrorInfo
{
    ErrorInfo (error_code_i code_, std::string const& token_,
        std::string const& message_)
        : code (code_)
        , token (token_)
        , message (message_)
    { }

    error_code_i code;
    std::string token;
    std::string message;
};

/*返回反映错误代码的错误信息。*/
ErrorInfo const& get_error_info (error_code_i code);

/*添加或更新JSON更新以反映错误代码。*/
/*@ {*/
template <class JsonValue>
void inject_error (error_code_i code, JsonValue& json)
{
    ErrorInfo const& info (get_error_info (code));
    json [jss::error] = info.token;
    json [jss::error_code] = info.code;
    json [jss::error_message] = info.message;
}

template <class JsonValue>
void inject_error (int code, JsonValue& json)
{
    inject_error (error_code_i (code), json);
}

template <class JsonValue>
void inject_error (
    error_code_i code, std::string const& message, JsonValue& json)
{
    ErrorInfo const& info (get_error_info (code));
    json [jss::error] = info.token;
    json [jss::error_code] = info.code;
    json [jss::error_message] = message;
}

/*@ }*/

/*返回反映错误代码的新JSON对象。*/
/*@ {*/
Json::Value make_error (error_code_i code);
Json::Value make_error (error_code_i code, std::string const& message);
/*@ }*/

/*返回指示无效参数的新JSON对象。*/
/*@ {*/
inline Json::Value make_param_error (std::string const& message)
{
    return make_error (rpcINVALID_PARAMS, message);
}

inline std::string missing_field_message (std::string const& name)
{
    return "Missing field '" + name + "'.";
}

inline Json::Value missing_field_error (std::string const& name)
{
    return make_param_error (missing_field_message (name));
}

inline Json::Value missing_field_error (Json::StaticString name)
{
    return missing_field_error (std::string (name));
}

inline std::string object_field_message (std::string const& name)
{
    return "Invalid field '" + name + "', not object.";
}

inline Json::Value object_field_error (std::string const& name)
{
    return make_param_error (object_field_message (name));
}

inline Json::Value object_field_error (Json::StaticString name)
{
    return object_field_error (std::string (name));
}

inline std::string invalid_field_message (std::string const& name)
{
    return "Invalid field '" + name + "'.";
}

inline std::string invalid_field_message (Json::StaticString name)
{
    return invalid_field_message (std::string(name));
}

inline Json::Value invalid_field_error (std::string const& name)
{
    return make_param_error (invalid_field_message (name));
}

inline Json::Value invalid_field_error (Json::StaticString name)
{
    return invalid_field_error (std::string (name));
}

inline std::string expected_field_message (
    std::string const& name, std::string const& type)
{
    return "Invalid field '" + name + "', not " + type + ".";
}

inline std::string expected_field_message (
    Json::StaticString name, std::string const& type)
{
    return expected_field_message (std::string (name), type);
}

inline Json::Value expected_field_error (
    std::string const& name, std::string const& type)
{
    return make_param_error (expected_field_message (name, type));
}

inline Json::Value expected_field_error (
    Json::StaticString name, std::string const& type)
{
    return expected_field_error (std::string (name), type);
}

/*@ }*/

/*如果JSON包含RPC错误规范，则返回“true”。*/
bool contains_error (Json::Value const& json);

} //RPC

/*返回包含RPC错误内容的单个字符串。*/
std::string rpcErrorString(Json::Value const& jv);

}

#endif
