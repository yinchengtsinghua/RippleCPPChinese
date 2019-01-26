
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

#include <ripple/json/Output.h>
#include <ripple/json/Writer.h>
#include <stack>
#include <set>

namespace Json {

namespace {

std::map <char, const char*> jsonSpecialCharacterEscape = {
    {'"',  "\\\""},
    {'\\', "\\\\"},
    {'/',  "\\/"},
    {'\b', "\\b"},
    {'\f', "\\f"},
    {'\n', "\\n"},
    {'\r', "\\r"},
    {'\t', "\\t"}
};

static size_t const jsonEscapeLength = 2;

//所有其他JSON标点符号。
const char closeBrace = '}';
const char closeBracket = ']';
const char colon = ':';
const char comma = ',';
const char openBrace = '{';
const char openBracket = '[';
const char quote = '"';

const std::string none;

static auto const integralFloatsBecomeInts = false;

size_t lengthWithoutTrailingZeros (std::string const& s)
{
    auto dotPos = s.find ('.');
    if (dotPos == std::string::npos)
        return s.size();

    auto lastNonZero = s.find_last_not_of ('0');
    auto hasDecimals = dotPos != lastNonZero;

    if (hasDecimals)
        return lastNonZero + 1;

    if (integralFloatsBecomeInts || lastNonZero + 2 > s.size())
        return lastNonZero;

    return lastNonZero + 2;
}

} //命名空间

class Writer::Impl
{
public:
    explicit
    Impl (Output const& output) : output_(output) {}
    ~Impl() = default;

    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    bool empty() const { return stack_.empty (); }

    void start (CollectionType ct)
    {
        char ch = (ct == array) ? openBracket : openBrace;
        output ({&ch, 1});
        stack_.push (Collection());
        stack_.top().type = ct;
    }

    void output (boost::beast::string_view const& bytes)
    {
        markStarted ();
        output_ (bytes);
    }

    void stringOutput (boost::beast::string_view const& bytes)
    {
        markStarted ();
        std::size_t position = 0, writtenUntil = 0;

        output_ ({&quote, 1});
        auto data = bytes.data();
        for (; position < bytes.size(); ++position)
        {
            auto i = jsonSpecialCharacterEscape.find (data[position]);
            if (i != jsonSpecialCharacterEscape.end ())
            {
                if (writtenUntil < position)
                {
                    output_ ({data + writtenUntil, position - writtenUntil});
                }
                output_ ({i->second, jsonEscapeLength});
                writtenUntil = position + 1;
            };
        }
        if (writtenUntil < position)
            output_ ({data + writtenUntil, position - writtenUntil});
        output_ ({&quote, 1});
    }

    void markStarted ()
    {
        check (!isFinished(), "isFinished() in output.");
        isStarted_ = true;
    }

    void nextCollectionEntry (CollectionType type, std::string const& message)
    {
        check (!empty() , "empty () in " + message);

        auto t = stack_.top ().type;
        if (t != type)
        {
            check (false, "Not an " +
                   ((type == array ? "array: " : "object: ") + message));
        }
        if (stack_.top ().isFirst)
            stack_.top ().isFirst = false;
        else
            output_ ({&comma, 1});
    }

    void writeObjectTag (std::string const& tag)
    {
#ifndef NDEBUG
//确保我们还没有看到这个标签。
        auto& tags = stack_.top ().tags;
        check (tags.find (tag) == tags.end (), "Already seen tag " + tag);
        tags.insert (tag);
#endif

        stringOutput (tag);
        output_ ({&colon, 1});
    }

    bool isFinished() const
    {
        return isStarted_ && empty();
    }

    void finish ()
    {
        check (!empty(), "Empty stack in finish()");

        auto isArray = stack_.top().type == array;
        auto ch = isArray ? closeBracket : closeBrace;
        output_ ({&ch, 1});
        stack_.pop();
    }

    void finishAll ()
    {
        if (isStarted_)
        {
            while (!isFinished())
                finish();
        }
    }

    Output const& getOutput() const { return output_; }

private:
//JSON集合要么是arrray，要么是对象。
    struct Collection
    {
        explicit Collection() = default;

        /*我们是什么类型的收藏品？*/
        Writer::CollectionType type;

        /*这是集合中的第一个条目吗？
         *如果为false，则必须在编写下一个条目之前发出。*/

        bool isFirst = true;

#ifndef NDEBUG
        /*我们在这个集合中已经看到了哪些标签？*/
        std::set <std::string> tags;
#endif
    };

    using Stack = std::stack <Collection, std::vector<Collection>>;

    Output output_;
    Stack stack_;

    bool isStarted_ = false;
};

Writer::Writer (Output const &output)
        : impl_(std::make_unique <Impl> (output))
{
}

Writer::~Writer()
{
    if (impl_)
        impl_->finishAll ();
}

Writer::Writer(Writer&& w) noexcept
{
    impl_ = std::move (w.impl_);
}

Writer& Writer::operator=(Writer&& w) noexcept
{
    impl_ = std::move (w.impl_);
    return *this;
}

void Writer::output (char const* s)
{
    impl_->stringOutput (s);
}

void Writer::output (std::string const& s)
{
    impl_->stringOutput (s);
}

void Writer::output (Json::Value const& value)
{
    impl_->markStarted();
    outputJson (value, impl_->getOutput());
}

void Writer::output (float f)
{
    auto s = ripple::to_string (f);
    impl_->output ({s.data (), lengthWithoutTrailingZeros (s)});
}

void Writer::output (double f)
{
    auto s = ripple::to_string (f);
    impl_->output ({s.data (), lengthWithoutTrailingZeros (s)});
}

void Writer::output (std::nullptr_t)
{
    impl_->output ("null");
}

void Writer::output (bool b)
{
    impl_->output (b ? "true" : "false");
}

void Writer::implOutput (std::string const& s)
{
    impl_->output (s);
}

void Writer::finishAll ()
{
    if (impl_)
        impl_->finishAll ();
}

void Writer::rawAppend()
{
    impl_->nextCollectionEntry (array, "append");
}

void Writer::rawSet (std::string const& tag)
{
    check (!tag.empty(), "Tag can't be empty");

    impl_->nextCollectionEntry (object, "set");
    impl_->writeObjectTag (tag);
}

void Writer::startRoot (CollectionType type)
{
    impl_->start (type);
}

void Writer::startAppend (CollectionType type)
{
    impl_->nextCollectionEntry (array, "startAppend");
    impl_->start (type);
}

void Writer::startSet (CollectionType type, std::string const& key)
{
    impl_->nextCollectionEntry (object, "startSet");
    impl_->writeObjectTag (key);
    impl_->start (type);
}

void Writer::finish ()
{
    if (impl_)
        impl_->finish ();
}

} //杰森
