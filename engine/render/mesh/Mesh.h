#pragma once

#include "CommonIncludes.h"
#include "VertexAttrib.h"
#include "utils/Math.h"
#include <gsl/span>

namespace Device
{
	class VulkanBuffer;
}

extern const int JOINT_PER_VERTEX_MAX;

class Mesh : public Common::Resource {
public:
    static constexpr uint32_t MESH_FLAG_HAS_NORMALS = 1 << 0;
    static constexpr uint32_t MESH_FLAG_HAS_TBN = 1 << 1;
    static constexpr uint32_t MESH_FLAG_HAS_UV0 = 1 << 2;
    static constexpr uint32_t MESH_FLAG_HAS_WEIGHTS = 1 << 3;

  static const int JOINTS_MAX = 70;
  static const int JOINT_PER_VERTEX_MAX = 4;

  static constexpr bool IsShortIndexCount(uint32_t count)
  {
      return count <= (std::numeric_limits<uint16_t>::max)();
  }

  class Layout
  {
      VertexLayout layout;
  public:
      Layout(VertexLayout layout) : layout(layout) {}
      bool HasPosition() const { return layout.HasAttrib(VertexAttrib::Position); }
      vec3& GetPosition(uint8_t* data, size_t index) const { return (vec3&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::Position)]; }
      bool HasNormal() const { return layout.HasAttrib(VertexAttrib::Normal); }
      Vector4_A2R10G10B10& GetNormal(uint8_t* data, size_t index) const { return (Vector4_A2R10G10B10&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::Normal)]; }
      bool HasTangent() const { return layout.HasAttrib(VertexAttrib::Tangent); }
      Vector4_A2R10G10B10& GetTangent(uint8_t* data, size_t index) const { return (Vector4_A2R10G10B10&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::Tangent)]; }
      bool HasCorner() const { return layout.HasAttrib(VertexAttrib::Corner); }
      vec2& GetCorner(uint8_t* data, size_t index) const { return (vec2&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::Corner)]; }
      bool HasColor() const { return layout.HasAttrib(VertexAttrib::VertexColor); }
      vec4& GetColor(uint8_t* data, size_t index) const { return (vec4&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::VertexColor)]; }
      bool HasUV0() const { return layout.HasAttrib(VertexAttrib::TexCoord0); }
      Vector2Half& GetUV0(uint8_t* data, size_t index) const { return (Vector2Half&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::TexCoord0)]; }
      bool HasIndices() const { return layout.HasAttrib(VertexAttrib::JointIndices); }
      Vector4b& GetIndices(uint8_t* data, size_t index) const { return (Vector4b&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::JointIndices)]; }
      bool HasWeights() const { return layout.HasAttrib(VertexAttrib::JointWeights); }
      Vector4b& GetWeights(uint8_t* data, size_t index) const { return (Vector4b&)data[index * layout.GetStride() + layout.GetAttribOffset(VertexAttrib::JointWeights)]; }
  };

  using Handle = Common::Handle<Mesh>;

  explicit Mesh(bool keepData = true, int componentCount = 3, bool isStatic = true);
  Mesh(uint32_t flags, uint8_t* vertices, uint32_t vertex_count, uint8_t* indices, uint32_t triangle_count, AABB aabb, bool keep_data = false);
  virtual ~Mesh();

  static size_t GetVertexStride(uint32_t flags);
  static Handle Create(bool keepData = true, int componentCount = 3, bool isStatic = true);
  static Handle Create(uint32_t flags, uint8_t* vertices, uint32_t vertex_count, uint8_t* indices, uint32_t triangle_count, AABB aabb, bool keep_data = false);

  uint32_t GetFlags() const { return flags; }

  Device::VulkanBuffer* vertexBuffer() const { return _vertexBuffer.get(); }
  Device::VulkanBuffer* indexBuffer() const { return _indexBuffer.get(); }
  bool UsesShortIndexes() const { return uses_short_indices; }

  const VertexLayout& GetVertexLayout() const { return layout; }

  const gsl::span<const uint8_t> GetVertexData() const { return gsl::make_span(vertex_data); }
  const gsl::span<const uint8_t> GetIndexData() const { return gsl::make_span(index_data); }

  void SetBoneRemap(const uint16_t* indices, int bone_count);
  uint16_t GetBoneRemapIndex(uint16_t mesh_bone_index) const { return bone_remap[mesh_bone_index]; }
  uint16_t GetBoneCount() const { return bone_remap.size(); }

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
    if (!_hasVertices || !_keepData) { 
        return vec3(); 
    }
    else { 
        return vec3(_vertices[index * 3], _vertices[index * 3 + 1], _vertices[index * 3 + 2]); 
    };
  }
  vec4 getWeights(int index) const {
    if (!_hasWeights|| !_keepData) { 
        return vec4(); 
    }
    else { 
        return _weights[index].ToNormalizedFloat();
    };
  }
  Vector4b getJointIndices(int index) const {
    if (!_hasWeights|| !_keepData) { 
        return Vector4b(0, 0, 0, 0); }
    else { 
        return _jointIndices[index];
    };
  }

  ivec4 GetSkeletonMappedJointIndices(int index) const {
      if (!_hasWeights || !_keepData) { 
          return ivec4(0, 0, 0, 0); 
      }
      else {
          return ivec4(GetBoneRemapIndex(_jointIndices[index].x), GetBoneRemapIndex(_jointIndices[index].y), GetBoneRemapIndex(_jointIndices[index].z), GetBoneRemapIndex(_jointIndices[index].w));
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
  uint32_t flags = 0;

  AABB _aabb;
  VertexLayout layout;

  Device::Handle<Device::VulkanBuffer> _vertexBuffer;
  Device::Handle<Device::VulkanBuffer> _indexBuffer;

  Device::Handle<Device::VulkanBuffer> last_frame_vertex_buffer;
  Device::Handle<Device::VulkanBuffer> last_frame_index_buffer;

  bool _isStatic;
  bool _keepData;
  int _componentCount;
  int _faceCount;
  int _stride;
  int _strideBytes;
  int _vertexCount;

  bool uses_short_indices;

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

  std::vector<uint16_t> bone_remap;
  std::vector<uint8_t> vertex_data;
  std::vector<uint8_t> index_data;

  // Attrib data
  std::vector<uint16_t> _indices;
  std::vector<float> _vertices;
  std::vector<float> _normals;
  std::vector<float> _tangents;
  std::vector<Vector2Half> _texCoord0;
  std::vector<float> _corners;
  std::vector<Vector4b> _weights;
  std::vector<Vector4b> _jointIndices;
  std::vector<float> _colors;
};

