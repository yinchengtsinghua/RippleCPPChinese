
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
   xxhash-极快哈希算法
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


#ifndef BEAST_HASH_XXHASH_H_INCLUDED
#define BEAST_HASH_XXHASH_H_INCLUDED

/***********************
   包括
***********************/

/*clude<stddef.h>/*大小\u t*/

命名空间Beast
命名空间详细信息

/***************************
   类型
***********************/

typedef enum { XXH_OK=0, XXH_ERROR } XXH_errorcode;



/***********************
   简单哈希函数
***********************/


unsigned int       XXH32 (const void* input, size_t length, unsigned seed);
unsigned long long XXH64 (const void* input, size_t length, unsigned long long seed);

/*
XXH32（）：
    计算存储在内存地址“input”的序列“length”字节的32位散列值。
    输入和输入+长度之间的内存必须有效（已分配并可读取）。
    “seed”可用于按可预见的方式更改结果。
    此函数成功通过所有smhash测试。
    核心2双核速度@3 GHz（单线程，smhasher基准）：5.4 GB/s
XXH64（）：
    计算存储在内存地址“input”的长度为“len”的序列的64位散列值。
**/




/***********************
   高级哈希函数
***********************/

typedef struct { long long ll[ 6]; } XXH32_state_t;
typedef struct { long long ll[11]; } XXH64_state_t;

/*
这些结构允许对XXH国家进行静态分配。
在首次使用之前，必须使用xxhnn_reset（）初始化状态。

如果您喜欢动态分配，请参阅下面的函数。
**/


XXH32_state_t* XXH32_createState(void);
XXH_errorcode  XXH32_freeState(XXH32_state_t* statePtr);

XXH64_state_t* XXH64_createState(void);
XXH_errorcode  XXH64_freeState(XXH64_state_t* statePtr);

/*
这些函数为XXH状态创建和释放内存。
在首次使用之前，必须使用xxhnn_reset（）初始化状态。
**/



XXH_errorcode XXH32_reset  (XXH32_state_t* statePtr, unsigned seed);
XXH_errorcode XXH32_update (XXH32_state_t* statePtr, const void* input, size_t length);
unsigned int  XXH32_digest (const XXH32_state_t* statePtr);

XXH_errorcode      XXH64_reset  (XXH64_state_t* statePtr, unsigned long long seed);
XXH_errorcode      XXH64_update (XXH64_state_t* statePtr, const void* input, size_t length);
unsigned long long XXH64_digest (const XXH64_state_t* statePtr);

/*
这些函数计算多个较小数据包中提供的输入的XXhash，
与作为单个块提供的输入相反。

必须首先使用上面提供的静态或动态方法分配XXH状态空间。

使用xxhnn_reset（）通过使用种子初始化状态来启动新哈希。

然后，根据需要多次调用xxhnn_update（）来提供散列状态。
显然，输入必须是有效的，意味着分配和读取是可访问的。
函数返回一个错误代码，0表示OK，任何其他值表示有错误。

最后，您可以使用xhnn_digest（）随时生成哈希。
此函数返回最终的nn位散列。
但是，您可以继续向哈希状态提供更多的输入，
因此，通过再次调用xxhnn_digest（）来获取一些新的哈希。

完成后，不要忘记释放XXH状态空间，通常使用xhnn_free state（）。
**/


} //细节
} //野兽

#endif
