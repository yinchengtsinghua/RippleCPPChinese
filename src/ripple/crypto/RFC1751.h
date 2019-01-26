
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

#ifndef RIPPLE_CRYPTO_RFC1751_H_INCLUDED
#define RIPPLE_CRYPTO_RFC1751_H_INCLUDED

#include <string>
#include <vector>

namespace ripple {

class RFC1751
{
public:
    static int getKeyFromEnglish (std::string& strKey, std::string const& strHuman);

    static void getEnglishFromKey (std::string& strHuman, std::string const& strKey);

    /*从数据中选择一个字典单词。

        这不是特别安全，但提供
        给定guid或固定数据的唯一名称。我们使用
        它将pubkey_节点变成一个易于记忆和识别的节点。
        4个字符串。
    **/

    static std::string getWordFromBlob (void const* blob, size_t bytes);

private:
    static unsigned long extract (char const* s, int start, int length);
    static void btoe (std::string& strHuman, std::string const& strData);
    static void insert (char* s, int x, int start, int length);
    static void standard (std::string& strWord);
    static int wsrch (std::string const& strWord, int iMin, int iMax);
    static int etob (std::string& strData, std::vector<std::string> vsHuman);

    static char const* s_dictionary [];
};

} //涟漪

#endif
