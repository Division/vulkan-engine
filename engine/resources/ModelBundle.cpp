//
// Created by Sidorenko Nikita on 4/6/18.
//

#include "ModelBundle.h"
#include "system/Logging.h"
#include "loader/ModelLoaderUtils.h"
#include "render/mesh/Mesh.h"
#include "loader/ModelLoader.h"

using namespace nlohmann;

void LightData::loadFromJSON(const json &jsonData) {
  type = jsonData["type"].get<std::string>();
  id = jsonData["id"].get<std::string>();
  if (jsonData.find("color") != jsonData.end()) {
    color = vec3(jsonData["color"][0], jsonData["color"][1], jsonData["color"][2]);
  }

  if (type == "spot") {
    coneAngle = jsonData["coneAngle"];
  }
}

void AnimationData::loadFromJSON(const json &jsonData) {
  duration = jsonData["duration"].get<float>();
  fps = jsonData["fps"].get<int>();
  frameCount = jsonData["frameCount"].get<int>();
  name = jsonData["name"].get<std::string>();

  hasPosition = jsonData["hasPosition"].get<bool>();
  hasRotation = jsonData["hasRotation"].get<bool>();
  hasScale = jsonData["hasScale"].get<bool>();
  isMatrix = jsonData["isMatrix"].get<bool>();

  stride = getStride();
}

int AnimationData::getStride() const {
  int result = 0;
  if (hasPosition) result += 3;
  if (hasRotation) result += 4;
  if (hasScale) result += 3;
  if (isMatrix) result += 16;

  return result;
}

int AnimationData::getElementCount() const {
  if (isMatrix) {
    return frameCount * 16;
  } else {
    int result = 0;
    if (hasPosition) result += frameCount * 3;
    if (hasRotation) result += frameCount * 4;
    if (hasScale) result += frameCount * 3;
    return result;
  }
}

void AnimationData::loadFrames(std::vector<float> &frames) {
  if (isMatrix) {
    matrices.resize(frameCount);
    memcpy(&matrices[0], &frames[0], sizeof(mat4) * frameCount);
  } else {
    int addedCount = 0;
    if (hasPosition) {
      positions.resize(frameCount);
      memcpy(&positions[0], &frames[addedCount], sizeof(vec3) * frameCount);
      addedCount += sizeof(vec3) * frameCount / sizeof(float);
    }
    if (hasRotation) {
      rotations.resize(frameCount);
      memcpy(&rotations[0], &frames[addedCount], sizeof(quat) * frameCount);
      addedCount += sizeof(quat) * frameCount / sizeof(float);
    }
    if (hasScale) {
      scales.resize(frameCount);
      memcpy(&scales[0], &frames[addedCount], sizeof(vec3) * frameCount);
    }
  }
}

void AnimationData::appendAnimationData(std::shared_ptr<AnimationData> animationData, std::string &name) {
  sequences.emplace_back(AnimationSequence{ name, frameCount, animationData->frameCount });

  duration += animationData->duration;
  frameCount += animationData->frameCount;

  if (hasPosition) {
    positions.insert(positions.end(), animationData->positions.begin(), animationData->positions.end());
  }
  if (hasRotation) {
    rotations.insert(rotations.end(), animationData->rotations.begin(), animationData->rotations.end());
  }
  if (hasScale) {
    scales.insert(scales.end(), animationData->scales.begin(), animationData->scales.end());
  }
  if (isMatrix) {
    matrices.insert(matrices.end(), animationData->matrices.begin(), animationData->matrices.end());
  }
}

Common::Handle<Mesh> ModelBundle::getMesh(const std::string &name) const {
    Common::Handle<Mesh> result;

    auto it = _meshes.find(name);
    if (it != _meshes.end()) {
        result = it->second;
    } else {
        ENGLog("Can't find bundle '%s'", name.c_str());
    }

  return result;
}

ModelBundle::ModelBundle(const std::wstring& filename) 
    : _hierarchy(std::make_shared<HierarchyData>())
{
    std::ifstream stream(filename, std::ios::binary);
    loader::loadModel(stream, *this);
}

void ModelBundle::loadLights(const json &lightsData) {
  for (auto &light : lightsData) {
    auto lightData = std::make_shared<LightData>();
    lightData->loadFromJSON(light);
    _lights[lightData->id] = lightData;
  }
}

void ModelBundle::loadHiererchy(const json &hierarchyData) {
  _hierarchy->loadFromJSON(hierarchyData);

  auto addHierarchy = [&](HierarchyDataPtr hierarchy) {
    _nameToHierarchy[hierarchy->name] = hierarchy;
    _idToHierarchy[hierarchy->id] = hierarchy;
  };

  _hierarchy->forEachChild(true, addHierarchy);
  addHierarchy(_hierarchy);
}

void ModelBundle::addMesh(std::string name, Common::Handle<Mesh> mesh) {
  _meshes[name] = mesh;
}

void ModelBundle::addAnimationData(AnimationDataPtr animationData) {
  _animations[animationData->name] = animationData;
}

void ModelBundle::loadSkinning(const json &skinningData) {
  for (auto it = skinningData.begin(); it != skinningData.end(); it++) {
    auto skin = std::make_shared<SkinningData>();
    skin->loadFromJSON(it.key(), it.value());
    _skinning[it.key()] = skin;
  }
}

void ModelBundle::appendAnimationBundle(ModelBundlePtr modelBundle, std::string name) {
  for (auto &iterator: modelBundle->_animations) {
    auto &srcAnim = iterator.second;
    AnimationDataPtr anim;
    if (!_animations.count(iterator.first)) {
      anim = std::make_shared<AnimationData>();
      _animations[iterator.first] = anim;
      anim->name = srcAnim->name;
      anim->fps = srcAnim->fps;
      anim->hasPosition = srcAnim->hasPosition;
      anim->hasRotation = srcAnim->hasRotation;
      anim->hasScale = srcAnim->hasScale;
      anim->isMatrix = srcAnim->isMatrix;
      anim->stride = srcAnim->stride;
      anim->name = srcAnim->name;
    } else {
      anim = _animations[iterator.first];
    }

    anim->appendAnimationData(srcAnim, name);
  }
}
