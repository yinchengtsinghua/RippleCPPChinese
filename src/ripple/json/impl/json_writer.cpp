
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

#include <ripple/json/json_writer.h>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace Json
{

static bool isControlCharacter (char ch)
{
    return ch > 0 && ch <= 0x1F;
}

static bool containsControlCharacter ( const char* str )
{
    while ( *str )
    {
        if ( isControlCharacter ( * (str++) ) )
            return true;
    }

    return false;
}
static void uintToString ( unsigned int value,
                           char*& current )
{
    *--current = 0;

    do
    {
        *--current = (value % 10) + '0';
        value /= 10;
    }
    while ( value != 0 );
}

std::string valueToString ( Int value )
{
    char buffer[32];
    char* current = buffer + sizeof (buffer);
    bool isNegative = value < 0;

    if ( isNegative )
        value = -value;

    uintToString ( UInt (value), current );

    if ( isNegative )
        *--current = '-';

    assert ( current >= buffer );
    return current;
}


std::string valueToString ( UInt value )
{
    char buffer[32];
    char* current = buffer + sizeof (buffer);
    uintToString ( value, current );
    assert ( current >= buffer );
    return current;
}

std::string valueToString( double value )
{
//分配一个足够大的缓冲区来存储
//精度要求如下。
    char buffer[32];
//打印到缓冲区。我们不需要请求其他代表
//它总是有一个小数点，因为JSON不区分
//实数和整数的概念。
#if defined(_MSC_VER) && defined(__STDC_SECURE_LIB__) //在Visual Studio 2005中使用安全版本可避免出现警告。
    sprintf_s(buffer, sizeof(buffer), "%.16g", value);
#else
    snprintf(buffer, sizeof(buffer), "%.16g", value);
#endif
    return buffer;
}

std::string valueToString ( bool value )
{
    return value ? "true" : "false";
}

std::string valueToQuotedString ( const char* value )
{
//不确定如何处理Unicode…
    if (strpbrk (value, "\"\\\b\f\n\r\t") == nullptr && !containsControlCharacter ( value ))
        return std::string ("\"") + value + "\"";

//我们必须走价值观和逃避任何特殊的字符。
//附加到std：：string是无效的，但这应该是罕见的。
//（注：正斜杠是*不*罕见的，但我没有逃过。）
unsigned maxsize = strlen (value) * 2 + 3; //allescaped+引号+空
    std::string result;
result.reserve (maxsize); //为了避免大量的malloc
    result += "\"";

    for (const char* c = value; *c != 0; ++c)
    {
        switch (*c)
        {
        case '\"':
            result += "\\\"";
            break;

        case '\\':
            result += "\\\\";
            break;

        case '\b':
            result += "\\b";
            break;

        case '\f':
            result += "\\f";
            break;

        case '\n':
            result += "\\n";
            break;

        case '\r':
            result += "\\r";
            break;

        case '\t':
            result += "\\t";
            break;

//案例“/”：
//尽管在JSON中\/被认为是合法的逃避，但是
//斜线也是合法的，所以我看不出有理由逃避它。
//（我希望我没有误解什么。
//Blep注意：实际上转义\/在javascript中可能有用，以避免</
//序列。
//应添加一个标志以允许此兼容模式并阻止此
//发生的顺序。
        default:
            if ( isControlCharacter ( *c ) )
            {
                std::ostringstream oss;
                oss << "\\u" << std::hex << std::uppercase << std::setfill ('0') << std::setw (4) << static_cast<int> (*c);
                result += oss.str ();
            }
            else
            {
                result += *c;
            }

            break;
        }
    }

    result += "\"";
    return result;
}

//类快速写入程序
//////////////////////////////////////////////


std::string
FastWriter::write ( const Value& root )
{
    document_ = "";
    writeValue ( root );
    return std::move (document_);
}


void
FastWriter::writeValue ( const Value& value )
{
    switch ( value.type () )
    {
    case nullValue:
        document_ += "null";
        break;

    case intValue:
        document_ += valueToString ( value.asInt () );
        break;

    case uintValue:
        document_ += valueToString ( value.asUInt () );
        break;

    case realValue:
        document_ += valueToString ( value.asDouble () );
        break;

    case stringValue:
        document_ += valueToQuotedString ( value.asCString () );
        break;

    case booleanValue:
        document_ += valueToString ( value.asBool () );
        break;

    case arrayValue:
    {
        document_ += "[";
        int size = value.size ();

        for ( int index = 0; index < size; ++index )
        {
            if ( index > 0 )
                document_ += ",";

            writeValue ( value[index] );
        }

        document_ += "]";
    }
    break;

    case objectValue:
    {
        Value::Members members ( value.getMemberNames () );
        document_ += "{";

        for ( Value::Members::iterator it = members.begin ();
                it != members.end ();
                ++it )
        {
            std::string const& name = *it;

            if ( it != members.begin () )
                document_ += ",";

            document_ += valueToQuotedString ( name.c_str () );
            document_ += ":";
            writeValue ( value[name] );
        }

        document_ += "}";
    }
    break;
    }
}


//类StyledWriter
//////////////////////////////////////////////

StyledWriter::StyledWriter ()
    : rightMargin_ ( 74 )
    , indentSize_ ( 3 )
{
}


std::string
StyledWriter::write ( const Value& root )
{
    document_ = "";
    addChildValues_ = false;
    indentString_ = "";
    writeValue ( root );
    document_ += "\n";
    return document_;
}


void
StyledWriter::writeValue ( const Value& value )
{
    switch ( value.type () )
    {
    case nullValue:
        pushValue ( "null" );
        break;

    case intValue:
        pushValue ( valueToString ( value.asInt () ) );
        break;

    case uintValue:
        pushValue ( valueToString ( value.asUInt () ) );
        break;

    case realValue:
        pushValue ( valueToString ( value.asDouble () ) );
        break;

    case stringValue:
        pushValue ( valueToQuotedString ( value.asCString () ) );
        break;

    case booleanValue:
        pushValue ( valueToString ( value.asBool () ) );
        break;

    case arrayValue:
        writeArrayValue ( value);
        break;

    case objectValue:
    {
        Value::Members members ( value.getMemberNames () );

        if ( members.empty () )
            pushValue ( "{}" );
        else
        {
            writeWithIndent ( "{" );
            indent ();
            Value::Members::iterator it = members.begin ();

            while ( true )
            {
                std::string const& name = *it;
                const Value& childValue = value[name];
                writeWithIndent ( valueToQuotedString ( name.c_str () ) );
                document_ += " : ";
                writeValue ( childValue );

                if ( ++it == members.end () )
                    break;

                document_ += ",";
            }

            unindent ();
            writeWithIndent ( "}" );
        }
    }
    break;
    }
}


void
StyledWriter::writeArrayValue ( const Value& value )
{
    unsigned size = value.size ();

    if ( size == 0 )
        pushValue ( "[]" );
    else
    {
        bool isArrayMultiLine = isMultineArray ( value );

        if ( isArrayMultiLine )
        {
            writeWithIndent ( "[" );
            indent ();
            bool hasChildValue = !childValues_.empty ();
            unsigned index = 0;

            while ( true )
            {
                const Value& childValue = value[index];

                if ( hasChildValue )
                    writeWithIndent ( childValues_[index] );
                else
                {
                    writeIndent ();
                    writeValue ( childValue );
                }

                if ( ++index == size )
                    break;

                document_ += ",";
            }

            unindent ();
            writeWithIndent ( "]" );
        }
else //单线输出
        {
            assert ( childValues_.size () == size );
            document_ += "[ ";

            for ( unsigned index = 0; index < size; ++index )
            {
                if ( index > 0 )
                    document_ += ", ";

                document_ += childValues_[index];
            }

            document_ += " ]";
        }
    }
}


bool
StyledWriter::isMultineArray ( const Value& value )
{
    int size = value.size ();
    bool isMultiLine = size * 3 >= rightMargin_ ;
    childValues_.clear ();

    for ( int index = 0; index < size  &&  !isMultiLine; ++index )
    {
        const Value& childValue = value[index];
        isMultiLine = isMultiLine  ||
                      ( (childValue.isArray()  ||  childValue.isObject())  &&
                        childValue.size () > 0 );
    }

if ( !isMultiLine ) //检查线条长度是否大于最大线条长度
    {
        childValues_.reserve ( size );
        addChildValues_ = true;
int lineLength = 4 + (size - 1) * 2; //“[”+“，”*n+“]”

        for ( int index = 0; index < size; ++index )
        {
            writeValue ( value[index] );
            lineLength += int ( childValues_[index].length () );
        }

        addChildValues_ = false;
        isMultiLine = isMultiLine  ||  lineLength >= rightMargin_;
    }

    return isMultiLine;
}


void
StyledWriter::pushValue ( std::string const& value )
{
    if ( addChildValues_ )
        childValues_.push_back ( value );
    else
        document_ += value;
}


void
StyledWriter::writeIndent ()
{
    if ( !document_.empty () )
    {
        char last = document_[document_.length () - 1];

if ( last == ' ' )     //已缩进
            return;

if ( last != '\n' )    //注释可以添加新行
            document_ += '\n';
    }

    document_ += indentString_;
}


void
StyledWriter::writeWithIndent ( std::string const& value )
{
    writeIndent ();
    document_ += value;
}


void
StyledWriter::indent ()
{
    indentString_ += std::string ( indentSize_, ' ' );
}


void
StyledWriter::unindent ()
{
    assert ( int (indentString_.size ()) >= indentSize_ );
    indentString_.resize ( indentString_.size () - indentSize_ );
}

//类StyledTreamWriter
//////////////////////////////////////////////

StyledStreamWriter::StyledStreamWriter ( std::string indentation )
    : document_ (nullptr)
    , rightMargin_ ( 74 )
    , indentation_ ( indentation )
{
}


void
StyledStreamWriter::write ( std::ostream& out, const Value& root )
{
    document_ = &out;
    addChildValues_ = false;
    indentString_ = "";
    writeValue ( root );
    *document_ << "\n";
document_ = nullptr; //为了安全，忘记小溪。
}


void
StyledStreamWriter::writeValue ( const Value& value )
{
    switch ( value.type () )
    {
    case nullValue:
        pushValue ( "null" );
        break;

    case intValue:
        pushValue ( valueToString ( value.asInt () ) );
        break;

    case uintValue:
        pushValue ( valueToString ( value.asUInt () ) );
        break;

    case realValue:
        pushValue ( valueToString ( value.asDouble () ) );
        break;

    case stringValue:
        pushValue ( valueToQuotedString ( value.asCString () ) );
        break;

    case booleanValue:
        pushValue ( valueToString ( value.asBool () ) );
        break;

    case arrayValue:
        writeArrayValue ( value);
        break;

    case objectValue:
    {
        Value::Members members ( value.getMemberNames () );

        if ( members.empty () )
            pushValue ( "{}" );
        else
        {
            writeWithIndent ( "{" );
            indent ();
            Value::Members::iterator it = members.begin ();

            while ( true )
            {
                std::string const& name = *it;
                const Value& childValue = value[name];
                writeWithIndent ( valueToQuotedString ( name.c_str () ) );
                *document_ << " : ";
                writeValue ( childValue );

                if ( ++it == members.end () )
                    break;

                *document_ << ",";
            }

            unindent ();
            writeWithIndent ( "}" );
        }
    }
    break;
    }
}


void
StyledStreamWriter::writeArrayValue ( const Value& value )
{
    unsigned size = value.size ();

    if ( size == 0 )
        pushValue ( "[]" );
    else
    {
        bool isArrayMultiLine = isMultineArray ( value );

        if ( isArrayMultiLine )
        {
            writeWithIndent ( "[" );
            indent ();
            bool hasChildValue = !childValues_.empty ();
            unsigned index = 0;

            while ( true )
            {
                const Value& childValue = value[index];

                if ( hasChildValue )
                    writeWithIndent ( childValues_[index] );
                else
                {
                    writeIndent ();
                    writeValue ( childValue );
                }

                if ( ++index == size )
                    break;

                *document_ << ",";
            }

            unindent ();
            writeWithIndent ( "]" );
        }
else //单线输出
        {
            assert ( childValues_.size () == size );
            *document_ << "[ ";

            for ( unsigned index = 0; index < size; ++index )
            {
                if ( index > 0 )
                    *document_ << ", ";

                *document_ << childValues_[index];
            }

            *document_ << " ]";
        }
    }
}


bool
StyledStreamWriter::isMultineArray ( const Value& value )
{
    int size = value.size ();
    bool isMultiLine = size * 3 >= rightMargin_ ;
    childValues_.clear ();

    for ( int index = 0; index < size  &&  !isMultiLine; ++index )
    {
        const Value& childValue = value[index];
        isMultiLine = isMultiLine  ||
                      ( (childValue.isArray()  ||  childValue.isObject())  &&
                        childValue.size () > 0 );
    }

if ( !isMultiLine ) //检查线条长度是否大于最大线条长度
    {
        childValues_.reserve ( size );
        addChildValues_ = true;
int lineLength = 4 + (size - 1) * 2; //“[”+“，”*n+“]”

        for ( int index = 0; index < size; ++index )
        {
            writeValue ( value[index] );
            lineLength += int ( childValues_[index].length () );
        }

        addChildValues_ = false;
        isMultiLine = isMultiLine  ||  lineLength >= rightMargin_;
    }

    return isMultiLine;
}


void
StyledStreamWriter::pushValue ( std::string const& value )
{
    if ( addChildValues_ )
        childValues_.push_back ( value );
    else
        *document_ << value;
}


void
StyledStreamWriter::writeIndent ()
{
    /*
      这种方法中的一些注释会很好。；-）

     如果（！）文档空（））
     {
        char last=document_[document_.length（）-1]；
        if（last=''）//已缩进
           返回；
        如果（最后）！='\n'）//注释可能会添加新行
           *文档<‘\n’；
     }
    **/

    *document_ << '\n' << indentString_;
}


void
StyledStreamWriter::writeWithIndent ( std::string const& value )
{
    writeIndent ();
    *document_ << value;
}


void
StyledStreamWriter::indent ()
{
    indentString_ += indentation_;
}


void
StyledStreamWriter::unindent ()
{
    assert ( indentString_.size () >= indentation_.size () );
    indentString_.resize ( indentString_.size () - indentation_.size () );
}


std::ostream& operator<< ( std::ostream& sout, const Value& root )
{
    Json::StyledStreamWriter writer;
    writer.write (sout, root);
    return sout;
}

} //命名空间JSON
