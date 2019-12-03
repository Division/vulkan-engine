//
// Created by Sidorenko Nikita on 2018-12-13.
//

#ifndef CPPWRAPPER_SKINNEDMESHOBJECT_H
#define CPPWRAPPER_SKINNEDMESHOBJECT_H

#include "MeshObject.h"
#include "render/shader/ShaderBufferStruct.h"
#include "resources/SkinningData.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class SkinnedMeshObject : public MeshObject {
public:
  SkinnedMeshObject() = default;

  void setSkinningData(ModelBundlePtr bundle, SkinningDataPtr skinningData);
  SkinningDataPtr skinningData() const { return _skinningData; };

  void render(std::function<void(core::Device::RenderOperation& rop, RenderQueue queue)> callback) override;
  void start() override;
  void postUpdate() override;

protected:
  void _processAnimations(float dt) override;
  void _debugDraw();

protected:
  GameObjectPtr _rootJoint = nullptr;
  SkinningDataPtr _skinningData;
  ShaderBufferStruct::SkinningMatrices _skinningMatrices;
  std::unordered_map<std::string, GameObjectPtr> _jointMap;
  std::vector<GameObjectPtr> _jointList;
};


#endif //CPPWRAPPER_SKINNEDMESHOBJECT_H
