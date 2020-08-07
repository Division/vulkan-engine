#pragma once

#include "CommonIncludes.h"
#include "HierarchyData.h"
#include <system/Logging.h>
#include "SkinningData.h"
#include "ResourceCache.h"

class Mesh;

struct LightData {
  std::string type;
  std::string id;
  vec3 color = vec3(1, 1, 1);
  float coneAngle;

  void loadFromJSON(const nlohmann::json &jsonData);
};

struct AnimationSequence {
  std::string name;
  int startFrame = 0;
  int count = 0;
};

struct AnimationData {
  float duration = 0;
  int fps = 0;
  int frameCount = 0;
  bool hasPosition = false;
  bool hasRotation = false;
  bool hasScale = false;
  bool isMatrix = false;
  int stride = 0;
  std::string name;

  std::vector<mat4> matrices;
  std::vector<vec3> positions;
  std::vector<quat> rotations;
  std::vector<vec3> scales;
  std::vector<AnimationSequence> sequences;

  mat4 getMatrix(int frame) { return matrices[frame]; }
  vec3 getPosition(int frame) { return positions[frame]; }
  quat getRotation(int frame) { return rotations[frame]; }
  vec3 getScale(int frame) { return scales[frame]; }

  mat4 getMatrix(int frame1, int frame2, float time) {
    auto m1 = getMatrix(frame1 % frameCount);
    auto m2 = getMatrix(frame2 % frameCount);
    return m1 + (m2 - m1) * time;
  }
  vec3 getPosition(int frame1, int frame2, float time) { return glm::lerp(getPosition(frame1 % frameCount), getPosition(frame2 % frameCount), time); }
  quat getRotation(int frame1, int frame2, float time) { return glm::slerp(getRotation(frame1 % frameCount), getRotation(frame2 % frameCount), time); }
  vec3 getScale(int frame1, int frame2, float time) { return glm::lerp(getScale(frame1 % frameCount), getScale(frame2 % frameCount), time); }

  void loadFrames(std::vector<float> &frames);
  int getStride() const;
  int getElementCount() const;
  int bytesPerFrame () { return stride * 4; }

  void appendAnimationData(std::shared_ptr<AnimationData> animationData, std::string &name);

  void loadFromJSON(const nlohmann::json &jsonData);
};

class ModelBundle;
typedef std::shared_ptr<ModelBundle> ModelBundlePtr;
typedef std::shared_ptr<AnimationData> AnimationDataPtr;
typedef std::shared_ptr<HierarchyData> HierarchyDataPtr;
typedef std::shared_ptr<LightData> LightDataPtr;

class ModelBundle {
public:
  explicit ModelBundle(const std::wstring& filename);
  virtual ~ModelBundle() { ENGLog("ModelBundle destroy"); }

  Common::Handle<Mesh> getMesh(const std::string &name) const;
  const HierarchyDataPtr hierarchy() const { return _hierarchy; }
  const HierarchyDataPtr getHierarchyByName(const std::string &name) const { return _nameToHierarchy.count(name) ? _nameToHierarchy.at(name) : nullptr;}
  const HierarchyDataPtr getHierarchyByID(const std::string &id) const { return _idToHierarchy.count(id) ? _idToHierarchy.at(id) : nullptr; }

  const SkinningDataPtr getSkinning(const std::string &name) const { return _skinning.at(name); }
  const SkinningDataPtr getDefaultSkinning() const { return _skinning.begin()->second; }

  const LightDataPtr getLight(const std::string &name) const { return _lights.at(name); }

  void addMesh(std::string name, Common::Handle<Mesh> mesh);
  void addAnimationData(AnimationDataPtr animationData);
  AnimationDataPtr getAnimationData(const std::string &name) { return _animations.count(name) ? _animations.at(name) : nullptr; }

  void loadLights(const nlohmann::json &lightsData);
  void loadHiererchy(const nlohmann::json &hierarchyData);
  void loadSkinning(const nlohmann::json &skinningData);
  void appendAnimationBundle(ModelBundlePtr modelBundle, std::string name);

private:
  std::unordered_map<std::string, Common::Handle<Mesh>> _meshes; // map of meshes
  std::unordered_map<std::string, SkinningDataPtr> _skinning; // map of skin data
  std::unordered_map<std::string, AnimationDataPtr> _animations; // map of meshes
  std::unordered_map<std::string, LightDataPtr> _lights; // map of lights
  std::unordered_map<std::string, HierarchyDataPtr> _nameToHierarchy; // map scene nodes by name
  std::unordered_map<std::string, HierarchyDataPtr> _idToHierarchy; // map scene nodes by id
  std::shared_ptr<HierarchyData> _hierarchy; // root hierarchy object
};

typedef Resources::Handle<ModelBundle> ModelBundleHandle;
