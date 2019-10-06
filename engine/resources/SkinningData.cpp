//
// Created by Sidorenko Nikita on 2018-12-13.
//

#include "SkinningData.h"
#include "system/Logging.h"
#include "loader/ModelLoaderUtils.h"

void SkinningData::loadFromJSON(const std::string &name, const json &jsonData) {
  this->name = name;
  jointNames.resize(0);

  joints = std::make_shared<HierarchyData>();

  auto &jsonJointNames = jsonData["jointNames"];
  jointNames.reserve(jsonJointNames.size());
  for (auto &joint : jsonJointNames) {
    jointNames.push_back(joint.get<std::string>());
  }

  auto &jsonBindPoses = jsonData["bindPoses"];
  size_t size = jsonBindPoses.size();
  bindPoses.resize(jsonBindPoses.size());
  for (size_t i = 0; i < size; i++) {
    bindPoses[i] = loader::getMatrixFromJSON(jsonBindPoses.at(i));
  }

  joints->loadFromJSON(jsonData["joints"]);
}