#include "Level.h"
#include "loader/HierarchyLoader.h"
#include "loader/ModelLoader.h"
#include "loader/TextureLoader.h"
#include "scene/GameObject.h"
#include "system/Logging.h"
#include "objects/LightObject.h"
#include "objects/Projector.h"
//#include "../objects/StainGlass.h"
//#include "render/texture/SpriteSheet.h"

namespace game {

	const unsigned int LAYER_DEFAULT = 1 << 0;
	const unsigned int LAYER_WINDOW = 1 << 5;
	const unsigned int LAYER_ROOM1 = 1 << 10;
	const unsigned int LAYER_ROOM2 = 1 << 11;
	const unsigned int LAYER_ROOM3 = 1 << 12;
	//const unsigned int LAYER_ALL_ROOMS = LAYER_ROOM1 | LAYER_ROOM2;
	const unsigned int LAYER_FLOOR = 1 << 9;

	const std::string ATLAS_ARCHITECTURE = "resources/level/Atlas_Base_01.jpg";
	const std::string ATLAS_ARCHITECTURE_NORMALMAP = "resources/level/Atlas_Base_01_Normal.jpg";
	const std::string ATLAS_FURNITURE = "resources/level/Atlas_Furniture_01.jpg";
	const std::string ATLAS_FURNITURE_NORMALMAP = "resources/level/Atlas_Furniture_01_Normal.jpg";
	const std::string ATLAS_PROPS1 = "resources/level/Atlas_Props_01.jpg";
	const std::string ATLAS_PROPS1_NORMALMAP = "resources/level/Atlas_Props_01_Normal.jpg";
	const std::string ATLAS_PROPS2 = "resources/level/Atlas_Props_02.jpg";
	const std::string ATLAS_PROPS2_NORMALMAP = "resources/level/Atlas_Props_02_Normal.jpg";
	const std::string ARCHITECTURE_FILE = "resources/level/architecture.mdl";
	const std::string PROPS_FILE = "resources/level/props.mdl";
	const std::string NO_SHADOW = "no_shadow";

	int lightIndex = 0;

	Level::Level(Scene* scene /*SpriteSheetPtr& decalsSpritesheet, std::shared_ptr<Texture>& decalsTexture*/)
		: _scene(scene)/*, _decalsSpritesheet(decalsSpritesheet), _decalsTexture(decalsTexture) */ {
		_levelRoot = CreateGameObject<GameObject>();
		_architecture = loader::loadModel(ARCHITECTURE_FILE);
		_props = loader::loadModel(PROPS_FILE);
		_textures[ATLAS_ARCHITECTURE] = loader::LoadTexture(ATLAS_ARCHITECTURE);
		_textures[ATLAS_ARCHITECTURE_NORMALMAP] = loader::LoadTexture(ATLAS_ARCHITECTURE_NORMALMAP, false);
		_textures[ATLAS_FURNITURE] = loader::LoadTexture(ATLAS_FURNITURE);
		_textures[ATLAS_FURNITURE_NORMALMAP] = loader::LoadTexture(ATLAS_FURNITURE_NORMALMAP, false);
		_textures[ATLAS_PROPS1] = loader::LoadTexture(ATLAS_PROPS1);
		_textures[ATLAS_PROPS1_NORMALMAP] = loader::LoadTexture(ATLAS_PROPS1_NORMALMAP, false);
		_textures[ATLAS_PROPS2] = loader::LoadTexture(ATLAS_PROPS2);
		_textures[ATLAS_PROPS2_NORMALMAP] = loader::LoadTexture(ATLAS_PROPS2_NORMALMAP, false);
		_flareTexture = loader::LoadTexture("resources/common/flare_1.png");
	}

	std::string Level::_getDecalName(const std::string& objectName) {
		std::string decalName = objectName;
		decalName.erase(0, decalName.find('_') + 1);
		return decalName;
	}

	std::shared_ptr<GameObject> Level::_createLight(HierarchyDataPtr& child) {
		auto lightData = _level->getLight(child->light);

		auto light = CreateGameObject<LightObject>();
		light->type(lightData->type == "spot" ? LightObjectType::Spot : LightObjectType::Point);

		if (child->name.find(NO_SHADOW) == std::string::npos) {
			light->castShadows(true);
		}

		light->color(lightData->color);

		if (light->type() == LightObjectType::Spot) {
			light->coneAngle(lightData->coneAngle);
		}
		else {
			light->coneAngle(160);
		}

		light->radius(10);
		light->attenuation(1.2f);
		light->setFlare(_flareTexture, 1);
		//  light->enableDebug();

		return light;
	}

	std::shared_ptr<GameObject> Level::_createProjector(HierarchyDataPtr& child) {
		auto projector = CreateGameObject<Projector>();
		projector->transform()->setMatrix(child->transform);
		vec3 scale = projector->transform()->scale();
		projector->transform()->scale(vec3(1));

		projector->type(ProjectorType::Projector);
		projector->zFar(fabs(scale.z));
		projector->aspect(scale.x / scale.y);

		projector->zNear(0.005f);
		projector->attenuation(0.0, 0.0);
		projector->orthographicSize(scale.y);
		projector->color(vec4(2, 2, 2, 1));
		//  projector->setDebugEnabled(true);
		projector->isOrthographic(true);
		projector->castShadows(true);
		//auto bounds = _decalsSpritesheet->getSpriteData(_getDecalName(child->name)).bounds;
		//projector->spriteBounds(bounds);

		return nullptr;
	}

	std::shared_ptr<MeshObject> Level::_createMeshObject(ModelBundlePtr& bundle, HierarchyDataPtr& referenceNode, std::shared_ptr<GameObject>& parent) {
		std::shared_ptr<MeshObject> meshObject;
		auto& name = parent->name();

		/*  if (name.find("Stain") != std::string::npos) {
			auto mesh = CreateGameObject<StainGlass>();
			mesh->setup(_decalsSpritesheet, _decalsTexture, _getDecalName(name));
			mesh->layer(LAYER_WINDOW);
			meshObject = mesh;
		  } else */ {
			auto mesh = CreateGameObject<MeshObject>();

			auto material = std::make_shared<Material>();
			material->texture0(_textures.at(*referenceNode->material->diffuse));
			auto normalMapName = *referenceNode->material->diffuse;
			normalMapName.replace(normalMapName.length() - 4, 0, "_Normal");
			material->normalMap(_textures.at(normalMapName));
			mesh->material(material);
			meshObject = mesh;
		}

		meshObject->mesh(bundle->getMesh(referenceNode->geometry));
		/*if (!meshObject->mesh()->hasTBN()) {
			meshObject->mesh()->calculateTBN();
			meshObject->mesh()->createBuffer();
		}*/

		return meshObject;
	}

	void Level::_assignLayer(std::shared_ptr<GameObject>& parent) {
		unsigned int currentLayer = LAYER_ROOM2;

		auto callback = [&](TransformPtr transform) {
			const auto& object = transform->gameObject();
			object->layer(currentLayer);

			if (auto light = std::dynamic_pointer_cast<LightObject>(object)) {
				light->cameraVisibilityMask(currentLayer | LAYER_DEFAULT);
			}
			else
				if (auto projector = std::dynamic_pointer_cast<Projector>(object)) {
					projector->cameraVisibilityMask(currentLayer | LAYER_DEFAULT);
				}

			if (object->name().find("Floor") != std::string::npos) {
				object->layer(object->layer() | LAYER_FLOOR);
			}
		};

		if (parent->name() == "room1") {
			currentLayer = LAYER_ROOM1;
			parent->transform()->forEachChild(true, callback);
		}
		else if (parent->name() == "room2") {
			currentLayer = LAYER_ROOM2;
			parent->transform()->forEachChild(true, callback);
		}
		else if (parent->name() == "room3") {
			currentLayer = LAYER_ROOM3;
			parent->transform()->forEachChild(true, callback);
		}

	}

	void Level::_setSphereBounds(std::shared_ptr<GameObject>& object) {
		CullingData data;
		data.type = CullingData::Type::Sphere;

		auto mesh = std::dynamic_pointer_cast<MeshObject>(object)->mesh();

		vec3 scale = object->transform()->parent()->scale();
		vec3 size = (mesh->aabb().max - mesh->aabb().min) * scale;
		vec3 halfSize = size / 2.0f;
		data.sphere.radius = glm::length(halfSize);
		data.sphere.position = object->transform()->worldPosition() + halfSize + mesh->aabb().min * scale;

		object->cullingData(data);
	}

	std::shared_ptr<GameObject> Level::_loadHierarchy(HierarchyDataPtr hierarchy, const std::shared_ptr<GameObject> parentObj) {

		auto parent = parentObj ? parentObj : _levelRoot;
		for (auto& child : hierarchy->children) {
			std::shared_ptr<GameObject> object;
			HierarchyDataPtr arch = _architecture->getHierarchyByName(child->originalNodeID);
			HierarchyDataPtr prop = _props->getHierarchyByName(child->originalNodeID);
			HierarchyDataPtr referenceNode;
			ModelBundlePtr bundle;

			if (arch) {
				referenceNode = arch;
				bundle = _architecture;
			}
			else if (prop) {
				referenceNode = prop;
				bundle = _props;
			}

			bool setSphereBounds = false;
			if (child->isLight) {
				object = _createLight(child);
			}
			else if (referenceNode && referenceNode->hasGeometry) {
				object = _createMeshObject(bundle, referenceNode, parent);
				setSphereBounds = (bool)arch; // Architecture has sphere bounds
			}
			else if (child->name.find("Projector") != std::string::npos) {
				object = _createProjector(child);
			}
			else {
				object = CreateGameObject<GameObject>();
			}

			if (object) {
				object->transform()->parent(parent->transform());
				object->transform()->setMatrix(child->transform);
				object->name(child->name);

				_loadHierarchy(child, object);

				if (setSphereBounds) {
					_setSphereBounds(object);
				}
			}
		}

		_assignLayer(parent);
		return parent;
	}

	void Level::load(std::string fileName) {
		_level = loader::loadModel(fileName);
		_loadHierarchy(_level->hierarchy(), nullptr);
	}

}
