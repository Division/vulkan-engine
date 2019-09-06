//
// Created by Sidorenko Nikita on 2018-12-13.
//

#include "HierarchyData.h"
#include "system/Logging.h"
#include "loader/ModelLoaderUtils.h"

void HierarchyData::loadFromJSON(const json &jsonData) {
  name = jsonData["name"].get<std::string>();
  id = jsonData["id"].get<std::string>();
  if (jsonData.find("sid") != jsonData.end()) {
    sid = jsonData["sid"].get<std::string>();
  }

  hasGeometry = jsonData.find("geometry") != jsonData.end() && !jsonData["geometry"].empty();
  isLight = jsonData.find("light") != jsonData.end() && !jsonData["light"].empty();
  bool hasMaterial = jsonData.find("material") != jsonData.end() && !jsonData["material"].empty();
  if (hasMaterial) {
    auto materialJSON = jsonData["material"];
    material = std::make_shared<MaterialData>();
    if (materialJSON.find("diffuse") != materialJSON.end() && !materialJSON["diffuse"].empty()) {
      material->diffuse = std::make_shared<std::string>(materialJSON["diffuse"][0]["texture"].get<std::string>());
      material->diffuseUV = 0;
    }
  }
//  hasMaterial = false; // ignore material for now

  geometry = hasGeometry ? jsonData["geometry"].get<std::string>() : "";
//  material = hasMaterial ? jsonData["material"] : "";
  light = isLight ? jsonData["light"].get<std::string>() : "";

  transform = loader::getMatrixFromJSON(jsonData["transform"]);

  if (jsonData.find("originalNodeID") != jsonData.end()) {
    originalNodeID = jsonData["originalNodeID"].get<std::string>();
  }

  // No children found
  if (jsonData.find("children") == jsonData.end() || !jsonData["children"].is_array()) {
    return;
  }

  // Children found, load them
  auto jsonChildren = jsonData["children"];
  size_t size = jsonChildren.size();
  children.resize(size);
  for (size_t i = 0; i < size; i++) {
    children[i] = std::make_shared<HierarchyData>();
    children[i]->loadFromJSON(jsonChildren.at(i));
  }
}

void HierarchyData::forEachChild(bool recursive, std::function<void(std::shared_ptr<HierarchyData>)> callback) const {
  for (auto &child : children) {
    callback(child);
  }

  if (recursive) {
    for (auto &child : children) {
      child->forEachChild(true, callback);
    }
  }
}
