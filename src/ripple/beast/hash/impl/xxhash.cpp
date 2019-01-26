
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
xxhash-快速哈希算法
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
-公共讨论板：https://groups.google.com/forum/！论坛/ LZ4C
**/


#include <ripple/beast/hash/impl/xxhash.h>

/*********************************
//整定参数
//***********************************************
//未对齐的内存访问会自动为“公用”CPU（如x86）启用。
//对于其他CPU，编译器会更加谨慎，并插入额外的代码以确保符合对齐的访问。
//如果知道目标CPU支持未对齐的内存访问，则需要手动强制此选项以提高性能。
//如果知道输入数据将始终对齐（U32的边界为4），也可以启用此参数。
如果定义了（uu arm_feature_unaligned）定义了（u i386）定义了（u m_ix86）定义了（u x86_64_uuu）定义了（_m_x64）
定义xxh使用未对齐的访问1
第二节

//xxh_accept_null_input_指针：
//如果输入指针为空指针，XXHASH默认行为是触发内存访问错误，因为它是一个错误的指针。
//启用此选项时，空输入指针的XXHASH输出将与空长度输入相同。
//此选项的性能成本非常小（仅在小输入上可测量）。
//默认情况下，此选项被禁用。要启用它，请取消下面的注释定义：
//定义xxh_接受_空_输入_指针1

//XXH_force_native_格式：
//默认情况下，XXhash库根据little endian约定提供endian独立散列值。
//因此，小端和大端CPU的结果是相同的。
//对于big-endian CPU来说，这是一个性能代价，因为模拟小endian格式需要一些交换。
//如果endian独立性对应用程序不重要，则可以将下面的define设置为1。
//这将提高big endian CPU的速度。
//此选项对小型CPU没有影响。
定义XXH_强制_本机_格式0

//***********************************************
//编译器特定选项
//***********************************************
//禁用一些可视警告消息
ifdef诳msc诳ver//Visual Studio
pragma warning（disable:4127）//disable:c4127:条件表达式为常量
第二节

ifdef诳msc诳ver//Visual Studio
定义力u inline静态u force inline
否则
ifdef_uu gnuc_uu
定义力u inline静态inline uu属性uuu（（始终inline））。
否则
定义力内联静态内联
第二节
第二节

//***********************************************
//包含与内存相关的函数（&M）
//***********************************************
//包括“xxash.h”
//如果要使用其他内存例程，请修改下面的本地函数
//对于malloc（），free（）。
include<stdlib.h>
静态void*xxh_malloc（大小_t s）返回malloc（s）；
静态空隙xxh_free（void*p）free（p）；
//用于MEMcPy.（）
include<string.h>
静态void*xxh_memcpy（void*dest，const void*src，size_t size）
{
    返回memcpy（dest，src，size）；
}


//***********************************************
//基本类型
//***********************************************
如果定义了（uu stdc_u version_uuu）&&u stdc_u version_uuu>=199901L//c99
include<stdint.h>
typedef uint8_t字节；
typedef uint16_t u16；
typedef uint32_t u32；
typedef int32_t s32；
typedef uint64_t u64；
否则
typedef无符号字符字节；
typedef无符号短型U16；
typedef无符号int u32；
typedef签名int s32；
typedef无符号长型U64；
第二节

如果定义了（GNUC）&！定义（xxh使用未对齐的访问）
定义打包属性（packed）
否则
定义包装
第二节

如果有的话！定义（xxh使用未对齐的访问）&！定义（uu gnuc_uuu）
ifdef_uuu ibmc_uu
杂注包（1）
否则
pragma pack（推送，1）
第二节
第二节

命名空间Beast
命名空间详细信息

typedef结构
{
    U32 V；
U32；
typedef结构
{
    U64 V；
包装U64_U；

如果有的话！定义（xxh使用未对齐的访问）&！定义（uu gnuc_uuu）
杂注包（pop）
第二节

定义A32（x）（（u32_s*）（x））->v）
定义a64（x）（（u64_s*）（x））->v）


//***********************************************
//编译器特定的函数和宏
//***********************************************
定义gcc_版本（uu gnuc_uux100+u gnuc_u minor_uuuuu）

//注意：虽然mingw存在rotl（windows下的gcc），但是性能似乎很差。
如果定义了（35u msc35u ver）
定义xxh_rotl32（x，r）_rotl（x，r）
定义xxh_rotl64（x，r）_rotl64（x，r）
否则
define xxh_rotl32（x，r）（（x<<r）（x>>（32-r）））
define xxh_rotl64（x，r）（（x<<r）（x>>（64-r）））
第二节

如果已定义（_msc_ver）//Visual Studio
定义xxh_swap32_byteswap_ulong
定义xxh_swap64_byteswap_uint64
elif gcc_版本>=403
定义XXHUSWAP32UUUU内置USWAP32
定义xxh_swap64_uu builtin_bswap64
否则
静态内联U32 XXHU SWAP32（U32 X）
{
    返回（（x<24）&0xff000000）
            （x<<8）&0x00ff0000）
            （（X>>8）&0x0000 ff00）
            （X>>24）&0x000000FF；
}
静态内联U64 XXHU SWAP64（U64 X）
{
    返回（（x<<56）&0xff000000000000ull）
            （（x<<40）&0x00ff000000000000ull）
            （（x<24）&0x000ff0000000000ull）
            （（x<8）&0x000000ff00000000ull）
            （（X>>8）&0x0000000ff000000ull）
            （（X>>24）&0x00000000000ff00000ull）
            （X>>40）&0x00000000000ff00ull）
            （（X>>56）&0x000000000000整）；
}
第二节


//***********************************************
/ /常量
//***********************************************
定义prime32謹1 2654435761U
定义prime32_2 2246822519u
定义prime32_3 326648917u
定义prime32_4 668265263u
定义prime32354761393U

定义prime64_1 11400714785074694791ull
定义prime64_2 14029467366897019727ull
定义prime64_3 1609587929392839161ull
定义prime64_4 965002924287828579ull
定义prime64_5 2870177450012600261ull

//***********************************************
//体系结构宏
//***********************************************
typedef枚举XXH_bigendian=0，XXH_littleendian=1 XXH_endianes；
ifndef xxh_cpu_little_endian//可以在外部定义xh_cpu_little_endian，例如使用编译器开关
静态常量1=1；
define xxh_cpu_little_endian（*（char*）（&one））。
第二节


//***********************************************
//宏指令
//***********************************************
define xxh_static_assert（c）enum_xh_static_assert=1/（！！（c））；//仅在*变量声明后使用*


//********************************
//内存读取
//********************************
typedef枚举xh_对齐，xh_未对齐xh_对齐；

强制_inline u32 xxh_readle32_-align（const void*ptr，xh endianes endian，xh_-align-align）
{
    如果（对齐==XXH_未对齐）
        返回endian==xx\u littleendian？A32（PTR）：XXHUSWAP32（A32（PTR））；
    其他的
        返回endian==xx\u littleendian？*（u32*）ptr:xxh_swap32（*（u32*）ptr；
}

强制_inline u32 xxh_readle32（const void*ptr，xh_endianes endian）
{
    返回xxh_readle32_-align（ptr，endian，xxh_unaligned）；
}

强制_inline u64 xxh_readle64_-align（const void*ptr，xh endianes endian，xh_-align-align）
{
    如果（对齐==XXH_未对齐）
        返回endian==xx\u littleendian？A64（PTR）：XXHU SWAP64（A64（PTR））；
    其他的
        返回endian==xx\u littleendian？*（u64*）ptr:xxh_swap64（*（u64*）ptr；
}

强制_inline u64 xxh_readle64（const void*ptr，xh_endianes endian）
{
    返回xxh_readle64_-align（ptr，endian，xxh_unaligned）；
}


//********************************
//简单哈希函数
//********************************
强制“inline u32 xxh32”对齐（const void*输入，大小“len”，u32 seed，xxh“endianes”endian，xxh“对齐对齐）
{
    const byte*p=（const byte*）输入；
    常量字节*弯曲=p+长度；
    U32 H32；
define xxh_get32bits（p）xxh_readle32_-align（p，endian，align）

ifdef xxh_accept_null_input_指针
    如果（p=＝null）
    {
        Le＝0；
        bend=p=（const byte*）（size_t）16；
    }
第二节

    如果（Le>＞16）
    {
        常量字节*常量限制=弯板-16；
        u32 v1=种子+prime32_1+prime32_2；
        u32 v2=种子+prime32_2；
        u32 v3=种子+0；
        u32 v4=种子-prime32_1；

        做
        {
            v1+=xxh_get32bits（p）*prime32_2；
            v1=xxh_rotl32（v1，13）；
            v1*=prime32_1；
            p+＝4；
            V2+=XXH_Get32Bits（P）*Prime32_2；
            v2=xxh_rotl32（v2，13）；
            V2*=prime32_1；
            p+＝4；
            v3+=xxh_get32bits（p）*prime32_2；
            v3=xxh_rotl32（v3，13）；
            v3*=prime32_1；
            p+＝4；
            v4+=xxh_get32bits（p）*prime32_2；
            v4=xxh_rotl32（v4，13）；
            v4*=prime32_1；
            p+＝4；
        }
        而（p<=极限）；

        h32=xxh洹rotl32（v1，1）+xh洹rotl32（v2，7）+xh洹rotl32（v3，12）+xh洹rotl32（v4，18）；
    }
    其他的
    {
        H32=种子+原生质32_5；
    }

    h32+=（u32）长度；

    同时（P+4<=弯曲）
    {
        h32+=xxh_get32bits（p）*prime32_3；
        h32=xxh_rotl32（h32，17）*prime32_4；
        p+＝4；
    }

    （p＜弯曲）
    {
        H32+=（*P）*Prime32_5；
        h32=xxh_rotl32（h32，11）*prime32_1；
        P+；
    }

    H32^=H32>>15；
    h32*=prime32_2；
    H32^=H32>>13；
    h32*=prime32_3；
    H32^=H32>>16；

    返回H32；
}


unsigned int xxh32（const void*输入，大小，无符号种子）
{
若0
    //简单的版本，适合代码维护，但不幸的是对于小的输入速度较慢
    XXH32州
    xxh32_重置（&state，seed）；
    xxh32_更新（&state，input，len）；
    返回XXH32_Digest（&state）；
否则
    检测到的xh端=（xh端端）xh CPU端；

如果有的话！定义（xxh使用未对齐的访问）
    如果（（（size_t）input）&3）==0）//输入对齐，让我们利用速度优势
    {
        if（（endian_detected==xxh_littleendian）xh_force_native_格式）
            返回xxh32撏endian_-align（input，len，seed，xxh撏littleendian，xxh_-aligned）；
        其他的
            返回xxh32撊endian撊align（input，len，seed，xxh撊bigendian，xxh撊aligned）；
    }
第二节

    if（（endian_detected==xxh_littleendian）xh_force_native_格式）
        返回xxh32撊endian_-align（input，len，seed，xxh撊littleendian，xxh_unaligned）；
    其他的
        返回xxh32撊endian撊align（input，len，seed，xxh撊bigendian，xxh撊unaligned）；
第二节
}

强制“inline”u64 xxh64“endian”对齐（const void*输入，大小“len”，u64 seed，xxh“endianes”endian，xh“alignment”对齐）
{
    const byte*p=（const byte*）输入；
    常量字节*弯曲=p+长度；
    U64 H64；
define xxh_get64bits（p）xxh_readle64_-align（p，endian，align）

ifdef xxh_accept_null_input_指针
    如果（p=＝null）
    {
        Le＝0；
        bend=p=（const byte*）（size_t）32；
    }
第二节

    如果（Le>＞32）
    {
        常量字节*常量限制=弯曲-32；
        u64 v1=种子+prime64_1+prime64_2；
        u64 v2=种子+prime64_2；
        u64 v3=种子+0；
        u64 v4=种子-prime64_1；

        做
        {
            v1+=xxh_get64bits（p）*prime64_2；
            p+＝8；
            v1=xxh_rotl64（v1，31）；
            v1*=prime64_1；
            v2+=xxh_get64bits（p）*prime64_2；
            p+＝8；
            V2=XXH_Rotl64（V2，31）；
            V2*=prime64_1；
            v3+=xxh_get64bits（p）*prime64_2；
            p+＝8；
            v3=xxh_rotl64（v3，31）；
            v3*=prime64_1；
            v4+=xxh_get64bits（p）*prime64_2；
            p+＝8；
            v4=xxh_rotl64（v4，31）；
            v4*=prime64_1；
        }
        而（p<=极限）；

        h64=xxh洹rotl64（v1，1）+xh洹rotl64（v2，7）+xh洹rotl64（v3，12）+xh洹rotl64（v4，18）；

        v1*=prime64_2；
        v1=xxh_rotl64（v1，31）；
        v1*=prime64_1；
        H64 ^＝V1；
        h64=h64*prime64_1+prime64_4；

        V2*=prime64_2；
        V2=XXH_Rotl64（V2，31）；
        V2*=prime64_1；
        H64 ^＝V2；
        h64=h64*prime64_1+prime64_4；

        v3*=prime64_2；
        v3=xxh_rotl64（v3，31）；
        v3*=prime64_1；
        H64 ^＝V3；
        h64=h64*prime64_1+prime64_4；

        v4*=prime64_2；
        v4=xxh_rotl64（v4，31）；
        v4*=prime64_1；
        H64 ^＝V4；
        h64=h64*prime64_1+prime64_4；
    }
    其他的
    {
        h64=种子+质数64_5；
    }

    h64+=（u64）长度；

    同时（P+8<=弯曲）
    {
        u64 k1=xxh_get64位（p）；
        k1*=prime64_2；
        k1=xxh_rotl64（k1,31）；
        k1*=prime64_1；
        H64 ^＝K1；
        h64=xxh_rotl64（h64,27）*prime64_1+prime64_4；
        p+＝8；
    }

    如果（p+ 4＜＝弯曲）
    {
        h64 ^=（u64）（xxh_get32bits（p））*prime64_1；
        h64=xxh_rotl64（h64，23）*prime64_2+prime64_3；
        p+＝4；
    }

    （p＜弯曲）
    {
        h64^=（*p）*prime64_5；
        h64=xxh_rotl64（h64，11）*prime64_1；
        P+；
    }

    H64^=H64>>33；
    h64*=prime64_2；
    H64^=H64>>29；
    h64*=prime64_3；
    H64^=H64>>32；

    返回H64；
}


无符号长XXH64（const void*输入，大小，无符号长种子）
{
若0
    //简单的版本，适合代码维护，但不幸的是对于小的输入速度较慢
    XXH64州；
    xxh64_重置（&state，seed）；
    xxh64_更新（&state，input，len）；
    返回xxh64_摘要（&state）；
否则
    检测到的xh端=（xh端端）xh CPU端；

如果有的话！定义（xxh使用未对齐的访问）
    如果（（（size_t）input）&7）==0）//输入对齐，让我们利用速度优势
    {
        if（（endian_detected==xxh_littleendian）xh_force_native_格式）
            返回xxh64-endian-unign（input，len，seed，xxh-littleendian，xxh-aligned）；
        其他的
            返回xxh64撊endian撊align（input，len，seed，xxh撊bigendian，xxh撊aligned）；
    }
第二节

    if（（endian_detected==xxh_littleendian）xh_force_native_格式）
        返回xxh64-endian-unign（input，len，seed，xxh-littleendian，xxh-unaligned）；
    其他的
        返回xxh64撊endian_-align（input，len，seed，xxh撊bigendian，xxh_unaligned）；
第二节
}

/………………………………………………………………
 *高级哈希函数
************************************************/


/**分配**/
typedef struct
{
    U64 total_len;
    U32 seed;
    U32 v1;
    U32 v2;
    U32 v3;
    U32 v4;
    /*MEM32[4]；/*定义为用于对齐的U32*/
    U32内存大小；
XXH坿ISTATE32坿T；

Type结构
{
    U64总计；
    U64种子；
    U64 V1；
    U64 V2；
    U64 V3；
    U64 V4；
    U64 MEM64[4]；/*定义为U64，用于对齐*/

    U32 memsize;
} XXH_istate64_t;


XXH32_state_t* XXH32_createState(void)
{
static_assert(sizeof(XXH32_state_t) >= sizeof(XXH_istate32_t), "");   //这里的编译错误意味着xxh32_state_t不够大
    return (XXH32_state_t*)XXH_malloc(sizeof(XXH32_state_t));
}
XXH_errorcode XXH32_freeState(XXH32_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
};

XXH64_state_t* XXH64_createState(void)
{
static_assert(sizeof(XXH64_state_t) >= sizeof(XXH_istate64_t), "");   //这里的编译错误意味着xxh64_state_t不够大
    return (XXH64_state_t*)XXH_malloc(sizeof(XXH64_state_t));
}
XXH_errorcode XXH64_freeState(XXH64_state_t* statePtr)
{
    XXH_free(statePtr);
    return XXH_OK;
};


/**哈希进给**/

XXH_errorcode XXH32_reset(XXH32_state_t* state_in, U32 seed)
{
    XXH_istate32_t* state = (XXH_istate32_t*) state_in;
    state->seed = seed;
    state->v1 = seed + PRIME32_1 + PRIME32_2;
    state->v2 = seed + PRIME32_2;
    state->v3 = seed + 0;
    state->v4 = seed - PRIME32_1;
    state->total_len = 0;
    state->memsize = 0;
    return XXH_OK;
}

XXH_errorcode XXH64_reset(XXH64_state_t* state_in, unsigned long long seed)
{
    XXH_istate64_t* state = (XXH_istate64_t*) state_in;
    state->seed = seed;
    state->v1 = seed + PRIME64_1 + PRIME64_2;
    state->v2 = seed + PRIME64_2;
    state->v3 = seed + 0;
    state->v4 = seed - PRIME64_1;
    state->total_len = 0;
    state->memsize = 0;
    return XXH_OK;
}


FORCE_INLINE XXH_errorcode XXH32_update_endian (XXH32_state_t* state_in, const void* input, size_t len, XXH_endianess endian)
{
    XXH_istate32_t* state = (XXH_istate32_t *) state_in;
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return XXH_ERROR;
#endif

    state->total_len += len;

if (state->memsize + len < 16)   //填写TMP缓冲器
    {
        XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, len);
        state->memsize += (U32)len;
        return XXH_OK;
    }

if (state->memsize)   //上次更新留下的一些数据
    {
        XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, 16-state->memsize);
        {
            const U32* p32 = state->mem32;
            state->v1 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v1 = XXH_rotl32(state->v1, 13);
            state->v1 *= PRIME32_1;
            p32++;
            state->v2 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v2 = XXH_rotl32(state->v2, 13);
            state->v2 *= PRIME32_1;
            p32++;
            state->v3 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v3 = XXH_rotl32(state->v3, 13);
            state->v3 *= PRIME32_1;
            p32++;
            state->v4 += XXH_readLE32(p32, endian) * PRIME32_2;
            state->v4 = XXH_rotl32(state->v4, 13);
            state->v4 *= PRIME32_1;
            p32++;
        }
        p += 16-state->memsize;
        state->memsize = 0;
    }

    if (p <= bEnd-16)
    {
        const BYTE* const limit = bEnd - 16;
        U32 v1 = state->v1;
        U32 v2 = state->v2;
        U32 v3 = state->v3;
        U32 v4 = state->v4;

        do
        {
            v1 += XXH_readLE32(p, endian) * PRIME32_2;
            v1 = XXH_rotl32(v1, 13);
            v1 *= PRIME32_1;
            p+=4;
            v2 += XXH_readLE32(p, endian) * PRIME32_2;
            v2 = XXH_rotl32(v2, 13);
            v2 *= PRIME32_1;
            p+=4;
            v3 += XXH_readLE32(p, endian) * PRIME32_2;
            v3 = XXH_rotl32(v3, 13);
            v3 *= PRIME32_1;
            p+=4;
            v4 += XXH_readLE32(p, endian) * PRIME32_2;
            v4 = XXH_rotl32(v4, 13);
            v4 *= PRIME32_1;
            p+=4;
        }
        while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd)
    {
        XXH_memcpy(state->mem32, p, bEnd-p);
        state->memsize = (int)(bEnd-p);
    }

    return XXH_OK;
}

XXH_errorcode XXH32_update (XXH32_state_t* state_in, const void* input, size_t len)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH32_update_endian(state_in, input, len, XXH_littleEndian);
    else
        return XXH32_update_endian(state_in, input, len, XXH_bigEndian);
}



FORCE_INLINE U32 XXH32_digest_endian (const XXH32_state_t* state_in, XXH_endianess endian)
{
    XXH_istate32_t* state = (XXH_istate32_t*) state_in;
    const BYTE * p = (const BYTE*)state->mem32;
    BYTE* bEnd = (BYTE*)(state->mem32) + state->memsize;
    U32 h32;

    if (state->total_len >= 16)
    {
        h32 = XXH_rotl32(state->v1, 1) + XXH_rotl32(state->v2, 7) + XXH_rotl32(state->v3, 12) + XXH_rotl32(state->v4, 18);
    }
    else
    {
        h32  = state->seed + PRIME32_5;
    }

    h32 += (U32) state->total_len;

    while (p+4<=bEnd)
    {
        h32 += XXH_readLE32(p, endian) * PRIME32_3;
        h32  = XXH_rotl32(h32, 17) * PRIME32_4;
        p+=4;
    }

    while (p<bEnd)
    {
        h32 += (*p) * PRIME32_5;
        h32 = XXH_rotl32(h32, 11) * PRIME32_1;
        p++;
    }

    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;

    return h32;
}


U32 XXH32_digest (const XXH32_state_t* state_in)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH32_digest_endian(state_in, XXH_littleEndian);
    else
        return XXH32_digest_endian(state_in, XXH_bigEndian);
}


FORCE_INLINE XXH_errorcode XXH64_update_endian (XXH64_state_t* state_in, const void* input, size_t len, XXH_endianess endian)
{
    XXH_istate64_t * state = (XXH_istate64_t *) state_in;
    const BYTE* p = (const BYTE*)input;
    const BYTE* const bEnd = p + len;

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
    if (input==NULL) return XXH_ERROR;
#endif

    state->total_len += len;

if (state->memsize + len < 32)   //填写TMP缓冲器
    {
        XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, len);
        state->memsize += (U32)len;
        return XXH_OK;
    }

if (state->memsize)   //上次更新留下的一些数据
    {
        XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, 32-state->memsize);
        {
            const U64* p64 = state->mem64;
            state->v1 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v1 = XXH_rotl64(state->v1, 31);
            state->v1 *= PRIME64_1;
            p64++;
            state->v2 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v2 = XXH_rotl64(state->v2, 31);
            state->v2 *= PRIME64_1;
            p64++;
            state->v3 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v3 = XXH_rotl64(state->v3, 31);
            state->v3 *= PRIME64_1;
            p64++;
            state->v4 += XXH_readLE64(p64, endian) * PRIME64_2;
            state->v4 = XXH_rotl64(state->v4, 31);
            state->v4 *= PRIME64_1;
            p64++;
        }
        p += 32-state->memsize;
        state->memsize = 0;
    }

    if (p+32 <= bEnd)
    {
        const BYTE* const limit = bEnd - 32;
        U64 v1 = state->v1;
        U64 v2 = state->v2;
        U64 v3 = state->v3;
        U64 v4 = state->v4;

        do
        {
            v1 += XXH_readLE64(p, endian) * PRIME64_2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= PRIME64_1;
            p+=8;
            v2 += XXH_readLE64(p, endian) * PRIME64_2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= PRIME64_1;
            p+=8;
            v3 += XXH_readLE64(p, endian) * PRIME64_2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= PRIME64_1;
            p+=8;
            v4 += XXH_readLE64(p, endian) * PRIME64_2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= PRIME64_1;
            p+=8;
        }
        while (p<=limit);

        state->v1 = v1;
        state->v2 = v2;
        state->v3 = v3;
        state->v4 = v4;
    }

    if (p < bEnd)
    {
        XXH_memcpy(state->mem64, p, bEnd-p);
        state->memsize = (int)(bEnd-p);
    }

    return XXH_OK;
}

XXH_errorcode XXH64_update (XXH64_state_t* state_in, const void* input, size_t len)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH64_update_endian(state_in, input, len, XXH_littleEndian);
    else
        return XXH64_update_endian(state_in, input, len, XXH_bigEndian);
}



FORCE_INLINE U64 XXH64_digest_endian (const XXH64_state_t* state_in, XXH_endianess endian)
{
    XXH_istate64_t * state = (XXH_istate64_t *) state_in;
    const BYTE * p = (const BYTE*)state->mem64;
    BYTE* bEnd = (BYTE*)state->mem64 + state->memsize;
    U64 h64;

    if (state->total_len >= 32)
    {
        U64 v1 = state->v1;
        U64 v2 = state->v2;
        U64 v3 = state->v3;
        U64 v4 = state->v4;

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

        v1 *= PRIME64_2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= PRIME64_1;
        h64 ^= v1;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v2 *= PRIME64_2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= PRIME64_1;
        h64 ^= v2;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v3 *= PRIME64_2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= PRIME64_1;
        h64 ^= v3;
        h64 = h64*PRIME64_1 + PRIME64_4;

        v4 *= PRIME64_2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= PRIME64_1;
        h64 ^= v4;
        h64 = h64*PRIME64_1 + PRIME64_4;
    }
    else
    {
        h64  = state->seed + PRIME64_5;
    }

    h64 += (U64) state->total_len;

    while (p+8<=bEnd)
    {
        U64 k1 = XXH_readLE64(p, endian);
        k1 *= PRIME64_2;
        k1 = XXH_rotl64(k1,31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64,27) * PRIME64_1 + PRIME64_4;
        p+=8;
    }

    if (p+4<=bEnd)
    {
        h64 ^= (U64)(XXH_readLE32(p, endian)) * PRIME64_1;
        h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p+=4;
    }

    while (p<bEnd)
    {
        h64 ^= (*p) * PRIME64_5;
        h64 = XXH_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}


unsigned long long XXH64_digest (const XXH64_state_t* state_in)
{
    XXH_endianess endian_detected = (XXH_endianess)XXH_CPU_LITTLE_ENDIAN;

    if ((endian_detected==XXH_littleEndian) || XXH_FORCE_NATIVE_FORMAT)
        return XXH64_digest_endian(state_in, XXH_littleEndian);
    else
        return XXH64_digest_endian(state_in, XXH_bigEndian);
}


} //细节
} //野兽
