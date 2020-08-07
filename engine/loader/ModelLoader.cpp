//
// Created by Sidorenko Nikita on 4/5/18.
//

#include <vector>
#include <string>
#include "ModelLoader.h"
#include "resources/ModelBundle.h"
#include "nlohmann/json.hpp"
#include "system/utils.h"
#include "render/mesh/Mesh.h"

using namespace nlohmann;

const auto ATTRIB_POSITION = "POSITION";
const auto ATTRIB_NORMAL = "NORMAL";
const auto ATTRIB_TEXCOORD0 = "TEXCOORD0";
const auto ATTRIB_TEXCOORD1 = "TEXCOORD1";
const auto ATTRIB_WEIGHT = "WEIGHT";

const std::map<std::string, int> ATTRIB_COUNT = {
  { ATTRIB_POSITION, 3 },
  { ATTRIB_NORMAL, 3 },
  { ATTRIB_TEXCOORD0, 2 },
  { ATTRIB_TEXCOORD1, 2 },
  { ATTRIB_WEIGHT, 6 }
};

void loadGeometry(std::istream &stream, json geometryJson, ModelBundle* bundle);
void loadAnimation(std::istream &stream, json animationJson, ModelBundle* bundle);
void flipVertices(std::vector<float> &vertices);
void flipIndices(std::vector<uint16_t> &indices);
template <typename T> void loadArray(std::istream &stream, std::vector<T> &data, int count);

/*ModelBundleHandle loader::loadModel(const std::string &filename) {
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(filename, std::ios::in | std::ios::binary);
  return loadModel(stream, filename);
}

ModelBundlePtr loader::loadModel(std::istream &stream, const std::string &url) 
{
    ModelBundlePtr bundle = std::make_shared<ModelBundle>(url);
    loadModel(stream, *bundle);
    return bundle;
}
*/

void loader::loadModel(std::istream &stream, ModelBundle& bundle) {
    unsigned int headerSize = 0;
    stream.read((char *)&headerSize, sizeof(headerSize));

    headerSize = swap_endian<unsigned int>(headerSize);
    //  std::vector<char> headerChars;
    //  headerChars.resize(headerSize + 1);
    //  headerChars[headerSize] = 0;
    //  stream.read(&headerChars[0], headerSize);
    //  std::string headerString(&headerChars[0]);

    std::string headerString;
    headerString.resize(headerSize);
    stream.read(&headerString[0], headerSize);

    //  ENGLog("JSON: \n%i\n %s", headerString.length(), headerString.c_str());
    json header = json::parse(headerString);
    //  ENGLog("JSON:\n%s", header.dump(2).c_str());

    if (header.find("lights") != header.end()) {
        bundle.loadLights(header["lights"]);
    }

    if (header.find("hierarchy") != header.end()) {
        bundle.loadHiererchy(header["hierarchy"]);
    }

    if (header.find("geometry") != header.end()) {
        loadGeometry(stream, header["geometry"], &bundle);
    }

    if (header.find("animation") != header.end()) {
        loadAnimation(stream, header["animation"], &bundle);
    }
}

void loadGeometry(std::istream &stream, json geometryJson, ModelBundle* bundle) {
  std::vector<float> attribData;
  std::vector<unsigned short> indices;
  for (auto &geom : geometryJson) {
    Mesh::Handle mesh = Mesh::Create(true);
    int indexCount = geom["indexCount"];
    int vertexCount = geom["vertexCount"];
    auto attributes = geom["attributes"];
    std::string name = geom["name"];

    indices.resize(indexCount);
    loadArray<unsigned short>(stream, indices, indexCount);
	//flipIndices(indices);
    mesh->setIndices(indices);

    for (auto &attribID : attributes) {

      auto totalAttribCount = ATTRIB_COUNT.at(attribID) * vertexCount;
      attribData.resize(totalAttribCount);
      loadArray<float>(stream, attribData, totalAttribCount);

      if (attribID == ATTRIB_POSITION) {
		//flipVertices(attribData);
        mesh->setVertices(&attribData[0], vertexCount);
      }

      if (attribID == ATTRIB_NORMAL) {
//        mesh->setNormals(&attribData[0], vertexCount);
      }

      if (attribID == ATTRIB_TEXCOORD0) {
        mesh->setTexCoord0(&attribData[0], vertexCount);
      }

      if (attribID == ATTRIB_WEIGHT) {
        mesh->setWeights(&attribData[0], vertexCount);
      }
    }

    if (!mesh->hasNormals()) {
      mesh->calculateNormals();
    }

    if (geom.find("caps") != geom.end()) {
      auto caps = geom["caps"];
      if (caps.find("bump") != caps.end()) {
        mesh->calculateTBN();
      }
    }

    mesh->createBuffer();
    bundle->addMesh(name, mesh);
  }
}

void loadAnimation(std::istream &stream, json animationJson, ModelBundle* bundle) {
  if (animationJson.find("skinning") != animationJson.end()) {
    bundle->loadSkinning(animationJson["skinning"]);
  }

  std::vector<float> tempAnimations;

  if (animationJson.find("objects") != animationJson.end()) {
    auto &animatedObjects = animationJson["objects"];
    auto size = animatedObjects.size();

    for (int i = 0; i < size; i++) {
      auto animationData = std::make_shared<AnimationData>();
      animationData->loadFromJSON(animatedObjects.at(i));
      bundle->addAnimationData(animationData);

      tempAnimations.resize((unsigned long)animationData->getElementCount());
      loadArray<float>(stream, tempAnimations, (int)tempAnimations.size());
      animationData->loadFrames(tempAnimations);
    }
  }
}

void flipIndices(std::vector<uint16_t> &indices) {
	for (int i = 0; i < indices.size() / 3; i++) {
		int index = i * 3;
		auto temp = indices[index];
		indices[index] = indices[index + 2];
		indices[index + 2] = temp;
	}
}

void flipVertices(std::vector<float> &vertices) {
	for (int i = 0; i < vertices.size() / 3; i++) {
		int index = i * 3;
		vertices[index + 2] = -vertices[index + 2];
	}
}

template <typename T> void loadArray(std::istream &stream, std::vector<T> &data, int count) {
  stream.read((char *)&data[0], sizeof(T) * count);
  for (int i = 0; i < count; i++) {
    data[i] = swap_endian<T>(data[i]);
  }
}
