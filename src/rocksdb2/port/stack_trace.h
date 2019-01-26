
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
namespace rocksdb {
namespace port {

//安装信号处理程序以在以下信号上打印调用堆栈：
//信号总线
//目前仅支持Linux。否则不得操作。
void InstallStackTraceHandler();

//打印堆栈，跳过“跳过第一帧”
void PrintStack(int first_frames_to_skip = 0);

}  //命名空间端口
}  //命名空间rocksdb
