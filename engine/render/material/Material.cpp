#include "Material.h"
#include "utils/Math.h"
#include "render/buffer/VulkanBuffer.h"

using namespace Device;
using namespace Resources;

namespace render
{
	struct ConstantBindingStorage::ConstantMetadata
	{
		uint16_t offset;
		uint16_t size;
		uint32_t name_hash;
	};

	gsl::span<ConstantBindingStorage::ConstantMetadata> ConstantBindingStorage::GetMetadata() const
	{
		return gsl::make_span<ConstantMetadata>(reinterpret_cast<ConstantMetadata*>(data.data()), constant_count);
	}

	gsl::span<uint8_t> ConstantBindingStorage::GetConstantsData() const
	{
		const size_t metadata_size = constant_count * sizeof(ConstantMetadata);
		return gsl::make_span<uint8_t>(data.data() + metadata_size, data.size() - metadata_size);
	}

	ConstantBindingStorage::ConstantData ConstantBindingStorage::GetConstantByIndex(uint32_t index) const
	{
		if (index >= constant_count)
			throw std::runtime_error("constant out of bounds");

		auto& meta = GetMetadata()[index];
		return { &GetConstantsData()[meta.offset], meta.name_hash, meta.size };
	}

	void* ConstantBindingStorage::GetConstantDataByOffset(size_t offset)
	{
		const size_t metadata_size = constant_count * sizeof(ConstantMetadata);
		const size_t data_offset =  metadata_size + offset;
		assert(data_offset < data.size());
		return data.data() + data_offset;
	}

	std::optional<float> ConstantBindingStorage::GetFloatConstant(const char* name)
	{
		return GetConstantValue<float>(name);
	}

	std::optional<vec2> ConstantBindingStorage::GetFloat2Constant(const char* name)
	{
		return GetConstantValue<vec2>(name);
	}

	std::optional<vec3> ConstantBindingStorage::GetFloat3Constant(const char* name)
	{
		return GetConstantValue<vec3>(name);
	}

	std::optional<float4> ConstantBindingStorage::GetFloat4Constant(const char* name)
	{
		return GetConstantValue<float4>(name);
	}

	template<typename T>
	std::optional<T> ConstantBindingStorage::GetConstantValue(const char* name)
	{
		auto data = FindConstant<T>(Device::ShaderProgram::GetParameterNameHash(name));
		if (!data)
			return std::nullopt;
		else
			return *data;
	}

	template<typename T>
	T* ConstantBindingStorage::FindConstant(uint32_t hash)
	{
		for (auto& meta : GetMetadata())
		{
			if (meta.name_hash == hash)
			{
				assert(sizeof(T) == meta.size);
				if (sizeof(T) != meta.size)
					return nullptr;

				return reinterpret_cast<T*>(GetConstantDataByOffset(meta.offset));
			}
		}

		return nullptr;
	}

	template<typename T>
	void ConstantBindingStorage::AddConstant(uint32_t hash, T value)
	{
		T* constant_data = FindConstant<T>(hash);
		
		if (!constant_data)
		{
			const size_t move_data_size = GetConstantsData().size_bytes();
			const size_t resize_amount = sizeof(ConstantMetadata) + sizeof(value);
			data.resize(data.size() + resize_amount);
			void* move_from = data.data() + constant_count * sizeof(ConstantMetadata);
			void* move_to = (uint8_t*)move_from + sizeof(ConstantMetadata);
			memmove(move_to, move_from, move_data_size);
			constant_count += 1;
			
			const size_t constant_offset = data.size() - sizeof(T) - constant_count * sizeof(ConstantMetadata);
			constant_data = reinterpret_cast<T*>(GetConstantDataByOffset(constant_offset));
			GetMetadata()[constant_count - 1] = { (uint16_t)move_data_size, (uint16_t)sizeof(value), hash };
		}
		
		assert(FindConstant<T>(hash));

		*constant_data = value;
	}

	void ConstantBindingStorage::AddFloatConstant(const char* name, float value)
	{
		AddConstant(Device::ShaderProgram::GetParameterNameHash(name), value);
	}

	void ConstantBindingStorage::AddFloat2Constant(const char* name, vec2 value)
	{
		AddConstant(Device::ShaderProgram::GetParameterNameHash(name), value);
	}

	void ConstantBindingStorage::AddFloat3Constant(const char* name, vec3 value)
	{
		AddConstant(Device::ShaderProgram::GetParameterNameHash(name), value);
	}

	void ConstantBindingStorage::AddFloat4Constant(const char* name, vec4 value)
	{
		AddConstant(Device::ShaderProgram::GetParameterNameHash(name), value);
	}

	void ConstantBindingStorage::RemoveConstant(const char* name)
	{
		RemoveConstant(Device::ShaderProgram::GetParameterNameHash(name));
	}

	void ConstantBindingStorage::Clear()
	{
		data.resize(0);
		constant_count = 0;
	}

	void ConstantBindingStorage::RemoveConstant(uint32_t hash)
	{
		auto metadata = GetMetadata();
		const auto metadata_size = metadata.size_bytes();
		const auto data_size = data.size();

		auto it = std::find_if(metadata.begin(), metadata.end(), [hash](const ConstantMetadata& meta) { return meta.name_hash == hash; });
		if (it != metadata.end())
		{
			const size_t offset_difference = it->size;
			assert(offset_difference > 0);

			const auto index = std::distance(metadata.begin(), it);
			for (uint32_t i = index + 1; i < metadata.size(); i++)
			{
				metadata[i].offset -= offset_difference;
			}

			uint8_t* meta_offset_start = data.data() + (index + 1) * sizeof(ConstantMetadata);
			const size_t meta_offset_size = sizeof(ConstantMetadata) * (metadata.size() - index - 1) + it->offset;
			memmove(meta_offset_start - sizeof(ConstantMetadata), meta_offset_start, meta_offset_size);

			uint8_t* data_offset_start = meta_offset_start + meta_offset_size + offset_difference;
			const size_t data_move_size = (size_t)data.data() + data.size() - (size_t)data_offset_start;
			const size_t data_offset_size = sizeof(ConstantMetadata) + offset_difference;
			assert(data.size() == (size_t)data_offset_start + data_move_size - (size_t)data.data());
			memmove(data_offset_start - data_offset_size, data_offset_start, data_move_size);
			data.resize(data.size() - data_offset_size);
			constant_count -= 1;
		}
	}

	void ConstantBindingStorage::Flush(Device::ConstantBindings& bindings) const
	{
		for (uint32_t i = 0; i < constant_count; i++)
		{
			auto& constant = GetConstantByIndex(i);
			bindings.AddDataBinding(constant.data, constant.size, constant.name_hash);
		}
	}
}

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
	ExtraResourceBinding value{ Device::ShaderProgram::GetParameterNameHash(name), texture };
	AddExtraResourceBinding(value);
}

void Material::AddExtraTexture(Device::Handle<Device::Texture> texture, const char* name)
{
	ExtraResourceBinding value{ Device::ShaderProgram::GetParameterNameHash(name), texture };
	AddExtraResourceBinding(value);
}

void Material::AddExtraBuffer(Device::Handle<Device::VulkanBuffer> buffer, const char* name)
{
	ExtraResourceBinding value{ Device::ShaderProgram::GetParameterNameHash(name), buffer };
	AddExtraResourceBinding(value);
}

void Material::AddExtraResourceBinding(const ExtraResourceBinding& value)
{
	assert(!value.resource.valueless_by_exception());
	auto it = std::find_if(extra_resource_bindings.begin(), extra_resource_bindings.end(), [hash = value.name_hash](const ExtraResourceBinding& binding) { return binding.name_hash == hash; });
	if (it != extra_resource_bindings.end())
		*it = value;
	else
		extra_resource_bindings.push_back(value);

	SetBindingsDirty();
}

void Material::ClearExtraResources()
{
	extra_resource_bindings.clear();
}

void Material::AddFloatConstant(const char* name, float value)
{
	constants_dirty = true;
	constant_storage.AddFloatConstant(name, value);
}

void Material::AddFloat2Constant(const char* name, vec2 value)
{
	constants_dirty = true;
	constant_storage.AddFloat2Constant(name, value);
}

void Material::AddFloat3Constant(const char* name, vec3 value)
{
	constants_dirty = true;
	constant_storage.AddFloat3Constant(name, value);
}

void Material::AddFloat4Constant(const char* name, vec4 value)
{
	constants_dirty = true;
	constant_storage.AddFloat4Constant(name, value);
}

void Material::ClearConstants()
{
	constants_dirty = true;
	constant_storage.Clear();
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

	if (alpha_cutoff) {
		shader_caps.addCap(ShaderCaps::AlphaCutoff);
	}
	else {
		shader_caps.removeCap(ShaderCaps::AlphaCutoff);
	}

	caps_dirty = false;
}

void Material::UpdateShaderHash() const
{
	if (!shader_hash_dirty) return;
	UpdateCaps();
	std::vector<ShaderProgramInfo::Macro> defines;
	ShaderCache::AppendCapsDefines(shader_caps, defines);

	auto vertex_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, vs_entry, defines);
	auto fragment_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, ps_entry, defines);
	auto vertex_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Vertex, shader_path, vs_entry, defines);
	auto fragment_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, L"shaders/noop.hlsl", "main");
	if (alpha_cutoff)
	{
		fragment_depth_only_data = ShaderProgramInfo::ShaderData(ShaderProgram::Stage::Fragment, shader_path, ps_entry, defines);
	}

	shader_info.Clear();
	shader_info.AddShader(std::move(vertex_data));
	shader_info.AddShader(std::move(fragment_data));

	depth_only_shader_info.Clear();
	depth_only_shader_info.AddShader(std::move(vertex_depth_only_data));
	depth_only_shader_info.AddShader(ShaderProgramInfo::ShaderData(fragment_depth_only_data));

	shader_hash_dirty = false;
}

void Material::UpdateBindings() const
{
	OPTICK_EVENT();
	if (!bindings_dirty)
		return;

	resource_bindings.Clear();
	if (has_texture0)
		resource_bindings.AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::Texture0), GetTexture0());

	if (has_normal_map)
		resource_bindings.AddTextureBinding(Device::GetShaderTextureNameHash(ShaderTextureName::NormalMap), GetNormalMap());

	for (auto& extra_texture : extra_resource_bindings)
	{
		if (extra_texture.resource.index() == 0)
			resource_bindings.AddTextureBinding(extra_texture.name_hash, std::get<0>(extra_texture.resource)->Get().get());
		else if (extra_texture.resource.index() == 0)
			resource_bindings.AddTextureBinding(extra_texture.name_hash, std::get<1>(extra_texture.resource).get());
		else
			resource_bindings.AddBufferBinding(extra_texture.name_hash, std::get<2>(extra_texture.resource).get(), std::get<2>(extra_texture.resource)->Size());
	}

	bindings_dirty = false;
}

void Material::UpdateConstants() const
{
	OPTICK_EVENT();
	if (!constants_dirty)
		return;

	constant_bindings.Clear();
	constant_bindings.AddFloat4Binding(&color, "color");
	constant_bindings.AddFloatBinding(&roughness, "roughness");
	constant_bindings.AddFloatBinding(&metalness, "metalness");
	
	constant_storage.Flush(constant_bindings);

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
