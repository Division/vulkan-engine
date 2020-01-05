#pragma once

#include <set>
#include <unordered_map>
#include "stdint.h"
#include "utils/Math.h"
#include "EntityChunks.h"
#include "components/Entity.h"

namespace core { namespace ECS {

	class EntityManager
	{
	public:
		static EntityManager* Get()
		{
			static std::unique_ptr<EntityManager> instance = nullptr;
			if (!instance)
				instance = std::make_unique<EntityManager>();
			return instance.get();
		}

		EntityID CreateEntity(ComponentSetHash set_hash = 0)
		{
			auto entity_id = ++id_counter;
			auto address = EntityAddress{ nullptr, (uint32_t)-1 };

			if (set_hash)
			{
				assert(false); // not ready yet
			}

			entity_address[entity_id] = address;

			AddComponent<EntityData>(entity_id); // EntityData component exists for all entities

			return entity_id;
		}

		void DestroyEntity(EntityID entity)
		{
			auto address_it = entity_address.find(entity);
			assert(address_it != entity_address.end());

			auto& address = address_it->second;
			address.chunk->RemoveEntity(address.index);
			entity_address.erase(address_it);
		}

		template<typename T>
		T* AddComponent(EntityID entity)
		{
			auto& old_address = entity_address.at(entity);
			auto layout = old_address.chunk ? old_address.chunk->GetComponentLayout() : ComponentLayout(CHUNK_SIZE);

			if (old_address.chunk)
			{
				auto old_hash = layout.GetHash();
				layout.AddComponent<T>();
				if (old_hash == layout.GetHash())
					return nullptr;
			}
			else
				layout.AddComponent<T>();

			auto chunk_list_it = chunks.find(layout.GetHash());
			if (chunk_list_it == chunks.end())
			{
				chunks[layout.GetHash()] = std::make_unique<ChunkList>(layout, std::bind(&EntityManager::OnEntityAddressChanged, this, std::placeholders::_1, std::placeholders::_2));
				chunk_list_it = chunks.find(layout.GetHash());
			}

			auto* chunk = chunk_list_it->second->GetFirstChunk();
			auto new_address = chunk->AddEntity(old_address.chunk ? &old_address : nullptr);
			if (old_address.chunk)
				old_address.chunk->RemoveEntity(old_address.index);

			return (T*)new_address.chunk->GetComponentPointer(new_address.index, GetComponentHash<T>());
		}

		template<typename T>
		T& GetComponent(EntityID entity)
		{
			auto address = entity_address.at(entity);
			return (T*)address.chunk->GetComponentPointer(address.index, GetComponentHash<T>());
		}

		void OnEntityAddressChanged(uint64_t entity_id, EntityAddress new_address)
		{
			auto& address = entity_address.at(entity_id);
			address = new_address;

			auto* entity = (EntityData*)address.chunk->GetComponentPointer(address.index, GetComponentHash<EntityData>());
			entity->address = address;
		}

	private:
		EntityID id_counter = 0;
		std::unordered_map<ComponentSetHash, std::unique_ptr<ChunkList>> chunks;
		std::unordered_map<EntityID, EntityAddress> entity_address;
	};

} }