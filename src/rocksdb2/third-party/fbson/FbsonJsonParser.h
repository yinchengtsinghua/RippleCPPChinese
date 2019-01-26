
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

/*
 *此文件定义了fbsonjsonParsert（模板）和fbsonjsonParser。
 *
 *fbsonjsonParsert是一个模板类，实现了一个JSON解析器。
 *fbsonjsonparsert解析json文本，并将其序列化为fbson二进制格式。
 *使用fbsonWritert对象。默认情况下，fbsonjsonParsert创建一个新的
 *具有输出流对象的fbsonWritert对象。但是，您也可以
 *传入fbsonWriter或任何实现某些基本的流对象
 *std:：ostream接口（见fbsonstream.h）。
 *
 *fbsonjsonParser专门使用fbsonjsonParsert和fbsonOutstream类型（请参见
 *fbsonstream.h）。所以除非你想提供一个不同的输出流
 *类型，使用fbsonjsonParser对象。
 *
 ***解析JSON**
 *fbsonjsonparsert解析json字符串，直接序列化为fbson
 *压缩字节。解析JSON字符串有三种方法：（1）使用
 *C-string，（2）使用带len的字符串，（3）使用std:：istream对象。你可以
 *使用custom streambuf重定向输出。fbsonOutBuffer是使用的streamBuf
 *如果输入是原始字符缓冲区，则在内部。
 *
 *可以重用一个fbsonjsonParsert对象来解析/序列化多个json
 *字符串，前一个fbson将被覆盖。
 *
 *如果解析失败（返回false），错误代码将设置为
 *fbsonerrType，可以通过调用getErrorCode（）来检索。
 *
 ***外部字典**
 *在解析JSON字符串时，可以传递回调函数来映射键。
 *字符串到一个ID，并将字典ID存储在fbson中以节省空间。这个
 *使用外部词典的目的更多是为了收集
 *文档（具有公共键）而不是单个文档，因此
 *节省空间非常重要。
 *
 ***结束键**
 *注意：fbson序列化不假定服务器是终结的。然而
 *您需要确保读卡器端的结束语是相同的。
 *在编写方也是如此（如果它们在不同的机器上）。否则，
 *当数字值返回到
 *呼叫者/作者。
 *
 *@author tian xia<tianx@fb.com>
 **/


#ifndef FBSON_FBSONPARSER_H
#define FBSON_FBSONPARSER_H

#include <cmath>
#include <limits>
#include "FbsonDocument.h"
#include "FbsonWriter.h"

namespace fbson {

const char* const kJsonDelim = " ,]}\t\r\n";
const char* const kWhiteSpace = " \t\n\r";

/*
 ＊错误代码
 **/

enum class FbsonErrType {
  E_NONE = 0,
  E_INVALID_VER,
  E_EMPTY_STR,
  E_OUTPUT_FAIL,
  E_INVALID_DOCU,
  E_INVALID_VALUE,
  E_INVALID_KEY,
  E_INVALID_STR,
  E_INVALID_OBJ,
  E_INVALID_ARR,
  E_INVALID_HEX,
  E_INVALID_OCTAL,
  E_INVALID_DECIMAL,
  E_INVALID_EXPONENT,
  E_HEX_OVERFLOW,
  E_OCTAL_OVERFLOW,
  E_DECIMAL_OVERFLOW,
  E_DOUBLE_OVERFLOW,
  E_EXPONENT_OVERFLOW,
};

/*
 *模板fbsonjsonparsert
 **/

template <class OS_TYPE>
class FbsonJsonParserT {
 public:
  FbsonJsonParserT() : err_(FbsonErrType::E_NONE) {}

  explicit FbsonJsonParserT(OS_TYPE& os)
      : writer_(os), err_(FbsonErrType::E_NONE) {}

//解析UTF-8 JSON字符串
  bool parse(const std::string& str, hDictInsert handler = nullptr) {
    return parse(str.c_str(), (unsigned int)str.size(), handler);
  }

//解析utf-8 JSON C样式字符串（以空结尾）
  bool parse(const char* c_str, hDictInsert handler = nullptr) {
    return parse(c_str, (unsigned int)strlen(c_str), handler);
  }

//解析具有长度的utf-8 json字符串
  bool parse(const char* pch, unsigned int len, hDictInsert handler = nullptr) {
    if (!pch || len == 0) {
      err_ = FbsonErrType::E_EMPTY_STR;
      return false;
    }

    FbsonInBuffer sb(pch, len);
    std::istream in(&sb);
    return parse(in, handler);
  }

//从输入流分析UTF-8 JSON文本
  bool parse(std::istream& in, hDictInsert handler = nullptr) {
    bool res = false;

//重置输出流
    writer_.reset();

    trim(in);

    if (in.peek() == '{') {
      in.ignore();
      res = parseObject(in, handler);
    } else if (in.peek() == '[') {
      in.ignore();
      res = parseArray(in, handler);
    } else {
      err_ = FbsonErrType::E_INVALID_DOCU;
    }

    trim(in);
    if (res && !in.eof()) {
      err_ = FbsonErrType::E_INVALID_DOCU;
      return false;
    }

    return res;
  }

  FbsonWriterT<OS_TYPE>& getWriter() { return writer_; }

  FbsonErrType getErrorCode() { return err_; }

//清除错误代码
  void clearErr() { err_ = FbsonErrType::E_NONE; }

 private:
//解析JSON对象（以逗号分隔的键值对列表）
  bool parseObject(std::istream& in, hDictInsert handler) {
    if (!writer_.writeStartObject()) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    trim(in);

    if (in.peek() == '}') {
      in.ignore();
//空对象
      if (!writer_.writeEndObject()) {
        err_ = FbsonErrType::E_OUTPUT_FAIL;
        return false;
      }
      return true;
    }

    while (in.good()) {
      if (in.get() != '"') {
        err_ = FbsonErrType::E_INVALID_KEY;
        return false;
      }

      if (!parseKVPair(in, handler)) {
        return false;
      }

      trim(in);

      char ch = in.get();
      if (ch == '}') {
//对象的结尾
        if (!writer_.writeEndObject()) {
          err_ = FbsonErrType::E_OUTPUT_FAIL;
          return false;
        }
        return true;
      } else if (ch != ',') {
        err_ = FbsonErrType::E_INVALID_OBJ;
        return false;
      }

      trim(in);
    }

    err_ = FbsonErrType::E_INVALID_OBJ;
    return false;
  }

//解析JSON数组（以逗号分隔的值列表）
  bool parseArray(std::istream& in, hDictInsert handler) {
    if (!writer_.writeStartArray()) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    trim(in);

    if (in.peek() == ']') {
      in.ignore();
//空数组
      if (!writer_.writeEndArray()) {
        err_ = FbsonErrType::E_OUTPUT_FAIL;
        return false;
      }
      return true;
    }

    while (in.good()) {
      if (!parseValue(in, handler)) {
        return false;
      }

      trim(in);

      char ch = in.get();
      if (ch == ']') {
//数组的结尾
        if (!writer_.writeEndArray()) {
          err_ = FbsonErrType::E_OUTPUT_FAIL;
          return false;
        }
        return true;
      } else if (ch != ',') {
        err_ = FbsonErrType::E_INVALID_ARR;
        return false;
      }

      trim(in);
    }

    err_ = FbsonErrType::E_INVALID_ARR;
    return false;
  }

//解析键-值对，用“：”分隔
  bool parseKVPair(std::istream& in, hDictInsert handler) {
    if (parseKey(in, handler) && parseValue(in, handler)) {
      return true;
    }

    return false;
  }

//分析键（必须是字符串）
  bool parseKey(std::istream& in, hDictInsert handler) {
    char key[FbsonKeyValue::sMaxKeyLen];
    int i = 0;
    while (in.good() && in.peek() != '"' && i < FbsonKeyValue::sMaxKeyLen) {
      key[i++] = in.get();
    }

    if (!in.good() || in.peek() != '"' || i == 0) {
      err_ = FbsonErrType::E_INVALID_KEY;
      return false;
    }

in.ignore(); //抛弃“

    int key_id = -1;
    if (handler) {
      key_id = handler(key, i);
    }

    if (key_id < 0) {
      writer_.writeKey(key, i);
    } else {
      writer_.writeKey(key_id);
    }

    trim(in);

    if (in.get() != ':') {
      err_ = FbsonErrType::E_INVALID_OBJ;
      return false;
    }

    return true;
  }

//解析值
  bool parseValue(std::istream& in, hDictInsert handler) {
    bool res = false;

    trim(in);

    switch (in.peek()) {
    case 'N':
    case 'n': {
      in.ignore();
      res = parseNull(in);
      break;
    }
    case 'T':
    case 't': {
      in.ignore();
      res = parseTrue(in);
      break;
    }
    case 'F':
    case 'f': {
      in.ignore();
      res = parseFalse(in);
      break;
    }
    case '"': {
      in.ignore();
      res = parseString(in);
      break;
    }
    case '{': {
      in.ignore();
      res = parseObject(in, handler);
      break;
    }
    case '[': {
      in.ignore();
      res = parseArray(in, handler);
      break;
    }
    default: {
      res = parseNumber(in);
      break;
    }
    }

    return res;
  }

//分析空值
  bool parseNull(std::istream& in) {
    if (tolower(in.get()) == 'u' && tolower(in.get()) == 'l' &&
        tolower(in.get()) == 'l') {
      writer_.writeNull();
      return true;
    }

    err_ = FbsonErrType::E_INVALID_VALUE;
    return false;
  }

//分析真值
  bool parseTrue(std::istream& in) {
    if (tolower(in.get()) == 'r' && tolower(in.get()) == 'u' &&
        tolower(in.get()) == 'e') {
      writer_.writeBool(true);
      return true;
    }

    err_ = FbsonErrType::E_INVALID_VALUE;
    return false;
  }

//分析假值
  bool parseFalse(std::istream& in) {
    if (tolower(in.get()) == 'a' && tolower(in.get()) == 'l' &&
        tolower(in.get()) == 's' && tolower(in.get()) == 'e') {
      writer_.writeBool(false);
      return true;
    }

    err_ = FbsonErrType::E_INVALID_VALUE;
    return false;
  }

//解析字符串
  bool parseString(std::istream& in) {
    if (!writer_.writeStartString()) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    bool escaped = false;
char buffer[4096]; //一次写入4KB
    int nread = 0;
    while (in.good()) {
      char ch = in.get();
      if (ch != '"' || escaped) {
        buffer[nread++] = ch;
        if (nread == 4096) {
//刷新缓冲器
          if (!writer_.writeString(buffer, nread)) {
            err_ = FbsonErrType::E_OUTPUT_FAIL;
            return false;
          }
          nread = 0;
        }
//设置/重置退出
        if (ch == '\\' || escaped) {
          escaped = !escaped;
        }
      } else {
//在缓冲区中写入所有剩余字节
        if (nread > 0) {
          if (!writer_.writeString(buffer, nread)) {
            err_ = FbsonErrType::E_OUTPUT_FAIL;
            return false;
          }
        }
//结束写入字符串
        if (!writer_.writeEndString()) {
          err_ = FbsonErrType::E_OUTPUT_FAIL;
          return false;
        }
        return true;
      }
    }

    err_ = FbsonErrType::E_INVALID_STR;
    return false;
  }

//解析一个数字
//数字格式可以是十六进制、八进制或十进制（包括浮点）。
//只有小数才能有（+/-）符号前缀。
  bool parseNumber(std::istream& in) {
    bool ret = false;
    switch (in.peek()) {
    case '0': {
      in.ignore();

      if (in.peek() == 'x' || in.peek() == 'X') {
        in.ignore();
        ret = parseHex(in);
      } else if (in.peek() == '.') {
        in.ignore();
        ret = parseDouble(in, 0, 0, 1);
      } else {
        ret = parseOctal(in);
      }

      break;
    }
    case '-': {
      in.ignore();
      ret = parseDecimal(in, -1);
      break;
    }
    case '+':
      in.ignore();
//跌倒
    default:
      ret = parseDecimal(in, 1);
      break;
    }

    return ret;
  }

//以十六进制格式分析数字
  bool parseHex(std::istream& in) {
    uint64_t val = 0;
    int num_digits = 0;
    char ch = tolower(in.peek());
    while (in.good() && !strchr(kJsonDelim, ch) && (++num_digits) <= 16) {
      if (ch >= '0' && ch <= '9') {
        val = (val << 4) + (ch - '0');
      } else if (ch >= 'a' && ch <= 'f') {
        val = (val << 4) + (ch - 'a' + 10);
} else { //无法识别的十六进制数字
        err_ = FbsonErrType::E_INVALID_HEX;
        return false;
      }

      in.ignore();
      ch = tolower(in.peek());
    }

    int size = 0;
    if (num_digits <= 2) {
      size = writer_.writeInt8((int8_t)val);
    } else if (num_digits <= 4) {
      size = writer_.writeInt16((int16_t)val);
    } else if (num_digits <= 8) {
      size = writer_.writeInt32((int32_t)val);
    } else if (num_digits <= 16) {
      size = writer_.writeInt64(val);
    } else {
      err_ = FbsonErrType::E_HEX_OVERFLOW;
      return false;
    }

    if (size == 0) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    return true;
  }

//以八进制格式分析数字
  bool parseOctal(std::istream& in) {
    int64_t val = 0;
    char ch = in.peek();
    while (in.good() && !strchr(kJsonDelim, ch)) {
      if (ch >= '0' && ch <= '7') {
        val = val * 8 + (ch - '0');
      } else {
        err_ = FbsonErrType::E_INVALID_OCTAL;
        return false;
      }

//检查数字是否溢出
      if (val < 0) {
        err_ = FbsonErrType::E_OCTAL_OVERFLOW;
        return false;
      }

      in.ignore();
      ch = in.peek();
    }

    int size = 0;
    if (val <= std::numeric_limits<int8_t>::max()) {
      size = writer_.writeInt8((int8_t)val);
    } else if (val <= std::numeric_limits<int16_t>::max()) {
      size = writer_.writeInt16((int16_t)val);
    } else if (val <= std::numeric_limits<int32_t>::max()) {
      size = writer_.writeInt32((int32_t)val);
} else { //Val<=Int64_最大值
      size = writer_.writeInt64(val);
    }

    if (size == 0) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    return true;
  }

//解析十进制数（包括浮点数）
  bool parseDecimal(std::istream& in, int sign) {
    int64_t val = 0;
    int precision = 0;

    char ch = 0;
    while (in.good() && (ch = in.peek()) == '0')
      in.ignore();

    while (in.good() && !strchr(kJsonDelim, ch)) {
      if (ch >= '0' && ch <= '9') {
        val = val * 10 + (ch - '0');
        ++precision;
      } else if (ch == '.') {
//注意，我们不会弹出“”。
        return parseDouble(in, static_cast<double>(val), precision, sign);
      } else {
        err_ = FbsonErrType::E_INVALID_DECIMAL;
        return false;
      }

      in.ignore();

//如果数字溢出到64，首先将其解析为双iff，我们会看到
//小数点后。否则，会将其视为溢出
      if (val < 0 && val > std::numeric_limits<int64_t>::min()) {
        return parseDouble(in, static_cast<double>(val), precision, sign);
      }

      ch = in.peek();
    }

    if (sign < 0) {
      val = -val;
    }

    int size = 0;
    if (val >= std::numeric_limits<int8_t>::min() &&
        val <= std::numeric_limits<int8_t>::max()) {
      size = writer_.writeInt8((int8_t)val);
    } else if (val >= std::numeric_limits<int16_t>::min() &&
               val <= std::numeric_limits<int16_t>::max()) {
      size = writer_.writeInt16((int16_t)val);
    } else if (val >= std::numeric_limits<int32_t>::min() &&
               val <= std::numeric_limits<int32_t>::max()) {
      size = writer_.writeInt32((int32_t)val);
} else { //Val<=Int64_最大值
      size = writer_.writeInt64(val);
    }

    if (size == 0) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    return true;
  }

//分析IEEE745双精度：
//有效精度长度-15
//最大指数值-308
//
//“如果最多有15个有效数字的十进制字符串转换为
//IEEE754双精度表示，然后转换回
//具有相同有效位数的字符串，然后是最后一个字符串
//应与原件匹配“
  bool parseDouble(std::istream& in, double val, int precision, int sign) {
    int integ = precision;
    int frac = 0;
    bool is_frac = false;

    char ch = in.peek();
    if (ch == '.') {
      is_frac = true;
      in.ignore();
      ch = in.peek();
    }

    int exp = 0;
    while (in.good() && !strchr(kJsonDelim, ch)) {
      if (ch >= '0' && ch <= '9') {
        if (precision < 15) {
          val = val * 10 + (ch - '0');
          if (is_frac) {
            ++frac;
          } else {
            ++integ;
          }
          ++precision;
        } else if (!is_frac) {
          ++exp;
        }
      } else if (ch == 'e' || ch == 'E') {
        in.ignore();
        int exp2;
        if (!parseExponent(in, exp2)) {
          return false;
        }

        exp += exp2;
//检查指数是否溢出
        if (exp > 308 || exp < -308) {
          err_ = FbsonErrType::E_EXPONENT_OVERFLOW;
          return false;
        }

        is_frac = true;
        break;
      }

      in.ignore();
      ch = in.peek();
    }

    if (!is_frac) {
      err_ = FbsonErrType::E_DECIMAL_OVERFLOW;
      return false;
    }

    val *= std::pow(10, exp - frac);
    if (std::isnan(val) || std::isinf(val)) {
      err_ = FbsonErrType::E_DOUBLE_OVERFLOW;
      return false;
    }

    if (sign < 0) {
      val = -val;
    }

    if (writer_.writeDouble(val) == 0) {
      err_ = FbsonErrType::E_OUTPUT_FAIL;
      return false;
    }

    return true;
  }

//分析双精度数的指数部分
  bool parseExponent(std::istream& in, int& exp) {
    bool neg = false;

    char ch = in.peek();
    if (ch == '+') {
      in.ignore();
      ch = in.peek();
    } else if (ch == '-') {
      neg = true;
      in.ignore();
      ch = in.peek();
    }

    exp = 0;
    while (in.good() && !strchr(kJsonDelim, ch)) {
      if (ch >= '0' && ch <= '9') {
        exp = exp * 10 + (ch - '0');
      } else {
        err_ = FbsonErrType::E_INVALID_EXPONENT;
        return false;
      }

      if (exp > 308) {
        err_ = FbsonErrType::E_EXPONENT_OVERFLOW;
        return false;
      }

      in.ignore();
      ch = in.peek();
    }

    if (neg) {
      exp = -exp;
    }

    return true;
  }

  void trim(std::istream& in) {
    while (in.good() && strchr(kWhiteSpace, in.peek())) {
      in.ignore();
    }
  }

 private:
  FbsonWriterT<OS_TYPE> writer_;
  FbsonErrType err_;
};

typedef FbsonJsonParserT<FbsonOutStream> FbsonJsonParser;

} //命名空间FBSON

#endif //fbson_fbson解析器
