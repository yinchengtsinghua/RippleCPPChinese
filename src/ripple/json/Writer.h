
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

#ifndef RIPPLE_JSON_WRITER_H_INCLUDED
#define RIPPLE_JSON_WRITER_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/basics/ToString.h>
#include <ripple/json/Output.h>
#include <ripple/json/json_value.h>
#include <memory>

namespace Json {

/*
 *编写器实现O（1）-空间，O（1）-粒度输出JSON编写器。
 *
 *o（1）-空格表示它使用固定数量的内存，并且
 *每一步都没有堆分配。
 *
 *o（1）-粒度输出是指写入程序只在
 *限定大小，使用限定的CPU周期数。这是
 *非常有助于安排长作业。
 *
 *权衡的是，您必须在JSON树中填写项目，
 *你永远不能后退。
 *
 *编写器可以编写单个JSON令牌，但典型的用途是编写一个
 *整个JSON对象。例如：
 *
 *{
 *作者W（out）；
 *
 *w.start object（）；//启动根对象。
 *W.set（“你好”，“世界”）；
 *w.set（“再见”，23）；
 *w.finishObject（）；//完成根对象。
 *}
 *
 *输出字符串
 *
 *“你好”：“世界”，“再见”：23
 *
 *对象内部可以有一个对象：
 *
 *{
 *作者W（out）；
 *
 *w.start object（）；//启动根对象。
 *W.set（“你好”，“世界”）；
 *
 *w.startobjectset（“sub object”）；//启动子对象。
 *w.set（“再见”，23）；//添加键，值赋值。
 *w.finishObject（）；//完成子对象。
 *
 *w.finishObject（）；//完成根对象。
 *}
 *
 *输出字符串
 *
 *“hello”：“world”，“subobject”：“再见”：23。
 *
 *阵列工作原理类似
 *
 *{
 *作者W（out）；
 *w.start object（）；//启动根对象。
 *
 *w.startarrayset（“hello”）；//启动一个数组。
 *w.append（23）//附加一些项。
 *W.Append（“Skidoo”）。
 *w.finishArray（）；//完成数组。
 *
 *w.finishObject（）；//完成根对象。
 *}
 *
 *输出字符串
 *
 *“您好”：[23，“Skidoo”]。
 *
 *
 *如果已到达长对象的末尾，则只需使用finishAll（）。
 *完成所有已启动的数组和对象。
 *
 *{
 *作者W（out）；
 *w.start object（）；//启动根对象。
 *
 *w.startarrayset（“hello”）；//启动一个数组。
 *w.append（23）//附加一个项目。
 *
 *w.startarrayappend（）//启动子数组。
 *w.append（“一”）；
 *w.append（“两个”）；
 *
 *w.startObjectAppend（）；//追加一个子对象。
 *w.finishAll（）；//完成所有操作。
 *}
 *
 *输出字符串
 *
 *“您好”：[23，[“一”，“二”，]。
 *
 *为了方便起见，writer的析构函数调用w.finishall（），它使
 *确保所有数组和对象都已关闭。这意味着你可以扔
 *一个例外，或者有一个协程只需清理堆栈，并确保
 *这样做实际上会生成一个完整的JSON对象。
 **/


class Writer
{
public:
    enum CollectionType {array, object};

    explicit Writer (Output const& output);
    Writer(Writer&&) noexcept;
    Writer& operator=(Writer&&) noexcept;

    ~Writer();

    /*在根级别启动新集合。*/
    void startRoot (CollectionType);

    /*在数组中启动新集合。*/
    void startAppend (CollectionType);

    /*在对象内启动新集合。*/
    void startSet (CollectionType, std::string const& key);

    /*完成最近开始的收藏。*/
    void finish ();

    /*完成所有对象和数组。调用finishArray（）之后，没有
     *可以执行更多操作。*/

    void finishAll ();

    /*向数组追加一个值。
     *
     *标量必须是标量-即数字、布尔值、字符串、字符串
     *literal、nullptr或json:：value
     **/

    template <typename Scalar>
    void append (Scalar t)
    {
        rawAppend();
        output (t);
    }

    /*如果不是数组中的第一个项，请在此下一个项之前添加逗号。
        如果您自己编写实际数组，则很有用。*/

    void rawAppend();

    /*向对象添加键、值分配。
     *
     *标量必须是标量-即数字、布尔值、字符串、字符串
     *literal或nullptr。
     *
     *虽然JSON规范没有明确禁止这一点，但是应该避免
     
     *
     *如果定义了check-json-writer，那么如果
     *您使用的标记已在此对象中使用。
     **/

    template <typename Type>
    void set (std::string const& tag, Type t)
    {
        rawSet (tag);
        output (t);
    }

    /*只发出“标记”：作为对象的一部分。如果您正在编写
        实际值数据。*/

    void rawSet (std::string const& key);

//在写单曲之前，你不需要打电话给下面的任何人
//JSON流中的项（数字、字符串、布尔值、空值）。

    /**输出字符串。*/
    void output (std::string const&);

    /**输出文字常量或C字符串。*/
    void output (char const*);

    /**输出一个json：：值。*/
    void output (Json::Value const&);

    /*输出空值。*/
    void output (std::nullptr_t);

    /*输出浮点数。*/
    void output (float);

    /*输出一个双精度。*/
    void output (double);

    /*输出一个布尔值。*/
    void output (bool);

    /*输出数字或布尔值。*/
    template <typename Type>
    void output (Type t)
    {
        implOutput (std::to_string (t));
    }

    void output (Json::StaticString const& t)
    {
        output (t.c_str());
    }

private:
    class Impl;
    std::unique_ptr <Impl> impl_;

    void implOutput (std::string const&);
};

inline void check (bool condition, std::string const& message)
{
    if (! condition)
        ripple::Throw<std::logic_error> (message);
}

} //杰森

#endif
