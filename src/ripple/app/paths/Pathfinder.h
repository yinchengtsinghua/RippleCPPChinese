
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

#ifndef RIPPLE_APP_PATHS_PATHFINDER_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHFINDER_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/core/LoadEvent.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/STPathSet.h>

namespace ripple {

/*计算付款路径。

    @ref ripplecalc确定找到的路径的质量。

    “见RippleCalc”
**/

class Pathfinder
{
public:
    /*构造没有颁发者的PathFinder*/
    Pathfinder (
        std::shared_ptr<RippleLineCache> const& cache,
        AccountID const& srcAccount,
        AccountID const& dstAccount,
        Currency const& uSrcCurrency,
        boost::optional<AccountID> const& uSrcIssuer,
        STAmount const& dstAmount,
        boost::optional<STAmount> const& srcAmount,
        Application& app);
    Pathfinder (Pathfinder const&) = delete;
    Pathfinder& operator= (Pathfinder const&) = delete;
    ~Pathfinder() = default;

    static void initPathTable ();

    bool findPaths (int searchLevel);

    /*计算路径的排名。*/
    void computePathRanks (int maxPaths);

    /*从mcompletePaths中获取最佳路径（最多为maxPaths个）。

       返回时，如果fullLiquidityPath不是空的，则它包含最好的
       额外的单一路径，可以消耗所有的流动性。
    **/

    STPathSet
    getBestPaths (
        int maxPaths,
        STPath& fullLiquidityPath,
        STPathSet const& extraPaths,
        AccountID const& srcIssuer);

    enum NodeType
    {
nt_SOURCE,     //源帐户：如果需要，使用发行者帐户。
nt_ACCOUNTS,   //从此源/货币连接的帐户。
nt_BOOKS,      //订购与此货币关联的帐簿。
nt_XRP_BOOK,   //从该货币到XRP的订单簿。
nt_DEST_BOOK,  //目的地货币/发行人的订单簿。
nt_DESTINATION //仅限目标帐户。
    };

//路径类型是路径的节点类型列表。
    using PathType = std::vector <NodeType>;

//PaymentType表示源货币和目标货币的类型
//在路径请求中。
    enum PaymentType
    {
        pt_XRP_to_XRP,
        pt_XRP_to_nonXRP,
        pt_nonXRP_to_XRP,
pt_nonXRP_to_same,   //目标货币与源货币相同。
pt_nonXRP_to_nonXRP  //目标货币与源货币不同。
    };

    struct PathRank
    {
        std::uint64_t quality;
        std::uint64_t length;
        STAmount liquidity;
        int index;
    };

private:
    /*
      探路者方法的调用图。

      findPaths：
          添加路径类型：
              附加链接：
                  AddLink：
                      获得路径
                      原始问题
                      伊斯诺普普拉特：
                          伊斯诺普普尔

      计算机路径等级：
          开褶皱的
          获取路径流动性：
              开褶皱的

      获得最佳路径
     **/



//将一种类型的所有路径添加到mcompletePaths。
    STPathSet& addPathsForType (PathType const& type);

    bool issueMatchesOrigin (Issue const&);

    int getPathsOut (
        Currency const& currency,
        AccountID const& account,
        bool isDestCurrency,
        AccountID const& dest);

    void addLink (
        STPath const& currentPath,
        STPathSet& incompletePaths,
        int addFlags);

//为当前路径中的每个路径调用addlink（）。
    void addLinks (
        STPathSet const& currentPaths,
        STPathSet& incompletePaths,
        int addFlags);

//计算路径的流动性。如果有足够的钱，返回Tesuccess
//流动性是值得保留的，否则就是一个错误。
    TER getPathLiquidity (
STPath const& path,            //在：要检查的路径。
STAmount const& minDstAmount,  //in:此路径必须的最小输出
//交付是值得保留的。
STAmount& amountOut,           //流出：实际流动性的路径。
uint64_t& qualityOut) const;   //输出：返回的初始质量

//此路径是否以最后一个帐户具有的“帐户到帐户”链接结束？
//在链接上设置“无波纹”标志？
    bool isNoRippleOut (STPath const& currentPath);

//从一个帐户到另一个帐户是否设置了“无波纹”标志？
    bool isNoRipple (
        AccountID const& fromAccount,
        AccountID const& toAccount,
        Currency const& currency);

    void rankPaths (
        int maxPaths,
        STPathSet const& paths,
        std::vector <PathRank>& rankedPaths);

    AccountID mSrcAccount;
    AccountID mDstAccount;
AccountID mEffectiveDst; //路径需要结束的帐户
    STAmount mDstAmount;
    Currency mSrcCurrency;
    boost::optional<AccountID> mSrcIssuer;
    STAmount mSrcAmount;
    /*在违约流动性
        被移除。*/

    STAmount mRemainingAmount;
    bool convert_all_;

    std::shared_ptr <ReadView const> mLedger;
    std::unique_ptr<LoadEvent> m_loadEvent;
    std::shared_ptr<RippleLineCache> mRLCache;

    STPathElement mSource;
    STPathSet mCompletePaths;
    std::vector<PathRank> mPathRanks;
    std::map<PathType, STPathSet> mPaths;

    hash_map<Issue, int> mPathsOutCountMap;

    Application& app_;
    beast::Journal j_;

//添加波纹路径
    static std::uint32_t const afADD_ACCOUNTS = 0x001;

//增加订单图书
    static std::uint32_t const afADD_BOOKS = 0x002;

//仅向XRP添加订单簿
    static std::uint32_t const afOB_XRP = 0x010;

//必须链接到目标货币
    static std::uint32_t const afOB_LAST = 0x040;

//仅目标帐户
    static std::uint32_t const afAC_LAST = 0x080;
};

} //涟漪

#endif
