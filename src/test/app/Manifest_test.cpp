
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
    版权所有2014 Ripple Labs Inc.

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

#include <ripple/app/misc/Manifest.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/StringUtilities.h>
#include <test/jtx.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/app/main/DBInit.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/STExchange.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/utility/in_place_factory.hpp>

namespace ripple {
namespace test {

class Manifest_test : public beast::unit_test::suite
{
private:
    static PublicKey randomNode ()
    {
        return derivePublicKey (
            KeyType::secp256k1,
            randomSecretKey());
    }

    static PublicKey randomMasterKey ()
    {
        return derivePublicKey (
            KeyType::ed25519,
            randomSecretKey());
    }

    static void cleanupDatabaseDir (boost::filesystem::path const& dbPath)
    {
        using namespace boost::filesystem;
        if (!exists (dbPath) || !is_directory (dbPath) || !is_empty (dbPath))
            return;
        remove (dbPath);
    }

    static void setupDatabaseDir (boost::filesystem::path const& dbPath)
    {
        using namespace boost::filesystem;
        if (!exists (dbPath))
        {
            create_directory (dbPath);
            return;
        }

        if (!is_directory (dbPath))
        {
//有人创建了一个文件，我们想把目录放在其中
            Throw<std::runtime_error> ("Cannot create directory: " +
                                      dbPath.string ());
        }
    }
    static boost::filesystem::path getDatabasePath ()
    {
        return boost::filesystem::current_path () / "manifest_test_databases";
    }

public:
    Manifest_test ()
    {
        try
        {
            setupDatabaseDir (getDatabasePath ());
        }
        catch (std::exception const&)
        {
        }
    }
    ~Manifest_test ()
    {
        try
        {
            cleanupDatabaseDir (getDatabasePath ());
        }
        catch (std::exception const&)
        {
        }
    }

    std::string
    makeManifestString (
        PublicKey const& pk,
        SecretKey const& sk,
        PublicKey const& spk,
        SecretKey const& ssk,
        int seq)
    {
        STObject st(sfGeneric);
        st[sfSequence] = seq;
        st[sfPublicKey] = pk;
        st[sfSigningPubKey] = spk;

        sign(st, HashPrefix::manifest, *publicKeyType(spk), ssk);
        sign(st, HashPrefix::manifest, *publicKeyType(pk), sk,
            sfMasterSignature);

        Serializer s;
        st.add(s);

        return base64_encode (std::string(
            static_cast<char const*> (s.data()), s.size()));
    }

    Manifest
    make_Manifest
        (SecretKey const& sk, KeyType type, SecretKey const& ssk, KeyType stype,
         int seq, bool invalidSig = false)
    {
        auto const pk = derivePublicKey(type, sk);
        auto const spk = derivePublicKey(stype, ssk);

        STObject st(sfGeneric);
        st[sfSequence] = seq;
        st[sfPublicKey] = pk;
        st[sfSigningPubKey] = spk;

        sign(st, HashPrefix::manifest, stype, ssk);
        BEAST_EXPECT(verify(st, HashPrefix::manifest, spk));

        sign(st, HashPrefix::manifest, type,
            invalidSig ? randomSecretKey() : sk, sfMasterSignature);
        BEAST_EXPECT(invalidSig ^ verify(
            st, HashPrefix::manifest, pk, sfMasterSignature));

        Serializer s;
        st.add(s);

        std::string const m (static_cast<char const*> (s.data()), s.size());
        if (auto r = Manifest::make_Manifest (std::move (m)))
            return std::move (*r);
        Throw<std::runtime_error> ("Could not create a manifest");
return *Manifest::make_Manifest(std::move(m)); //静默编译器警告。
    }

    std::string
    makeRevocation
        (SecretKey const& sk, KeyType type, bool invalidSig = false)
    {
        auto const pk = derivePublicKey(type, sk);

        STObject st(sfGeneric);
        st[sfSequence] = std::numeric_limits<std::uint32_t>::max ();
        st[sfPublicKey] = pk;

        sign(st, HashPrefix::manifest, type,
            invalidSig ? randomSecretKey() : sk, sfMasterSignature);
        BEAST_EXPECT(invalidSig ^ verify(
            st, HashPrefix::manifest, pk, sfMasterSignature));

        Serializer s;
        st.add(s);

        return base64_encode (std::string(
            static_cast<char const*> (s.data()), s.size()));
    }

    Manifest
    clone (Manifest const& m)
    {
        return Manifest (m.serialized, m.masterKey, m.signingKey, m.sequence);
    }

    void testLoadStore (ManifestCache& m)
    {
        testcase ("load/store");

        std::string const dbName("ManifestCacheTestDB");
        {
            DatabaseCon::Setup setup;
            setup.dataDir = getDatabasePath ();
            DatabaseCon dbCon(setup, dbName, WalletDBInit, WalletDBCount);

            auto getPopulatedManifests =
                    [](ManifestCache const& cache) -> std::vector<Manifest const*>
                    {
                        std::vector<Manifest const*> result;
                        result.reserve (32);
                        cache.for_each_manifest (
                            [&result](Manifest const& m)
            {result.push_back (&m);});
                        return result;
                    };
            auto sort =
                    [](std::vector<Manifest const*> mv) -> std::vector<Manifest const*>
                    {
                        std::sort (mv.begin (),
                                   mv.end (),
                                   [](Manifest const* lhs, Manifest const* rhs)
            {return lhs->serialized < rhs->serialized;});
                        return mv;
                    };
            std::vector<Manifest const*> const inManifests (
                sort (getPopulatedManifests (m)));

            jtx::Env env (*this);
            auto unl = std::make_unique<ValidatorList> (
                m, m, env.timeKeeper(), env.journal);

            {
//保存不应将不受信任的主密钥存储到数据库
//撤销除外
                m.save (dbCon, "ValidatorManifests",
                    [&unl](PublicKey const& pubKey)
                    {
                        return unl->listed (pubKey);
                    });

                ManifestCache loaded;

                loaded.load (dbCon, "ValidatorManifests");

//检查所有加载的清单是否都是吊销的
                std::vector<Manifest const*> const loadedManifests (
                    sort (getPopulatedManifests (loaded)));

                for (auto const& man : loadedManifests)
                    BEAST_EXPECT(man->revoked());
            }
            {
//保存应将所有受信任的主密钥存储到数据库
                PublicKey emptyLocalKey;
                std::vector<std::string> s1;
                std::vector<std::string> keys;
                std::string cfgManifest;
                for (auto const& man : inManifests)
                    s1.push_back (toBase58(
                        TokenType::NodePublic, man->masterKey));
                unl->load (emptyLocalKey, s1, keys);

                m.save (dbCon, "ValidatorManifests",
                    [&unl](PublicKey const& pubKey)
                    {
                        return unl->listed (pubKey);
                    });
                ManifestCache loaded;
                loaded.load (dbCon, "ValidatorManifests");

//检查清单缓存是否相同
                std::vector<Manifest const*> const loadedManifests (
                    sort (getPopulatedManifests (loaded)));

                if (inManifests.size () == loadedManifests.size ())
                {
                    BEAST_EXPECT(std::equal
                            (inManifests.begin (), inManifests.end (),
                             loadedManifests.begin (),
                             [](Manifest const* lhs, Manifest const* rhs)
                             {return *lhs == *rhs;}));
                }
                else
                {
                    fail ();
                }
            }
            {
//加载配置清单
                ManifestCache loaded;
                std::vector<std::string> const emptyRevocation;

                std::string const badManifest = "bad manifest";
                BEAST_EXPECT(! loaded.load (
                    dbCon, "ValidatorManifests", badManifest, emptyRevocation));

                auto const sk  = randomSecretKey();
                auto const pk  = derivePublicKey(KeyType::ed25519, sk);
                auto const kp = randomKeyPair(KeyType::secp256k1);

                std::string const cfgManifest =
                    makeManifestString (pk, sk, kp.first, kp.second, 0);

                BEAST_EXPECT(loaded.load (
                    dbCon, "ValidatorManifests", cfgManifest, emptyRevocation));
            }
            {
//加载配置吊销
                ManifestCache loaded;
                std::string const emptyManifest;

                std::vector<std::string> const badRevocation = { "bad revocation" };
                BEAST_EXPECT(! loaded.load (
                    dbCon, "ValidatorManifests", emptyManifest, badRevocation));

                auto const sk  = randomSecretKey();
                auto const keyType = KeyType::ed25519;
                auto const pk  = derivePublicKey(keyType, sk);
                auto const kp = randomKeyPair(KeyType::secp256k1);
                std::vector<std::string> const nonRevocation =
                    { makeManifestString (pk, sk, kp.first, kp.second, 0) };

                BEAST_EXPECT(! loaded.load (
                    dbCon, "ValidatorManifests", emptyManifest, nonRevocation));
                BEAST_EXPECT(! loaded.revoked(pk));

                std::vector<std::string> const badSigRevocation =
                    /*akervocation（sk，keytype，true/*无效sig*/）
                期待！负荷负荷
                    dbcon，“validatomanifests”，emptymanifest，badsigrevocation））；
                期待！已加载。已吊销（pk））；

                std:：vector<std:：string>const cfgrevocation=
                    撤销（sk，keytype）
                Beast_Expect（加载.load（
                    dbcon，“validatomanifests”，emptymanifest，cfgrevocation））；

                Beast_Expect（已加载。已吊销（pk））；
            }
        }
        boost:：filesystem:：remove（getdatabasepath（）/
                                   boost:：filesystem:：path（dbname））；
    }

    无效的testGetSignature（）
    {
        测试用例（“GetSignature”）；
        auto const sk=randomscretkey（）；
        auto const pk=派生公钥（keytype:：ed25519，sk）；
        auto const kp=随机密钥对（keytype:：secp256k1）；
        auto const m=生成清单（
            sk，keytype:：ed25519，kp.second，keytype:：secp256k1，0）；

        Stobject ST（sfgeneric）；
        st[sfsequence]=0；
        st[sfpublickey]=pk；
        st[sfsigningpubkey]=kp.first；
        Serializer ss；
        ss.add32（hashprefix:：manifest）；
        st.addwithout igningfields（ss）不带点火字段；
        auto const sig=符号（keytype:：secp256k1，kp.second，ss.slice（））；
        beast_expect（strhex（sig）==strhex（m.getsignature（））；

        auto const mastersig=符号（keytype:：ed25519，sk，ss.slice（））；
        beast_expect（strhex（mastersig）==strhex（m.getmastersignature（））；
    }

    无效的testgetkeys（）。
    {
        测试用例（“getkeys”）；

        manifestcache缓存；
        auto const sk=randomscretkey（）；
        auto const pk=派生公钥（keytype:：ed25519，sk）；

        //如果没有清单，GetSigningKey应返回相同的键
        beast_expect（cache.getsigningkey（pk）==pk）；

        //GetSigningKey应返回临时公钥
        //对于列出的验证程序主公钥
        //GetMasterKey应返回列出的验证程序主密钥
        //对于该临时公钥
        auto const kp0=随机密钥对（keytype:：secp256k1）；
        auto const m0=生成清单（
            sk，keytype:：ed25519，kp0.second，keytype:：secp256k1，0）；
        Beast_Expect（cache.applyManifest（clone（m0））=
                manifestDisposition:：已接受）；
        beast_expect（cache.getsigningkey（pk）==kp0.first）；
        beast_expect（cache.getmasterkey（kp0.first）==pk）；

        //GetSigningKey应返回最新的临时公钥
        //对于列出的验证程序主公钥
        //getmasterkey只应返回最新的主密钥
        //临时公钥
        auto const kp1=随机密钥对（keytype:：secp256k1）；
        auto const m1=生成清单（
            sk，keytype:：ed25519，kp1.second，keytype:：secp256k1，1）；
        Beast_Expect（cache.applyManifest（clone（m1））=
                manifestDisposition:：已接受）；
        beast_expect（cache.getsigningkey（pk）==kp1.first）；
        beast_expect（cache.getmasterkey（kp1.first）==pk）；
        beast_expect（cache.getmasterkey（kp0.first）==kp0.first）；

        //GetSigningKey和GetMasterKey应返回相同的键，如果
        //使用相同的签名密钥应用了一个新清单，但使用的签名密钥更高
        //序列
        auto const m2=生成清单（
            sk，keytype:：ed25519，kp1.second，keytype:：secp256k1，2）；
        Beast_Expect（cache.applyManifest（clone（m2））=
                manifestDisposition:：已接受）；
        beast_expect（cache.getsigningkey（pk）==kp1.first）；
        beast_expect（cache.getmasterkey（kp1.first）==pk）；
        beast_expect（cache.getmasterkey（kp0.first）==kp0.first）；

        //GetSigningKey应返回Boost:：None for a
        //已吊销的主公钥
        //对于临时公钥，getmasterkey应返回boost:：none
        //来自已吊销的主公钥
        auto const kpmax=随机密钥对（keytype:：secp256k1）；
        auto const mmax=生成清单（
            sk，keytype:：ed25519，kpmax.second，keytype:：secp256k1，
            std:：numeric_limits<std:：uint32_t>：max（））；
        Beast_Expect（cache.applyManifest（clone（mmax））=
                manifestDisposition:：已接受）；
        Beast_Expect（cache.revocated（pk））；
        beast_expect（cache.getsigningkey（pk）==pk）；
        beast_expect（cache.getmasterkey（kpmax.first）==kpmax.first）；
        beast_expect（cache.getmasterkey（kp1.first）==kp1.first）；
    }

    void testvalidatorken（）。
    {
        测试用例（“验证令牌”）；

        {
            auto const valsecret=parsebase58<secretkey>（）
                标记类型：：nodeprivate，
                “paqmjz37pkpmrgadblsuf9ab7y7eunz27lqrzoexpas31nji”）；

            //将标记字符串格式化为测试trim（）。
            std:：vector<std:：string>const tokenboob=
                “eyj2ywxpzgf0aw9ux3nly3jldf9rzxkioii5zwq0nwy4njyndfjyze4yti3nddint\n”，
                “\tqzoddjmdyntkwnzk3mmy0ztcxotaymzfmywe5mzc0ntdmytlkyyy2iwibwfuawzl \n”，
                “\tc3qioijkqufbqufgeel lmuz0d21pbxzhdegyaunjtupxqzlnvkzlawxhzncxl3zdee \n”，
                “\t hywexwbgmyr25naefrrtfhz3fyeej3rhdeyklenk9nu1ltbgrefsceftms4u0tg \t\t\n”，
                “bjdntzjmzgtjd1jrswhbt25ndtlzqutxwflvduorbjwmfcrc0fpa1zc1psuzzqu2\n”，
                “hsskfmvxnyzkfpqnnwskdlc2fhze9kyy9hqvpva1mxdnltr21wcmxiuetxwdnzexd1 \n”，
                “nmluoehbu1fluhvnqk2n2tnyvjgr3ztcefusgxhs0pkkrgbfdqwxk1qxfezwrgdj \n”，
                “VusMeyDzBpmJFLCTNWxl3tfzkwm5gt3i3qzbrdzjbavr6u0nqsxpkaxrrod0ifq==\n”
            }；

            自动常量清单=
                “Jaaaaafxie1ftwmmvgth2iccmjqc9gvfkilgfw1/vcxhxxlplc2Gnmhake1agqxxxbwd”
                “wdbid6omsyum0fdapagnk8skfn7mo2fdkwrqihaongu9sakqxyouj+l2v0w+saokvb”
                “+zrs6pshljafusxfaibsvjgesadojc/aazoks1vymgmvrlhpkwx3ywu6in8hasqkpu”
                “gbd67kmarfgvmpathlgkjdvdflwpy5aqedfvv5tja2w0i21eq3mywlvjznfor7c0kw”
                “2aitzscjizditq8=”；

            auto const token=validatortoken：：生成validatortoken（tokenbob）；
            野兽期望（令牌）；
            Beast_Expect（token->validationSecret==*valsecret）；
            Beast_Expect（token->manifest==manifest）；
        }
        {
            std:：vector<std:：string>const bad token=“bad token”
            期待！validatorken：：使_validatorken（badtoken））；
        }
    }

    无效的测试生成清单（）
    {
        测试用例（“生成清单”）；

        std:：array<keytype，2>const keytype_
            键类型：：ed25519，
            键类型：：secp256k1；

        std:：uint32_t序列=0；

        //类型无效的公钥
        auto const ret=strunhex（“9930e7fc9d56bb25d6893ba3f317ae5bcf33b3291b32d63db2654a3313222f7fd020”）；
        auto const badkey=slice ret.first.data（），ret.first.size（）

        //短公钥
        auto const retshort=strunhex（“0330”）；
        auto const shortkey=slice retshort.first.data（），retshort.first.size（）

        自动ToString=[]（stobject const&st）
        {
            串行器S；
            ST. ADD（S）；

            返回std:：string（static_cast<char const*>（s.data（）），s.size（））；
        }；

        for（自动常量键类型：键类型）
        {
            auto const sk=generatesecretkey（keytype，randomseed（））；
            auto const pk=派生公钥（keytype，sk）；

            for（自动构造skeytype:keytype）
            {
                auto const ssk=generatesecretkey（skeytype，randomseed（））；
                auto const spk=派生发布键（skeyType，ssk）；

                auto-buildManifestObject=[&]
                    标准：：uint32_t const&seq，
                    bool nosingpublic=false，
                    bool nosignature=false）
                {
                    Stobject ST（sfgeneric）；
                    st[sfsequence]=序列；
                    st[sfpublickey]=pk；

                    如果（！）不公开）
                        st[sfsigningpubkey]=spk；

                    符号（st，hashprefix:：manifest，keytype，sk，
                        sfmastersignature）；

                    如果（！）鼻音）
                        符号（st，hashprefix:：manifest，skeytype，ssk）；

                    返回ST；
                }；

                auto const st=buildManifestObject（++序列）；

                {
                    //有效清单
                    auto const m=to字符串（st）；

                    auto const manifest=manifest：：生成清单（m）；

                    野兽期待（舱单）；
                    Beast_Expect（manifest->masterkey==pk）；
                    Beast_Expect（manifest->signingkey==spk）；
                    Beast_Expect（manifest->sequence==sequence）；
                    Beast_Expect（manifest->serialized==m）；
                    Beast_Expect（manifest->verify（））；
                }
                {
                    //签名无效的有效清单
                    auto badsigst=st；
                    badsigst[sfpublickey]=badsigst[sfsigningpubkey]；

                    auto const m=to字符串（badsigst）；
                    auto const manifest=manifest：：生成清单（m）；

                    野兽期待（舱单）；
                    Beast_Expect（manifest->masterkey==spk）；
                    Beast_Expect（manifest->signingkey==spk）；
                    Beast_Expect（manifest->sequence==sequence）；
                    Beast_Expect（manifest->serialized==m）；
                    期待！manifest->verify（））；
                }
                {
                    //拒绝缺少的序列
                    自动BADST=ST；
                    Beast_Expect（badst.delfield（sfsequence））；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝缺少的公钥
                    自动BADST=ST；
                    Beast_Expect（badst.delfield（sfpublickey））；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝无效的公钥类型
                    自动BADST=ST；
                    badst[sfpublickey]=badkey；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝短公钥
                    自动BADST=ST；
                    badst[sfpublickey]=短键；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝缺少签名的公钥
                    自动BADST=ST；
                    Beast_Expect（badst.delfield（sfsigningpubkey））；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝无效的签名公钥类型
                    自动BADST=ST；
                    badst[sfsigningpubkey]=badkey；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝短签名公钥
                    自动BADST=ST；
                    badst[sfsigningpubkey]=短键；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝缺少的签名
                    自动BADST=ST；
                    Beast_Expect（badst.delfield（sfmastersignature））；
                    期待！manifest：：生成清单（toString（badst））；
                }
                {
                    //拒绝缺少的签名密钥签名
                    自动BADST=ST；
                    Beast_Expect（badst.delfield（sfsignature））；
                    期待！manifest：：生成清单（toString（badst））；
                }

                //测试撤销（最大序列撤销主密钥）
                自动测试吊销=[&]（Stobject const&st）
                {
                    auto const m=to字符串（st）；
                    auto const manifest=manifest：：生成清单（m）；

                    野兽期待（舱单）；
                    Beast_Expect（manifest->masterkey==pk）；
                    beast_expect（manifest->signingkey==publickey（））；
                    Beast_Expect（manifest->sequence==
                        std:：numeric_limits<std:：uint32_t>：max（））；
                    Beast_Expect（manifest->serialized==m）；
                    Beast_Expect（manifest->verify（））；
                }；

                //有效吊销
                {
                    auto const revst=buildManifestObject（
                        std:：numeric_limits<std:：uint32_t>：max（））；
                    测试撤销（revst）；
                }

                //签名密钥和签名在吊销中是可选的
                {
                    auto const revst=buildManifestObject（
                        std:：numeric_limits<std:：uint32_t>：max（），
                        真/*无签名密钥*/);

                    testRevocation(revSt);
                }
                {
                    auto const revSt = buildManifestObject(
                        std::numeric_limits<std::uint32_t>::max (),
                        /*SE，真/*无签名*/）；
                    测试撤销（revst）；

                }
                {
                    auto const revst=buildManifestObject（
                        std:：numeric_limits<std:：uint32_t>：max（），
                        真/*无签名密钥*/,

                        /*e/*无签名*/）；
                    测试撤销（revst）；
                }
            }
        }
    }

    无效
    Run（）重写
    {
        manifestcache缓存；
        {
            测试用例（“应用”）；
            auto const accepted=manifestDisposition:：accepted；
            auto const stale=manifestDisposition:：stale；
            auto const invalid=manifestDisposition:：invalid；

            auto const sk_a=randomsecretkey（）；
            auto const pk_a=派生公钥（keytype:：ed25519，sk_a）；
            auto const kp_a=随机密钥对（keytype:：secp256k1）；
            auto const s_a0=生成清单（
                sk_a，keytype:：ed25519，kp_a.second，keytype:：secp256k1，0）；
            auto const s_a1=生成清单（
                sk_a，keytype:：ed25519，kp_a.second，keytype:：secp256k1，1）；
            auto const s_amax=生成清单（
                sk_a，keytype:：ed25519，kp_a.second，keytype:：secp256k1，
                std:：numeric_limits<std:：uint32_t>：max（））；

            auto const sk_b=randomsecretkey（）；
            auto const kp_b=随机密钥对（keytype:：secp256k1）；
            auto const s_b0=生成清单（
                sk_b，keytype:：ed25519，kp_b.second，keytype:：secp256k1，0）；
            auto const s_b1=生成清单（
                sk_b，keytype:：ed25519，kp_b.second，keytype:：secp256k1，1）；
            auto const s_b2=生成清单（
                sk_b，键类型：：ed25519，kp_b.second，键类型：：secp256k1，2，
                true）；//invalidsig
            auto const fake=s_b1.serialized+'\0'；

            //ApplyManifest应接受具有
            //更高的序列号
            beast_expect（cache.applymanifest（clone（s_a0））=接受）；
            Beast_Expect（cache.applyManifest（clone（s_a0））==stale）；

            Beast_Expect（cache.applyManifest（clone（s_a1））=接受）；
            Beast_Expect（cache.applyManifest（clone（s_a1））==stale）；
            Beast_Expect（cache.applyManifest（clone（s_a0））==stale）；

            //ApplyManifest应接受具有最大序列号的清单
            //撤消主公钥
            期待！cache.revocated（pk_a））；
            Beast_Expect（s_amax.revoked（））；
            beast_expect（cache.applymanifest（clone（s_amax））=接受）；
            Beast_Expect（cache.applyManifest（clone（s_amax））==stale）；
            Beast_Expect（cache.applyManifest（clone（s_a1））==stale）；
            Beast_Expect（cache.applyManifest（clone（s_a0））==stale）；
            Beast_Expect（cache.revocated（pk_a））；

            //ApplyManifest应拒绝具有无效签名的清单
            Beast_Expect（cache.applyManifest（clone（s_b0））=接受）；
            Beast_Expect（cache.applyManifest（clone（s_b0））==stale）；

            期待！清单：：制作清单（假的）；
            Beast_Expect（cache.applyManifest（clone（s_b2））=无效）；
        }
        testloadstore（缓存）；
        测试获取签名（）；
        TestGKEY（）；
        testvalidatorken（）；
        测试生成清单（）；
    }
}；

Beast定义测试套件（manifest、app、ripple）；

}／／检验
} /纹波
