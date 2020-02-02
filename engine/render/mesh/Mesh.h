#pragma once

#include "CommonIncludes.h"
#include "VertexAttrib.h"
#include "utils/Math.h"

namespace core { namespace Device{
	
	class VulkanBuffer;

} }

extern const int JOINT_PER_VERTEX_MAX;

class Mesh {
public:
  explicit Mesh(bool keepData = true, int componentCount = 3, bool isStatic = true);
  virtual ~Mesh();

  static const int JOINTS_MAX = 70;
  static const int JOINT_PER_VERTEX_MAX = 3;

  core::Device::VulkanBuffer* vertexBuffer() const { return _vertexBuffer.get(); }
  core::Device::VulkanBuffer* indexBuffer() const { return _indexBuffer.get(); }
  const core::VertexLayout& GetVertexLayout() const { return layout; }

  void setVertices(const vec3 *vertices, int vertexCount);
  void setVertices(const float *vertexComponents, int vertexCount);
  void setVertices(const std::vector<vec3> &vertices);

  void setColors(const vec4 *colors, int colorCount);
  void setColors(const float *colorComponents, int colorCount);
  void setColors(const std::vector<vec4> &colors);

  void setWeights(const std::vector<vec2> &weights);
  void setWeights(const float *weightComponents, int count);

  void setTexCoord0(const std::vector<vec2> &texcoords);
  void setTexCoord0(const float *texcoordComponents, int count);

  void setCorners(const std::vector<vec2> &corners);
  void setCorners(const float *components, int count);

  void setIndices(const uint16_t *indices, int indexCount);
  void setIndices(const std::vector<uint16_t> &indices);

  void calculateNormals();
  void calculateTBN();

  void createBuffer();

  AABB aabb() const { return _aabb; }

  // Properties

  int componentCount() const { return _componentCount; } // Number of vertices to complete the primitive (3 for triangle)
  int strideBytes() const { return _strideBytes; };
  int faceCount() const {return _faceCount; }
  int indexCount() const { return _faceCount * _componentCount; }
  int vertexCount() const { return _vertexCount; }
  vec3 getVertex(int index) const {
    if (!_hasVertices || !_keepData) { return vec3(); }
    else { return vec3(_vertices[index * 3], _vertices[index * 3 + 1], _vertices[index * 3 + 2]); };
  }
  vec4 getWeights(int index) const {
    if (!_hasWeights|| !_keepData) { return vec4(); }
    else { return vec4(_weights[index * JOINT_PER_VERTEX_MAX], _weights[index * JOINT_PER_VERTEX_MAX + 1], _weights[index * JOINT_PER_VERTEX_MAX + 2], 0); };
  }
  ivec4 getJointIndices(int index) const {
    if (!_hasWeights|| !_keepData) { return ivec4(); }
    else { return ivec4(lround(_jointIndices[index * JOINT_PER_VERTEX_MAX]),
                        lround(_jointIndices[index * JOINT_PER_VERTEX_MAX + 1]),
                        lround(_jointIndices[index * JOINT_PER_VERTEX_MAX + 2]),
                        0);
    };
  }

  bool hasVertices() const { return _hasVertices; }
  bool hasIndices() const { return _hasIndices; }
  bool hasNormals() const { return _hasNormals; }
  bool hasTBN() const { return _hasTBN; }
  bool hasTexCoord0() const { return _hasTexCoord0; }
  bool hasWeights() const { return _hasWeights; }
  bool hasColors() const { return _hasColors; }

  int vertexOffsetBytes() const { return _vertexOffsetBytes; };
  int normalOffsetBytes() const { return _normalOffsetBytes; };
  int tangentOffsetBytes() const { return _tangentOffsetBytes; };
  int bitangentOffsetBytes() const { return _bitangentOffsetBytes; };
  int texCoordOffsetBytes() const { return _texCoord0OffsetBytes; };
  int cornerOffsetBytes() const { return _cornerOffsetBytes; };
  int jointIndexOffsetBytes() const { return _jointIndexOffsetBytes; };
  int weightOffsetBytes() const { return _weightOffsetBytes; };
  int colorOffsetBytes() const { return _colorOffsetBytes; };

private:
  int _getStrideSize();
  void _deleteBuffer();
  void _updateFaceCount();
  void _prepareVAO();
  void _calculateAABB();

private:
  AABB _aabb;
  core::VertexLayout layout;

  std::unique_ptr<core::Device::VulkanBuffer> _vertexBuffer;
  std::unique_ptr<core::Device::VulkanBuffer> _indexBuffer;

  std::unique_ptr<core::Device::VulkanBuffer> last_frame_vertex_buffer;
  std::unique_ptr<core::Device::VulkanBuffer> last_frame_index_buffer;

  bool _isStatic;
  bool _keepData;
  int _componentCount;
  int _faceCount;
  int _stride;
  int _strideBytes;
  int _vertexCount;

  // Attrib flags
  bool _hasIndices;
  bool _hasVertices;
  bool _hasNormals;
  bool _hasTBN;
  bool _hasTexCoord0;
  bool _hasCorners;
  bool _hasWeights;
  bool _hasColors;

  // Offsets
  int _vertexOffset;
  int _vertexOffsetBytes;
  int _normalOffset;
  int _normalOffsetBytes;
  int _tangentOffset;
  int _tangentOffsetBytes;
  int _bitangentOffset;
  int _bitangentOffsetBytes;
  int _texCoord0Offset;
  int _texCoord0OffsetBytes;
  int _cornerOffset;
  int _cornerOffsetBytes;
  int _jointIndexOffset;
  int _jointIndexOffsetBytes;
  int _weightOffset;
  int _weightOffsetBytes;
  int _colorOffset;
  int _colorOffsetBytes;

  // Attrib data
  std::vector<uint16_t> _indices;
  std::vector<float> _vertices;
  std::vector<float> _normals;
  std::vector<float> _tangents;
  std::vector<float> _bitangents;
  std::vector<float> _texCoord0;
  std::vector<float> _corners;
  std::vector<float> _weights;
  std::vector<float> _jointIndices;
  std::vector<float> _colors;
};

typedef std::shared_ptr<Mesh> MeshPtr;
