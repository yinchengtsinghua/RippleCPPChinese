
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//Vinnie Falco为libprotobuf创建的Unity构建文件<vinnie.falco@gmail.com>
//

#include "google/protobuf/descriptor.cc"
#include "google/protobuf/descriptor.pb.cc"
#include "google/protobuf/descriptor_database.cc"
#include "google/protobuf/dynamic_message.cc"
#include "google/protobuf/extension_set.cc"
#include "google/protobuf/extension_set_heavy.cc"
#include "google/protobuf/generated_message_reflection.cc"
#include "google/protobuf/generated_message_util.cc"
#include "google/protobuf/message.cc"
#include "google/protobuf/message_lite.cc"
#include "google/protobuf/reflection_ops.cc"
#include "google/protobuf/repeated_field.cc"
#include "google/protobuf/service.cc"
#include "google/protobuf/text_format.cc"
#include "google/protobuf/unknown_field_set.cc"
#include "google/protobuf/wire_format.cc"
#include "google/protobuf/wire_format_lite.cc"

#include "google/protobuf/compiler/importer.cc"
#include "google/protobuf/compiler/parser.cc"

#include "google/protobuf/io/coded_stream.cc"
#include "google/protobuf/io/gzip_stream.cc"
#include "google/protobuf/io/tokenizer.cc"
#include "google/protobuf/io/zero_copy_stream.cc"
#include "google/protobuf/io/zero_copy_stream_impl.cc"
#include "google/protobuf/io/zero_copy_stream_impl_lite.cc"

#include "google/protobuf/stubs/common.cc"
#include "google/protobuf/stubs/once.cc"
#include "google/protobuf/stubs/stringprintf.cc"
#include "google/protobuf/stubs/structurally_valid.cc"
#include "google/protobuf/stubs/strutil.cc"
#include "google/protobuf/stubs/substitute.cc"

#ifdef _MSC_VER //MSVC
#include "google/protobuf/stubs/atomicops_internals_x86_msvc.cc"
#endif

