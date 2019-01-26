
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

#ifndef RIPPLE_JSON_JSON_WRITER_H_INCLUDED
#define RIPPLE_JSON_JSON_WRITER_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <ripple/json/json_value.h>
#include <ostream>
#include <vector>

namespace Json
{

class Value;

/*\编写者摘要类。
 **/

class WriterBase
{
public:
    virtual ~WriterBase () {}
    virtual std::string write ( const Value& root ) = 0;
};

/*\brief在不带格式的格式（不适合人类）中输出值。
 *
 *JSON文档是以单行形式编写的。它不是用来“人类”消费的，
 *但可能有助于支持诸如rpc等带宽有限的功能。
 *\sa读卡器，值
 **/


class FastWriter : public WriterBase
{
public:
    FastWriter () = default;
    virtual ~FastWriter () {}

public: //从编写器重写
    std::string write ( const Value& root ) override;

private:
    void writeValue ( const Value& value );

    std::string document_;
};

/*\brief以人性化的方式在json格式中写入值。
 *
 *换行和缩进的规则如下：
 *-对象值：
 *-如果为空，则打印不带缩进和换行符
 *-如果不清空打印“”，换行和缩进，则每行打印一个值。
 *然后取消缩进、换行并打印“”。
 *-数组值：
 *-如果为空，则打印不带缩进和换行符的[]
 *如果数组不包含对象值、空数组或其他一些值类型，
 *所有值都适合一行，然后在一行上打印数组。
 *否则，如果值不适合一行，或者数组包含
 *对象或非空数组，然后每行打印一个值。
 *
 *如果值有注释，则根据注释位置输出注释。
 *
 *\sa reader，value，value:：setcomment（）。
 **/

class StyledWriter: public WriterBase
{
public:
    StyledWriter ();
    virtual ~StyledWriter () {}

public: //从编写器重写
    /*\brief序列化json格式中的值。
     *\param要序列化的根值。
     *\返回包含表示根值的JSON文档的字符串。
     **/

    std::string write ( const Value& root ) override;

private:
    void writeValue ( const Value& value );
    void writeArrayValue ( const Value& value );
    bool isMultineArray ( const Value& value );
    void pushValue ( std::string const& value );
    void writeIndent ();
    void writeWithIndent ( std::string const& value );
    void indent ();
    void unindent ();

    using ChildValues = std::vector<std::string>;

    ChildValues childValues_;
    std::string document_;
    std::string indentString_;
    int rightMargin_;
    int indentSize_;
    bool addChildValues_;
};

/*\brief以人性化的方式在json格式中写入值，
     流而不是字符串。
 *
 *换行和缩进的规则如下：
 *-对象值：
 *-如果为空，则打印不带缩进和换行符
 *-如果不清空打印“”，换行和缩进，则每行打印一个值。
 *然后取消缩进、换行并打印“”。
 *-数组值：
 *-如果为空，则打印不带缩进和换行符的[]
 *如果数组不包含对象值、空数组或其他一些值类型，
 *所有值都适合一行，然后在一行上打印数组。
 *否则，如果值不适合一行，或者数组包含
 *对象或非空数组，然后每行打印一个值。
 *
 *如果值有注释，则根据注释位置输出注释。
 *
 *\param indentation每个级别都将额外缩进这个数量。
 *\sa reader，value，value:：setcomment（）。
 **/

class StyledStreamWriter
{
public:
    StyledStreamWriter ( std::string indentation = "\t" );
    ~StyledStreamWriter () {}

public:
    /*\brief序列化json格式中的值。
     *\param输出要写入的流。（可以是排斥流，例如）
     *\param要序列化的根值。
     *\注意，从writer派生没有意义，因为write（）不应返回值。
     **/

    void write ( std::ostream& out, const Value& root );

private:
    void writeValue ( const Value& value );
    void writeArrayValue ( const Value& value );
    bool isMultineArray ( const Value& value );
    void pushValue ( std::string const& value );
    void writeIndent ();
    void writeWithIndent ( std::string const& value );
    void indent ();
    void unindent ();

    using ChildValues = std::vector<std::string>;

    ChildValues childValues_;
    std::ostream* document_;
    std::string indentString_;
    int rightMargin_;
    std::string indentation_;
    bool addChildValues_;
};

std::string valueToString ( Int value );
std::string valueToString ( UInt value );
std::string valueToString ( double value );
std::string valueToString ( bool value );
std::string valueToQuotedString ( const char* value );

///\使用StyledTreamWriter简要输出。
///\n请参阅json:：operator>>（英文）
std::ostream& operator<< ( std::ostream&, const Value& root );

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//流的帮助程序
namespace detail {

template <class Write>
void
write_string(Write const& write, std::string const& s)
{
    write(s.data(), s.size());
}

template <class Write>
void
write_value(Write const& write, Value const& value)
{
    switch (value.type())
    {
        case nullValue:
            write("null", 4);
            break;

        case intValue:
            write_string(write, valueToString(value.asInt()));
            break;

        case uintValue:
            write_string(write, valueToString(value.asUInt()));
            break;

        case realValue:
            write_string(write, valueToString(value.asDouble()));
            break;

        case stringValue:
            write_string(write, valueToQuotedString(value.asCString()));
            break;

        case booleanValue:
            write_string(write, valueToString(value.asBool()));
            break;

        case arrayValue:
        {
            write("[", 1);
            int const size = value.size();
            for (int index = 0; index < size; ++index)
            {
                if (index > 0)
                    write(",", 1);
                write_value(write, value[index]);
            }
            write("]", 1);
            break;
        }

        case objectValue:
        {
            Value::Members const members = value.getMemberNames();
            write("{", 1);
            for (auto it = members.begin(); it != members.end(); ++it)
            {
                std::string const& name = *it;
                if (it != members.begin())
                    write(",", 1);

                write_string(write, valueToQuotedString(name.c_str()));
                write(":", 1);
                write_value(write, value[name]);
            }
            write("}", 1);
            break;
        }
    }
}

}  //命名空间详细信息

/*将JSON流压缩到指定的函数。

    @param jv要写入的json：：值
    @param用签名void（void const*，std：：size_t）编写invocable，
                 当输出应写入流时调用。
**/

template <class Write>
void
stream(Json::Value const& jv, Write const& write)
{
    detail::write_value(write, jv);
    write("\n", 1);
}

/*流式输出紧凑JSON的装饰器

    使用

        JV:：价值合资公司；
        输出<<json:：compact jv

    在流中编写一行紧凑的“jv”，而不是
    而不是来自未修饰流的样式化格式。
**/

class Compact
{
    Json::Value jv_;

public:
    /*为压缩流打包json:：value

        @param jv将json:：value转换为流

        @注意，目前我们不支持包装lvalues以避免
              可能成本高昂的拷贝。如果我们发现需要，我们可以考虑
              在将来增加对紧凑型左值流的支持。
    **/

    Compact(Json::Value&& jv) : jv_{std::move(jv)}
    {
    }

    friend std::ostream&
    operator<<(std::ostream& o, Compact const& cJv)
    {
        detail::write_value(
            [&o](void const* data, std::size_t n) {
                o.write(static_cast<char const*>(data), n);
            },
            cJv.jv_);
        return o;
    }
};

}  //命名空间JSON

#endif //包括日本作家
