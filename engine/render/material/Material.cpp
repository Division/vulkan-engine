#include "Material.h"
#include "utils/Math.h"

using namespace Device;

Material::Material()
{
}

Material::~Material()
{

}

void Material::SetDirty()
{
	shader_hash_dirty = true;
	caps_dirty = true;
}

void Material::Texture0(Device::Handle<Device::Texture> texture) {
	texture0 = texture;
	
	if ((bool)texture != has_texture0) {
		has_texture0 = (bool)texture;
		SetDirty();
	}
}

void Material::NormalMap(Device::Handle<Device::Texture> normal_map) {
	normal_map = normal_map;

	if ((bool)normal_map != has_normal_map) {
		has_normal_map = (bool)normal_map;
		SetDirty();
	}
}

void Material::LightingEnabled(bool lighting_enabled_) {
	if (lighting_enabled_ != lighting_enabled) {
		lighting_enabled = lighting_enabled_;
		SetDirty();
	}
}

void Material::VertexColorEnabled(bool vertex_color_enabled_) {
	if (vertex_color_enabled_ != vertex_color_enabled) {
		vertex_color_enabled = vertex_color_enabled_;
		SetDirty();
	}
}

void Material::UpdateCaps() const {
	if (!caps_dirty) return;

	if (has_texture0) {
		shader_caps.addCap(ShaderCaps::Texture0);
	} else {
		shader_caps.removeCap(ShaderCaps::Texture0);
	}

	if (has_normal_map) {
		shader_caps.addCap(ShaderCaps::NormalMap);
	} else {
		shader_caps.removeCap(ShaderCaps::NormalMap);
	}

	if (lighting_enabled) {
		shader_caps.addCap(ShaderCaps::Lighting);
	} else {
		shader_caps.removeCap(ShaderCaps::Lighting);
	}

	if (vertex_color_enabled) {
		shader_caps.addCap(ShaderCaps::VertexColor);
	} else {
		shader_caps.removeCap(ShaderCaps::VertexColor);
	}

	shader_caps_skinning = shader_caps;
	shader_caps_skinning.addCap(ShaderCaps::Skinning);

	caps_dirty = false;
}

void Material::UpdateShaderHash() const
{
	if (!shader_hash_dirty) return;
	UpdateCaps();
	std::vector<ShaderProgramInfo::Macro> defines;
	ShaderCache::AppendCapsDefines(shader_caps, defines);

	auto vertex_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines);
	auto fragment_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, "ps_main", defines);
	auto vertex_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", { {"DEPTH_ONLY", "1" } });
	auto fragment_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, L"shaders/noop.hlsl");
	vertex_hash = ShaderCache::GetShaderDataHash(vertex_data);
	vertex_hash_depth_only = ShaderCache::GetShaderDataHash(vertex_depth_only_data);
	fragment_hash = ShaderCache::GetShaderDataHash(fragment_data);

	shader_info.Clear();
	shader_info.AddShader(std::move(vertex_data));
	shader_info.AddShader(std::move(fragment_data));

	depth_only_shader_info.Clear();
	depth_only_shader_info.AddShader(std::move(vertex_depth_only_data));
	depth_only_shader_info.AddShader(std::move(fragment_depth_only_data));

	shader_hash_dirty = false;
}

uint32_t Material::GetHash() const
{
	UpdateCaps();
	UpdateShaderHash();
	uint32_t hashes[] = {
		vertex_hash,
		fragment_hash,
		shader_caps.getBitmask(),
		texture0 ? texture0->GetHash() : 0,
		normal_map ? normal_map->GetHash() : 0,
		lighting_enabled,
		vertex_color_enabled,
		FastHash(&roughness, sizeof(roughness))
	};

	return FastHash(hashes, sizeof(hashes));
}
