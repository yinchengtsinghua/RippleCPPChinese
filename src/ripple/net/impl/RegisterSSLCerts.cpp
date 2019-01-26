
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
    版权所有（c）2017 Ripple Labs Inc.

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
#include <ripple/net/RegisterSSLCerts.h>
#include <boost/predef.h>
#if BOOST_OS_WINDOWS
#include <boost/asio/ssl/error.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <wincrypt.h>
#endif

namespace ripple {

void
registerSSLCerts(
    boost::asio::ssl::context& ctx,
    boost::system::error_code& ec,
    beast::Journal j)
{
#if BOOST_OS_WINDOWS
    auto certStoreDelete = [](void* h) {
        if (h != nullptr)
            CertCloseStore(h, 0);
    };
    std::unique_ptr<void, decltype(certStoreDelete)> hStore{
        CertOpenSystemStore(0, "ROOT"), certStoreDelete};

    if (!hStore)
    {
        ec = boost::system::error_code(
            GetLastError(), boost::system::system_category());
        return;
    }

    ERR_clear_error();

    std::unique_ptr<X509_STORE, decltype(X509_STORE_free)*> store{
        X509_STORE_new(), X509_STORE_free};

    if (!store)
    {
        ec = boost::system::error_code(
            static_cast<int>(::ERR_get_error()),
            boost::asio::error::get_ssl_category());
        return;
    }

    auto warn = [&](std::string const& mesg) {
//基于ASIO推荐大小的缓冲区
        char buf[256];
        ::ERR_error_string_n(ec.value(), buf, sizeof(buf));
        JLOG(j.warn()) << mesg << " " << buf;
        ::ERR_clear_error();
    };

    PCCERT_CONTEXT pContext = NULL;
    while ((pContext = CertEnumCertificatesInStore(hStore.get(), pContext)) !=
           NULL)
    {
        const unsigned char* pbCertEncoded = pContext->pbCertEncoded;
        std::unique_ptr<X509, decltype(X509_free)*> x509{
            d2i_X509(NULL, &pbCertEncoded, pContext->cbCertEncoded), X509_free};
        if (!x509)
        {
            warn("Error decoding certificate");
            continue;
        }

        if (X509_STORE_add_cert(store.get(), x509.get()) != 1)
        {
            warn("Error adding certificate");
        }
        else
        {
//成功添加到存储取得所有权
            x509.release();
        }
    }

//这占据了商店的所有权
    SSL_CTX_set_cert_store(ctx.native_handle(), store.release());

#else

    ctx.set_default_verify_paths(ec);
#endif
}

}  //命名空间波纹
