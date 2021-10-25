#include "Shader.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "utils/Math.h"
#include "ShaderBindings.h"
#include "ShaderDefines.h"

namespace Device {
	
	uint32_t ShaderProgram::DescriptorSetLayout::GetBindingIndexByName(std::string name)
	{
		for (uint32_t i = 0; i < bindings.size(); i++)
			if (bindings[i].name == name)
				return i;

		return (uint32_t)-1;
	}

	ShaderProgram::BindingAddress ShaderProgram::GetBindingAddress(const std::string& name)
	{
		WaitLoaded();

		for (int set = 0; set < descriptor_sets.size(); set++)
		{
			auto index = descriptor_sets[set].GetBindingIndexByName(name);
			if (index != -1)
			{
				return { (unsigned)set, (unsigned)index };
			}
		}

		return { (unsigned)-1, (unsigned)-1 };
	}

	void ShaderModule::Load(void* data, size_t size)
	{
		assert(state.load() == State::Loading);
		vk::ShaderModuleCreateInfo create_info(vk::ShaderModuleCreateFlags(), size, (uint32_t*)data);
		shader_module = Engine::GetVulkanContext()->GetDevice().createShaderModuleUnique(create_info);
		reflection_info = std::make_unique<ReflectionInfo>((uint32_t*)data, size / sizeof(uint32_t));
		state = State::Loaded;
	}

	bool ShaderModule::TransitionState(State old_state, State new_state)
	{
		return state.compare_exchange_strong(old_state, new_state);
	}

	void ShaderModule::WaitLoaded()
	{
		OPTICK_EVENT();
		while ((int)state.load() < (int)State::Loaded)
			std::this_thread::yield();
	}
	
	void ShaderProgram::AddModule(ShaderModule* shader_module, Stage stage)
	{
		assert(state == ShaderModule::State::Loading);

		ShaderModule** module_var = nullptr;
	
		if (stage == Stage::Vertex)
			vertex_module = shader_module;
		if (stage == Stage::Fragment)
			fragment_module = shader_module;
		if (stage == Stage::Compute)
			compute_module = shader_module;
	}

	std::unordered_map<ShaderProgram::Stage, vk::ShaderStageFlagBits> shader_stage_flag_map =
	{
		{ ShaderProgram::Stage::Vertex, vk::ShaderStageFlagBits::eVertex },
		{ ShaderProgram::Stage::Fragment, vk::ShaderStageFlagBits::eFragment },
		{ ShaderProgram::Stage::Compute, vk::ShaderStageFlagBits::eCompute },
	};

	std::unordered_map<ShaderProgram::BindingType, vk::DescriptorType> binding_type_map =
	{
		{ ShaderProgram::BindingType::CombinedImageSampler, vk::DescriptorType::eCombinedImageSampler },
		{ ShaderProgram::BindingType::UniformBuffer, vk::DescriptorType::eUniformBuffer },
		{ ShaderProgram::BindingType::UniformBufferDynamic, vk::DescriptorType::eUniformBufferDynamic },
		{ ShaderProgram::BindingType::StorageBuffer, vk::DescriptorType::eStorageBuffer },
		{ ShaderProgram::BindingType::StorageBufferDynamic, vk::DescriptorType::eStorageBufferDynamic },
		{ ShaderProgram::BindingType::SampledImage, vk::DescriptorType::eSampledImage },
		{ ShaderProgram::BindingType::Sampler, vk::DescriptorType::eSampler },
	};

	vk::ShaderStageFlags GetShaderStageFlags(unsigned stage_flags)
	{
		vk::ShaderStageFlags result;

		for (auto iter : shader_stage_flag_map)
			if (stage_flags & (unsigned)iter.first)
				result |= iter.second;

		return result;
	}

	void ShaderProgram::AppendBindings(const ShaderModule& module, ShaderProgram::Stage stage, std::map<std::pair<unsigned, unsigned>, int>& existing_bindings)
	{
		auto* reflection = module.GetReflectionInfo();
		auto stage_flags = shader_stage_flag_map.at(stage);
		
		auto append_binding_internal = [&](BindingType type, uint32_t id, const std::string& name, const unsigned set, const unsigned binding)
		{
			auto& descriptor_set = descriptor_sets[set];
			auto iterator = existing_bindings.find(std::make_pair(set, binding));
			BindingData* binding_data = nullptr;
			if (iterator != existing_bindings.end())
			{
				binding_data = &descriptor_set.bindings[iterator->second];
				if (binding_data->id != id || binding_data->name != name || binding_data->type != type || binding_data->stage_flags & (unsigned)stage)
					throw std::runtime_error("appearance of the binding doesn't match the existing one");

				binding_data->stage_flags |= (unsigned)stage;
			} else
			{
				if (name_binding_map.find(name) != name_binding_map.end()) 
					throw std::runtime_error("binding already exists: " + name);

				descriptor_set.bindings.push_back(BindingData());
				existing_bindings[std::make_pair(set, binding)] = descriptor_set.bindings.size() - 1;

				binding_data = &descriptor_set.bindings.back();
				binding_data->name = name;

				binding_data->name_hash = GetParameterNameHash(name);
				binding_data->address = { set, binding };
				binding_data->stage_flags = (unsigned)stage;
				binding_data->id = id;
				binding_data->type = type;
				name_binding_map[name] = binding_data->address;
			}

			return binding_data;
		};

		for (auto& sampler : reflection->CombinedImageSamplers())
		{
			append_binding_internal(BindingType::CombinedImageSampler, (uint32_t)sampler.shader_texture, sampler.name, sampler.set, sampler.binding);
		}

		for (auto& ubo : reflection->UniformBuffers())
		{
			const auto binding_type = SHADER_DYNAMIC_OFFSET_BUFFERS.find(ubo.shader_buffer) == SHADER_DYNAMIC_OFFSET_BUFFERS.end()
				? BindingType::UniformBuffer
				: BindingType::UniformBufferDynamic;
;
			auto* binding = append_binding_internal(binding_type, (uint32_t)ubo.shader_buffer, ubo.name, ubo.set, ubo.binding);
			binding->size = ubo.size;
			for (auto& member : ubo.members)
				binding->members.emplace_back(BufferMember{ member.name, member.name_hash, member.offset, member.size } );
		}

		for (auto& storage_buffer : reflection->StorageBuffers())
		{
			const auto binding_type = SHADER_DYNAMIC_OFFSET_BUFFERS.find(storage_buffer.storage_buffer_name) == SHADER_DYNAMIC_OFFSET_BUFFERS.end()
				? BindingType::StorageBuffer
				: BindingType::StorageBufferDynamic;

			append_binding_internal(binding_type, (uint32_t)storage_buffer.storage_buffer_name, storage_buffer.name, storage_buffer.set, storage_buffer.binding);
		}

		for (auto& image : reflection->SeparateImages())
		{
			append_binding_internal(BindingType::SampledImage, (uint32_t)image.shader_texture, image.name, image.set, image.binding);
		}

		for (auto& sampler : reflection->Samplers())
		{
			append_binding_internal(BindingType::Sampler, (uint32_t)sampler.sampler_name, sampler.name, sampler.set, sampler.binding);
		}
	}

	void ShaderProgram::AppendPushConstants(const ShaderModule& module, ShaderProgram::Stage stage)
	{
		auto* reflection = module.GetReflectionInfo();
		auto stage_flags = shader_stage_flag_map.at(stage);

		auto find_or_create_push_constants = [&](uint32_t id)
		{
			auto existing = std::find_if(push_constants.begin(), push_constants.end(), [id](PushConstants& item) {
				return item.id == id;
			});

			PushConstants* item = nullptr;

			if (existing != push_constants.end())
			{
				item = existing;
			}
			else
			{
				PushConstants pc;
				pc.id = id;
				pc.stage_flags = 0;
				push_constants.push_back(pc);
				item = &push_constants[push_constants.size() - 1];
			}

			return item;
		};

		for (auto& push_constant : reflection->PushConstants())
		{
			auto* item = find_or_create_push_constants(push_constant.id);
			item->offset = push_constant.offset;
			item->range = push_constant.size;
			item->name = push_constant.name;
			item->stage_flags |= (unsigned)stage_flags;
		}
	}

	ShaderProgram::ShaderProgram()
	{
		for (int i = 0; i < max_descriptor_sets; i++)
			descriptor_sets[i].set_index = i;
	}

	void ShaderProgram::Prepare()
	{
		assert(state == ShaderModule::State::Loading);

		std::map<std::pair<unsigned, unsigned>, int> existing_bindings;

		uint32_t vertex_hash = 0;
		uint32_t fragment_hash = 0;
		uint32_t compute_hash = 0;
		if (vertex_module)
		{
			vertex_hash = vertex_module->GetHash();
			AppendBindings(*vertex_module, Stage::Vertex, existing_bindings);
			AppendPushConstants(*vertex_module, Stage::Vertex);
		}

		if (fragment_module)
		{
			fragment_hash = fragment_module->GetHash();
			AppendBindings(*fragment_module, Stage::Fragment, existing_bindings);
			AppendPushConstants(*fragment_module, Stage::Fragment);
		}

		if (compute_module)
		{
			compute_hash = compute_module->GetHash();
			AppendBindings(*compute_module, Stage::Compute, existing_bindings);
			AppendPushConstants(*compute_module, Stage::Compute);
		}

		for (auto& set : descriptor_sets)
			std::sort(set.bindings.begin(), set.bindings.end());

		hash = ShaderProgram::CalculateHash(fragment_hash, vertex_hash, compute_hash);

		auto device = Engine::GetVulkanDevice();

		std::vector<vk::DescriptorSetLayoutBinding> bindings;

		for (auto& set : descriptor_sets)
		{
			if (set.Empty())
				continue;

			set.layout_bindings.clear();

			for (auto& binding : set.bindings)
			{
				set.layout_bindings.push_back(vk::DescriptorSetLayoutBinding(
					binding.address.binding, binding_type_map.at(binding.type), 1, GetShaderStageFlags(binding.stage_flags)
				));

			}

			set.layout_hash = FastHash(set.layout_bindings.data(), sizeof(vk::DescriptorSetLayoutBinding) * set.layout_bindings.size());

			vk::DescriptorSetLayoutCreateInfo layout_info({}, set.layout_bindings.size(), set.layout_bindings.data());
			set.layout = device.createDescriptorSetLayoutUnique(layout_info);
		}

	}

	uint32_t ShaderProgram::CalculateHash(uint32_t fragment_hash, uint32_t vertex_hash, uint32_t compute_hash)
	{
		std::array<uint32_t, 3> combined = { vertex_hash, fragment_hash, compute_hash };
		return FastHash(combined.data(), sizeof(combined));
	}

	uint32_t ShaderProgram::GetParameterNameHash(const std::string& name)
	{
		return FastHash(name);
	}

	uint32_t ShaderProgram::GetParameterNameHash(const char* name)
	{
		return FastHash(name);
	}

	const char* ShaderProgram::GetEntryPoint(Stage stage) const
	{
		WaitLoaded();

		switch (stage)
		{
			case Stage::Vertex: return vertex_module ? vertex_module->GetReflectionInfo()->EntryPoints()[0].name.c_str() : 0;
			case Stage::Fragment: return fragment_module ? fragment_module->GetReflectionInfo()->EntryPoints()[0].name.c_str() : 0;
			case Stage::Compute: return compute_module ? compute_module->GetReflectionInfo()->EntryPoints()[0].name.c_str() : 0;
			default: return 0;
		}
	}

	uint32_t ShaderProgram::GetHash() const
	{
		return hash;
	}

	const ShaderProgram::BindingData* ShaderProgram::GetBinding(unsigned set, unsigned binding) const
	{
		WaitLoaded();

		if (set >= descriptor_sets.size() || binding >= descriptor_sets[set].bindings.size())
			throw std::runtime_error("binding doesn't exist");

		return &descriptor_sets[set].bindings[binding];
	}

	const ShaderProgram::BindingData* ShaderProgram::GetBindingByName(const std::string& name) const
	{
		WaitLoaded();

		auto iter = name_binding_map.find(name);
		if (iter == name_binding_map.end())
			return nullptr;

		auto& address = iter->second;
		return GetBinding(address.set, address.binding);
	}

	bool ShaderProgram::TransitionState(ShaderModule::State old_state, ShaderModule::State new_state)
	{
		OPTICK_EVENT();
		return state.compare_exchange_strong(old_state, new_state);
	}
}