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

void Material::SetAlphaCutoff(bool value) 
{ 
	if (alpha_cutoff != value)
	{
		alpha_cutoff = value; 
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

void Material::AddExtraTexture(Resources::TextureResource::Handle texture, const char* name)
{
	ExtraTextureBinding value{ Device::ShaderProgram::GetParameterNameHash(name), texture };
	AddExtraTextureBinding(value);
}

void Material::AddExtraTexture(Device::Handle<Device::Texture> texture, const char* name)
{
	ExtraTextureBinding value{ Device::ShaderProgram::GetParameterNameHash(name), texture };
	AddExtraTextureBinding(value);
}

void Material::AddExtraTextureBinding(const ExtraTextureBinding& value)
{
	assert(!value.texture.valueless_by_exception());
	auto it = std::find_if(extra_texture_bindings.begin(), extra_texture_bindings.end(), [hash = value.name_hash](const ExtraTextureBinding& binding) { return binding.name_hash == hash; });
	if (it != extra_texture_bindings.end())
		*it = value;
	else
		extra_texture_bindings.push_back(value);

	SetBindingsDirty();
}

void Material::ClearExtraTextures()
{
	extra_texture_bindings.clear();
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
	if (alpha_cutoff)
	{
		defines.push_back({ "ALPHA_CUTOFF", "1" });
	}

	std::vector<ShaderProgramInfo::Macro> defines_depth_only = defines;
	defines_depth_only.push_back({ "DEPTH_ONLY", "1" });

	auto vertex_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines);
	auto fragment_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, "ps_main", defines);
	auto vertex_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines_depth_only);
	auto fragment_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, L"shaders/noop.hlsl");
	if (alpha_cutoff)
	{
		fragment_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, "ps_main", defines_depth_only);
	}

	ShaderCapsSet skinning_caps;
	skinning_caps.addCap(ShaderCaps::Skinning);
	ShaderCache::AppendCapsDefines(skinning_caps, defines);
	ShaderCache::AppendCapsDefines(skinning_caps, defines_depth_only);
	
	auto vertex_data_skinning = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines);
	auto vertex_depth_only_data_skinning = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, "vs_main", defines_depth_only);
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

	for (auto& extra_texture : extra_texture_bindings)
	{
		if (extra_texture.texture.index() == 0)
			resource_bindings.AddTextureBinding(extra_texture.name_hash, std::get<0>(extra_texture.texture)->Get().get());
		else
			resource_bindings.AddTextureBinding(extra_texture.name_hash, std::get<1>(extra_texture.texture).get());
	}

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
