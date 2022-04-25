//
// Created by Sidorenko Nikita on 2/15/18.
//

#include "Mesh.h"
#include "Engine.h"
#include <iostream>
#include "system/Logging.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/device/Types.h"
#include "utils/Math.h"

namespace
{
    const int JOINT_PER_VERTEX_MAX = Mesh::JOINT_PER_VERTEX_MAX;

    const int VERTEX_SIZE = 3 * 4;
    const Device::Format VERTEX_FORMAT = Device::Format::R32G32B32_float;

    const int NORMAL_SIZE = 4;
    const Device::Format NORMAL_FORMAT = Device::Format::A2R10G10B10_snorm;

    const int TEXCOORD_SIZE = 2 * 2;
    const Device::Format TEXCOORD_FORMAT = Device::Format::R16G16_float;

    const int CORNER_SIZE = 2 * 4;
    const Device::Format CORNER_FORMAT = Device::Format::R32G32_float;

    const int JOINT_INDEX_SIZE = 4;
    const Device::Format JOINT_INDEX_FORMAT = Device::Format::R8G8B8A8_uint;

    const int WEIGHT_SIZE = 4;
    const Device::Format WEIGHTS_FORMAT = Device::Format::R8G8B8A8_unorm;

    const int COLOR_SIZE = 4 * 4;
    const Device::Format COLOR_FORMAT = Device::Format::R32G32B32A32_float;

    const int ORIGIN_SIZE = 3 * 4;
    const Device::Format ORIGIN_FORMAT = Device::Format::R32G32B32_float;

    uint32_t GetIndexSize(bool is_short_index)
    {
        return is_short_index ? sizeof(uint16_t) : sizeof(uint32_t);
    }

    uint32_t GetIndex(uint32_t index, gsl::span<const uint8_t> indices, bool is_short_index)
    {
        const uint8_t* ptr = &indices[(size_t)index * GetIndexSize(is_short_index)];
        return is_short_index ? reinterpret_cast<const uint16_t&>(*ptr) : reinterpret_cast<const uint32_t&>(*ptr);
    }

}

Mesh::Layout::Layout(uint32_t flags)
{
    layout.AddAttrib(VertexAttrib::Position, VERTEX_FORMAT, VERTEX_SIZE);

    if (flags & MESH_FLAG_HAS_NORMALS)
        layout.AddAttrib(VertexAttrib::Normal, NORMAL_FORMAT, NORMAL_SIZE);

    if (flags & MESH_FLAG_HAS_TBN)
    {
        layout.AddAttrib(VertexAttrib::Tangent, NORMAL_FORMAT, NORMAL_SIZE);
    }

    if (flags & MESH_FLAG_HAS_UV0)
        layout.AddAttrib(VertexAttrib::TexCoord0, TEXCOORD_FORMAT, TEXCOORD_SIZE);

    if (flags & MESH_FLAG_HAS_WEIGHTS)
    {
        layout.AddAttrib(VertexAttrib::JointIndices, JOINT_INDEX_FORMAT, JOINT_INDEX_SIZE);
        layout.AddAttrib(VertexAttrib::JointWeights, WEIGHTS_FORMAT, WEIGHT_SIZE);
    }

    if (flags & MESH_FLAG_HAS_ORIGIN)
        layout.AddAttrib(VertexAttrib::Origin, ORIGIN_FORMAT, ORIGIN_SIZE);
}

Mesh::Handle Mesh::Create(bool keepData, int componentCount, bool isStatic)
{
    return Mesh::Handle(std::make_unique<Mesh>(keepData, componentCount, isStatic));
}

Mesh::Handle Mesh::Create(uint32_t flags, uint8_t* vertices, uint32_t vertex_count, uint8_t* indices, uint32_t triangle_count, AABB aabb, bool keep_data, const std::string& debug_name)
{
    return Mesh::Handle(std::make_unique<Mesh>(flags, vertices, vertex_count, indices, triangle_count, aabb, keep_data, debug_name));
}

Mesh::Mesh(bool keepData, int componentCount, bool isStatic, const std::string& debug_name)
    : _keepData(keepData)
    , _componentCount(componentCount)
    , _isStatic(isStatic)
    , debug_name(debug_name)
{
}

Mesh::Mesh(uint32_t flags, uint8_t* vertices, uint32_t vertex_count, uint8_t* indices, uint32_t triangle_count, AABB aabb, bool keep_data, const std::string& debug_name)
    : Mesh(keep_data, 3, true, debug_name)
{
    this->flags = flags;
    this->_aabb = aabb;

    _strideBytes = GetVertexStride(flags);
    _stride = _strideBytes / 4;

    this->_vertexCount = vertex_count;
    this->_faceCount = triangle_count;

    _hasNormals = flags & MESH_FLAG_HAS_NORMALS;
    _hasTBN = flags & MESH_FLAG_HAS_TBN;
    _hasTexCoord0 = flags & MESH_FLAG_HAS_UV0;
    _hasWeights = flags & MESH_FLAG_HAS_WEIGHTS;
    _hasOrigin = flags & MESH_FLAG_HAS_ORIGIN;
    _hasIndices = true;
    _hasVertices = true;
    layout = Layout(flags).GetVertexLayout();

    uint32_t index_count = triangle_count * 3;
    uses_short_indices = IsShortIndexCount(vertex_count);

    const size_t vertex_data_size = vertex_count * _strideBytes;

    auto vertex_initializer = Device::VulkanBufferInitializer(vertex_data_size)
        .SetVertex()
        .MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .Data(vertices)
        .Name(debug_name);
    
    _vertexBuffer = Device::VulkanBuffer::Create(vertex_initializer);

    unsigned int indexes_size = (uses_short_indices ? sizeof(uint16_t) : sizeof(uint32_t)) * index_count;
    auto index_initializer = Device::VulkanBufferInitializer(indexes_size)
        .SetIndex()
        .MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .Data(indices)
        .Name(debug_name);
    _indexBuffer = Device::VulkanBuffer::Create(index_initializer);

    if (_keepData)
    {
        vertex_data.resize(vertex_data_size);
        index_data.resize(indexes_size);

        memcpy(vertex_data.data(), vertices, vertex_data_size);
        memcpy(index_data.data(), indices, indexes_size);
    }
}

Mesh::~Mesh() 
{
}

// Setting mesh data

size_t Mesh::GetVertexStride(uint32_t flags)
{
    size_t result = 0;

    result += sizeof(vec3); // position
    if (flags & Mesh::MESH_FLAG_HAS_NORMALS) result += sizeof(Vector4_A2R10G10B10); // normals
    if (flags & Mesh::MESH_FLAG_HAS_TBN) result += sizeof(Vector4_A2R10G10B10); // tangent
    if (flags & Mesh::MESH_FLAG_HAS_UV0) result += sizeof(Vector2Half); // uv
    if (flags & Mesh::MESH_FLAG_HAS_WEIGHTS) result += sizeof(uint32_t) * 2; // bone weights and indices
    if (flags & Mesh::MESH_FLAG_HAS_ORIGIN) result += sizeof(vec3); // origin

    return result;
}

void Mesh::SetBoneRemap(const uint16_t* indices, int bone_count)
{
    bone_remap.resize(bone_count);
    memcpy(bone_remap.data(), indices, sizeof(uint16_t) * bone_count);
}

void Mesh::setVertices(const vec3 *vertices, int vertexCount) {
  _vertices.resize(vertexCount * 3);
  _vertexCount = vertexCount;
  memcpy(&_vertices[0], vertices, sizeof(vec3) * vertexCount);

  _hasVertices = true;

  _updateFaceCount();
}

void Mesh::setVertices(const float *vertexComponents, int vertexCount) {
  _vertices.resize(vertexCount * 3);
  _vertexCount = vertexCount;
  memcpy(&_vertices[0], vertexComponents, sizeof(vec3) * vertexCount);

  _hasVertices = true;

  _updateFaceCount();
}

void Mesh::setVertices(const std::vector<vec3> &vertices) {
  _vertices.resize(vertices.size() * 3);
  _vertexCount = (int)vertices.size();
  memcpy(&_vertices[0], &vertices[0], sizeof(vec3) * vertices.size());

  _hasVertices = true;

  _updateFaceCount();
}

void Mesh::setColors(const vec4 *colors, int colorCount) {
  _colors.resize(colorCount * 4);
  memcpy(&_colors[0], colors, sizeof(vec4) * colorCount);

  _hasColors = true;
}

void Mesh::setColors(const float *colorComponents, int colorCount) {
  _colors.resize(colorCount * 4);
  memcpy(&_colors[0], colorComponents, sizeof(vec4) * colorCount);

  _hasColors = true;
}

void Mesh::setColors(const std::vector<vec4> &colors) {
  _colors.resize(colors.size() * 4);
  memcpy(&_colors[0], &colors[0], sizeof(vec4) * colors.size());

  _hasColors = true;
}

void Mesh::setWeights(const std::vector<vec2> &weights) {
    throw std::runtime_error("not supported");
}

void Mesh::setWeights(const float *weightComponents, int count) {
    throw std::runtime_error("not supported");
}

void Mesh::setTexCoord0(const float *components, int count) {
  _texCoord0.resize(count);
  for (int i = 0; i < count; i++)
      _texCoord0[i] = Vector2Half(components[i * 2], components[i * 2 + 1]);

  _hasTexCoord0 = true;
}

void Mesh::setTexCoord0(const std::vector<vec2> &texcoords) {
  _texCoord0.resize(texcoords.size());
  for (int i = 0; i < texcoords.size(); i++)
      _texCoord0[i] = Vector2Half(texcoords[i].x, texcoords[i].y);

  _hasTexCoord0 = true;
}

void Mesh::setCorners(const float *components, int count) {
  _corners.resize(count * CORNER_SIZE);
  memcpy(&_corners[0], components, sizeof(vec2) * count);

  _hasCorners = true;
}

void Mesh::setCorners(const std::vector<vec2> &corners) {
  _corners.resize(corners.size() * CORNER_SIZE);
  memcpy(&_corners[0], &corners[0], sizeof(vec2) * corners.size());

  _hasCorners = true;
}

void Mesh::_updateFaceCount() {
  if (!_hasIndices) {
    _faceCount = (int)floor((float)_vertexCount/ (float)_componentCount);
  } else {
    _faceCount = (int)floor((float)_indices.size() / GetIndexSize(UsesShortIndexes()) / (float)_componentCount);
  }
}

void Mesh::setIndices(gsl::span<const uint16_t> indices) {
  _hasIndices = true;
  uses_short_indices = true;

  const uint32_t index_size = GetIndexSize(uses_short_indices);
  _indices.resize(indices.size() * index_size);
  memcpy_s(_indices.data(), _indices.size(), indices.data(), indices.size() * index_size);
  _updateFaceCount();
}

void Mesh::setIndices(gsl::span<const uint32_t> indices)
{
    _hasIndices = true;
    uses_short_indices = false;

    const uint32_t index_size = GetIndexSize(uses_short_indices);
    _indices.resize(indices.size() * index_size);
    memcpy_s(_indices.data(), _indices.size(), indices.data(), indices.size() * index_size);
    _updateFaceCount();
}

void Mesh::createBuffer() {
  _stride = _getStrideSize();
  _strideBytes = _stride;
  layout.Clear();

  int currentOffset = 0;
  if (_hasVertices) {
    _vertexOffset = currentOffset;
    _vertexOffsetBytes = currentOffset;
    currentOffset += VERTEX_SIZE;
    _vertexCount = (int)floor(_vertices.size() / 3.0f);
    layout.AddAttrib(VertexAttrib::Position, VERTEX_FORMAT, VERTEX_SIZE);
  }
  if (_hasNormals) {
    _normalOffset = currentOffset;
    _normalOffsetBytes = currentOffset;
    currentOffset += NORMAL_SIZE;
    layout.AddAttrib(VertexAttrib::Normal, NORMAL_FORMAT, NORMAL_SIZE);
  }
  if (_hasTBN) {
    _tangentOffset = currentOffset;
    _tangentOffsetBytes = currentOffset;
    currentOffset += NORMAL_SIZE;
    layout.AddAttrib(VertexAttrib::Tangent, NORMAL_FORMAT, NORMAL_SIZE);
  }
  if (_hasTexCoord0) {
    _texCoord0Offset = currentOffset;
    _texCoord0OffsetBytes = currentOffset;
    currentOffset += TEXCOORD_SIZE;
    layout.AddAttrib(VertexAttrib::TexCoord0, TEXCOORD_FORMAT, TEXCOORD_SIZE);
  }
  if (_hasCorners) {
    _cornerOffset = currentOffset;
    _cornerOffsetBytes = currentOffset;
    currentOffset += CORNER_SIZE;
    layout.AddAttrib(VertexAttrib::Corner, CORNER_FORMAT, CORNER_SIZE);
  }
  if (_hasWeights) {
    _weightOffset = currentOffset;
    _weightOffsetBytes = currentOffset;
    currentOffset += WEIGHT_SIZE;
    layout.AddAttrib(VertexAttrib::JointWeights, WEIGHTS_FORMAT, WEIGHT_SIZE);

    _jointIndexOffset = currentOffset;
    _jointIndexOffsetBytes = currentOffset;
    currentOffset += JOINT_INDEX_SIZE;
    layout.AddAttrib(VertexAttrib::JointIndices, JOINT_INDEX_FORMAT, JOINT_INDEX_SIZE);
  }
  if (_hasColors) {
    _colorOffset = currentOffset;
    _colorOffsetBytes = currentOffset;
    currentOffset += COLOR_SIZE;
    layout.AddAttrib(VertexAttrib::VertexColor, COLOR_FORMAT, COLOR_SIZE);
  }

  assert(_strideBytes == layout.GetStride());

  std::vector<uint8_t> data_buffer(_strideBytes * _vertexCount);

  Layout mesh_layout(layout);

  auto bufferData = (uint8_t*)data_buffer.data();
  for (int i = 0; i < _vertexCount; i++) {
    int offset = i * _stride;
    if (_hasVertices) {
      auto& position = mesh_layout.GetPosition(bufferData, i);
      position = vec3(_vertices[i * 3], _vertices[i * 3 + 1], _vertices[i * 3 + 2]);
    }

    if (_hasNormals) {
        auto& normal = mesh_layout.GetNormal(bufferData, i);
        normal = Vector4_A2R10G10B10::FromSignedNormalizedFloat(vec4(_normals[i * 3], _normals[i * 3 + 1], _normals[i * 3 + 2], 0));
    }

    if (_hasTBN) {
        auto& tangent = mesh_layout.GetTangent(bufferData, i);
        tangent = Vector4_A2R10G10B10::FromSignedNormalizedFloat(vec4(_tangents[i * 3], _tangents[i * 3 + 1], _tangents[i * 3 + 2], 0));
    }

    if (_hasTexCoord0) {
        auto& uv0 = mesh_layout.GetUV0(bufferData, i);
        uv0 = _texCoord0[i];
    }

    if (_hasCorners) {
        auto& corner = mesh_layout.GetCorner(bufferData, i);
        corner = vec2(_corners[i * 2], _corners[i * 2 + 1]);
    }

    if (_hasColors) {
        auto& color = mesh_layout.GetColor(bufferData, i);
        color = vec4(_colors[i * 4], _colors[i * 4 + 1], _colors[i * 4 + 2], _colors[i * 4 + 3]);
    }
  }

  auto vertex_initializer = Device::VulkanBufferInitializer(data_buffer.size())
	  .SetVertex()
	  .MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
	  .Data(data_buffer.data())
      .Name(debug_name);
  _vertexBuffer = Device::VulkanBuffer::Create(vertex_initializer);

  if (_hasIndices) {
	auto index_initializer = Device::VulkanBufferInitializer((unsigned int)_indices.size())
		.SetIndex()
		.MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
		.Data(_indices.data())
        .Name(debug_name);
	_indexBuffer = Device::VulkanBuffer::Create(index_initializer);
  }

  _calculateAABB();

  // Free data arrays
    std::vector<float>().swap(_vertices);
    std::vector<float>().swap(_normals);
    std::vector<float>().swap(_tangents);
    std::vector<Vector2Half>().swap(_texCoord0);
    std::vector<Vector4b>().swap(_weights);
    std::vector<Vector4b>().swap(_jointIndices);
    std::vector<float>().swap(_colors);
    std::vector<uint8_t>().swap(_indices);
}

void Mesh::_prepareVAO() {
	
}

int Mesh::_getStrideSize() {
  int result = 0;
  if (_hasVertices) result += VERTEX_SIZE;
  if (_hasNormals) result += NORMAL_SIZE;
  if (_hasTBN) result += NORMAL_SIZE * 2;
  if (_hasColors) result += COLOR_SIZE;
  if (_hasTexCoord0) result += TEXCOORD_SIZE;
  if (_hasCorners) result += CORNER_SIZE;
  if (_hasWeights) result += JOINT_INDEX_SIZE + WEIGHT_SIZE;
  return result;
}

void Mesh::calculateNormals() {
  _normals.resize(_vertices.size());
  memset(&_normals[0], 0, _normals.size() * sizeof(float));

  for (int i = 0; i < _faceCount; i++) {
    int faceOffset = i * 3;
    int indexA = _hasIndices ? GetIndex(faceOffset + 0, _indices, uses_short_indices) * 3 : faceOffset * 3;
    int indexB = _hasIndices ? GetIndex(faceOffset + 1, _indices, uses_short_indices) * 3: faceOffset * 3 + 3;
    int indexC = _hasIndices ? GetIndex(faceOffset + 2, _indices, uses_short_indices) * 3: faceOffset * 3 + 6;

    vec3 a(_vertices[indexA], _vertices[indexA + 1], _vertices[indexA + 2]);
    vec3 b(_vertices[indexB], _vertices[indexB + 1], _vertices[indexB + 2]);
    vec3 c(_vertices[indexC], _vertices[indexC + 1], _vertices[indexC + 2]);
    b = b - a;
    c = c - a;

    vec3 normal = glm::cross(b, c);

    for (int j = 0; j < 3; j++) {
      _normals[indexA + j] += normal[j];
      _normals[indexB + j] += normal[j];
      _normals[indexC + j] += normal[j];
    }
  }

  for (int i = 0; i < _vertexCount; i++) {
    auto *normal = (vec3 *)&_normals[i * 3];
    *normal = glm::normalize(*normal);
  }

  _hasNormals = true;
}

void Mesh::calculateTBN() {
  if (!this->_hasTexCoord0) {
    throw std::runtime_error("Can't calculate tangent space without TexCoord0");
  }

  _tangents.resize(_vertices.size());
  memset(&_tangents[0], 0, _tangents.size() * sizeof(float));

  for (int i = 0; i < _faceCount; i++) {
    int faceOffset = i * 3;
    int indexA = _hasIndices ? GetIndex(faceOffset + 0, _indices, uses_short_indices) * 3 : faceOffset * 3;
    int indexB = _hasIndices ? GetIndex(faceOffset + 1, _indices, uses_short_indices) * 3 : faceOffset * 3 + 3;
    int indexC = _hasIndices ? GetIndex(faceOffset + 2, _indices, uses_short_indices) * 3 : faceOffset * 3 + 6;

    int indexUVA = _hasIndices ? GetIndex(faceOffset + 0, _indices, uses_short_indices) * 2 : faceOffset * 2;
    int indexUVB = _hasIndices ? GetIndex(faceOffset + 1, _indices, uses_short_indices) * 2 : faceOffset * 2 + 2;
    int indexUVC = _hasIndices ? GetIndex(faceOffset + 2, _indices, uses_short_indices) * 2 : faceOffset * 2 + 4;

    vec3 a(_vertices[indexA], _vertices[indexA + 1], _vertices[indexA + 2]);
    vec3 b(_vertices[indexB], _vertices[indexB + 1], _vertices[indexB + 2]);
    vec3 c(_vertices[indexC], _vertices[indexC + 1], _vertices[indexC + 2]);

    vec2 uvA(_texCoord0[indexUVA]);
    vec2 uvB(_texCoord0[indexUVB]);
    vec2 uvC(_texCoord0[indexUVC]);

    vec3 deltaPos1 = b - a;
    vec3 deltaPos2 = c - a;

    vec2 deltaUV1 = uvB - uvA;
    vec2 deltaUV2 = uvC - uvA;

    float r = 1.0f / (deltaUV1[0] * deltaUV2[1] - deltaUV1[1] * deltaUV2[0]);
    a = deltaPos1 * (deltaUV2[1] * r);
    b = deltaPos2 * (deltaUV1[1] * r);
    vec3 tangent = a - b;

    for (int j = 0; j < 3; j++) {
      _tangents[indexA + j] += tangent[j];
      _tangents[indexB + j] += tangent[j];
      _tangents[indexC + j] += tangent[j];
    }
  }

  for (int i = 0; i < _vertexCount; i++) {
   /* vec3 *a = (vec3 *)&_tangents[i * 3];
    vec3 *c = (vec3 *)&_normals[i * 3];

   // Orthonormalize matrix. Since it's almost orthonormal we can just correct tangent a little.
    // t = normalize(t - n * dot(n, t));
    *a = *a - *c * glm::dotf(*c, *a);

    // Check the tangent direction
    if (glm::dotf(glm::cross(*c, *a), *b) < 0) {
      *a = -*a; // invert tangent
    }

    *a = -glm::normalize(*a);
    *b = -glm::normalize(*b); // have no idea why it should be inverted to look visually correct
    */
  }

  _hasTBN = true;
}

void Mesh::_calculateAABB() {
  if (_vertexCount == 0) {
    _aabb.min = vec3(0);
    _aabb.max = vec3(0);
    return;
  }

  vec3 *vertices = (vec3 *)&_vertices[0];
  _aabb.min = vertices[0];
  _aabb.max = vertices[0];
  for (int i = 1; i < _vertexCount; i++) {
    _aabb.min = glm::min(_aabb.min, vertices[i]);
    _aabb.max = glm::max(_aabb.max, vertices[i]);
  }
}
