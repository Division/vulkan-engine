#include "Material.h"
#include "utils/Math.h"

using namespace Device;
using namespace Resources;

Material::Material()
{
}

Material::Material(const std::wstring& shader_path) 
	: shader_path(shader_path)
{
}

Material::~Material()
{
}

Material::Handle Material::Clone() const
{
	return Handle(std::make_unique<Material>(*this));
}

void Material::SetDirty()
{
	shader_hash_dirty = true;
	caps_dirty = true;
}

void Material::SetBindingsDirty()
{
	bindings_dirty = true;
}

void Material::SetConstantsDirty()
{
	constants_dirty = true;
}

void Material::SetShaderPath(std::wstring shader_path)
{
	this->shader_path = std::move(shader_path);
	SetDirty();
}

void Material::Texture0(Device::Handle<Device::Texture> texture) 
{
	if (texture0 != texture)
	{
		texture0 = texture;
		texture0_resource = TextureResource::Handle();
		SetBindingsDirty();
	}

	if ((bool)texture != has_texture0) {
		has_texture0 = (bool)texture;
		SetDirty();
	}
}

Texture* Material::GetTexture0() const
{
	if (!has_texture0) return nullptr;

	if (!GetTexture0Resource() && Texture0())
		return Texture0().get();
	else
		return GetTexture0Resource()->Get().get();
}

void Material::NormalMap(Device::Handle<Device::Texture> normal_map) 
{
	if (this->normal_map != normal_map)
	{
		this->normal_map = normal_map;
		normal_map_resource = TextureResource::Handle();
		SetBindingsDirty();
	}

	if ((bool)normal_map != has_normal_map) {
		has_normal_map = (bool)normal_map;
		SetDirty();
	}
}

Device::Texture* Material::GetNormalMap() const
{
	if (!has_normal_map) return nullptr;

	if (!GetNormalMapResource() && NormalMap())
		return NormalMap().get();
	else
		return GetNormalMapResource()->Get().get();
}

void Material::SetTexture0Resource(TextureResource::Handle texture)
{
	if (texture0_resource != texture)
	{
		texture0_resource = texture;
		this->texture0.Reset();
		SetBindingsDirty();
	}

	if ((bool)texture0_resource != has_texture0) {
		has_texture0 = (bool)texture;
		SetDirty();
	}
}

void Material::SetNormalMapResource(Resources::TextureResource::Handle normal_map)
{
	if (this->normal_map_resource != normal_map)
	{
		this->normal_map_resource = normal_map;
		this->normal_map.Reset();
		SetBindingsDirty();
	}

	if ((bool)normal_map_resource != has_normal_map) {
		has_normal_map = (bool)normal_map;
		SetDirty();
	}
}

void Material::LightingEnabled(bool lighting_enabled_)
{
	if (lighting_enabled_ != lighting_enabled) {
		lighting_enabled = lighting_enabled_;
		SetDirty();
	}
}

void Material::VertexColorEnabled(bool vertex_color_enabled_)
{
	if (vertex_color_enabled_ != vertex_color_enabled) {
		vertex_color_enabled = vertex_color_enabled_;
		SetDirty();
	}
}

void Material::UpdateCaps() const
{
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

	ShaderCapsSet skinning_caps;
	skinning_caps.addCap(ShaderCaps::Skinning);
	ShaderCache::AppendCapsDefines(skinning_caps, defines);
	auto vertex_data_skinning = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines);
	auto vertex_depth_only_data_skinning = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", { {"DEPTH_ONLY", "1" }, { "SKINNING", "1"} });
	auto fragment_data_skinning = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, "ps_main", defines);

	shader_info.Clear();
	shader_info.AddShader(std::move(vertex_data));
	shader_info.AddShader(std::move(fragment_data));

	shader_info_skinning.Clear();
	shader_info_skinning.AddShader(std::move(vertex_data_skinning));
	shader_info_skinning.AddShader(std::move(fragment_data_skinning));

	depth_only_shader_info.Clear();
	depth_only_shader_info.AddShader(std::move(vertex_depth_only_data));
	depth_only_shader_info.AddShader(ShaderProgramInfo::ShaderData(fragment_depth_only_data));

	depth_only_shader_info_skinning.Clear();
	depth_only_shader_info_skinning.AddShader(std::move(vertex_depth_only_data_skinning));
	depth_only_shader_info_skinning.AddShader(ShaderProgramInfo::ShaderData(fragment_depth_only_data));

	shader_hash_dirty = false;
}

void Material::UpdateBindings() const
{
	if (!bindings_dirty)
		return;

	resource_bindings.Clear();
	if (has_texture0)
		resource_bindings.AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::Texture0), GetTexture0());

	if (has_normal_map)
		resource_bindings.AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::NormalMap), GetNormalMap());

	bindings_dirty = false;
}

void Material::UpdateConstants() const
{
	if (!constants_dirty)
		return;

	constant_bindings.Clear();
	constant_bindings.AddFloat4Binding(&color, "color");
	constant_bindings.AddFloatBinding(&roughness, "roughness");
	constant_bindings.AddFloatBinding(&metalness, "metalness");

	constants_dirty = false;
}

uint32_t Material::GetHash() const
{
	UpdateCaps();
	UpdateShaderHash();
	uint32_t hashes[] = {
		shader_caps.getBitmask(),
		texture0 ? texture0->GetHash() : 0,
		normal_map ? normal_map->GetHash() : 0,
		lighting_enabled,
		vertex_color_enabled,
		FastHash(&roughness, sizeof(roughness)),
		FastHash(&metalness, sizeof(metalness)),
		FastHash(&color, sizeof(color))
	};

	return FastHash(hashes, sizeof(hashes));
}
