//
// Created by Sidorenko Nikita on 4/8/18.
//

#ifndef CPPWRAPPER_HIERARCHYLOADER_H
#define CPPWRAPPER_HIERARCHYLOADER_H

#include "resources/ModelBundle.h"
#include "resources/HierarchyData.h"
#include "objects/MeshObject.h"
//#include <render/material/MaterialTypes.h>
#include "EngineTypes.h"
#include <memory>
#include "resources/SkinningData.h"
#include "objects/SkinnedMeshObject.h"

namespace loader {

  GameObjectPtr loadHierarchy(ModelBundlePtr bundle, const HierarchyDataPtr hierarchyToLoad);

  template <typename T>
  std::shared_ptr<T> loadSkinnedMesh(ModelBundlePtr bundle, const SkinningDataPtr skinningData = nullptr) {
    auto skinning = skinningData ? skinningData : bundle->getDefaultSkinning();
    auto skinnedMeshObject = CreateGameObject<T>();

    auto hierarchy = bundle->getHierarchyByName(skinning->name);
    if (!hierarchy) {
      throw std::runtime_error("Skinned mesh hierarchy not found");
    }

    skinnedMeshObject->transform()->setMatrix(hierarchy->transform);
    skinnedMeshObject->setSkinningData(bundle, skinning);
    skinnedMeshObject->mesh(bundle->getMesh(hierarchy->geometry));
    //skinnedMeshObject->material(std::make_shared<MaterialLighting>());

    return skinnedMeshObject;
  }

  SkinnedMeshObjectPtr loadSkinnedMesh(ModelBundlePtr bundle, const SkinningDataPtr skinningData = nullptr);
}


#endif //CPPWRAPPER_HIERARCHYLOADER_H
