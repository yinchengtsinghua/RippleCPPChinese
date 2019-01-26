
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

#ifndef RIPPLE_JSON_JSONPROPERTYSTREAM_H_INCLUDED
#define RIPPLE_JSON_JSONPROPERTYSTREAM_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/beast/utility/PropertyStream.h>

namespace ripple {

/*产生objectValue类型的json:：value的propertystream:：sink。*/
class JsonPropertyStream : public beast::PropertyStream
{
public:
    Json::Value m_top;
    std::vector <Json::Value*> m_stack;

public:
    JsonPropertyStream ();
    Json::Value const& top() const;

protected:

    void map_begin () override;
    void map_begin (std::string const& key) override;
    void map_end () override;
    void add (std::string const& key, short value) override;
    void add (std::string const& key, unsigned short value) override;
    void add (std::string const& key, int value) override;
    void add (std::string const& key, unsigned int value) override;
    void add (std::string const& key, long value) override;
    void add (std::string const& key, float v) override;
    void add (std::string const& key, double v) override;
    void add (std::string const& key, std::string const& v) override;
    void array_begin () override;
    void array_begin (std::string const& key) override;
    void array_end () override;

    void add (short value) override;
    void add (unsigned short value) override;
    void add (int value) override;
    void add (unsigned int value) override;
    void add (long value) override;
    void add (float v) override;
    void add (double v) override;
    void add (std::string const& v) override;
};

}

#endif
