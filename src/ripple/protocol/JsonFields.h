
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

#ifndef RIPPLE_PROTOCOL_JSONFIELDS_H_INCLUDED
#define RIPPLE_PROTOCOL_JSONFIELDS_H_INCLUDED

#include <ripple/json/json_value.h>

namespace ripple {
namespace jss {

//JSON静态字符串

#define JSS(x) constexpr ::Json::StaticString x ( #x )

/*使用“staticString”字段名而不是字符串文本
   优化访问json:：value对象成员的性能。
**/


JSS ( AL_hit_rate );                //退出：获得总数
JSS ( Account );                    //在：transactionsign；字段中。
JSS ( Amount );                     //在：transactionsign；字段中。
JSS ( ClearFlag );                  //字段。
JSS ( Destination );                //在：transactionsign；字段中。
JSS ( DeliverMin );                 //在：交易签名
JSS ( Fee );                        //输入/输出：事务信号；字段。
JSS ( Flags );                      //输入/输出：事务信号；字段。
JSS ( Invalid );                    //
JSS ( LastLedgerSequence );         //在：transactionsign；字段中
JSS ( LimitAmount );                //字段。
JSS ( OfferSequence );              //字段。
JSS ( Paths );                      //进出：交易信号
JSS ( TransferRate );               //In:传输速率
JSS ( historical_perminute );       //历史记录
JSS ( SLE_hit_rate );               //退出：获得总数
JSS ( SettleDelay );                //在：交易签名
JSS ( SendMax );                    //在：交易签名
JSS ( Sequence );                   //输入/输出：事务信号；字段。
JSS ( SetFlag );                    //字段。
JSS ( SigningPubKey );              //领域
JSS ( TakerGets );                  //字段。
JSS ( TakerPays );                  //字段。
JSS ( TxnSignature );               //领域
JSS ( TransactionType );            //在：交易签名
JSS ( aborted );                    //输出：内部分类帐
JSS ( accepted );                   //输出：Ledgertojson，OwnerInfo
JSS ( account );                    //内/外：许多
JSS ( accountState );               //输出：Ledgertojson
JSS ( accountTreeHash );            //出：分类帐/ledger.cpp
JSS ( account_data );               //输出：accountinfo
JSS ( account_hash );               //输出：Ledgertojson
JSS ( account_id );                 //输出：WalletPropose
JSS ( account_objects );            //输出：accountObjects
JSS ( account_root );               //在：
JSS ( accounts );                   //在：分类帐，订阅，
//
JSS ( accounts_proposed );          //在：订阅，取消订阅
JSS ( action );
JSS ( acquiring );                  //输出：LedgerRequest
JSS ( address );                    //走出：皮尔滕
JSS ( affected );                   //输出：接受的分类账Rtx
JSS ( age );                        //输出：网络操作、对等
JSS ( alternatives );               //输出：路径请求，Ripplepathfind
JSS ( amendment_blocked );          //走出网络
JSS ( amendments );                 //输入：accountobjects，输出：networkops
JSS ( amount );                     //输出：accountchannels
JSS ( asks );                       //输出：订阅
JSS ( assets );                     //输出：网关余额
JSS ( authorized );                 //出：账户行
JSS ( auth_change );                //输出：accountinfo
JSS ( auth_change_queued );         //输出：accountinfo
JSS ( available );                  //出：验证列表
JSS ( balance );                    //出：账户行
JSS ( balances );                   //输出：网关余额
JSS ( base );                       //输出：日志级别
JSS ( base_fee );                   //走出网络
JSS ( base_fee_xrp );               //走出网络
JSS ( bids );                       //输出：订阅
JSS ( binary );                     //在：accounttx，ledgerntry，
//帐户txold，tx LedgerData
JSS ( books );                      //在：订阅，取消订阅
JSS ( both );                       //在：订阅，取消订阅
JSS ( both_sides );                 //在：订阅，取消订阅
JSS ( build_path );                 //在：交易签名
JSS ( build_version );              //走出网络
JSS ( cancel_after );               //输出：accountchannels
JSS ( can_delete );                 //外：烛台
JSS ( channel_id );                 //输出：accountchannels
JSS ( channels );                   //输出：accountchannels
JSS ( check );                      //在：AccountObjects中
JSS ( check_nodes );                //在：窗台清洁器
JSS ( clear );                      //输入/输出：fetchinfo
JSS ( close_flags );                //输出：Ledgertojson
JSS ( close_time );                 //输入：应用程序，输出：网络操作，
//RlcxPeerpos，Ledgertojson公司
JSS ( close_time_estimated );       //输入：应用程序，输出：Ledgertojson
JSS ( close_time_human );           //输出：Ledgertojson
JSS ( close_time_offset );          //走出网络
JSS ( close_time_resolution );      //输入：应用程序；输出：LedgertoJSON
JSS ( closed );                     //输出：网络操作，Ledgertojson，
//处理程序/ Ledger
JSS ( closed_ledger );              //走出网络
JSS ( cluster );                    //走出：皮尔滕
JSS ( code );                       //错误：错误
JSS ( command );                    //在RPCHandler
JSS ( complete );                   //输出：网络操作，内部分类帐
JSS ( complete_ledgers );           //输出：网络操作，PeerImp
JSS ( complete_shards );            //输出：overlasimpl，peerimp
JSS ( consensus );                  //输出：网络操作、分类帐连接
JSS ( converge_time );              //走出网络
JSS ( converge_time_s );            //走出网络
JSS ( count );                      //在：accounttx*中，验证程序列表
JSS ( counters );                   //输入/输出：检索计数器
JSS ( currency );                   //在：路径/路径请求，stamount
//out:路径/节点，stpathset，stamount，
//会计科目
JSS ( current );                    //输出：所有者信息
JSS ( current_activities );
JSS ( current_ledger_size );        //输出：TXQ
JSS ( current_queue_size );         //输出：TXQ
JSS ( data );                       //莱德数据
JSS ( date );                       //输出：Tx/事务，网络操作
JSS ( dbKBLedger );                 //退出：获得总数
JSS ( dbKBTotal );                  //退出：获得总数
JSS ( dbKBTransaction );            //退出：获得总数
JSS ( debug_signing );              //在：交易签名
JSS ( delivered_amount );           //输出：AddPaymentDeliveredAmount
JSS ( deposit_authorized );         //出：授权存款
JSS ( deposit_preauth );            //在：AccountObjects，LedgerData中
JSS ( deprecated );                 //外面的
JSS ( descending );                 //会计：
JSS ( destination_account );        //在：pathrequest、ripplepathfind、account\u lines中
//输出：accountchannels
JSS ( destination_amount );         //在：PathRequest、Ripplepathfind中
JSS ( destination_currencies );     //在：PathRequest、Ripplepathfind中
JSS ( destination_tag );            //在：PaTrESQuestQuest:
//输出：accountchannels
JSS ( dir_entry );                  //输出：DirectoryEntryIterator
JSS ( dir_index );                  //输出：DirectoryEntryIterator
JSS ( dir_root );                   //输出：DirectoryEntryIterator
JSS ( directory );                  //在：
JSS ( drops );                      //输出：TXQ
JSS ( duration_us );                //走出网络
JSS ( enabled );                    //出：修正案
JSS ( engine_result );              //输出：网络操作、事务发送、提交
JSS ( engine_result_code );         //输出：网络操作、事务发送、提交
JSS ( engine_result_message );      //输出：网络操作、事务发送、提交
JSS ( error );                      //错误：错误
JSS ( errored );
JSS ( error_code );                 //错误：错误
JSS ( error_exception );            //出：提交
JSS ( error_message );              //错误：错误
JSS ( escrow );                     //在：
JSS ( expand );                     //在：处理程序/分类帐中
JSS ( expected_ledger_size );       //输出：TXQ
JSS ( expiration );                 //输出：accountoffers，accountchannels，
//验证列表
JSS ( fail_hard );                  //在：签名，提交
JSS ( failed );                     //输出：内部分类帐
JSS ( feature );                    //特点：
JSS ( features );                   //输出：特征
JSS ( fee );                        //输出：网络操作、对等
JSS ( fee_base );                   //走出网络
JSS ( fee_div_max );                //在：交易签名
JSS ( fee_level );                  //输出：accountinfo
JSS ( fee_mult_max );               //在：交易签名
JSS ( fee_ref );                    //走出网络
JSS ( fetch_pack );                 //走出网络
JSS ( first );                      //输出：rpc/版本
JSS ( finished );
JSS ( fix_txns );                   //在：窗台清洁器
JSS ( flags );                      //out:路径/节点，accountoffers，
//网络操作系统
JSS ( forward );                    //在：会计
JSS ( freeze );                     //出：账户行
JSS ( freeze_peer );                //出：账户行
JSS ( frozen_balances );            //输出：网关余额
JSS ( full );                       //在：分类帐接收器、处理程序/分类帐中
JSS ( full_reply );                 //出局：PathFind
JSS ( fullbelow_size );             //在：计数
JSS ( generator );                  //在：
JSS ( good );                       //输出：RPCED
JSS ( hash );                       //输出：网络操作，内部分类帐，
//Ledgertojson，STTX；字段
JSS ( hashes );                     //在：AccountObjects中
JSS ( have_header );                //输出：内部分类帐
JSS ( have_state );                 //输出：内部分类帐
JSS ( have_transactions );          //输出：内部分类帐
JSS ( highest_sequence );           //输出：accountinfo
JSS ( hostid );                     //走出网络
JSS ( hotwallet );                  //在：网关余额
JSS ( id );                         //WebSoCube。
JSS ( ident );                      //在：accountcurrences，accountinfo，
//所有者信息
JSS ( inLedger );                   //输出：Tx/事务
JSS ( inbound );                    //走出：皮尔滕
JSS ( index );                      //在：LedgerEntry，DownloadShard
//输出：pathstate，stledgerentry，
//LedgerEntry、TXHistory和LedgerData
//领域
JSS ( info );                       //输出：serverinfo、consenssinfo、fetchinfo
JSS ( internal_command );           //在内部
JSS ( io_latency_ms );              //走出网络
JSS ( ip );                         //输入：连接，输出：叠加Impl
JSS ( issuer );                     //在：Ripplepathfind，subscribe，
//退订，预订
//输出：路径/节点、stpathset、stamount
JSS ( job );
JSS ( job_queue );
JSS ( jobs );
JSS ( jsonrpc );                    //JSON版本
JSS ( jq_trans_overflow );          //JobQueue事务限制溢出。
JSS ( key );                        //外面的
JSS ( key_type );                   //进出：钱包提议、交易签名
JSS ( latency );                    //走出：皮尔滕
JSS ( last );                       //输出：RPCED
JSS ( last_close );                 //走出网络
JSS ( last_refresh_time );          //出：验证站点
JSS ( last_refresh_status );        //出：验证站点
JSS ( last_refresh_message );       //出：验证站点
JSS ( ledger );                     //在：网络操作，分类帐清理，
//刺猬
//输出：网络操作，PeerImp
JSS ( ledger_current_index );       //输出：网络操作、rpchelpers，
//Ledgercurrent、LedgerAccept和
//会计科目
JSS ( ledger_data );                //出口：壁架
JSS ( ledger_hash );                //在：Rpchelpers，LedgerRequest，
//Ripplepathfind，交易中心，
//处理程序/ Ledger
//输出：网络操作、rpchelpers，
//壁架闭合，壁架数据，
//会计科目
JSS ( ledger_hit_rate );            //退出：获得总数
JSS ( ledger_index );               //内/外：许多
JSS ( ledger_index_max );           //输入，输出：accounttx*
JSS ( ledger_index_min );           //输入，输出：accounttx*
JSS ( ledger_max );                 //输入，输出：accounttx*
JSS ( ledger_min );                 //输入，输出：accounttx*
JSS ( ledger_time );                //走出网络
JSS ( levels );                     //对数级
JSS ( limit );                      //输入/输出：accounttx*，accountoffers，
//账户行、账户对象
//在：LedgerData，BookOffers
JSS ( limit_peer );                 //出：账户行
JSS ( lines );                      //出：账户行
JSS ( list );                       //出：验证列表
JSS ( load );                       //输出：网络操作，PeerImp
JSS ( load_base );                  //走出网络
JSS ( load_factor );                //走出网络
JSS ( load_factor_cluster );        //走出网络
JSS ( load_factor_fee_escalation ); //走出网络
JSS ( load_factor_fee_queue );      //走出网络
JSS ( load_factor_fee_reference );  //走出网络
JSS ( load_factor_local );          //走出网络
JSS ( load_factor_net );            //走出网络
JSS ( load_factor_server );         //走出网络
JSS ( load_fee );                   //输出：LoadFeetrackImp、NetworkOps
JSS ( local );                      //输出：资源/逻辑.h
JSS ( local_txs );                  //退出：获得总数
JSS ( local_static_keys );          //出：验证列表
JSS ( lowest_sequence );            //输出：accountinfo
JSS ( majority );                   //输出：RPC功能
JSS ( marker );                     //输入/输出：accounttx，accountoffers，
//账户行、账户对象，
//莱格特数据
//在：提供图书
JSS ( master_key );                 //输出：WalletPropose
JSS ( master_seed );                //输出：WalletPropose
JSS ( master_seed_hex );            //输出：WalletPropose
JSS ( master_signature );           //输出：发布清单
JSS ( max_ledger );                 //输入/输出：窗台清洁器
JSS ( max_queue_size );             //输出：TXQ
JSS ( max_spend_drops );            //输出：accountinfo
JSS ( max_spend_drops_total );      //输出：accountinfo
JSS ( median_fee );                 //输出：TXQ
JSS ( median_level );               //输出：TXQ
JSS ( message );                    //错误。
JSS ( meta );                       //输出：networkops，accounttx*，tx
JSS ( metaData );
JSS ( metadata );                   //输出：事务管理
JSS ( method );                     //RPC
JSS ( methods );
JSS ( min_count );                  //在：计数
JSS ( min_ledger );                 //在：窗台清洁器
JSS ( minimum_fee );                //输出：TXQ
JSS ( minimum_level );              //输出：TXQ
JSS ( missingCommand );             //错误
JSS ( name );                       //输出：修正表impl，peerimp
JSS ( needed_state_hashes );        //输出：内部分类帐
JSS ( needed_transaction_hashes );  //输出：内部分类帐
JSS ( network_ledger );             //走出网络
JSS ( next_refresh_time );          //出：验证站点
JSS ( no_ripple );                  //出：账户行
JSS ( no_ripple_peer );             //出：账户行
JSS ( node );                       //输出：LedgerEntry
JSS ( node_binary );                //输出：LedgerEntry
JSS ( node_hit_rate );              //退出：获得总数
JSS ( node_read_bytes );            //退出：获得总数
JSS ( node_reads_hit );             //退出：获得总数
JSS ( node_reads_total );           //退出：获得总数
JSS ( node_writes );                //退出：获得总数
JSS ( node_written_bytes );         //退出：获得总数
JSS ( nodes );                      //走出：病态
JSS ( obligations );                //输出：网关余额
JSS ( offer );                      //在：
JSS ( offers );                     //输出：网络操作、帐户提供、订阅
JSS ( offline );                    //在：交易签名
JSS ( offset );                     //输入/输出：accounttxold
JSS ( open );                       //输出：处理程序/分类帐
JSS ( open_ledger_fee );            //输出：TXQ
JSS ( open_ledger_level );          //输出：TXQ
JSS ( owner );                      //输入：LedgerEntry，输出：NetworkOps
JSS ( owner_funds );                //输入/输出：分类帐、网络操作、接受的分类帐RTX
JSS ( params );                     //RPC
JSS ( parent_close_time );          //输出：Ledgertojson
JSS ( parent_hash );                //输出：Ledgertojson
JSS ( partition );                  //在：日志级别
JSS ( passphrase );                 //在：WalletPropose
JSS ( password );                   //订阅：
JSS ( paths );                      //在：Ripplepathfind
JSS ( paths_canonical );            //输出：Ripplepathfind
JSS ( paths_computed );             //输出：路径请求，Ripplepathfind
JSS ( payment_channel );            //在：
JSS ( peer );                       //在：账户行
JSS ( peer_authorized );            //出：账户行
JSS ( peer_id );                    //输出：rclcxpeerpos
JSS ( peers );                      //输出：InboundLedger、处理程序/对等机、覆盖
JSS ( peer_disconnects );           //切断的对等连接计数器。
JSS ( peer_disconnects_resources ); //由于以下原因切断了对等连接
//过度的资源消耗。
JSS ( port );                       //在：连接
JSS ( previous_ledger );            //输出：凸台
JSS ( proof );                      //在：提供图书
JSS ( propose_seq );                //输出：凸台
JSS ( proposers );                  //输出：网络操作、分类帐连接
JSS ( protocol );                   //走出：皮尔滕
JSS ( pubkey_node );                //走出网络
JSS ( pubkey_publisher );           //出：验证列表
JSS ( pubkey_validator );           //输出：网络操作，验证程序列表
JSS ( public_key );                 //输出：overlayimpl、peerimp、walletpropose
JSS ( public_key_hex );             //输出：WalletPropose
JSS ( published_ledger );           //走出网络
JSS ( publisher_lists );            //出：验证列表
JSS ( quality );                    //走出网络
JSS ( quality_in );                 //出：账户行
JSS ( quality_out );                //出：账户行
JSS ( queue );                      //会计信息
JSS ( queue_data );                 //输出：accountinfo
JSS ( queued );
JSS ( queued_duration_us );
JSS ( random );                     //随机：
JSS ( raw_meta );                   //输出：接受的分类账Rtx
JSS ( receive_currencies );         //输出：会计货币
JSS ( reference_level );            //输出：TXQ
JSS ( refresh_interval_min );       //出：验证网站
JSS ( regular_seed );               //进出：分类帐
JSS ( remote );                     //逻辑：H
JSS ( request );                    //RPC
JSS ( reserve_base );               //走出网络
JSS ( reserve_base_xrp );           //走出网络
JSS ( reserve_inc );                //走出网络
JSS ( reserve_inc_xrp );            //走出网络
JSS ( response );                   //网络套接字
JSS ( result );                     //RPC
JSS ( ripple_lines );               //走出网络
JSS ( ripple_state );               //在LedgerEntr
JSS ( ripplerpc );                  //Ripple RPC版本
JSS ( role );                       //出局：Ping.cpp
JSS ( rpc );
JSS ( rt_accounts );                //在：订阅，取消订阅
JSS ( running_duration_us );
JSS ( sanity );                     //走出：皮尔滕
JSS ( search_depth );               //在：Ripplepathfind
JSS ( secret );                     //在：交易签名，
//validationCreate、validationSeed和
//频道授权
JSS ( seed );                       //
JSS ( seed_hex );                   //在：WalletPropose，Transactionsign
JSS ( send_currencies );            //输出：会计货币
JSS ( send_max );                   //在：PathRequest、Ripplepathfind中
JSS ( seq );                        //在：分类帐；
//输出：网络运营、RPCSUB、AccountOffers，
//验证列表
JSS ( seqNum );                     //输出：Ledgertojson
JSS ( server_state );               //走出网络
JSS ( server_state_duration_us );   //走出网络
JSS ( server_status );              //走出网络
JSS ( settle_delay );               //输出：accountchannels
JSS ( severity );                   //在：日志级别
JSS ( shards );                     //输入/输出：getcounts，downloadshard
JSS ( signature );                  //输出：网络操作、通道授权
JSS ( signature_verified );         //输出：通道验证
JSS ( signing_key );                //走出网络
JSS ( signing_keys );               //出：验证列表
JSS ( signing_time );               //走出网络
JSS ( signer_list );                //在：AccountObjects中
JSS ( signer_lists );               //输入/输出：accountinfo
JSS ( snapshot );                   //订阅：
JSS ( source_account );             //在：PathRequest、Ripplepathfind中
JSS ( source_amount );              //在：PathRequest、Ripplepathfind中
JSS ( source_currencies );          //在：PathRequest、Ripplepathfind中
JSS ( source_tag );                 //输出：accountchannels
JSS ( stand_alone );                //走出网络
JSS ( start );                      //在：TX历史
JSS ( started );
JSS ( state );                      //输出：logic.h、serverstate、ledgerdata
JSS ( state_accounting );           //走出网络
JSS ( state_now );                  //订阅：
JSS ( status );                     //错误
JSS ( stop );                       //在：窗台清洁器
JSS ( streams );                    //在：订阅，取消订阅
JSS ( strict );                     //在：accountcurrences，accountinfo
JSS ( sub_index );                  //在：
JSS ( subcommand );                 //在：PathFind
JSS ( success );                    //RPC
JSS ( supported );                  //输出：修正表impl
JSS ( system_time_offset );         //走出网络
JSS ( tag );                        //外：同龄人
JSS ( taker );                      //在：订阅，预订
JSS ( taker_gets );                 //在：订阅、取消订阅、预订
JSS ( taker_gets_funded );          //走出网络
JSS ( taker_pays );                 //在：订阅、取消订阅、预订
JSS ( taker_pays_funded );          //走出网络
JSS ( threshold );                  //黑名单
JSS ( ticket );                     //在：AccountObjects中
JSS ( time );
JSS ( timeouts );                   //输出：内部分类帐
JSS ( traffic );                    //外：覆盖
JSS ( total );                      //柜台：柜台
JSS ( totalCoins );                 //输出：Ledgertojson
JSS ( total_coins );                //输出：Ledgertojson
JSS ( transTreeHash );              //出：分类帐/ledger.cpp
JSS ( transaction );                //在Tx
//输出：NetworkOps，AcceptedLedgerTx，
JSS ( transaction_hash );           //输出：rclcxpeerpos，ledgertojson
JSS ( transactions );               //输出：Ledgertojson，
//在：accounttx*中，取消订阅
JSS ( transitions );                //走出网络
JSS ( treenode_cache_size );        //退出：获得总数
JSS ( treenode_track_size );        //退出：获得总数
JSS ( trusted );                    //输出：UNLIST
JSS ( trusted_validator_keys );     //出：验证列表
JSS ( tx );                         //输出：sttx，accounttx*
JSS ( tx_blob );                    //输入/输出：提交，
//在：交易签名，accountTx*
JSS ( tx_hash );                    //在：交易中心
JSS ( tx_json );                    //进出：交易信号
//输出：事务管理
JSS ( tx_signing_hash );            //输出：交易信号
JSS ( tx_unsigned );                //输出：交易信号
JSS ( txn_count );                  //走出网络
JSS ( txs );                        //输出：TX历史
JSS ( type );                       //在：AccountObjects中
//走出网络
//路径/node.cpp，覆盖impl，逻辑
JSS ( type_hex );                   //输出：STATSET
JSS ( unl );                        //输出：UNLIST
JSS ( unlimited);                   //输出：连接.h
JSS ( uptime );                     //退出：获得总数
JSS ( uri );                        //出：验证网站
JSS ( url );                        //输入/输出：订阅、取消订阅
JSS ( url_password );               //订阅：
JSS ( url_username );               //订阅：
JSS ( urlgravatar );                //
JSS ( username );                   //订阅：
JSS ( validate );                   //在：下载shard
JSS ( validated );                  //输出：NetworkOps、Rpchelpers、AccountTx*
//德克萨斯州
JSS ( validator_list_expires );     //输出：网络操作，验证程序列表
JSS ( validator_list );             //输出：网络操作，验证程序列表
JSS ( validators );
JSS ( validated_ledger );           //走出网络
JSS ( validated_ledgers );          //走出网络
JSS ( validation_key );             //输出：validationcreate，validationseed
JSS ( validation_private_key );     //输出：验证创建
JSS ( validation_public_key );      //输出：validationcreate，validationseed
JSS ( validation_quorum );          //走出网络
JSS ( validation_seed );            //输出：validationcreate，validationseed
JSS ( validations );                //输出：修正表impl
JSS ( validator_sites );            //出：验证网站
JSS ( value );                      //出：
JSS ( version );                    //输出：RPCED
JSS ( vetoed );                     //输出：修正表impl
JSS ( vote );                       //特点：
JSS ( warning );                    //RPC:
JSS ( workers );
JSS ( write_load );                 //退出：获得总数

#undef JSS

} //JSS
} //涟漪

#endif
