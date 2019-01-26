
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//

#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocksdb {

class Slice;

extern std::vector<std::string> StringSplit(const std::string& arg, char delim);

template <typename T>
inline std::string ToString(T value) {
#if !(defined OS_ANDROID) && !(defined CYGWIN) && !(defined OS_FREEBSD)
  return std::to_string(value);
#else
//ANDORID或CyGWin不支持C++ 11、STD::toyStRung（）的全部
//不支持的功能之一。
  std::ostringstream os;
  os << value;
  return os.str();
#endif
}

//将“num”的可读打印输出附加到*str
extern void AppendNumberTo(std::string* str, uint64_t num);

//将“value”的可读打印输出附加到*str。
//转义在“value”中找到的任何不可打印字符。
extern void AppendEscapedStringTo(std::string* str, const Slice& value);

//返回“num”的字符串打印输出
extern std::string NumberToString(uint64_t num);

//返回num的可读版本。
//对于num>=10.000，打印“XXK”
//对于num>=10000.000，打印“XXM”
//对于num>=10.000.000.000，打印“xxg”
extern std::string NumberToHumanString(int64_t num);

//返回人类可读的字节版本
//例如：1048576->1.00 GB
extern std::string BytesToHumanString(uint64_t bytes);

//在micros中附加一个人类可读时间。
int AppendHumanMicros(uint64_t micros, char* output, int len,
                      bool fixed_format);

//附加以字节为单位的可读大小
int AppendHumanBytes(uint64_t bytes, char* output, int len);

//返回“value”的可读版本。
//转义在“value”中找到的任何不可打印字符。
extern std::string EscapeString(const Slice& value);

//将人类可读的数字从“*in”解析为“*value”。关于成功，
//提前“*in”超过消耗的数量，并将“*val”设置为
//数值。否则，返回false并在
//未指定状态。
extern bool ConsumeDecimalNumber(Slice* in, uint64_t* val);

//如果输入字符“c”被视为特殊字符，则返回true
//当调用escapeOptionString（）时，将对其进行转义。
//
//@param c输入字符
//@如果输入字符“c”被视为特殊字符，则返回true。
//@请参见escapeOptionString
bool isSpecialChar(const char c);

//如果输入字符是转义字符，它将返回
//关联的原始字符。否则，函数只返回
//原始输入字符。
char UnescapeChar(const char c);

//如果输入字符是控制字符，它将返回
//关联的转义字符。否则，函数只返回
//原始输入字符。
char EscapeChar(const char c);

//将原始字符串转换为转义字符串。转义字符是
//通过isSpecialChar（）函数定义。当输入中有一个字符时
//字符串“raw_string”被分类为特殊字符，然后
//将在输出中以'\'作为前缀。
//
//它的逆函数是unescapeoptionString（）。
//@param raw_string输入字符串
//@返回输入“raw”字符串的\“转义字符串”
//@见特殊章节，无标题
std::string EscapeOptionString(const std::string& raw_string);

//escapeOptionString的逆函数。它皈依
//'\'将字符串转义回原始字符串。
//
//@param escaped_string输入\'转义字符串
//@返回“转义的字符串”输入的原始字符串
std::string UnescapeOptionString(const std::string& escaped_string);

std::string trim(const std::string& str);

#ifndef ROCKSDB_LITE
bool ParseBoolean(const std::string& type, const std::string& value);

uint32_t ParseUint32(const std::string& value);
#endif

uint64_t ParseUint64(const std::string& value);

int ParseInt(const std::string& value);

double ParseDouble(const std::string& value);

size_t ParseSizeT(const std::string& value);

std::vector<int> ParseVectorInt(const std::string& value);

bool SerializeIntVector(const std::vector<int>& vec, std::string* value);

extern const std::string kNullptrString;

}  //命名空间rocksdb
