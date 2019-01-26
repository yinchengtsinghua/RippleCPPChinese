
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

#include "utilities/geodb/geodb_impl.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <limits>
#include <map>
#include <string>
#include <vector>
#include "util/coding.h"
#include "util/filename.h"
#include "util/string_util.h"

//
//有两种类型的钥匙。第一类键值
//将地理位置映射到对象ID集及其值。
//表1
//键：P+：+$QuadKey+：+$ID+
//：+$纬度+：+$经度
//值：对象的值
//此表可用于查找位于附近的所有对象
//指定的地理位置。
//
//表2
//key:'k'+：+$id
//值：$quadkey

namespace rocksdb {

const double GeoDBImpl::PI = 3.141592653589793;
const double GeoDBImpl::EarthRadius = 6378137;
const double GeoDBImpl::MinLatitude = -85.05112878;
const double GeoDBImpl::MaxLatitude = 85.05112878;
const double GeoDBImpl::MinLongitude = -180;
const double GeoDBImpl::MaxLongitude = 180;

GeoDBImpl::GeoDBImpl(DB* db, const GeoDBOptions& options) :
  GeoDB(db, options), db_(db), options_(options) {
}

GeoDBImpl::~GeoDBImpl() {
}

Status GeoDBImpl::Insert(const GeoObject& obj) {
  WriteBatch batch;

//此ID可能已与
//有不同的位置。我们得先把那个取下来
//在插入新的关联之前。

//删除现有对象（如果存在）
  GeoObject old;
  Status status = GetById(obj.id, &old);
  if (status.ok()) {
    assert(obj.id.compare(old.id) == 0);
    std::string quadkey = PositionToQuad(old.position, Detail);
    std::string key1 = MakeKey1(old.position, old.id, quadkey);
    std::string key2 = MakeKey2(old.id);
    batch.Delete(Slice(key1));
    batch.Delete(Slice(key2));
  } else if (status.IsNotFound()) {
//如果另一个线程试图同时插入同一个ID呢？
  } else {
    return status;
  }

//插入新对象
  std::string quadkey = PositionToQuad(obj.position, Detail);
  std::string key1 = MakeKey1(obj.position, obj.id, quadkey);
  std::string key2 = MakeKey2(obj.id);
  batch.Put(Slice(key1), Slice(obj.value));
  batch.Put(Slice(key2), Slice(quadkey));
  return db_->Write(woptions_, &batch);
}

Status GeoDBImpl::GetByPosition(const GeoPosition& pos,
                                const Slice& id,
                                std::string* value) {
  std::string quadkey = PositionToQuad(pos, Detail);
  std::string key1 = MakeKey1(pos, id, quadkey);
  return db_->Get(roptions_, Slice(key1), value);
}

Status GeoDBImpl::GetById(const Slice& id, GeoObject* object) {
  Status status;
  std::string quadkey;

//创建迭代器以便获得一致的图片
//
  Iterator* iter = db_->NewIterator(roptions_);

//为表2创建键
  std::string kt = MakeKey2(id);
  Slice key2(kt);

  iter->Seek(key2);
  if (iter->Valid() && iter->status().ok()) {
    if (iter->key().compare(key2) == 0) {
      quadkey = iter->value().ToString();
    }
  }
  if (quadkey.size() == 0) {
    delete iter;
    return Status::NotFound(key2);
  }

//
//查找quadkey+id前缀
//
  std::string prefix = MakeKey1Prefix(quadkey, id);
  iter->Seek(Slice(prefix));
  assert(iter->Valid());
  if (!iter->Valid() || !iter->status().ok()) {
    delete iter;
    return Status::NotFound();
  }

//将密钥拆分为p+quadkey+id+lat+lon
  Slice key = iter->key();
  std::vector<std::string> parts = StringSplit(key.ToString(), ':');
  assert(parts.size() == 5);
  assert(parts[0] == "p");
  assert(parts[1] == quadkey);
  assert(parts[2] == id);

//填充输出参数
  object->position.latitude = atof(parts[3].c_str());
  object->position.longitude = atof(parts[4].c_str());
object->id = id.ToString();  //这是多余的
  object->value = iter->value().ToString();
  delete iter;
  return Status::OK();
}


Status GeoDBImpl::Remove(const Slice& id) {
//从数据库中读取对象
  GeoObject obj;
  Status status = GetById(id, &obj);
  if (!status.ok()) {
    return status;
  }

//通过原子地从两个表中删除对象来删除该对象
  std::string quadkey = PositionToQuad(obj.position, Detail);
  std::string key1 = MakeKey1(obj.position, obj.id, quadkey);
  std::string key2 = MakeKey2(obj.id);
  WriteBatch batch;
  batch.Delete(Slice(key1));
  batch.Delete(Slice(key2));
  return db_->Write(woptions_, &batch);
}

class GeoIteratorImpl : public GeoIterator {
 private:
  std::vector<GeoObject> values_;
  std::vector<GeoObject>::iterator iter_;
 public:
  explicit GeoIteratorImpl(std::vector<GeoObject> values)
    : values_(std::move(values)) {
    iter_ = values_.begin();
  }
  virtual void Next() override;
  virtual bool Valid() const override;
  virtual const GeoObject& geo_object() override;
  virtual Status status() const override;
};

class GeoErrorIterator : public GeoIterator {
 private:
  Status status_;
 public:
  explicit GeoErrorIterator(Status s) : status_(s) {}
  virtual void Next() override {};
  virtual bool Valid() const override { return false; }
  virtual const GeoObject& geo_object() override {
    GeoObject* g = new GeoObject();
    return *g;
  }
  virtual Status status() const override { return status_; }
};

void GeoIteratorImpl::Next() {
  assert(Valid());
  iter_++;
}

bool GeoIteratorImpl::Valid() const {
  return iter_ != values_.end();
}

const GeoObject& GeoIteratorImpl::geo_object() {
  assert(Valid());
  return *iter_;
}

Status GeoIteratorImpl::status() const {
  return Status::OK();
}

GeoIterator* GeoDBImpl::SearchRadial(const GeoPosition& pos,
  double radius,
  int number_of_values) {
  std::vector<GeoObject> values;

//收集所有绑定四键
  std::vector<std::string> qids;
  Status s = searchQuadIds(pos, radius, &qids);
  if (!s.ok()) {
    return new GeoErrorIterator(s);
  }

//创建迭代器
  Iterator* iter = db_->NewIterator(ReadOptions());

//处理每个潜在的四键
  for (std::string qid : qids) {
//用户只对这许多对象感兴趣。
    if (number_of_values == 0) {
      break;
    }

//将quadkey转换为db key前缀
    std::string dbkey = MakeQuadKeyPrefix(qid);

    for (iter->Seek(dbkey);
         number_of_values > 0 && iter->Valid() && iter->status().ok();
         iter->Next()) {
//将密钥拆分为p+quadkey+id+lat+lon
      Slice key = iter->key();
      std::vector<std::string> parts = StringSplit(key.ToString(), ':');
      assert(parts.size() == 5);
      assert(parts[0] == "p");
      std::string* quadkey = &parts[1];

//如果我们要查找的密钥是密钥的前缀
//我们从数据库中找到的，这是其中一个键
//我们正在寻找。
      auto res = std::mismatch(qid.begin(), qid.end(), quadkey->begin());
      if (res.first == qid.end()) {
        GeoPosition obj_pos(atof(parts[3].c_str()), atof(parts[4].c_str()));
        GeoObject obj(obj_pos, parts[4], iter->value().ToString());
        values.push_back(obj);
        number_of_values--;
      } else {
        break;
      }
    }
  }
  delete iter;
  return new GeoIteratorImpl(std::move(values));
}

std::string GeoDBImpl::MakeKey1(const GeoPosition& pos, Slice id,
                                std::string quadkey) {
  std::string lat = rocksdb::ToString(pos.latitude);
  std::string lon = rocksdb::ToString(pos.longitude);
  std::string key = "p:";
  key.reserve(5 + quadkey.size() + id.size() + lat.size() + lon.size());
  key.append(quadkey);
  key.append(":");
  key.append(id.ToString());
  key.append(":");
  key.append(lat);
  key.append(":");
  key.append(lon);
  return key;
}

std::string GeoDBImpl::MakeKey2(Slice id) {
  std::string key = "k:";
  key.append(id.ToString());
  return key;
}

std::string GeoDBImpl::MakeKey1Prefix(std::string quadkey,
                                      Slice id) {
  std::string key = "p:";
  key.reserve(3 + quadkey.size() + id.size());
  key.append(quadkey);
  key.append(":");
  key.append(id.ToString());
  return key;
}

std::string GeoDBImpl::MakeQuadKeyPrefix(std::string quadkey) {
  std::string key = "p:";
  key.append(quadkey);
  return key;
}

//将度转换为弧度
double GeoDBImpl::radians(double x) {
  return (x * PI) / 180;
}

//将弧度转换为度数
double GeoDBImpl::degrees(double x) {
  return (x * 180) / PI;
}

//将GPS位置转换为四坐标
std::string GeoDBImpl::PositionToQuad(const GeoPosition& pos,
                                      int levelOfDetail) {
  Pixel p = PositionToPixel(pos, levelOfDetail);
  Tile tile = PixelToTile(p);
  return TileToQuadKey(tile, levelOfDetail);
}

GeoPosition GeoDBImpl::displaceLatLon(double lat, double lon,
                                      double deltay, double deltax) {
  double dLat = deltay / EarthRadius;
  double dLon = deltax / (EarthRadius * cos(radians(lat)));
  return GeoPosition(lat + degrees(dLat),
                     lon + degrees(dLon));
}

//
//返回地球上两个位置之间的距离
//
double GeoDBImpl::distance(double lat1, double lon1,
                           double lat2, double lon2) {
  double lon = radians(lon2 - lon1);
  double lat = radians(lat2 - lat1);

  double a = (sin(lat / 2) * sin(lat / 2)) +
              cos(radians(lat1)) * cos(radians(lat2)) *
              (sin(lon / 2) * sin(lon / 2));
  double angle = 2 * atan2(sqrt(a), sqrt(1 - a));
  return angle * EarthRadius;
}

//
//返回搜索范围内的所有四键
//
Status GeoDBImpl::searchQuadIds(const GeoPosition& position,
                                double radius,
                                std::vector<std::string>* quadKeys) {
//获取搜索方块的轮廓
  GeoPosition topLeftPos = boundingTopLeft(position, radius);
  GeoPosition bottomRightPos = boundingBottomRight(position, radius);

  Pixel topLeft =  PositionToPixel(topLeftPos, Detail);
  Pixel bottomRight =  PositionToPixel(bottomRightPos, Detail);

//要查找多少个详细级别
  int numberOfTilesAtMaxDepth = static_cast<int>(std::floor((bottomRight.x - topLeft.x) / 256));
  int zoomLevelsToRise = static_cast<int>(std::floor(std::log(numberOfTilesAtMaxDepth) / std::log(2)));
  zoomLevelsToRise++;
  int levels = std::max(0, Detail - zoomLevelsToRise);

  quadKeys->push_back(PositionToQuad(GeoPosition(topLeftPos.latitude,
                                                 topLeftPos.longitude),
                                     levels));
  quadKeys->push_back(PositionToQuad(GeoPosition(topLeftPos.latitude,
                                                 bottomRightPos.longitude),
                                     levels));
  quadKeys->push_back(PositionToQuad(GeoPosition(bottomRightPos.latitude,
                                                 topLeftPos.longitude),
                                     levels));
  quadKeys->push_back(PositionToQuad(GeoPosition(bottomRightPos.latitude,
                                                 bottomRightPos.longitude),
                                     levels));
  return Status::OK();
}

//确定指定的地面分辨率（以米/像素为单位）
//纬度和详细程度。
//测量地面分辨率的纬度（度）。
//细节级别，从1（最低细节）到23（最高细节）。
//返回地面分辨率，单位为米/像素。
double GeoDBImpl::GroundResolution(double latitude, int levelOfDetail) {
  latitude = clip(latitude, MinLatitude, MaxLatitude);
  return cos(latitude * PI / 180) * 2 * PI * EarthRadius /
         MapSize(levelOfDetail);
}

//从纬度/经度wgs-84坐标转换点（度）
//以指定的细节级别转换为像素xy坐标。
GeoDBImpl::Pixel GeoDBImpl::PositionToPixel(const GeoPosition& pos,
                                            int levelOfDetail) {
  double latitude = clip(pos.latitude, MinLatitude, MaxLatitude);
  double x = (pos.longitude + 180) / 360;
  double sinLatitude = sin(latitude * PI / 180);
  double y = 0.5 - std::log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * PI);
  double mapSize = MapSize(levelOfDetail);
  double X = std::floor(clip(x * mapSize + 0.5, 0, mapSize - 1));
  double Y = std::floor(clip(y * mapSize + 0.5, 0, mapSize - 1));
  return Pixel((unsigned int)X, (unsigned int)Y);
}

GeoPosition GeoDBImpl::PixelToPosition(const Pixel& pixel, int levelOfDetail) {
  double mapSize = MapSize(levelOfDetail);
  double x = (clip(pixel.x, 0, mapSize - 1) / mapSize) - 0.5;
  double y = 0.5 - (clip(pixel.y, 0, mapSize - 1) / mapSize);
  double latitude = 90 - 360 * atan(exp(-y * 2 * PI)) / PI;
  double longitude = 360 * x;
  return GeoPosition(latitude, longitude);
}

//将像素转换为平铺
GeoDBImpl::Tile GeoDBImpl::PixelToTile(const Pixel& pixel) {
  unsigned int tileX = static_cast<unsigned int>(std::floor(pixel.x / 256));
  unsigned int tileY = static_cast<unsigned int>(std::floor(pixel.y / 256));
  return Tile(tileX, tileY);
}

GeoDBImpl::Pixel GeoDBImpl::TileToPixel(const Tile& tile) {
  unsigned int pixelX = tile.x * 256;
  unsigned int pixelY = tile.y * 256;
  return Pixel(pixelX, pixelY);
}

//将图块转换为四键
std::string GeoDBImpl::TileToQuadKey(const Tile& tile, int levelOfDetail) {
  std::stringstream quadKey;
  for (int i = levelOfDetail; i > 0; i--) {
    char digit = '0';
    int mask = 1 << (i - 1);
    if ((tile.x & mask) != 0) {
      digit++;
    }
    if ((tile.y & mask) != 0) {
      digit++;
      digit++;
    }
    quadKey << digit;
  }
  return quadKey.str();
}

//
//将QuadKey转换为平铺及其详细程度
//
void GeoDBImpl::QuadKeyToTile(std::string quadkey, Tile* tile,
                              int* levelOfDetail) {
  tile->x = tile->y = 0;
  *levelOfDetail = static_cast<int>(quadkey.size());
  const char* key = reinterpret_cast<const char*>(quadkey.c_str());
  for (int i = *levelOfDetail; i > 0; i--) {
    int mask = 1 << (i - 1);
    switch (key[*levelOfDetail - i]) {
      case '0':
        break;

      case '1':
        tile->x |= mask;
        break;

      case '2':
        tile->y |= mask;
        break;

      case '3':
        tile->x |= mask;
        tile->y |= mask;
        break;

      default:
        std::stringstream msg;
        msg << quadkey;
        msg << " Invalid QuadKey.";
        throw std::runtime_error(msg.str());
    }
  }
}
}  //命名空间rocksdb

#endif  //摇滚乐
