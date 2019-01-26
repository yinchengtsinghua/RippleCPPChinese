
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
   xxhash-快速哈希算法
   头文件
   版权所有（c）2012-2014，Yann Collet。
   BSD 2条款许可证（http://www.opensource.org/licenses/bsd-license.php）

   以源和二进制形式重新分配和使用，有或无
   允许修改，前提是以下条件
   遇见：

       *源代码的再分配必须保留上述版权。
   注意，此条件列表和以下免责声明。
       *二进制形式的再分配必须复制上述内容
   版权声明、此条件列表和以下免责声明
   在提供的文件和/或其他材料中，
   分布。

   本软件由版权所有者和贡献者提供。
   “原样”和任何明示或暗示的保证，包括但不包括
   仅限于对适销性和适用性的暗示保证
   不承认特定目的。在任何情况下，版权
   所有人或出资人对任何直接、间接、附带的，
   特殊、惩戒性或后果性损害（包括但不包括
   仅限于采购替代货物或服务；使用损失，
   数据或利润；或业务中断），无论如何引起的
   责任理论，无论是合同责任、严格责任还是侵权责任。
   （包括疏忽或其他）因使用不当而引起的
   即使已告知此类损坏的可能性。

   您可以通过以下方式联系作者：
   -xxash源存储库：http://code.google.com/p/xxash/
**/


/*从XXhash主页提取的通知：

XXHASH是一种非常快速的哈希算法，以RAM速度限制运行。
它还成功地通过了smhasher套件中的所有测试。

比较（单线程，Windows 7个32位，在核心2 Duo@3GHz上使用smhasher）

姓名：speed q.score author
XXhash 5.4 GB/s 10
CRAPOW 3.2 GB/s 2安德鲁
Mumurhash 3A 2.7 GB/s 10奥斯汀Appleby
spokyhash 2.0 GB/s 10鲍勃·詹金斯
SBOX 1.4 GB/s 9布雷特·穆维
查找3 1.2 GB/s 9 Bob Jenkins
Superfasthash 1.2 GB/s 1 Paul Hsieh
CityHash64 1.05 GB/S 10 Pike和Alakuijala
FNv 0.55 GB/s 5福勒、诺尔、沃
CRC32 0.43 GB/s 9
MD5-32 0.33 GB/s 10罗纳德L.里维斯
sha1-32 0.28 GB/s 10个

q.得分是散列函数质量的度量。
这取决于能否成功通过smhasher测试集。
10是一个完美的分数。
**/


#pragma once

#if defined (__cplusplus)
namespace rocksdb {
#endif


/***********************
/ /类型
//********************************
typedef枚举XXH_OK=0，XXH_错误XXH_错误代码；



//********************************
//简单哈希函数
//********************************

unsigned int xxh32（const void*input，int len，unsigned int seed）；

/*
XXH32（）：
    计算存储在内存地址“input”的长度为“len”的序列的32位散列值。
    输入和输入+长度之间的内存必须有效（已分配并可读取）。
    “seed”可用于按可预见的方式更改结果。
    此函数成功通过所有smhash测试。
    核心2双核速度@3 GHz（单线程，smhasher基准）：5.4 GB/s
    请注意，“len”是“int”类型，这意味着它仅限于2^31-1。
    如果数据较大，请使用下面的高级功能。
**/




/***********************
//高级哈希函数
//********************************

void*xxh32_init（无符号int seed）；
xxh_错误代码xxh32_更新（void*状态，const void*输入，int len）；
unsigned int XXH32_digest（void*状态）；

/*
这些函数计算在几个小数据包中提供的输入的XXhash，
与作为单个块提供的输入相反。

必须从以下内容开始：
void*xxh32_init（）无效
函数返回保持计算状态的指针。

此指针必须作为xxh32_update（）的“void*state”参数提供。
xxh32_update（）可以根据需要多次调用。
用户必须提供有效（已分配）的输入。
函数返回一个错误代码，0表示OK，任何其他值表示有错误。
请注意，“len”是“int”类型，这意味着它仅限于2^31-1。
如果数据较大，建议将数据分成块
以避免任何“int”溢出问题。

最后，您可以使用xxh32_digest（）随时结束计算。
此函数返回最后的32位哈希。
必须提供由xxh32_init（）创建的相同“void*state”参数。
内存将由xxh32_digest（）释放。
**/



int           XXH32_sizeofState();
XXH_errorcode XXH32_resetState(void* state, unsigned int seed);

#define       XXH32_SIZEOFSTATE 48
typedef struct { long long ll[(XXH32_SIZEOFSTATE+(sizeof(long long)-1))/sizeof(long long)]; } XXH32_stateSpace_t;
/*
这些函数允许用户应用程序为状态进行自己的分配。

xxh32_sizeofstate（）用于知道必须为xxhash 32位状态分配多少空间。
请注意，状态必须对齐才能访问“long long”字段。内存必须由指针分配和引用。
然后必须将此指针作为'state'提供到xxh32_resetstate（）中，后者初始化状态。

出于静态分配目的（如堆栈上的分配，或没有malloc（）的独立系统），
使用结构xxh32_statespace_t，这将确保内存空间足够大并正确对齐以访问“long long”字段。
**/



unsigned int XXH32_intermediateDigest (void* state);
/*
此函数的作用与xxh32_digest（）相同，生成一个32位哈希，
但保留内存上下文。
这样，就可以生成中间散列，然后继续使用xxh32_update（）提供数据。
要释放内存上下文，请使用xxh32_digest（）或free（）。
**/




/***********************
//已弃用的函数名
//********************************
//提供以下翻译以简化代码转换
//建议您不再使用此函数名
定义xxh32_feed xxh32_更新
定义XXH32_结果XXH32_摘要
定义XXH32_获取IntermediateResult XXH32_IntermediateDigest



如果定义了（35u cplusplus）
//命名空间rocksdb
第二节
