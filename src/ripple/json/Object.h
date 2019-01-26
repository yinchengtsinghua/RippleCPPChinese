
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

#ifndef RIPPLE_JSON_OBJECT_H_INCLUDED
#define RIPPLE_JSON_OBJECT_H_INCLUDED

#include <ripple/json/Writer.h>
#include <memory>

namespace Json {

/*
    集合是数组和对象的基类，这些类提供
    O（1）JSON编写器的JSON集合外观，但仍使用no
    堆内存和堆栈的数量非常少。

    从http://json.org，json有两种类型的集合：数组和对象。
    其他一切都是一个*标量*—一个数字、一个字符串、一个布尔值、一个特殊的
    值为空或旧的json:：value。

    集合必须“按原样”编写JSON，以便
    性能保证。这对API用户造成了限制：

    1。一次只能打开一个集合进行更改。

       此条件将自动强制执行，如果执行此条件，则会引发std:：logic_错误
       被侵犯了。

    2。标记在对象中只能使用一次。

       有些对象有许多标记，因此这种情况可能有点
       昂贵。在调试版本和
       第二次添加标记时引发std:：logic_错误。

    代码示例：

        作家作家；

        //空对象。
        {
            对象：：根（编写器）；
        }
        //输出{}

        //具有一个标量值的对象。
        {
            对象：：根（编写器）；
            write[“hello”]=“世界”；
        }
        //输出“hello”：“world”

        //相同，使用链接。
        {
            object:：root（writer）[“hello”]=“世界”；
        }
        //输出相同。

        //添加多个带链接的标量。
        {
            对象：：根（编写器）
                .set（“你好”，“世界”）
                .set（“flag”，错误）
                设置（“x”，42）；
        }
        //输出“hello”：“world”，“flag”：false，“x”：42

        //添加数组。
        {
            对象：：根（编写器）；
            {
               auto array=root.setarray（“hands”）；
               array.append（“左”）；
               array.append（“right”）；
            }
        }
        //输出“手”：[“左”，“右”]

        //相同，使用链接。
        {
            对象：：根（编写器）
                .setarray（“手”）。
                .append（“左”）
                .追加（“权利”）；
        }
        //输出相同。

        //添加一个对象。
        {
            对象：：根（编写器）；
            {
               auto-object=root.setobject（“hands”）；
               对象[“左”]=假；
               对象[“右”]=真；
            }
        }
        //输出“手”：“左”：假，“右”：真

        //相同，使用链接。
        {
            对象：：根（编写器）
                .setobject（“手”）。
                .set（“左”，假）
                .set（“右”，真）；
            }
        }
        //输出“手”：“左”：假，“右”：真


   犯错误和获得std:：logic_错误的典型方法：

        作家作家；
        对象：：根（编写器）；

        //重复一个标记。
        {
            root[“Hello”]=“世界”；
            root[“hello”]=“那里”；//抛出！在调试生成中。
        }

        //打开子集合，然后设置其他内容。
        {
            auto-object=root.setobject（“foo”）；
            root[“hello”]=“世界”；//抛出！
        }

        //一次打开两个子集合。
        {
            auto-object=root.setobject（“foo”）；
            auto array=root.setarray（“bar”）；//抛出！！
        }

   有关更多示例，请检查单元测试。
 **/


class Collection
{
public:
    Collection (Collection&& c) noexcept;
    Collection& operator= (Collection&& c) noexcept;
    Collection() = delete;

    ~Collection();

protected:
//空父级意味着“根本没有父级”。
//写入程序不能为空。
    Collection (Collection* parent, Writer*);
    void checkWritable (std::string const& label);

    Collection* parent_;
    Writer* writer_;
    bool enabled_;
};

class Array;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*表示正在写入写入写入程序的JSON对象。*/
class Object : protected Collection
{
public:
    /*对象：：根是唯一具有公共构造函数的集合。*/
    class Root;

    /*在对象中为键设置一个标量值。

        JSON标量是单个值-数字、字符串、布尔值、nullptr或
        JSON:：值。

        `set（）`如果此对象被禁用，则引发异常（这意味着
        其中一个子项已启用）。

        在调试生成中，如果键具有
        之前已设置（）。

        提供了一个运算符[]以允许写入'object[“key”]=scalar；`。
     **/

    template <typename Scalar>
    void set (std::string const& key, Scalar const&);

    void set (std::string const& key, Json::Value const&);

//用于实现运算符[]的详细类和方法。
    class Proxy;

    Proxy operator[] (std::string const& key);
    Proxy operator[] (Json::StaticString const& key);

    /*在键处生成一个新对象并返回它。

        在销毁子对象之前，此对象将被禁用。
        如果此对象已被禁用，则引发异常。
     **/

    Object setObject (std::string const& key);

    /*在键处创建一个新数组并返回它。

        在销毁子数组之前，此对象将被禁用。
        如果此对象已被禁用，则引发异常。
     **/

    Array setArray (std::string const& key);

protected:
    friend class Array;
    Object (Collection* parent, Writer* w) : Collection (parent, w) {}
};

class Object::Root : public Object
{
  public:
    /*每个对象：：根必须用自己的唯一编写器构造。*/
    Root (Writer&);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*表示正在写入写入写入程序的JSON数组。*/
class Array : private Collection
{
public:
    /*向数组追加一个标量。

        如果此数组被禁用，则引发异常（这意味着
        其子集合已启用）。
    **/

    template <typename Scalar>
    void append (Scalar const&);

    /*
       向数组追加json：：值。
       如果此数组被禁用，则引发异常。
     **/

    void append (Json::Value const&);

    /*附加一个新对象并返回它。

        在销毁子对象之前，此数组将被禁用。
        如果此数组被禁用，则引发异常。
     **/

    Object appendObject ();

    /*附加一个新数组并返回它。

        在销毁子数组之前，此数组将被禁用。
        如果此数组已被禁用，则引发异常。
     **/

    Array appendArray ();

  protected:
    friend class Object;
    Array (Collection* parent, Writer* w) : Collection (parent, w) {}
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//允许json:：value和collection
//互操作。

/*在JSON对象的命名键处添加新的子数组。*/
Json::Value& setArray (Json::Value&, Json::StaticString const& key);

/*在JSON对象的命名键处添加新的子数组。*/
Array setArray (Object&, Json::StaticString const& key);


/*在JSON对象的命名键处添加新的子对象。*/
Json::Value& addObject (Json::Value&, Json::StaticString const& key);

/*在JSON对象的命名键处添加新的子对象。*/
Object addObject (Object&, Json::StaticString const& key);


/*将新的子数组附加到JSON数组。*/
Json::Value& appendArray (Json::Value&);

/*将新的子数组附加到JSON数组。*/
Array appendArray (Array&);


/*将新的子对象附加到JSON对象。*/
Json::Value& appendObject (Json::Value&);

/*将新的子对象附加到JSON对象。*/
Object appendObject (Array&);


/*将所有键和值从一个对象复制到另一个对象。*/
void copyFrom (Json::Value& to, Json::Value const& from);

/*将所有键和值从一个对象复制到另一个对象。*/
void copyFrom (Object& to, Json::Value const& from);


/*包含自己的编写器的对象。*/
class WriterObject
{
public:
    WriterObject (Output const& output)
            : writer_ (std::make_unique<Writer> (output)),
              object_ (std::make_unique<Object::Root> (*writer_))
    {
    }

    WriterObject (WriterObject&& other) = default;

    Object* operator->()
    {
        return object_.get();
    }

    Object& operator*()
    {
        return *object_;
    }

private:
    std::unique_ptr <Writer> writer_;
    std::unique_ptr<Object::Root> object_;
};

WriterObject stringWriterObject (std::string&);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//实施细节。

//对象：：运算符[]的详细信息类。
class Object::Proxy
{
private:
    Object& object_;
    std::string const key_;

public:
    Proxy (Object& object, std::string const& key);

    template <class T>
    void operator= (T const& t)
    {
        object_.set (key_, t);
//注意：这个函数不应该返回*这个，因为它是一个陷阱。
//
//在json:：value中，foo[jss:：key]返回对
//可变的json：：包含在foo中的值。但在这种情况下
//对象，我们只写一次，没有这样的对象
//可返回的引用。返回*这将返回
//对象“比json:：value中的更高一级”，这会导致隐藏的错误，
//尤其是在通用代码中。
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <typename Scalar>
void Array::append (Scalar const& value)
{
    checkWritable ("append");
    if (writer_)
        writer_->append (value);
}

template <typename Scalar>
void Object::set (std::string const& key, Scalar const& value)
{
    checkWritable ("set");
    if (writer_)
        writer_->set (key, value);
}

inline
Json::Value& setArray (Json::Value& json, Json::StaticString const& key)
{
    return (json[key] = Json::arrayValue);
}

inline
Array setArray (Object& json, Json::StaticString const& key)
{
    return json.setArray (std::string (key));
}

inline
Json::Value& addObject (Json::Value& json, Json::StaticString const& key)
{
    return (json[key] = Json::objectValue);
}

inline
Object addObject (Object& object, Json::StaticString const& key)
{
    return object.setObject (std::string (key));
}

inline
Json::Value& appendArray (Json::Value& json)
{
    return json.append (Json::arrayValue);
}

inline
Array appendArray (Array& json)
{
    return json.appendArray ();
}

inline
Json::Value& appendObject (Json::Value& json)
{
    return json.append (Json::objectValue);
}

inline
Object appendObject (Array& json)
{
    return json.appendObject ();
}

} //杰森

#endif
