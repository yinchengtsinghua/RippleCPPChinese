
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

#include <ripple/core/DatabaseCon.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/main/NodeIdentity.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/core/ConfigSections.h>
#include <boost/format.hpp>
#include <boost/optional.hpp>

namespace ripple {

std::pair<PublicKey, SecretKey>
loadNodeIdentity (Application& app)
{
//如果在配置文件中指定了种子，则直接使用该种子。
    if (app.config().exists(SECTION_NODE_SEED))
    {
        auto const seed = parseBase58<Seed>(
            app.config().section(SECTION_NODE_SEED).lines().front());

        if (!seed)
            Throw<std::runtime_error>(
                "NodeIdentity: Bad [" SECTION_NODE_SEED "] specified");

        auto secretKey =
            generateSecretKey (KeyType::secp256k1, *seed);
        auto publicKey =
            derivePublicKey (KeyType::secp256k1, secretKey);

        return { publicKey, secretKey };
    }

//尝试从数据库加载节点标识：
    boost::optional<PublicKey> publicKey;
    boost::optional<SecretKey> secretKey;

    auto db = app.getWalletDB ().checkoutDb ();

    {
        boost::optional<std::string> pubKO, priKO;
        soci::statement st = (db->prepare <<
            "SELECT PublicKey, PrivateKey FROM NodeIdentity;",
                soci::into(pubKO),
                soci::into(priKO));
        st.execute ();
        while (st.fetch ())
        {
            auto const sk = parseBase58<SecretKey>(
                TokenType::NodePrivate, priKO.value_or(""));
            auto const pk = parseBase58<PublicKey>(
                TokenType::NodePublic, pubKO.value_or(""));

//仅当公钥和密钥是对时使用
            if (sk && pk && (*pk == derivePublicKey (KeyType::secp256k1, *sk)))
            {
                secretKey = sk;
                publicKey = pk;
            }
        }
    }

//如果找不到有效身份，我们会随机生成一个新身份：
    if (!publicKey || !secretKey)
    {
        std::tie(publicKey, secretKey) = randomKeyPair(KeyType::secp256k1);

        *db << str (boost::format (
            "INSERT INTO NodeIdentity (PublicKey,PrivateKey) VALUES ('%s','%s');")
                % toBase58 (TokenType::NodePublic, *publicKey)
                % toBase58 (TokenType::NodePrivate, *secretKey));
    }

    return { *publicKey, *secretKey };
}

} //涟漪
