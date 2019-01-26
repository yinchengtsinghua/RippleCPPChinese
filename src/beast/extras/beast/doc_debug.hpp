
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#ifndef BEAST_DOC_DEBUG_HPP
#define BEAST_DOC_DEBUG_HPP

namespace beast {

#if BEAST_DOXYGEN

///doc类型（文档调试帮助程序）
using doc_type = int;

///doc enum（文档调试帮助程序）
enum doc_enum
{
///one（文档调试帮助程序）
    one,

///two（文档调试助手）
    two
};

///doc枚举类（文档调试帮助程序）
enum class doc_enum_class : unsigned
{
///one（文档调试帮助程序）
    one,

///two（文档调试助手）
    two
};

///doc func（文档调试助手）
void doc_func();

///doc类（文档调试帮助程序）
struct doc_class
{
///doc类成员func（文档调试助手）
    void func();
};

///（文档调试帮助程序）
namespace nested {

///doc类型（文档调试帮助程序）
using nested_doc_type = int;

///doc enum（文档调试帮助程序）
enum nested_doc_enum
{
///one（文档调试帮助程序）
    one,

///two（文档调试助手）
    two
};

///doc枚举类（文档调试帮助程序）
enum class nested_doc_enum_class : unsigned
{
///one（文档调试帮助程序）
    one,

///two（文档调试助手）
    two
};

///doc func（文档调试助手）
void nested_doc_func();

///doc类（文档调试帮助程序）
struct nested_doc_class
{
///doc类成员func（文档调试助手）
    void func();
};

} //嵌套的

/*这是为了帮助解决doc/reference.xsl问题

    嵌入的引用：

    @li type@ref文件类型

    @li枚举@ref doc\u枚举

    @li枚举项@ref doc_enum:：one

    @li枚举类@ref文档\u枚举类

    @li枚举\u class item@ref doc \u枚举\u class:：one

    @li func@ref文档\u func

    @li类@ref文档类

    @li class func@ref文档\u class:：func

    @li嵌套类型@ref嵌套：：嵌套\u文档类型

    @li嵌套枚举@ref嵌套：：嵌套\u文档\u枚举

    @li嵌套枚举项@ref嵌套：：嵌套\u文档\u枚举：：one

    @li嵌套枚举类@ref嵌套：：嵌套\u文档\u枚举类

    @li嵌套的枚举类项@ref嵌套：：嵌套的\u文档\u枚举类：：one

    @li嵌套func@ref嵌套：：嵌套\u文档\u func

    @li嵌套类@ref嵌套：：嵌套\u文档\u类

    @li嵌套类func@ref嵌套：：嵌套\u文档\u类：：func
**/

void doc_debug();

namespace nested {

/*这是为了帮助解决doc/reference.xsl问题

    嵌入的引用：

    @li type@ref文件类型

    @li枚举@ref doc\u枚举

    @li枚举项@ref doc_enum:：one

    @li枚举类@ref文档\u枚举类

    @li枚举\u class item@ref doc \u枚举\u class:：one

    @li func@ref文档\u func

    @li类@ref文档类

    @li class func@ref文档\u class:：func

    @li嵌套类型@ref嵌套类型

    @li嵌套枚举@ref嵌套\u文档\u枚举

    @li嵌套枚举项@ref嵌套\u文档\u枚举：：one

    @li嵌套枚举类@ref嵌套\u文档\u枚举类

    @li嵌套枚举类项@ref嵌套\u文档\u枚举类：：一

    @li嵌套func@ref嵌套\u文档\u func

    @li嵌套类@ref嵌套类

    @li嵌套类func@ref嵌套\u文档\u类：：func
**/

void nested_doc_debug();

} //嵌套的

#endif

} //野兽

#endif
