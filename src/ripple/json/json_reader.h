
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

#ifndef RIPPLE_JSON_JSON_READER_H_INCLUDED
#define RIPPLE_JSON_JSON_READER_H_INCLUDED

# define CPPTL_JSON_READER_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <ripple/json/json_value.h>
#include <boost/asio/buffer.hpp>
#include <stack>

namespace Json
{

/*\brief unserialize a<a href=“http://www.json.org”>json</a>document into a value.
 *
 **/

class Reader
{
public:
    using Char = char;
    using Location = const Char*;

    /*\brief构造一个允许所有功能的读卡器
     *用于解析。
     **/

    Reader () = default;

    /*\brief从a<a href=“http://www.json.org”>json<a>document读取值。
     *\param document utf-8编码字符串，包含要读取的文档。
     *\param root[out]包含文档的根值（如果是）
     *成功分析。
     *\返回\c如果文档分析成功，则返回true；如果出现错误，则返回false。
     **/

    bool parse ( std::string const& document, Value& root);

    /*\brief从a<a href=“http://www.json.org”>json<a>document读取值。
     *\param document utf-8编码字符串，包含要读取的文档。
     *\param root[out]包含文档的根值（如果是）
     *成功分析。
     *\返回\c如果文档分析成功，则返回true；如果出现错误，则返回false。
     **/

    bool parse ( const char* beginDoc, const char* endDoc, Value& root);

///\brief从输入流分析。
///\n请参见json:：operator>>（std:：istream&，json:：value&）。
    bool parse ( std::istream& is, Value& root);

    /*\brief从a<a href=“http://www.json.org”>json<a>buffer sequence读取值。
     *\param root[out]包含文档的根值（如果是）
     *成功分析。
     *\param utf-8编码缓冲序列。
     *\返回\c如果缓冲区分析成功，则返回TRUE；如果发生错误，则返回FALSE。
     **/

    template<class BufferSequence>
    bool
    parse(Value& root, BufferSequence const& bs);

    /*\brief返回一个用户友好的字符串，列出已分析文档中的错误。
     *\返回带错误列表的格式化错误消息，错误位置在
     *解析的文档。如果没有发生错误，则返回空字符串
     *在分析过程中。
     **/

    std::string getFormatedErrorMessages () const;

    static constexpr unsigned nest_limit {25};

private:
    enum TokenType
    {
        tokenEndOfStream = 0,
        tokenObjectBegin,
        tokenObjectEnd,
        tokenArrayBegin,
        tokenArrayEnd,
        tokenString,
        tokenInteger,
        tokenDouble,
        tokenTrue,
        tokenFalse,
        tokenNull,
        tokenArraySeparator,
        tokenMemberSeparator,
        tokenComment,
        tokenError
    };

    class Token
    {
    public:
        explicit Token() = default;

        TokenType type_;
        Location start_;
        Location end_;
    };

    class ErrorInfo
    {
    public:
        explicit ErrorInfo() = default;

        Token token_;
        std::string message_;
        Location extra_;
    };

    using Errors = std::deque<ErrorInfo>;

    bool expectToken ( TokenType type, Token& token, const char* message );
    bool readToken ( Token& token );
    void skipSpaces ();
    bool match ( Location pattern,
                 int patternLength );
    bool readComment ();
    bool readCStyleComment ();
    bool readCppStyleComment ();
    bool readString ();
    Reader::TokenType readNumber ();
    bool readValue(unsigned depth);
    bool readObject(Token& token, unsigned depth);
    bool readArray (Token& token, unsigned depth);
    bool decodeNumber ( Token& token );
    bool decodeString ( Token& token );
    bool decodeString ( Token& token, std::string& decoded );
    bool decodeDouble ( Token& token );
    bool decodeUnicodeCodePoint ( Token& token,
                                  Location& current,
                                  Location end,
                                  unsigned int& unicode );
    bool decodeUnicodeEscapeSequence ( Token& token,
                                       Location& current,
                                       Location end,
                                       unsigned int& unicode );
    bool addError ( std::string const& message,
                    Token& token,
                    Location extra = 0 );
    bool recoverFromError ( TokenType skipUntilToken );
    bool addErrorAndRecover ( std::string const& message,
                              Token& token,
                              TokenType skipUntilToken );
    void skipUntilSpace ();
    Value& currentValue ();
    Char getNextChar ();
    void getLocationLineAndColumn ( Location location,
                                    int& line,
                                    int& column ) const;
    std::string getLocationLineAndColumn ( Location location ) const;
    void skipCommentTokens ( Token& token );

    using Nodes = std::stack<Value*>;
    Nodes nodes_;
    Errors errors_;
    std::string document_;
    Location begin_;
    Location end_;
    Location current_;
    Location lastValueEnd_;
    Value* lastValue_;
};

template<class BufferSequence>
bool
Reader::parse(Value& root, BufferSequence const& bs)
{
    using namespace boost::asio;
    std::string s;
    s.reserve (buffer_size(bs));
    for (auto const& b : bs)
        s.append(buffer_cast<char const*>(b), buffer_size(b));
    return parse(s, root);
}

/*\brief从'sin'读取到'root'。

 始终保留输入JSON的注释。

 这可用于将文件读取到特定子对象中。
 例如：
 代码
 json：：值根；
 cin>>根[“dir”][“file”]；
 Cuth<根；
 终止码
 结果：
 逐字逐句
 {
“dir”：{
    “文件”：{
 //输入流json将嵌套在这里。
    }
}
 }
 结束语
 \throw std：：分析错误时出现异常。
 \请参阅json:：operator<<（）
**/

std::istream& operator>> ( std::istream&, Value& );

} //命名空间JSON

#endif //包含cpptl_json_reader_h_
