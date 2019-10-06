#pragma once

#include "CommonIncludes.h"

#include "nlohmann/json.hpp"
#include "HierarchyData.h"

using namespace nlohmann;

struct SkinningData {
  std::string name;
  std::vector<std::string> jointNames;
  std::vector<mat4> bindPoses;
  HierarchyDataPtr joints;

  void loadFromJSON(const std::string &name, const json &jsonData);
};

typedef std::shared_ptr<SkinningData> SkinningDataPtr;

