#include "GroundItems.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Batching.h"
#include "ecs/components/BehaviourList.h"
#include "render/debug/DebugDraw.h"
#include "stb/stb_image.h"
#include "loader/FileLoader.h"

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
			float scale = 1.0f;
			float density = 1.0f;
		};

		constexpr GrassData GRASS_LIST[] = {
			{ L"assets/top-down-shooter/vegetation/Kentucky.mat", L"assets/top-down-shooter/vegetation/Kentucky.mesh", 4, 1.0f },
			{ L"assets/top-down-shooter/vegetation/Clovers.mat", L"assets/top-down-shooter/vegetation/Clovers.mesh", 3.0f, 0.1f },
			{ L"assets/top-down-shooter/vegetation/Lamium.mat", L"assets/top-down-shooter/vegetation/Lamium.mesh", 4, 0.3f },
			{ L"assets/top-down-shooter/vegetation/CamomileFlower.mat", L"assets/top-down-shooter/vegetation/CamomileFlower.mesh", 2.5f, 1.5f },
		};

		constexpr float WorldToUVMultiplier = 0.3f * 0.05f;
	}

	void GrassTile::Awake()
	{
		OPTICK_EVENT();

		{
			auto transform = AddComponent<components::Transform>();
		}

		{
			auto renderer = AddComponent<components::MultiMeshRenderer>();
		}

		{
			auto batching = AddComponent<components::BatchingVolume>();
			batching->include_origin = true;

			const vec3 origin = GroundItemsCache::GetTileOrigin(position);
			
			const auto texel_world_size = GroundItemsCache::GetInstance()->GetGrassTexelSize();
			const auto texel_count = vec2(GroundItemsCache::TILE_SIZE) / texel_world_size;

			for (uint32_t i = 0; i < texel_count.x; i++)
			{
				for (uint32_t j = 0; j < texel_count.y; j++)
				{
					const auto position = origin + vec3(i * texel_world_size.x, 0, j * texel_world_size.y);
					const auto grass_value = GroundItemsCache::GetInstance()->GetGrassValueFromWorldPos(position);
					for (uint32_t g = 0; g < 4; g++)
					{
						auto grass_type = (GroundItemsCache::GrassType)g;
						auto& grass_data = GRASS_LIST[g];
						if (grass_value[g] < 0.3f)
							continue;

						float count = 0.5f * grass_data.density;
						auto fracpart = std::modf(count, &count);

						if (fracpart > 0.01f && Random() < fracpart)
							count += 1;

						for (uint32_t c = 0; c < count; c++)
						{
							auto& grass_handle = GroundItemsCache::GetInstance()->GetGrassHandle(grass_type);
							auto& src = batching->src_meshes.emplace_back();
							src.position = position + vec3(Random() * texel_world_size.x, 0, Random() * texel_world_size.y);
							src.rotation = glm::angleAxis(Random() * (float)M_PI * 2, vec3(0, 1, 0));
							src.scale = vec3(grass_data.scale) * grass_value[g] * (0.8f + 0.4f * Random());
							src.mesh = grass_handle.mesh->GetMesh(0);
							src.material = grass_handle.material->Get();
						}
					}
				}
			}

		}
	}

	GroundItemsCache::GroundItemsCache(ECS::EntityManager& manager, const std::wstring& grass_texture)
		: manager(manager)
	{
		instance = this;

		auto data = loader::LoadFile(grass_texture);
		assert(data.size());

		if (data.size())
		{
			grass_map = GrassMap();

			int channels;
			grass_map->data = GrassMap::TextureData(stbi_load_from_memory(data.data(), data.size(), &grass_map->width, &grass_map->height, &channels, 4), [](unsigned char* data) { stbi_image_free(data); });
			if (!(grass_map->data.get()))
				grass_map.reset();
			else
			{
				grass_map->texel_size = vec2(1.0f / grass_map->width, 1.0f / grass_map->height) / WorldToUVMultiplier;
			}
		}

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

		/*
		for (int i = -10; i <= 10; i++)
			for (int j = -10; j <= 10; j++)
			{
				vec3 world_pos = player_pos + vec3(i, 0, j) * 0.5f;
				Engine::Get()->GetDebugDraw()->DrawPoint(world_pos, GetGrassValueFromWorldPos(world_pos), 20);
			}
		*/
		
	}

	void GroundItemsCache::OnMovedToTile(glm::ivec2 tile)
	{
		OPTICK_EVENT();
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

	vec4 GroundItemsCache::GetGrassValueFromWorldPos(vec3 world_pos) const
	{
		const auto uv = vec2(world_pos.x, world_pos.z) * WorldToUVMultiplier;
		return GetGrassValue(uv);
	}

	ivec2 GroundItemsCache::TileFromCoord(vec3 coord)
	{
		return ivec2((int)floorf(coord.x / TILE_SIZE), (int)floorf(coord.z / TILE_SIZE));
	}

	vec4 GroundItemsCache::GrassMap::GetInterpolatedValue(vec2 coord) const
	{
		float intpart;
		auto uv = vec2(std::modf(coord.x, &intpart), std::modf(coord.y, &intpart));
		if (uv.x < 0) uv.x += 1.0f;
		if (uv.y < 0) uv.y += 1.0f;

		uv *= vec2(width, height);
		ivec2 offset00 = ivec2(uv);

		const uint32_t stride = 4;
		const uint32_t row_size = stride * width;

		Vector4b& color00 = (Vector4b&)(*(data.get() + offset00.x * stride + offset00.y * row_size));

		//Vector4b()

		return color00.ToNormalizedFloat();
		//ivec2 offsetB = offsetA + ivec2(1);
	}

	AABBIVec2 GroundItemsCache::VisibleAABBForTile(ivec2 tile)
	{
		return AABBIVec2(tile - ivec2((int)TILE_VISIBLE_RADIUS), tile + ivec2((int)TILE_VISIBLE_RADIUS));
	}
}