
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
**/



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
//默认情况下，XXhash库根据little endian约定提供与endian无关的哈希值。
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
pragma warning（disable:4804）//disable:c4804:“operation”：在操作中不安全地使用类型“bool”（静态断言行313）
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
包括“xxash.h”
//如果要使用其他与内存相关的例程，请修改下面的本地函数
//对于malloc（），free（）。
include<stdlib.h>
强制_inline void*xxh_malloc（尺寸_t s）返回malloc（s）；
强制内联void xxh_free（void*p）free（p）；
//用于MEMcPy.（）
include<string.h>
force_inline void*xxh_memcpy（void*dest，const void*src，size_t size）返回memcpy（dest，src，size）；


名称空间RocksDB
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

typedef结构_u32_s u32 v；_packed u32_s；

如果有的话！定义（xxh使用未对齐的访问）&！定义（uu gnuc_uuu）
杂注包（pop）
第二节

定义A32（x）（（u32_s*）（x））->v）


//***********************************************
//编译器特定的函数和宏
//***********************************************
定义gcc_版本（uu gnuc_uux100+u gnuc_u minor_uuuuu）

//注意：虽然mingw存在rotl（windows下的gcc），但是性能似乎很差。
如果定义了（35u msc35u ver）
定义xxh_rotl32（x，r）_rotl（x，r）
否则
define xxh_rotl32（x，r）（（x<<r）（x>>（32-r）））
第二节

如果已定义（_msc_ver）//Visual Studio
定义xxh_swap32_byteswap_ulong
elif gcc_版本>=403
定义XXHUSWAP32UUUU内置USWAP32
否则
静态内联U32 XXHU SWAP32（U32 X）
    返回（（x<24）&0xff000000）
        （x<<8）&0x00ff0000）
        （（X>>8）&0x0000 ff00）
        （（X>>24）&0x000000FF）；
第二节


//***********************************************
/ /常量
//***********************************************
定义prime32謹1 2654435761U
定义prime32_2 2246822519u
定义prime32_3 326648917u
定义prime32_4 668265263u
定义prime32354761393U


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

强制_inline u32 xxh_readle32_-align（const u32*ptr，xh_endianes endian，xh_-align-align）
{
    如果（对齐==XXH_未对齐）
        返回endian==xx\u littleendian？A32（PTR）：XXHUSWAP32（A32（PTR））；
    其他的
        返回endian==xx\u littleendian？*PTR:XXHU SWAP32（*PTR）；
}

force_inline u32 xxh_readle32（const u32*ptr，xh_endianes endian）return xh_readle32_align（ptr，endian，xx h_unaligned）；


//********************************
//简单哈希函数
//********************************
强制内联U32 XXH32_-endian_-align（const void*输入，int len，U32 seed，XXH_-endianes-endian，XXH_-align-align）
{
    const byte*p=（const byte*）输入；
    常量字节*常量弯板=p+len；
    U32 H32；

ifdef xxh_accept_null_input_指针
    if（p==null）len=0；p=（const byte*）（size_t）16；
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
            v1+=xxh_readle32_align（（const u32*）p，endian，align）*prime32_2；v1=xh_rotl32（v1，13）；v1*=prime32_1；p+=4；
            v2+=xxh_readle32_align（（const u32*）p，endian，align）*prime32_2；v2=xh_rotl32（v2，13）；v2*=prime32_1；p+=4；
            v3+=xxh_readle32_-align（（const u32*）p，endian，align）*prime32_2；v3=xxh_rotl32（v3，13）；v3*=prime32_1；p+=4；
            v4+=xxh_readle32_align（（const u32*）p，endian，align）*prime32_2；v4=xh_rotl32（v4，13）；v4*=prime32_1；p+=4；
        同时（p<=极限）；

        h32=xxh洹rotl32（v1，1）+xh洹rotl32（v2，7）+xh洹rotl32（v3，12）+xh洹rotl32（v4，18）；
    }
    其他的
    {
        H32=种子+原生质32_5；
    }

    h32+=（u32）长度；

    同时（P<=弯曲-4）
    {
        h32+=xxh_readle32_-align（（const u32*）p，endian，align）*prime32_3；
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


u32 xxh32（const void*输入，int len，u32 seed）
{
若0
    //简单的版本，适合代码维护，但不幸的是对于小的输入速度较慢
    void*状态=xxh32_init（种子）；
    xxh32_更新（状态、输入、长度）；
    返回XXH32_Digest（State）；
否则
    检测到的xh端=（xh端端）xh CPU端；

如果有的话！定义（xxh使用未对齐的访问）
    如果（（（size_t）input）&3）//输入对齐，让我们利用速度优势
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


//********************************
//高级哈希函数
//********************************

结构XXH状态32
{
    U64总计；
    U32种子；
    U32 V1；
    U32 V2；
    U32 V3；
    U32 V4；
    内存大小；
    字符存储器[16]；
}；


int xxh32_sizeofstate（）。
{
    xxh_static_assert（xxh32_sizeofstate>=sizeof（struct xxh_state32_t））；//此处的编译错误表示xxh32_sizeofstate不够大
    返回sizeof（struct XXH_state32_t）；
}


xxh_错误代码xxh32_resetstate（void*state_in，u32 seed）
{
    struct XXH_state32_t*state=（struct XXH_state32_t*）state_in；
    状态->种子=种子；
    state->v1=seed+prime32_1+prime32_2；
    state->v2=seed+prime32_2；
    状态->v3=种子+0；
    状态->v4=种子-prime32_1；
    状态->总长度=0；
    状态->内存大小=0；
    返回；
}


void*xxh32_init（u32种子）
{
    void*状态=XXH_malloc（sizeof（struct XXH_state32_t））；
    xxh32_resetstate（状态，种子）；
    返回状态；
}


强制_inline xh_错误代码xxh32_update_endian（void*state_in，const void*input，int len，xh endianes endian）
{
    struct XXH_state32_t*state=（struct XXH_state32_t*）state_in；
    const byte*p=（const byte*）输入；
    常量字节*常量弯板=p+len；

ifdef xxh_accept_null_input_指针
    if（input==null）返回xxh_错误；
第二节

    状态->总长度+=长度；

    if（state->memsize+len<16）//填充tmp缓冲区
    {
        xxh_memcpy（状态->内存+状态->内存大小，输入，长度）；
        状态->Memsize+=len；
        返回；
    }

    if（state->memsize）//上次更新留下的一些数据
    {
        xxh_memcpy（状态->内存+状态->内存大小，输入，16状态->内存大小）；
        {
            const u32*p32=（const u32*）状态->内存；
            state->v1+=xxh_readle32（p32，endian）*prime32_2；state->v1=xh_rotl32（state->v1，13）；state->v1*=prime32_1；p32++；
            state->v2+=xxh_readle32（p32，endian）*prime32_2；state->v2=xh_rotl32（state->v2，13）；state->v2*=prime32_1；p32++；
            state->v3+=xxh_readle32（p32，endian）*prime32_2；state->v3=xh_rotl32（state->v3，13）；state->v3*=prime32_1；p32++；
            state->v4+=xxh_readle32（p32，endian）*prime32_2；state->v4=xh_rotl32（state->v4，13）；state->v4*=prime32_1；p32++；
        }
        P+=16状态->内存大小；
        状态->内存大小=0；
    }

    如果（P<=弯曲-16）
    {
        常量字节*常量限制=弯板-16；
        u32 v1=状态->v1；
        u32 v2=状态->v2；
        u32 v3=状态->v3；
        u32 v4=状态->v4；

        做
        {
            v1+=xxh_readle32（（const u32*）p，endian）*prime32_2；v1=xh_rotl32（v1，13）；v1*=prime32_1；p+=4；
            V2+=XXH_readle32（（const u32*）P，endian）*prime32_2；V2=XXH_rotl32（V2，13）；V2*=prime32_1；P+=4；
            v3+=xxh_readle32（（const u32*）p，endian）*prime32_2；v3=xh_rotl32（v3，13）；v3*=prime32_1；p+=4；
            v4+=xxh_readle32（（const u32*）p，endian）*prime32_2；v4=xh_rotl32（v4，13）；v4*=prime32_1；p+=4；
        同时（p<=极限）；

        状态> V1= V1；
        状态> V2= V2；
        状态> v3= v3；
        状态> v4= v4；
    }

    如果（p＜弯曲）
    {
        xxh_memcpy（状态->内存，p，bend-p）；
        状态->内存大小=（int）（bend-p）；
    }

    返回；
}

xxh_错误代码xxh32_更新（void*state_in，const void*input，int len）
{
    检测到的xh端=（xh端端）xh CPU端；

    if（（endian_detected==xxh_littleendian）xh_force_native_格式）
        返回XXH32_update_endian（state_in，input，len，XXH_littleendian）；
    其他的
        返回XXH32_update_endian（state_in，input，len，XXH_bigendian）；
}



强制_inline u32 xxh32_intermediatedigest_endian（void*state_in，xxh endianes endian）
{
    struct XXH_state32_t*state=（struct XXH_state32_t*）state_in；
    const byte*p=（const byte*）状态->内存；
    byte*bend=（byte*）状态->内存+状态->内存大小；
    U32 H32；

    if（状态->总长度>=16）
    {
        h32=xxh_rotl32（状态->v1，1）+xh_rotl32（状态->v2，7）+xh_rotl32（状态->v3，12）+xh_rotl32（状态->v4，18）；
    }
    其他的
    {
        h32=状态->种子+prime32_5；
    }

    h32+=（u32）状态->总长度；

    同时（P<=弯曲-4）
    {
        h32+=xxh_readle32（（const u32*）p，endian）*prime32_3；
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


U32 XXH32_IntermediateDigest（void*state_in）
{
    检测到的xh端=（xh端端）xh CPU端；

    if（（endian_detected==xxh_littleendian）xh_force_native_格式）
        返回XXH32_intermediatedigest_endian（state_in，XXH_littleendian）；
    其他的
        返回xxh32_intermediatedigest_endian（state_in，xxh_bigendian）；
}


U32 XXH32_Digest（void*state_in）
{
    u32 h32=xxh32_intermediatedigest（state_in）；

    XXH-自由（州内）

    返回H32；
}

//命名空间rocksdb
