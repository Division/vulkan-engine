#pragma once 

#include "CommonIncludes.h"
#include "Engine.h"
#include "resources/ModelBundle.h"
#include "resources/HierarchyData.h"
#include "render/mesh/Mesh.h"

class GameObject;
class MeshObject;

namespace core { namespace Device {

	class Texture;

} }

namespace game {

	using namespace core::Device;

	class Level {
	public:
		Level(Scene* scene /*SpriteSheetPtr &decalsSpritesheet, std::shared_ptr<Texture> &decalsTexture*/);
		void load(std::string filename);

	private:
		std::shared_ptr<GameObject> _loadHierarchy(HierarchyDataPtr hierarchy, const std::shared_ptr<GameObject> parent);

		std::shared_ptr<GameObject> _levelRoot;
		Scene* _scene;
		//SpriteSheetPtr _decalsSpritesheet;
		ModelBundlePtr _architecture;
		ModelBundlePtr _props;
		ModelBundlePtr _level;
		std::shared_ptr<Texture> _decalsTexture;
		std::shared_ptr<Texture> _flareTexture;

		std::unordered_map<std::string, std::shared_ptr<Texture>> _textures;

	private:
		std::string _getDecalName(const std::string &objectName);
		void _setSphereBounds(std::shared_ptr<GameObject> &object);
		std::shared_ptr<GameObject> _createLight(HierarchyDataPtr &child);
		std::shared_ptr<GameObject> _createProjector(HierarchyDataPtr &child);
		std::shared_ptr<MeshObject> _createMeshObject(ModelBundlePtr &bundle, HierarchyDataPtr &referenceNode, std::shared_ptr<GameObject> &parent);
		void _assignLayer(std::shared_ptr<GameObject> &parent);
	};

}
