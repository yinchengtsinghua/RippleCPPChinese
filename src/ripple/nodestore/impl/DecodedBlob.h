
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

#ifndef RIPPLE_NODESTORE_DECODEDBLOB_H_INCLUDED
#define RIPPLE_NODESTORE_DECODEDBLOB_H_INCLUDED

#include <ripple/nodestore/NodeObject.h>

namespace ripple {
namespace NodeStore {

/*已将键/值blob解析为nodeObject组件。

    这将提取构造nodeObject所需的信息。它
    同时执行一致性检查并返回结果，因此
    以确定数据是否已损坏而不引发异常。不是
    检测到所有形式的腐败，因此需要进一步分析
    消除假阴性。

    @注意，这定义了nodeObject的数据库格式！
**/

class DecodedBlob
{
public:
    /*从原始数据构造解码的blob。*/
    DecodedBlob (void const* key, void const* value, int valueBytes);

    /*确定解码是否成功。*/
    bool wasOk () const noexcept { return m_success; }

    /*从此数据创建nodeObject。*/
    std::shared_ptr<NodeObject> createObject ();

private:
    bool m_success;

    void const* m_key;
    NodeObjectType m_objectType;
    unsigned char const* m_objectData;
    int m_dataBytes;
};

}
}

#endif
