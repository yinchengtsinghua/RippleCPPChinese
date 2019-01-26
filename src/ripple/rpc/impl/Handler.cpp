
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

#include <ripple/rpc/impl/Handler.h>
#include <ripple/rpc/handlers/Handlers.h>
#include <ripple/rpc/handlers/Version.h>

namespace ripple {
namespace RPC {
namespace {

/*调整旧样式的处理程序以供引用调用。*/
template <typename Function>
Handler::Method<Json::Value> byRef (Function const& f)
{
    return [f] (Context& context, Json::Value& result)
    {
        result = f (context);
        if (result.type() != Json::objectValue)
        {
            assert (false);
            result = RPC::makeObjectValue (result);
        }

        return Status();
    };
}

template <class Object, class HandlerImpl>
Status handle (Context& context, Object& object)
{
    HandlerImpl handler (context);

    auto status = handler.check ();
    if (status)
        status.inject (object);
    else
        handler.writeResult (object);
    return status;
};

Handler const handlerArray[] {
//此处未指定的某些处理程序通过addhandler（）添加到表中。
//请求-响应方法
    {   "account_info",         byRef (&doAccountInfo),         Role::USER,  NO_CONDITION  },
    {   "account_currencies",   byRef (&doAccountCurrencies),   Role::USER,  NO_CONDITION  },
    {   "account_lines",        byRef (&doAccountLines),        Role::USER,  NO_CONDITION  },
    {   "account_channels",     byRef (&doAccountChannels),     Role::USER,  NO_CONDITION  },
    {   "account_objects",      byRef (&doAccountObjects),      Role::USER,  NO_CONDITION  },
    {   "account_offers",       byRef (&doAccountOffers),       Role::USER,  NO_CONDITION  },
    {   "account_tx",           byRef (&doAccountTxSwitch),     Role::USER,  NO_CONDITION  },
    {   "blacklist",            byRef (&doBlackList),           Role::ADMIN,   NO_CONDITION     },
    {   "book_offers",          byRef (&doBookOffers),          Role::USER,  NO_CONDITION  },
    {   "can_delete",           byRef (&doCanDelete),           Role::ADMIN,   NO_CONDITION     },
    {   "channel_authorize",    byRef (&doChannelAuthorize),    Role::USER,  NO_CONDITION  },
    {   "channel_verify",       byRef (&doChannelVerify),       Role::USER,  NO_CONDITION  },
    {   "connect",              byRef (&doConnect),             Role::ADMIN,   NO_CONDITION     },
    {   "consensus_info",       byRef (&doConsensusInfo),       Role::ADMIN,   NO_CONDITION     },
    {   "deposit_authorized",   byRef (&doDepositAuthorized),   Role::USER,  NO_CONDITION  },
    {   "download_shard",       byRef (&doDownloadShard),       Role::ADMIN,   NO_CONDITION     },
    {   "gateway_balances",     byRef (&doGatewayBalances),     Role::USER,  NO_CONDITION  },
    {   "get_counts",           byRef (&doGetCounts),           Role::ADMIN,   NO_CONDITION     },
    {   "feature",              byRef (&doFeature),             Role::ADMIN,   NO_CONDITION     },
    {   "fee",                  byRef (&doFee),                 Role::USER,    NEEDS_CURRENT_LEDGER     },
    {   "fetch_info",           byRef (&doFetchInfo),           Role::ADMIN,   NO_CONDITION     },
    {   "ledger_accept",        byRef (&doLedgerAccept),        Role::ADMIN,   NEEDS_CURRENT_LEDGER  },
    {   "ledger_cleaner",       byRef (&doLedgerCleaner),       Role::ADMIN,   NEEDS_NETWORK_CONNECTION  },
    {   "ledger_closed",        byRef (&doLedgerClosed),        Role::USER,  NO_CONDITION   },
    {   "ledger_current",       byRef (&doLedgerCurrent),       Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "ledger_data",          byRef (&doLedgerData),          Role::USER,  NO_CONDITION  },
    {   "ledger_entry",         byRef (&doLedgerEntry),         Role::USER,  NO_CONDITION  },
    {   "ledger_header",        byRef (&doLedgerHeader),        Role::USER,  NO_CONDITION  },
    {   "ledger_request",       byRef (&doLedgerRequest),       Role::ADMIN,   NO_CONDITION     },
    {   "log_level",            byRef (&doLogLevel),            Role::ADMIN,   NO_CONDITION     },
    {   "logrotate",            byRef (&doLogRotate),           Role::ADMIN,   NO_CONDITION     },
    {   "noripple_check",       byRef (&doNoRippleCheck),       Role::USER,  NO_CONDITION  },
    {   "owner_info",           byRef (&doOwnerInfo),           Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "peers",                byRef (&doPeers),               Role::ADMIN,   NO_CONDITION     },
    {   "path_find",            byRef (&doPathFind),            Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "ping",                 byRef (&doPing),                Role::USER,  NO_CONDITION     },
    {   "print",                byRef (&doPrint),               Role::ADMIN,   NO_CONDITION     },
//“profile”，byref（&doprofile），role：：user，needs_current_ledger，
    {   "random",               byRef (&doRandom),              Role::USER,  NO_CONDITION     },
    {   "ripple_path_find",     byRef (&doRipplePathFind),      Role::USER,  NO_CONDITION  },
    {   "sign",                 byRef (&doSign),                Role::USER,  NO_CONDITION     },
    {   "sign_for",             byRef (&doSignFor),             Role::USER,  NO_CONDITION     },
    {   "submit",               byRef (&doSubmit),              Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "submit_multisigned",   byRef (&doSubmitMultiSigned),   Role::USER,  NEEDS_CURRENT_LEDGER  },
    {   "server_info",          byRef (&doServerInfo),          Role::USER,  NO_CONDITION     },
    {   "server_state",         byRef (&doServerState),         Role::USER,  NO_CONDITION     },
    {   "crawl_shards",         byRef (&doCrawlShards),         Role::ADMIN,  NO_CONDITION     },
    {   "stop",                 byRef (&doStop),                Role::ADMIN,   NO_CONDITION     },
    {   "transaction_entry",    byRef (&doTransactionEntry),    Role::USER,  NO_CONDITION  },
    {   "tx",                   byRef (&doTx),                  Role::USER,  NEEDS_NETWORK_CONNECTION  },
    {   "tx_history",           byRef (&doTxHistory),           Role::USER,  NO_CONDITION     },
    {   "unl_list",             byRef (&doUnlList),             Role::ADMIN,   NO_CONDITION     },
    {   "validation_create",    byRef (&doValidationCreate),    Role::ADMIN,   NO_CONDITION     },
    {   "validators",           byRef (&doValidators),          Role::ADMIN,   NO_CONDITION     },
    {   "validator_list_sites", byRef (&doValidatorListSites),  Role::ADMIN,   NO_CONDITION     },
    {   "wallet_propose",       byRef (&doWalletPropose),       Role::ADMIN,   NO_CONDITION     },

//规避方法
    {   "subscribe",            byRef (&doSubscribe),           Role::USER,  NO_CONDITION     },
    {   "unsubscribe",          byRef (&doUnsubscribe),         Role::USER,  NO_CONDITION     },
};

class HandlerTable {
  private:
    template<std::size_t N>
    explicit
    HandlerTable (const Handler(&entries)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            auto const& entry = entries[i];
            assert (table_.find(entry.name_) == table_.end());
            table_[entry.name_] = entry;
        }

//这是添加新样式处理程序的地方。
        addHandler<LedgerHandler>();
        addHandler<VersionHandler>();
    }

  public:
    static HandlerTable const& instance()
    {
        static HandlerTable const handlerTable (handlerArray);
        return handlerTable;
    }

    Handler const* getHandler(std::string name) const
    {
        auto i = table_.find(name);
        return i == table_.end() ? nullptr : &i->second;
    }

    std::vector<char const*>
    getHandlerNames() const
    {
        std::vector<char const*> ret;
        ret.reserve(table_.size());
        for (auto const& i : table_)
            ret.push_back(i.second.name_);
        return ret;
    }

  private:
    std::map<std::string, Handler> table_;

    template <class HandlerImpl>
    void addHandler()
    {
        assert (table_.find(HandlerImpl::name()) == table_.end());

        Handler h;
        h.name_ = HandlerImpl::name();
        h.valueMethod_ = &handle<Json::Value, HandlerImpl>;
        h.role_ = HandlerImpl::role();
        h.condition_ = HandlerImpl::condition();

        table_[HandlerImpl::name()] = h;
    }
};

} //命名空间

Handler const* getHandler(std::string const& name)
{
    return HandlerTable::instance().getHandler(name);
}

std::vector<char const*>
getHandlerNames()
{
    return HandlerTable::instance().getHandlerNames();
};

} //RPC
} //涟漪
