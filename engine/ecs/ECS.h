#pragma once

#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <functional>
#include "stdint.h"
#include "utils/Math.h"
#include "EntityChunks.h"
#include "components/Entity.h"

namespace ECS {

	class EntityManager
	{
	public:
		typedef std::function<void(EntityID)> EntityCallback;
		
		struct CallbackHandle
		{
			CallbackHandle() : manager(nullptr), id(0) {}
			CallbackHandle(EntityManager* manager, uint64_t id)
				: manager(manager), id(id) {}
			CallbackHandle(CallbackHandle&&) = default;
			CallbackHandle& operator=(CallbackHandle&& other)
			{
				if (id)
					manager->RemoveEntityDestroyCallback(id);
				id = std::move(other.id);
				manager = other.manager;
				other.id = 0;
				other.manager = nullptr;
				return *this;
			}
			CallbackHandle& operator=(const CallbackHandle&) = delete;

			~CallbackHandle()
			{
				if (id)
					manager->RemoveEntityDestroyCallback(id);
			}

			EntityManager* manager;
			uint64_t id;
		};

		struct EntityCallbackData
		{
			EntityCallback callback;
			uint64_t id;
			bool operator==(uint64_t compare_id) { return id == compare_id; }
		};

		struct ProcessingHandle
		{
			ProcessingHandle(EntityManager& manager) 
				: manager(manager)
			{
				manager.processing_counter += 1;
			}

			ProcessingHandle() = delete;
			ProcessingHandle(ProcessingHandle&&) = delete;
			ProcessingHandle(const ProcessingHandle&) = delete;

			~ProcessingHandle()
			{
				manager.processing_counter -= 1;
			}

			EntityManager& manager;
		};

		~EntityManager()
		{
			assert(processing_counter.load() == 0);

			while (!entity_address.empty())
				DestroyEntity((*entity_address.begin()).first);
		}

		template<typename T>
		void AddStaticComponent(T* component)
		{
			std::unique_lock lock(static_component_mutex);
			const auto hash = GetComponentHash<T>();
			if (static_components.find(hash) != static_components.end())
				throw std::runtime_error("Static component already exists");

			static_components.insert(std::make_pair(hash, component));
		}

		template<typename T>
		T* GetStaticComponent()
		{
			std::shared_lock lock(static_component_mutex);
			auto it = static_components.find(GetComponentHash<T>());
			return it == static_components.end() ? nullptr : reinterpret_cast<T*>(it->second);
		}

		ProcessingHandle GetProcessingHandle()
		{
			return ProcessingHandle(*this);
		}

		CallbackHandle AddEntityDestroyCallback(EntityCallback callback)
		{
			entity_destroy_callbacks.push_back({ callback, ++callback_id });

			return CallbackHandle(this, callback_id);
		}

		void RemoveEntityDestroyCallback(uint64_t id)
		{
			auto found = std::find(entity_destroy_callbacks.begin(), entity_destroy_callbacks.end(), id);
			if (found != entity_destroy_callbacks.end())
			{
				entity_destroy_callbacks.erase(found);
			}
		}

		EntityID CreateEntity()
		{
			ValidateNoProcessing();
			auto entity_id = ++id_counter;

			{
				std::unique_lock lock(mutex);
				auto address = EntityAddress{ nullptr, (uint32_t)-1 };
				entity_address[entity_id] = address;
			}

			auto* entity = AddComponent<EntityData>(entity_id); // EntityData component exists for all entities
			entity->id = entity_id;
			return entity_id;
		}

		void DestroyEntity(EntityID entity)
		{
			std::unique_lock lock(mutex);
			ValidateNoProcessing();

			auto address_it = entity_address.find(entity);
			assert(address_it != entity_address.end());
			TriggerDestroyCallbacks(entity);

			auto& address = address_it->second;
			address.chunk->RemoveEntity(address.index);
			entity_address.erase(address_it);
		}

		template<typename T, typename ...Args>
		T* AddComponent(EntityID entity, Args&& ...args)
		{
			std::unique_lock lock(mutex);
			ValidateNoProcessing();

			auto entity_address_it = entity_address.find(entity);
			assert(entity_address_it != entity_address.end());

			auto old_address = entity_address_it->second;
			auto layout = old_address.chunk ? old_address.chunk->GetComponentLayout() : ComponentLayout(CHUNK_SIZE);

			if (old_address.chunk)
			{
				if (!layout.AddComponent<T>())
					return nullptr;
			}
			else
				layout.AddComponent<T>();

			auto* chunk_list = GetOrCreateChunkList(layout);
			auto* chunk = chunk_list->GetFirstChunk();
			auto new_address = chunk->AddEntity(entity, old_address.chunk ? &old_address : nullptr);

			if (old_address.chunk)
				old_address.chunk->RemoveEntity(old_address.index);

			auto* pointer = (T*)new_address.chunk->GetComponentPointer(new_address.index, GetComponentHash<T>());
			new (pointer) T(std::forward<Args>(args)...); // Apply constructor

			return pointer;
		}

		void* AddComponent(EntityID entity, ComponentData&& data)
		{
			std::unique_lock lock(mutex);
			ValidateNoProcessing();

			auto entity_address_it = entity_address.find(entity);
			assert(entity_address_it != entity_address.end());

			auto old_address = entity_address_it->second;
			auto layout = old_address.chunk ? old_address.chunk->GetComponentLayout() : ComponentLayout(CHUNK_SIZE);

			auto hash = data.hash;

			if (old_address.chunk)
			{
				if (!layout.AddComponent(std::move(data)))
					return nullptr;
			}
			else
				layout.AddComponent(std::move(data));

			auto* chunk_list = GetOrCreateChunkList(layout);
			auto* chunk = chunk_list->GetFirstChunk();
			auto new_address = chunk->AddEntity(entity, old_address.chunk ? &old_address : nullptr);

			if (old_address.chunk)
				old_address.chunk->RemoveEntity(old_address.index);

			auto* pointer = new_address.chunk->GetComponentPointer(new_address.index, hash);

			return pointer;
		}

		template<typename T>
		void RemoveComponent(EntityID entity)
		{
			RemoveComponent(entity, GetComponentHash<T>());
		}

		void RemoveComponent(EntityID entity, ComponentHash hash)
		{
			std::unique_lock lock(mutex);
			ValidateNoProcessing();

			auto entity_address_it = entity_address.find(entity);
			assert(entity_address_it != entity_address.end());

			auto old_address = entity_address_it->second;
			assert(old_address.chunk);
			auto layout = old_address.chunk->GetComponentLayout();
			auto* component_data = layout.GetComponentData(hash);

			if (!component_data)
				throw std::runtime_error("Component doesn't exist");

			layout.RemoveComponent(hash);

			auto* chunk_list = GetOrCreateChunkList(layout);
			auto* chunk = chunk_list->GetFirstChunk();
			auto new_address = chunk->AddEntity(entity, &old_address);
			old_address.chunk->RemoveEntity(old_address.index);
		}

		ChunkList* GetOrCreateChunkList(const ComponentLayout& layout)
		{
			ValidateNoProcessing();

			auto chunk_list_it = chunks.find(layout.GetHash());
			if (chunk_list_it == chunks.end())
			{
				auto it = chunks.insert(std::make_pair(layout.GetHash(), std::make_unique<ChunkList>(layout, std::bind(&EntityManager::OnEntityAddressChanged, this, std::placeholders::_1, std::placeholders::_2))));
				chunk_list_it = it.first;
			}

			return chunk_list_it->second.get();
		}

		template<typename T>
		T* GetComponent(EntityID entity)
		{
			std::shared_lock lock(mutex);
			auto address = entity_address.at(entity);
			return (T*)address.chunk->GetComponentPointer(address.index, GetComponentHash<T>());
		}

		void OnEntityAddressChanged(uint64_t entity_id, EntityAddress new_address)
		{
			auto& address = entity_address.at(entity_id);
			address = new_address;
		}

		const auto& GetChunkMap() const { return chunks; }
		const auto& GetEntityAddressMap() const { return entity_address; }

		void ForEachChunkList(std::function<void(ChunkList*)> callback, std::function<bool(ChunkList*)> predicate)
		{
			std::shared_lock lock(mutex);

			for (auto& chunk : chunks)
				if (predicate(chunk.second.get()))
					callback(chunk.second.get());
		}

		ChunkList::List GetChunkLists(std::function<bool(ChunkList*)> predicate)
		{
			ChunkList::List list;

			ForEachChunkList([&list](ChunkList* chunk_list) {
				list.push_back(chunk_list);
			}, predicate);

			return list;
		}

		template <typename T>
		ChunkList::List GetChunkListsWithComponent()
		{
			auto hash = GetComponentHash<T>();
			return GetChunkLists([=](ChunkList* chunk_list) {
				return chunk_list->HasComponent(hash);
			});
		}

		template <typename T1, typename T2>
		ChunkList::List GetChunkListsWithComponents()
		{
			auto hash1 = GetComponentHash<T1>();
			auto hash2 = GetComponentHash<T2>();
			return GetChunkLists([=](ChunkList* chunk_list) {
				return chunk_list->HasComponent(hash1) && chunk_list->HasComponent(hash2);
			});
		}

		template <typename T>
		ChunkList::List GetChunkListsWithoutComponent()
		{
			auto hash = GetComponentHash<T>();
			return GetChunkLists([=](ChunkList* chunk_list) {
				return !chunk_list->HasComponent(hash);
			});
		}

		template <typename T1, typename T2>
		ChunkList::List GetChunkListsWithoutComponents()
		{
			auto hash1 = GetComponentHash<T1>();
			auto hash2 = GetComponentHash<T2>();
			return GetChunkLists([=](ChunkList* chunk_list) {
				return !(chunk_list->HasComponent(hash1) && chunk_list->HasComponent(hash2));
			});
		}

		void ValidateNoProcessing()
		{
			if (processing_counter.load() != 0)
				throw std::runtime_error("Invalid operation during system processing");
		}

		bool EntityExists(EntityID entity) const { return entity_address.find(entity) != entity_address.end(); }

	private:
		void TriggerDestroyCallbacks(EntityID id)
		{
			for (auto& callback : entity_destroy_callbacks)
				callback.callback(id);
		}

	private:
		std::shared_mutex mutex;
		std::shared_mutex static_component_mutex;
		std::atomic_uint32_t processing_counter = 0;
		uint64_t callback_id = 0;
		std::atomic<EntityID> id_counter = 0;
		std::unordered_map<ComponentSetHash, std::unique_ptr<ChunkList>> chunks;
		std::unordered_map<EntityID, EntityAddress> entity_address;
		std::unordered_map<ComponentHash, void*> static_components;
		std::vector<EntityCallbackData> entity_destroy_callbacks;
	};

}