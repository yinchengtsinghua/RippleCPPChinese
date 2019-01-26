
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

#include <ripple/app/tx/impl/PayChan.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/PayChan.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/XRPAmount.h>

namespace ripple {

/*
    支付渠道

        付款渠道允许XRP付款流的账外检查点
        在一个方向上。频道将所有者的xrp单独隔离
        分类账项。所有者可以授权接收者在
        通过给接收者一条已签名的消息（分类账外）给出余额。这个
        收件人可以使用此签名邮件在
        通道保持打开状态。所有者可以根据需要结束该行。如果
        该频道尚未付清所有资金，所有者必须等待
        延迟关闭频道以给收件人提供任何
        声称。收件人可以随时关闭频道。任何交易
        在到期时间后触摸频道将关闭
        通道。支付的总金额随着新索赔的增加而单调增加。
        发行。当通道关闭时，返回任何剩余余额
        给主人。通道用于允许间断的分类账外
        随着余额的增加，ILP信托额度的结算。为了
        双向通道，每个方向都可以使用一个支付通道。

    付款渠道创建

        创建单向通道。参数为：
        目的地
            频道末尾的收件人。
        数量
            立即存入通道的XRP数量。
        沉降延迟
            每个人（收件人除外）必须等待的时间量
            上级索赔。
        公钥
            将签署针对频道的声明的密钥。
        取消后（可选）
            在
            `cancelafter`时间将关闭它。
        destinationtag（可选）
            目标标记允许宿主中的不同帐户
            钱包将被映射回Ripple分类账。目的地标签
            告诉服务器托管钱包中的资金流向
            打算去。如果目标具有lsfrequeuiredesttag，则为必需
            集合。
        sourcetag（可选）
            源标记允许托管钱包内的不同帐户
            将被映射回Ripple分类账。源标记类似于
            目的地标签，但供频道所有者识别自己的
            交易。

    支付渠道基金

        向支付渠道添加额外资金。只有频道所有者可以
        使用此事务。参数为：
        通道
            通道的256位ID。
        数量
            要添加的XRP数量。
        过期（可选）
            频道关闭的时间。如果过期，事务将失败
            时间不满足SettleDelay约束。

    付款渠道索赔

        对现有渠道提出索赔。参数为：
        通道
            通道的256位ID。
        平衡（可选）
            处理此索赔后交付的XRP总量（可选，非
            如果只是关闭，则需要）。
        金额（可选）
            签名的XRP金额（如果等于余额或仅为余额，则不需要
            关闭生产线）。
        签名（可选）
            上述余额的授权，由业主签字（可选，
            如果关闭或所有者正在执行事务，则不需要。这个
            以下消息的签名if:clm\0后跟
            256位通道ID和64位整数下降。
        公钥（可选）
           生成签名的公钥（可选，如果签名是必需的
           存在）
        旗帜
            TFFSET
                请求关闭通道
            特弗雷涅
                请求重置通道的过期。只有业主
                可以续订频道。

**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static
TER
closeChannel (
    std::shared_ptr<SLE> const& slep,
    ApplyView& view,
    uint256 const& key,
    beast::Journal j)
{
    AccountID const src = (*slep)[sfAccount];
//从所有者目录中删除paychan
    {
        auto const page = (*slep)[sfOwnerNode];
        if (! view.dirRemove(keylet::ownerDir(src), page, key, true))
        {
            return tefBAD_LEDGER;
        }
    }

//将金额转移回所有者，减少所有者计数
    auto const sle = view.peek (keylet::account (src));
    assert ((*slep)[sfAmount] >= (*slep)[sfBalance]);
    (*sle)[sfBalance] =
        (*sle)[sfBalance] + (*slep)[sfAmount] - (*slep)[sfBalance];
    adjustOwnerCount(view, sle, -1, j);
    view.update (sle);

//从分类帐中删除paychan
    view.erase (slep);
    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

NotTEC
PayChanCreate::preflight (PreflightContext const& ctx)
{
    if (!ctx.rules.enabled (featurePayChan))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (!isXRP (ctx.tx[sfAmount]) || (ctx.tx[sfAmount] <= beast::zero))
        return temBAD_AMOUNT;

    if (ctx.tx[sfAccount] == ctx.tx[sfDestination])
        return temDST_IS_SRC;

    if (!publicKeyType(ctx.tx[sfPublicKey]))
        return temMALFORMED;

    return preflight2 (ctx);
}

TER
PayChanCreate::preclaim(PreclaimContext const &ctx)
{
    auto const account = ctx.tx[sfAccount];
    auto const sle = ctx.view.read (keylet::account (account));

//检查准备金和资金可用性
    {
        auto const balance = (*sle)[sfBalance];
        auto const reserve =
                ctx.view.fees ().accountReserve ((*sle)[sfOwnerCount] + 1);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + ctx.tx[sfAmount])
            return tecUNFUNDED;
    }

    auto const dst = ctx.tx[sfDestination];

    {
//检查目标帐户
        auto const sled = ctx.view.read (keylet::account (dst));
        if (!sled)
            return tecNO_DST;
        if (((*sled)[sfFlags] & lsfRequireDestTag) &&
            !ctx.tx[~sfDestinationTag])
            return tecDST_TAG_NEEDED;

//遵守lsfdisallowxrp标志是一个错误。背上
//FeatureDepositauth删除错误。
        if (!ctx.view.rules().enabled(featureDepositAuth) &&
            ((*sled)[sfFlags] & lsfDisallowXRP))
            return tecNO_TARGET;
    }

    return tesSUCCESS;
}

TER
PayChanCreate::doApply()
{
    auto const account = ctx_.tx[sfAccount];
    auto const sle = ctx_.view ().peek (keylet::account (account));
    auto const dst = ctx_.tx[sfDestination];

//在分类帐中创建paychan
    auto const slep = std::make_shared<SLE> (
        keylet::payChan (account, dst, (*sle)[sfSequence] - 1));
//本渠道持有资金
    (*slep)[sfAmount] = ctx_.tx[sfAmount];
//频道已支付的金额
    (*slep)[sfBalance] = ctx_.tx[sfAmount].zeroed ();
    (*slep)[sfAccount] = account;
    (*slep)[sfDestination] = dst;
    (*slep)[sfSettleDelay] = ctx_.tx[sfSettleDelay];
    (*slep)[sfPublicKey] = ctx_.tx[sfPublicKey];
    (*slep)[~sfCancelAfter] = ctx_.tx[~sfCancelAfter];
    (*slep)[~sfSourceTag] = ctx_.tx[~sfSourceTag];
    (*slep)[~sfDestinationTag] = ctx_.tx[~sfDestinationTag];

    ctx_.view ().insert (slep);

//将paychan添加到所有者目录
    {
        auto page = dirAdd (ctx_.view(), keylet::ownerDir(account), slep->key(),
            false, describeOwnerDir (account), ctx_.app.journal ("View"));
        if (!page)
            return tecDIR_FULL;
        (*slep)[sfOwnerNode] = *page;
    }

//扣除所有者余额，增加所有者计数
    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    adjustOwnerCount(ctx_.view(), sle, 1, ctx_.journal);
    ctx_.view ().update (sle);

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

NotTEC
PayChanFund::preflight (PreflightContext const& ctx)
{
    if (!ctx.rules.enabled (featurePayChan))
        return temDISABLED;

    if (ctx.rules.enabled(fix1543) && ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (!isXRP (ctx.tx[sfAmount]) || (ctx.tx[sfAmount] <= beast::zero))
        return temBAD_AMOUNT;

    return preflight2 (ctx);
}

TER
PayChanFund::doApply()
{
    Keylet const k (ltPAYCHAN, ctx_.tx[sfPayChannel]);
    auto const slep = ctx_.view ().peek (k);
    if (!slep)
        return tecNO_ENTRY;

    AccountID const src = (*slep)[sfAccount];
    auto const txAccount = ctx_.tx[sfAccount];
    auto const expiration = (*slep)[~sfExpiration];

    {
        auto const cancelAfter = (*slep)[~sfCancelAfter];
        auto const closeTime =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count ();
        if ((cancelAfter && closeTime >= *cancelAfter) ||
            (expiration && closeTime >= *expiration))
            return closeChannel (
                slep, ctx_.view (), k.key, ctx_.app.journal ("View"));
    }

    if (src != txAccount)
//只有所有者才能增加资金或延长
        return tecNO_PERMISSION;

    if (auto extend = ctx_.tx[~sfExpiration])
    {
        auto minExpiration =
            ctx_.view ().info ().parentCloseTime.time_since_epoch ().count () +
                (*slep)[sfSettleDelay];
        if (expiration && *expiration < minExpiration)
            minExpiration = *expiration;

        if (*extend < minExpiration)
            return temBAD_EXPIRATION;
        (*slep)[~sfExpiration] = *extend;
        ctx_.view ().update (slep);
    }

    auto const sle = ctx_.view ().peek (keylet::account (txAccount));

    {
//检查准备金和资金可用性
        auto const balance = (*sle)[sfBalance];
        auto const reserve =
            ctx_.view ().fees ().accountReserve ((*sle)[sfOwnerCount]);

        if (balance < reserve)
            return tecINSUFFICIENT_RESERVE;

        if (balance < reserve + ctx_.tx[sfAmount])
            return tecUNFUNDED;
    }

    (*slep)[sfAmount] = (*slep)[sfAmount] + ctx_.tx[sfAmount];
    ctx_.view ().update (slep);

    (*sle)[sfBalance] = (*sle)[sfBalance] - ctx_.tx[sfAmount];
    ctx_.view ().update (sle);

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

NotTEC
PayChanClaim::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featurePayChan))
        return temDISABLED;

//数据团队对历史性的Mainnet分类账的搜索没有发现
//出现FIX1512修复的错误的事务。那
//意味着没有可能出现这种错误的旧事务
//需要重播。因此，移除了对fix1512的检查。APR 2018。
//bool const notecs=ctx.rules.enabled（fix1512）；

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto const bal = ctx.tx[~sfBalance];
    if (bal && (!isXRP (*bal) || *bal <= beast::zero))
        return temBAD_AMOUNT;

    auto const amt = ctx.tx[~sfAmount];
    if (amt && (!isXRP (*amt) || *amt <= beast::zero))
        return temBAD_AMOUNT;

    if (bal && amt && *bal > *amt)
        return temBAD_AMOUNT;

    {
        auto const flags = ctx.tx.getFlags();

        if (ctx.rules.enabled(fix1543) && (flags & tfPayChanClaimMask))
            return temINVALID_FLAG;

        if ((flags & tfClose) && (flags & tfRenew))
            return temMALFORMED;
    }

    if (auto const sig = ctx.tx[~sfSignature])
    {
        if (!(ctx.tx[~sfPublicKey] && bal))
            return temMALFORMED;

//检查签名
//如果txaccount==src，则不需要签名，但如果
//在场，检查一下

        auto const reqBalance = bal->xrp ();
        auto const authAmt = amt ? amt->xrp() : reqBalance;

        if (reqBalance > authAmt)
            return temBAD_AMOUNT;

        Keylet const k (ltPAYCHAN, ctx.tx[sfPayChannel]);
        if (!publicKeyType(ctx.tx[sfPublicKey]))
            return temMALFORMED;
        PublicKey const pk (ctx.tx[sfPublicKey]);
        Serializer msg;
        serializePayChanAuthorization (msg, k.key, authAmt);
        /*（！验证（pk，msg.slice（），*sig，/*canonical*/true））
            返回Tembad_签名；
    }

    返回预照明2（CTX）；
}

特尔
paychanClaim:：doApply（）。
{
    keylet const k（ltpaychan，ctx_u.tx[sfpaychannel]）；
    auto const slep=ctx_u.view（）.peek（k）；
    如果（！）枕木）
        返回Tecno_目标；

    accountID const src=（*slep）[sfaccount]；
    accountID const dst=（*slep）[sfdestination]；
    accountID const txaccount=ctx_.tx[sfaccount]；

    auto const curexpiration=（*slep）[~sfexpiration]；
    {
        auto const cancelafter=（*slep）[~sfcancelafter]；
        自动常数关闭时间=
            ctx_uuview（）.info（）.parentclosetime.time_epoch（）.count（）；
        if（（cancelafter和closetime>=*cancelafter）
            （curexpiration和closetime>=*curexpiration））
            返回CloseChannel（
                slep，ctx_u.view（），k.key，ctx_u.app.journal（“view”）；
    }

    如果（TXACK）！=SRC和TXACCOUNT！= DST）
        归还技术许可；

    如果（ctx_uux[~sfbalance]）
    {
        auto const chanbalance=slep->getfieldamount（sfbalance）.xrp（）；
        auto const chanfunds=slep->getfieldamount（sfamount）.xrp（）；
        auto const reqbalance=ctx_.tx[sfbalance].xrp（）；

        如果（txaccount==dst&&！ctx_uux[~sfsignature]）
            返回Tembad_签名；

        if（ctx_u.tx[~sfsignature]）
        {
            publickey const pk（（*slep）[sfpublickey]）；
            如果（ctx_uux[sfpublickey]！= PK）
                返回Tembad_签名者；
        }

        如果（reqbalance>chanfunds）
            归还Tecunfunded_款项；

        如果（reqbalance<=chanbalance）
            //未请求任何内容
            归还Tecunfunded_款项；

        auto const sled=ctx_u.view（）.peek（keylet:：account（dst））；
        如果（！）雪橇
            返回terno_帐户；

        //遵守lsfdisallowxrp标志是一个错误。背上
        //FeatureDepositauth删除错误。
        bool const depositauth ctx_.view（）.rules（）.enabled（featuredepositauth）
        如果（！）存款与证券
            （txaccount==src&&（sled->getflags（）&lsfdisallowxrp）））
            返回Tecno_目标；

        //检查目的账户是否需要存款授权。
        if（depositauth和（&（sled->getflags（）&lsfDepositauth））。
        {
            //需要授权的目标帐户有两个
            //获取付款渠道索赔的方法：
            / / 1。如果account==目的地，或
            / / 2。如果帐户是按目的地预先授权的存款。
            如果（TXACK）！= DST）
            {
                如果（！）view（）。存在（keylet:：depositpreadauth（dst，txaccount）））
                    归还技术许可；
            }
        }

        （*slep）[sfbalance]=ctx_u.tx[sfbalance]；
        xrpamount const reqdelta=reqbalance-chanbalance；
        断言（reqdelta>=beast：：零）；
        （*sled）[sfbalance]=（*sled）[sfbalance]+请求增量；
        CTX视图（）更新（雪橇）；
        CTX视图（）更新（SLEP）；
    }

    if（ctx_uux.getflags（）和tfrenew）
    {
        如果（SRC）！= txNACK）
            归还技术许可；
        （*slep）[~sfexpiration]=boost:：none；
        CTX视图（）更新（SLEP）；
    }

    if（ctx_uux.getflags（）和tfclose）
    {
        //如果干燥或接收器关闭，通道将立即关闭
        if（dst==txaccount（*slep）[sfbalance]=（*slep）[sfamount]）
            返回CloseChannel（
                slep，ctx_u.view（），k.key，ctx_u.app.journal（“view”）；

        自动常量结算到期=
            CTX视图（）。信息（）。parentCloseTime.time\自\u epoch（）。计数（）+
                （*slep）[结算延迟]；

        如果（！）curexpiration*curexpiration>结算到期）
        {
            （*slep）[~sfexpiration]=结算到期；
            CTX视图（）更新（SLEP）；
        }
    }

    返回Tesuccess；
}

} /纹波

