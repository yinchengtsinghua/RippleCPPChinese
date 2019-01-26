
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

#ifndef ROCKSDB_LITE

#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "rocksdb/utilities/geo_db.h"
#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/env.h"
#include "rocksdb/status.h"

namespace rocksdb {

//GEODB的具体实现

class GeoDBImpl : public GeoDB {
 public:
  GeoDBImpl(DB* db, const GeoDBOptions& options);
  ~GeoDBImpl();

//将GPS位置与“id”标识的关联。价值
//是与此对象关联的Blob。
  virtual Status Insert(const GeoObject& object) override;

//检索位于指定GPS的对象的值
//位置并由“id”标识。
  virtual Status GetByPosition(const GeoPosition& pos, const Slice& id,
                               std::string* value) override;

//检索由“id”标识的对象的值。这种方法
//可能比GetByPosition慢
  virtual Status GetById(const Slice& id, GeoObject* object) override;

//删除指定的对象
  virtual Status Remove(const Slice& id) override;

//返回圆半径内所有项的列表
//指定GPS位置
  virtual GeoIterator* SearchRadial(const GeoPosition& pos, double radius,
                                    int number_of_values) override;

 private:
  DB* db_;
  const GeoDBOptions options_;
  const WriteOptions woptions_;
  const ReadOptions roptions_;

//MSVC要求此静态常量的定义位于.cc文件中
//PI值
  static const double PI;

//将度转换为弧度
  static double radians(double x);

//将弧度转换为度数
  static double degrees(double x);

//捕获X和Y坐标的像素类
  class Pixel {
   public:
    unsigned int x;
    unsigned int y;
    Pixel(unsigned int a, unsigned int b) :
     x(a), y(b) {
    }
  };

//大地水准面上的瓷砖
  class Tile {
   public:
    unsigned int x;
    unsigned int y;
    Tile(unsigned int a, unsigned int b) :
     x(a), y(b) {
    }
  };

//将GPS位置转换为四坐标
  static std::string PositionToQuad(const GeoPosition& pos, int levelOfDetail);

//wgs84 via的任意恒定使用
//http://en.wikipedia.org/wiki/world_geoditic_系统
//http://mathforum.org/library/drmath/view/51832.html网站
//http://msdn.microsoft.com/en-us/library/bb259689.aspx
//http://www.tuicool.com/articles/nbre73
//
  const int Detail = 23;
//MSVC要求此静态常量的定义位于.cc文件中
  static const double EarthRadius;
  static const double MinLatitude;
  static const double MaxLatitude;
  static const double MinLongitude;
  static const double MaxLongitude;

//将数字剪辑为指定的最小值和最大值。
  static double clip(double n, double minValue, double maxValue) {
    return fmin(fmax(n, minValue), maxValue);
  }

//确定指定级别上的地图宽度和高度（像素）
//细节，从1（最低细节）到23（最高细节）。
//返回以像素为单位的地图宽度和高度。
  static unsigned int MapSize(int levelOfDetail) {
    return (unsigned int)(256 << levelOfDetail);
  }

//确定指定的地面分辨率（以米/像素为单位）
//纬度和详细程度。
//测量地面分辨率的纬度（度）。
//细节级别，从1（最低细节）到23（最高细节）。
//返回地面分辨率，单位为米/像素。
  static double GroundResolution(double latitude, int levelOfDetail);

//从纬度/经度wgs-84坐标转换点（度）
//以指定的细节级别转换为像素xy坐标。
  static Pixel PositionToPixel(const GeoPosition& pos, int levelOfDetail);

  static GeoPosition PixelToPosition(const Pixel& pixel, int levelOfDetail);

//将像素转换为平铺
  static Tile PixelToTile(const Pixel& pixel);

  static Pixel TileToPixel(const Tile& tile);

//将图块转换为四键
  static std::string TileToQuadKey(const Tile& tile, int levelOfDetail);

//将QuadKey转换为平铺及其详细程度
  static void QuadKeyToTile(std::string quadkey, Tile* tile,
                            int *levelOfDetail);

//返回地球上两个位置之间的距离
  static double distance(double lat1, double lon1,
                         double lat2, double lon2);
  static GeoPosition displaceLatLon(double lat, double lon,
                                    double deltay, double deltax);

//
//将增量应用于后返回左上角位置
//指定的位置
//
  static GeoPosition boundingTopLeft(const GeoPosition& in, double radius) {
    return displaceLatLon(in.latitude, in.longitude, -radius, -radius);
  }

//
//将增量应用于后返回右下角位置
//指定的位置
  static GeoPosition boundingBottomRight(const GeoPosition& in,
                                         double radius) {
    return displaceLatLon(in.latitude, in.longitude, radius, radius);
  }

//
//获取指定位置半径内的所有四键
//
  Status searchQuadIds(const GeoPosition& position,
                       double radius,
                       std::vector<std::string>* quadKeys);

//
//创建用于访问RockSDB表的键
//
  static std::string MakeKey1(const GeoPosition& pos,
                              Slice id,
                              std::string quadkey);
  static std::string MakeKey2(Slice id);
  static std::string MakeKey1Prefix(std::string quadkey,
                                    Slice id);
  static std::string MakeQuadKeyPrefix(std::string quadkey);
};

}  //命名空间rocksdb

#endif  //摇滚乐
