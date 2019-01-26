
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#if !defined(ROCKSDB_LITE) 

#include <string>

#include "env.h"

namespace rocksdb {

class EncryptionProvider;

//返回一个env，该env在存储在磁盘上时加密数据，在
//从磁盘读取。
Env* NewEncryptedEnv(Env* base_env, EncryptionProvider* provider);

//BlockAccessCipherStream是任何
//支持块级别的随机访问（不需要来自其他块的数据）。
//例如，ctr（计数器操作模式）支持此要求。
class BlockAccessCipherStream {
    public:
      virtual ~BlockAccessCipherStream() {};

//BlockSize返回此密码流支持的每个块的大小。
      virtual size_t BlockSize() = 0;

//在文件偏移量处加密一个或多个（部分）数据块。
//数据长度以数据大小表示。
      virtual Status Encrypt(uint64_t fileOffset, char *data, size_t dataSize);

//在文件偏移量处解密一个或多个（部分）数据块。
//数据长度以数据大小表示。
      virtual Status Decrypt(uint64_t fileOffset, char *data, size_t dataSize);

    protected:
//分配传递给encryptblock/decryptblock的临时空间。
      virtual void AllocateScratch(std::string&) = 0;

//在给定的块索引处加密数据块。
//数据长度等于blockSize（）；
      virtual Status EncryptBlock(uint64_t blockIndex, char *data, char* scratch) = 0;

//在给定的块索引处解密数据块。
//数据长度等于blockSize（）；
      virtual Status DecryptBlock(uint64_t blockIndex, char *data, char* scratch) = 0;
};

//分组密码
class BlockCipher {
    public:
      virtual ~BlockCipher() {};

//BlockSize返回此密码流支持的每个块的大小。
      virtual size_t BlockSize() = 0;

//加密数据块。
//数据长度等于blockSize（）。
      virtual Status Encrypt(char *data) = 0;

//解密一块数据。
//数据长度等于blockSize（）。
      virtual Status Decrypt(char *data) = 0;
};

//使用rot13实现块密码。
//
//注意：这是blockcipher的一个示例实现，
//它被认为不安全，不应用于生产。
class ROT13BlockCipher : public BlockCipher {
    private: 
      size_t blockSize_;
    public:
      ROT13BlockCipher(size_t blockSize) 
        : blockSize_(blockSize) {}
      virtual ~ROT13BlockCipher() {};

//BlockSize返回此密码流支持的每个块的大小。
      virtual size_t BlockSize() override { return blockSize_; }

//加密数据块。
//数据长度等于blockSize（）。
      virtual Status Encrypt(char *data) override;

//解密一块数据。
//数据长度等于blockSize（）。
      virtual Status Decrypt(char *data) override;
};

//ctrcipherstream使用
//计数器操作模式。
//参见https://en.wikipedia.org/wiki/block-cipher-mode-of-operation
//
//注意：这是BlockAccessCipherStream的一个可能实现，
//它被认为适合使用。
class CTRCipherStream final : public BlockAccessCipherStream {
    private:
      BlockCipher& cipher_;
      std::string iv_;
      uint64_t initialCounter_;
    public:
      CTRCipherStream(BlockCipher& c, const char *iv, uint64_t initialCounter) 
        : cipher_(c), iv_(iv, c.BlockSize()), initialCounter_(initialCounter) {};
      virtual ~CTRCipherStream() {};

//BlockSize返回此密码流支持的每个块的大小。
      virtual size_t BlockSize() override { return cipher_.BlockSize(); }

    protected:
//分配传递给encryptblock/decryptblock的临时空间。
      virtual void AllocateScratch(std::string&) override;

//在给定的块索引处加密数据块。
//数据长度等于blockSize（）；
      virtual Status EncryptBlock(uint64_t blockIndex, char *data, char *scratch) override;

//在给定的块索引处解密数据块。
//数据长度等于blockSize（）；
      virtual Status DecryptBlock(uint64_t blockIndex, char *data, char *scratch) override;
};

//加密提供程序用于为特定文件创建密码流。
//返回的密码流将用于实际的加密/解密
//行动。
class EncryptionProvider {
 public:
    virtual ~EncryptionProvider() {};

//GetPrefixLength返回添加到每个文件的前缀的长度
//用于存储加密选项。
//为了获得最佳性能，前缀长度应该是
//A页大小。
    virtual size_t GetPrefixLength() = 0;

//CreateNewPrefix初始化了分配的前缀内存块
//一个新文件。
    virtual Status CreateNewPrefix(const std::string& fname, char *prefix, size_t prefixLength) = 0;

//createCipherstream为给定的文件创建块访问密码流
//给定名称和选项。
    virtual Status CreateCipherStream(const std::string& fname, const EnvOptions& options,
      Slice& prefix, unique_ptr<BlockAccessCipherStream>* result) = 0;
};

//此加密提供程序使用带有给定块密码的ctr密码流
//IV.
//
//注意：这是EncryptionProvider的一个可能实现，
//如果使用安全的分组密码，则认为它适合使用。
class CTREncryptionProvider : public EncryptionProvider {
    private:
      BlockCipher& cipher_;
    protected:
      const static size_t defaultPrefixLength = 4096;

 public:
      CTREncryptionProvider(BlockCipher& c) 
        : cipher_(c) {};
    virtual ~CTREncryptionProvider() {}

//GetPrefixLength返回添加到每个文件的前缀的长度
//用于存储加密选项。
//为了获得最佳性能，前缀长度应该是
//A页大小。
    virtual size_t GetPrefixLength() override;

//CreateNewPrefix初始化了分配的前缀内存块
//一个新文件。
    virtual Status CreateNewPrefix(const std::string& fname, char *prefix, size_t prefixLength) override;

//createCipherstream为给定的文件创建块访问密码流
//给定名称和选项。
    virtual Status CreateCipherStream(const std::string& fname, const EnvOptions& options,
      Slice& prefix, unique_ptr<BlockAccessCipherStream>* result) override;

  protected:
//PopulateSecretPrefixPart将数据初始化为新的前缀块
//这将被加密。此函数将以纯文本形式存储数据。
//稍后将对其进行加密（在写入磁盘之前）。
//返回空间量（从前缀开头开始）
//已初始化。
    virtual size_t PopulateSecretPrefixPart(char *prefix, size_t prefixLength, size_t blockSize);

//createCipherstreamFromPrefix为给定的文件创建块访问密码流
//给定名称和选项。给定的前缀已被解密。
    virtual Status CreateCipherStreamFromPrefix(const std::string& fname, const EnvOptions& options,
      uint64_t initialCounter, const Slice& iv, const Slice& prefix, unique_ptr<BlockAccessCipherStream>* result);
};

}  //命名空间rocksdb

#endif  //！已定义（RocksDB-Lite）
