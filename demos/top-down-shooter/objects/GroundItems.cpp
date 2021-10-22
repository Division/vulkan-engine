#include "GroundItems.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Batching.h"
#include "ecs/components/BehaviourList.h"
#include "render/debug/DebugDraw.h"

namespace scene
{
	using namespace ECS;

	GrassTile::GrassTile(glm::ivec2 position)
		: position(position)
	{

	}

	namespace
	{
		struct GrassData
		{
			const wchar_t* material = nullptr;
			const wchar_t* mesh = nullptr;
		};

		constexpr GrassData GRASS_LIST[] = {
			{ L"assets/top-down-shooter/vegetation/Kentucky.mat", L"assets/top-down-shooter/vegetation/Kentucky.mesh" },
			{ L"assets/top-down-shooter/vegetation/Lamium.mat", L"assets/top-down-shooter/vegetation/Lamium.mesh" }
		};
	}

	void GrassTile::Awake()
	{
		{
			auto transform = AddComponent<components::Transform>();
		}

		{
			auto renderer = AddComponent<components::MultiMeshRenderer>();
		}

		{
			auto batching = AddComponent<components::BatchingVolume>();

			auto& grass_handle = GroundItemsCache::GetInstance()->GetGrassHandle(GroundItemsCache::GrassType::Lamium);

			const vec3 origin = GroundItemsCache::GetTileOrigin(position);

			for (uint32_t i = 0; i < 10; i++)
			{
				for (uint32_t j = 0; j < 10; j++)
				{
					auto& src = batching->src_meshes.emplace_back();
					src.position = origin + vec3(i * 0.4f, 0, j * 0.4f);
					src.rotation = glm::angleAxis(Random() * (float)M_PI * 2, vec3(0, 1, 0));
					src.scale = vec3(4);
					src.mesh = grass_handle.mesh->GetMesh(0);
					src.material = grass_handle.material->Get();
				}
			}

		}
	}

	GroundItemsCache::GroundItemsCache(ECS::EntityManager& manager)
		: manager(manager)
	{
		instance = this;

		for (uint32_t i = 0; i < grass_handles.size(); i++)
		{
			grass_handles[i].material = Resources::MaterialResource::Handle(GRASS_LIST[i].material);
			grass_handles[i].mesh = Resources::MultiMesh::KeepDataHandle(GRASS_LIST[i].mesh);
		}
	}

	void GroundItemsCache::Update(float dt)
	{
		const auto player_pos = Game::GetInstance()->GetPlayerPosition();
		const auto tile = TileFromCoord(player_pos);
		if (tile != current_tile)
			OnMovedToTile(tile);
	}

	void GroundItemsCache::OnMovedToTile(glm::ivec2 tile)
	{
		//std::cout << "PLAYER TILE " << tile.x << ", " << tile.y << "\n";

		if (current_tile == INIT_TILE)
		{
			for (int i = tile.x - TILE_VISIBLE_RADIUS; i <= tile.x + TILE_VISIBLE_RADIUS; i++)
				for (int j = tile.y - TILE_VISIBLE_RADIUS; j <= tile.y + TILE_VISIBLE_RADIUS; j++)
					AwakeTile(glm::ivec2(i, j));
		}
		else
		{
			const auto last_aabb = VisibleAABBForTile(current_tile);
			const auto new_aabb = VisibleAABBForTile(tile);

			ForEachTile(last_aabb, [&new_aabb, this](ivec2 value) {
				if (!new_aabb.IntersectsPoint(value))
					SleepTile(value);
			});

			ForEachTile(new_aabb, [&last_aabb, this](ivec2 value) {
				if (!last_aabb.IntersectsPoint(value))
					AwakeTile(value);
			});
		}

		current_tile = tile;
	}

	void GroundItemsCache::AwakeTile(glm::ivec2 tile)
	{
		auto id = manager.CreateEntity();
		auto behaviour = manager.AddComponent<components::BehaviourList>(id);
		auto grass = std::make_unique<GrassTile>(tile);

		auto it = tiles.find(tile);
		if (it != tiles.end())
		{

		}
		else
		{
			it = tiles.insert({ tile, {} }).first;
		}

		assert(!it->second.tile);
		it->second.tile = grass.get();
		behaviour->AddBehaviour(std::move(grass));
	}

	void GroundItemsCache::SleepTile(glm::ivec2 tile)
	{
		auto& cached_tile = tiles.at(tile);
		assert(cached_tile.tile);

		manager.DestroyEntity(cached_tile.tile->GetEntity());
		cached_tile.tile = nullptr;

		//std::cout << "SleepTile " << tile.x << ", " << tile.y << "\n";
	}

	ivec2 GroundItemsCache::TileFromCoord(vec3 coord)
	{
		return ivec2((int)floorf(coord.x / TILE_SIZE), (int)floorf(coord.z / TILE_SIZE));
	}

	AABBIVec2 GroundItemsCache::VisibleAABBForTile(ivec2 tile)
	{
		return AABBIVec2(tile - ivec2((int)TILE_VISIBLE_RADIUS), tile + ivec2((int)TILE_VISIBLE_RADIUS));
	}
}