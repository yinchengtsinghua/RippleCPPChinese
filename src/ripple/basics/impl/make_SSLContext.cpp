
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

#include <ripple/basics/chrono.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/make_SSLContext.h>
#include <ripple/beast/container/aged_unordered_set.h>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace ripple {
namespace openssl {
namespace detail {

//我们限制我们请求的密码，并允许确保
//不使用密码。但这并不是
//波纹服务器服务器用例，我们只需要mitm
//检测/预防，我们还有WebSocket和RPC场景
//要确保弱密码不能被使用。
std::string const defaultCipherList =
    "HIGH:MEDIUM:!aNULL:!MD5:!DSS:!3DES:!RC4:!EXPORT";

template <class>
struct custom_delete;

template <>
struct custom_delete <RSA>
{
    explicit custom_delete() = default;

    void operator() (RSA* rsa) const
    {
        RSA_free (rsa);
    }
};

template <>
struct custom_delete <EVP_PKEY>
{
    explicit custom_delete() = default;

    void operator() (EVP_PKEY* evp_pkey) const
    {
        EVP_PKEY_free (evp_pkey);
    }
};

template <>
struct custom_delete <X509>
{
    explicit custom_delete() = default;

    void operator() (X509* x509) const
    {
        X509_free (x509);
    }
};

template <>
struct custom_delete <DH>
{
    explicit custom_delete() = default;

    void operator() (DH* dh) const
    {
        DH_free (dh);
    }
};

template <class T>
using custom_delete_unique_ptr = std::unique_ptr <T, custom_delete <T>>;

//RSA

using rsa_ptr = custom_delete_unique_ptr <RSA>;

static rsa_ptr rsa_generate_key (int n_bits)
{
#if OPENSSL_VERSION_NUMBER >= 0x00908000L
    BIGNUM *bn = BN_new();
    BN_set_word(bn, RSA_F4);

    RSA* rsa = RSA_new();
    if (RSA_generate_key_ex(rsa, n_bits, bn, nullptr) != 1)
    {
        RSA_free(rsa);
        rsa = nullptr;
    }

    BN_free(bn);
#else
    RSA* rsa = RSA_generate_key (n_bits, RSA_F4, nullptr, nullptr);
#endif

    if (rsa == nullptr)
        LogicError ("RSA_generate_key failed");

    return rsa_ptr (rsa);
}

//EppH-pKEY

using evp_pkey_ptr = custom_delete_unique_ptr <EVP_PKEY>;

static evp_pkey_ptr evp_pkey_new()
{
    EVP_PKEY* evp_pkey = EVP_PKEY_new();

    if (evp_pkey == nullptr)
        LogicError ("EVP_PKEY_new failed");

    return evp_pkey_ptr (evp_pkey);
}

static void evp_pkey_assign_rsa (EVP_PKEY* evp_pkey, rsa_ptr rsa)
{
    if (! EVP_PKEY_assign_RSA (evp_pkey, rsa.get()))
        LogicError ("EVP_PKEY_assign_RSA failed");

    rsa.release();
}

//X509

using x509_ptr = custom_delete_unique_ptr <X509>;

static x509_ptr x509_new()
{
    X509* x509 = X509_new();

    if (x509 == nullptr)
        LogicError ("X509_new failed");

    X509_set_version (x509, NID_X509);

int const margin =                    60 * 60;  //3600，一小时
int const length = 10 * 365.25 * 24 * 60 * 60;  //315576000，10年

    X509_gmtime_adj (X509_get_notBefore (x509), -margin);
    X509_gmtime_adj (X509_get_notAfter  (x509),  length);

    return x509_ptr (x509);
}

static void x509_set_pubkey (X509* x509, EVP_PKEY* evp_pkey)
{
    X509_set_pubkey (x509, evp_pkey);
}

static void x509_sign (X509* x509, EVP_PKEY* evp_pkey)
{
    if (! X509_sign (x509, evp_pkey, EVP_sha1()))
        LogicError ("X509_sign failed");
}

static void ssl_ctx_use_certificate (SSL_CTX* const ctx, x509_ptr cert)
{
    if (SSL_CTX_use_certificate (ctx, cert.get()) <= 0)
        LogicError ("SSL_CTX_use_certificate failed");
}

static void ssl_ctx_use_privatekey (SSL_CTX* const ctx, evp_pkey_ptr key)
{
    if (SSL_CTX_use_PrivateKey (ctx, key.get()) <= 0)
        LogicError ("SSL_CTX_use_PrivateKey failed");
}

#ifdef SSL_FLAGS_NO_RENEGOTIATE_CIPHERS
static
bool
disallowRenegotiation (SSL const* ssl, bool isNew)
{
//跟踪上次协商SSL连接的时间以及
//不允许连接重新协商超过
//每4分钟一次
    struct StaticData
    {
        std::mutex lock;
        beast::aged_unordered_set <SSL const*> set;

        StaticData()
            : set (ripple::stopwatch())
        {
        }
    };

    static StaticData sd;
    std::lock_guard <std::mutex> lock (sd.lock);
    auto const expired (sd.set.clock().now() - std::chrono::minutes(4));

//删除过期条目
    for (auto iter (sd.set.chronological.begin ());
        (iter != sd.set.chronological.end ()) && (iter.when () <= expired);
        iter = sd.set.chronological.begin ())
    {
        sd.set.erase (iter);
    }

    auto iter = sd.set.find (ssl);
    if (iter != sd.set.end ())
    {
        if (! isNew)
        {
//这是一次重新谈判，上次谈判是最近的。
            return true;
        }

        sd.set.touch (iter);
    }
    else
    {
        sd.set.emplace (ssl);
    }

    return false;
}

static
void
info_handler (SSL const* ssl, int event, int)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    if ((ssl->s3) && (event & SSL_CB_HANDSHAKE_START))
    {
        if (disallowRenegotiation (ssl, SSL_in_before (ssl)))
            ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
    }
#else
//空，openssl 1.1中删除的标志
#endif
}
#endif

static
std::string
error_message (std::string const& what,
    boost::system::error_code const& ec)
{
    std::stringstream ss;
    ss <<
        what << ": " <<
        ec.message() <<
        " (" << ec.value() << ")";
    return ss.str();
}

static
void
initAnonymous (
    boost::asio::ssl::context& context)
{
    using namespace openssl;

    evp_pkey_ptr pkey = evp_pkey_new();
    evp_pkey_assign_rsa (pkey.get(), rsa_generate_key (2048));

    x509_ptr cert = x509_new();
    x509_set_pubkey (cert.get(), pkey.get());
    x509_sign       (cert.get(), pkey.get());

    SSL_CTX* const ctx = context.native_handle();
    ssl_ctx_use_certificate (ctx, std::move(cert));
    ssl_ctx_use_privatekey  (ctx, std::move(pkey));
}

static
void
initAuthenticated (
    boost::asio::ssl::context& context,
    std::string const& key_file,
    std::string const& cert_file,
    std::string const& chain_file)
{
    SSL_CTX* const ssl = context.native_handle ();

    bool cert_set = false;

    if (! cert_file.empty ())
    {
        boost::system::error_code ec;

        context.use_certificate_file (
            cert_file, boost::asio::ssl::context::pem, ec);

        if (ec)
        {
            LogicError (error_message (
                "Problem with SSL certificate file.", ec).c_str());
        }

        cert_set = true;
    }

    if (! chain_file.empty ())
    {
//vfalc用raii替换fopen（）。
        FILE* f = fopen (chain_file.c_str (), "r");

        if (!f)
        {
            LogicError (error_message (
                "Problem opening SSL chain file.", boost::system::error_code (errno,
                boost::system::generic_category())).c_str());
        }

        try
        {
            for (;;)
            {
                X509* const x = PEM_read_X509 (f, nullptr, nullptr, nullptr);

                if (x == nullptr)
                    break;

                if (! cert_set)
                {
                    if (SSL_CTX_use_certificate (ssl, x) != 1)
                        LogicError ("Problem retrieving SSL certificate from chain file.");

                    cert_set = true;
                }
                else if (SSL_CTX_add_extra_chain_cert (ssl, x) != 1)
                {
                    X509_free (x);
                    LogicError ("Problem adding SSL chain certificate.");
                }
            }

            fclose (f);
        }
        catch (std::exception const&)
        {
            fclose (f);
            LogicError ("Reading the SSL chain file generated an exception.");
        }
    }

    if (! key_file.empty ())
    {
        boost::system::error_code ec;

        context.use_private_key_file (key_file,
            boost::asio::ssl::context::pem, ec);

        if (ec)
        {
            LogicError (error_message (
                "Problem using the SSL private key file.", ec).c_str());
        }
    }

    if (SSL_CTX_check_private_key (ssl) != 1)
    {
        LogicError ("Invalid key in SSL private key file.");
    }
}

std::shared_ptr<boost::asio::ssl::context>
get_context (std::string const& cipherList)
{
    auto c = std::make_shared<boost::asio::ssl::context> (
        boost::asio::ssl::context::sslv23);

    c->set_options (
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use);

    {
        auto const& l = !cipherList.empty() ? cipherList : defaultCipherList;
        auto result = SSL_CTX_set_cipher_list (
            c->native_handle (), l.c_str());
        if (result != 1)
            LogicError ("SSL_CTX_set_cipher_list failed");
    }

//这些是Ripple实验室的原始DH参数
//选择Ripple的二进制格式
//D2IY-DHPARAMS。
//
    unsigned char const params[] =
    {
        0x30, 0x82, 0x01, 0x08, 0x02, 0x82, 0x01, 0x01, 0x00, 0x8f, 0xca, 0x66,
        0x85, 0x33, 0xcb, 0xcf, 0x36, 0x27, 0xb2, 0x4c, 0xb8, 0x50, 0xb8, 0xf9,
        0x53, 0xf8, 0xb9, 0x2d, 0x1c, 0xa2, 0xad, 0x86, 0x58, 0x29, 0x3b, 0x88,
        0x3e, 0xf5, 0x65, 0xb8, 0xda, 0x22, 0xf4, 0x8b, 0x21, 0x12, 0x18, 0xf7,
        0x16, 0xcd, 0x7c, 0xc7, 0x3a, 0x2d, 0x61, 0xb7, 0x11, 0xf6, 0xb0, 0x65,
        0xa0, 0x5b, 0xa4, 0x06, 0x95, 0x28, 0xa4, 0x4f, 0x76, 0xc0, 0xeb, 0xfa,
        0x95, 0xdf, 0xbf, 0x19, 0x90, 0x64, 0x8f, 0x60, 0xd5, 0x36, 0xba, 0xab,
        0x0d, 0x5a, 0x5c, 0x94, 0xd5, 0xf7, 0x32, 0xd6, 0x2a, 0x76, 0x77, 0x83,
        0x10, 0xc4, 0x2f, 0x10, 0x96, 0x3e, 0x37, 0x84, 0x45, 0x9c, 0xef, 0x33,
        0xf6, 0xd0, 0x2a, 0xa7, 0xce, 0x0a, 0xce, 0x0d, 0xa1, 0xa7, 0x44, 0x5d,
        0x18, 0x3f, 0x4f, 0xa4, 0x23, 0x9c, 0x5d, 0x74, 0x4f, 0xee, 0xdf, 0xaa,
        0x0d, 0x0a, 0x52, 0x57, 0x73, 0xb1, 0xe4, 0xc5, 0x72, 0x93, 0x9d, 0x03,
        0xe9, 0xf5, 0x48, 0x8c, 0xd1, 0xe6, 0x7c, 0x21, 0x65, 0x4e, 0x16, 0x51,
        0xa3, 0x16, 0x51, 0x10, 0x75, 0x60, 0x37, 0x93, 0xb8, 0x15, 0xd6, 0x14,
        0x41, 0x4a, 0x61, 0xc9, 0x1a, 0x4e, 0x9f, 0x38, 0xd8, 0x2c, 0xa5, 0x31,
        0xe1, 0x87, 0xda, 0x1f, 0xa4, 0x31, 0xa2, 0xa4, 0x42, 0x1e, 0xe0, 0x30,
        0xea, 0x2f, 0x9b, 0x77, 0x91, 0x59, 0x3e, 0xd5, 0xd0, 0xc5, 0x84, 0x45,
        0x17, 0x19, 0x74, 0x8b, 0x18, 0xb0, 0xc1, 0xe0, 0xfc, 0x1c, 0xaf, 0xe6,
        0x2a, 0xef, 0x4e, 0x0e, 0x8a, 0x5c, 0xc2, 0x91, 0xb9, 0x2b, 0xf8, 0x17,
        0x8d, 0xed, 0x44, 0xaa, 0x47, 0xaa, 0x52, 0xa2, 0xdb, 0xb6, 0xf5, 0xa1,
        0x88, 0x85, 0xa1, 0xd5, 0x87, 0xb8, 0x07, 0xd3, 0x97, 0xbe, 0x37, 0x74,
        0x72, 0xf1, 0xa8, 0x29, 0xf1, 0xa7, 0x7d, 0x19, 0xc3, 0x27, 0x09, 0xcf,
        0x23, 0x02, 0x01, 0x02
    };

    unsigned char const *data = &params[0];

    custom_delete_unique_ptr<DH> const dh {d2i_DHparams (nullptr, &data, sizeof(params))};
    if (!dh)
        LogicError ("d2i_DHparams returned nullptr.");

    SSL_CTX_set_tmp_dh (c->native_handle (), dh.get());

#ifdef SSL_FLAGS_NO_RENEGOTIATE_CIPHERS
    SSL_CTX_set_info_callback (c->native_handle (), info_handler);
#endif

    return c;
}

} //细节
} //OpenSSL

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
std::shared_ptr<boost::asio::ssl::context>
make_SSLContext(std::string const& cipherList)
{
    auto context = openssl::detail::get_context(cipherList);
    openssl::detail::initAnonymous(*context);
//vvalco注意到，似乎websocket上下文从来没有
//为websocket的任一设置设置调用了set_verify_模式
    context->set_verify_mode(boost::asio::ssl::verify_none);
    return context;
}

std::shared_ptr<boost::asio::ssl::context>
make_SSLContextAuthed (
    std::string const& keyFile,
    std::string const& certFile,
    std::string const& chainFile,
    std::string const& cipherList)
{
    auto context = openssl::detail::get_context(cipherList);
    openssl::detail::initAuthenticated(*context,
        keyFile, certFile, chainFile);
    return context;
}

} //涟漪
