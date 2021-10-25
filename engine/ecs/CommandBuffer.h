#pragma once

#include <memory>
#include "ECS.h"

namespace ECS
{
	class EntityManager;

	class CommandBuffer
	{
	public:
		typedef uint32_t TransientEntityID;

	private:
		struct Command
		{
			virtual void Execute(EntityManager& manager) {};
			virtual ~Command() = default;
		};

		template <typename T>
		struct AddComponentCommand : public Command
		{
			AddComponentCommand(EntityID entity, T&& component, ComponentData&& data)
				: entity(entity)
				, transient_id(0)
				, component(std::move(component))
				, data(std::move(data))
				, use_transient_id(false)
				, command_buffer(nullptr)
			{}

			AddComponentCommand(CommandBuffer* command_buffer, TransientEntityID transient_id, T&& component, ComponentData&& data)
				: entity(0)
				, transient_id(transient_id)
				, component(std::move(component))
				, data(std::move(data))
				, use_transient_id(true)
				, command_buffer(command_buffer)
			{}

			AddComponentCommand(AddComponentCommand&&) = default;

			AddComponentCommand(const AddComponentCommand&) = delete;

			void Execute(EntityManager& manager) override
			{
				if (use_transient_id)
				{
					bool success = command_buffer->GetCreatedEntityID(transient_id, entity);
					if (!success)
						throw std::runtime_error("Invalid transient entity ID");
				}

				void* ptr = manager.AddComponent(entity, std::move(data));
				new (ptr) T(std::move(component));
			}

			CommandBuffer* command_buffer;
			EntityID entity;
			TransientEntityID transient_id;
			ComponentData data;
			T component;
			bool use_transient_id;
		};

		struct RemoveComponentCommand : public Command
		{
			RemoveComponentCommand(EntityID entity, ComponentHash component_hash)
				: component_hash(component_hash)
				, entity(entity)
				, transient_id(0)
				, use_transient_id(false)
				, command_buffer(nullptr)
			{}

			RemoveComponentCommand(CommandBuffer* command_buffer, TransientEntityID transient_id, ComponentHash component_hash)
				: component_hash(component_hash)
				, entity(0)
				, transient_id(transient_id)
				, use_transient_id(true)
				, command_buffer(command_buffer)
			{}

			void Execute(EntityManager& manager) override
			{
				if (use_transient_id)
				{
					bool success = command_buffer->GetCreatedEntityID(transient_id, entity);
					if (!success)
						throw std::runtime_error("Invalid transient entity ID");
				}

				manager.RemoveComponent(entity, component_hash);
			}

			CommandBuffer* command_buffer;
			ComponentHash component_hash;
			EntityID entity;
			TransientEntityID transient_id;
			bool use_transient_id;
		};

		struct CreateEntityCommand : public Command
		{
			CreateEntityCommand(CommandBuffer* command_buffer, TransientEntityID transient_id) : transient_id(transient_id), command_buffer(command_buffer) {}

			void Execute(EntityManager& manager) override
			{
				auto entity = manager.CreateEntity();
				command_buffer->EntityCreated(transient_id, entity);
			}

			TransientEntityID transient_id;
			CommandBuffer* command_buffer;
		};

		struct DestroyEntityCommand : public Command
		{
			DestroyEntityCommand(EntityID entity) : entity(entity) {}

			void Execute(EntityManager& manager) override
			{
				manager.DestroyEntity(entity);
			}

			EntityID entity;
		};

	public:

		CommandBuffer(EntityManager& manager) : manager(manager) {}

		template <typename T>
		void AddComponent(EntityID entity, T&& component)
		{
			auto data = ComponentData{ 
				GetComponentHash<T>(), 0, sizeof(T),
				[](const void* x){ static_cast<const T*>(x)->~T(); },
				[](void* x, void* other) { new (x) T(std::move(*(T*)other)); }
			};

			commands.emplace_back(std::make_unique<AddComponentCommand<T>>(entity, std::move(component), std::move(data)));
		}

		template <typename T>
		void AddComponent(TransientEntityID transient_id, T&& component)
		{
			auto data = ComponentData{ 
				GetComponentHash<T>(), 0, sizeof(T),
				[](const void* x){ static_cast<const T*>(x)->~T(); },
				[](void* x, void* other) { new (x) T(std::move(*(T*)other)); }
			};

			commands.emplace_back(std::make_unique<AddComponentCommand<T>>(this, transient_id, std::move(component), std::move(data)));
		}

		template <typename T>
		void RemoveComponent(EntityID entity)
		{
			commands.emplace_back(std::make_unique<RemoveComponentCommand>(entity, GetComponentHash<T>()));
		}

		template <typename T>
		void RemoveComponent(TransientEntityID transient_id)
		{
			commands.emplace_back(std::make_unique<RemoveComponentCommand>(this, transient_id, GetComponentHash<T>()));
		}

		void DestroyEntity(EntityID entity)
		{
			commands.emplace_back(std::make_unique<DestroyEntityCommand>(entity));
		}

		TransientEntityID CreateEntity()
		{
			commands.emplace_back(std::make_unique<CreateEntityCommand>(this, current_id));
			return current_id++;
		}

		bool GetCreatedEntityID(TransientEntityID transient_id, EntityID& out_entity)
		{
			auto it = created_entity_map.find(transient_id);
			if (it == created_entity_map.end())
				return false;

			out_entity = it->second;

			return true;
		}

		void EntityCreated(TransientEntityID transient_id, EntityID entity)
		{
			assert(created_entity_map.find(transient_id) == created_entity_map.end());
			created_entity_map[transient_id] = entity;
		}

		void Flush()
		{
			for (auto& command : commands)
			{
				command->Execute(manager);
			}

			commands.clear();
			created_entity_map.clear();
		}

	private:
		TransientEntityID current_id = 1;
		EntityManager& manager;
		std::unordered_map<TransientEntityID, EntityID> created_entity_map;
		std::vector<std::unique_ptr<Command>> commands;
	};
}