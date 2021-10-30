#pragma once

#include <memory>
#include <physx/PxPhysicsAPI.h>

class IGamePhysicsDelegate
{
public:
	virtual ~IGamePhysicsDelegate() = default;
	virtual physx::PxSimulationFilterShader GetFilterShader() = 0;
	virtual physx::PxVec3 GetGravity() = 0;
	virtual void UpdatePhysics(float dt) = 0;
};

class IGame {
public:
	struct InitParams
	{
		uint32_t width = 1024;
		uint32_t height = 768;
		std::string title = "Vulkan Engine";
	};

	virtual ~IGame() = default;
	virtual void init() {}
	virtual void update(float dt) {}
	virtual IGamePhysicsDelegate* GetPhysicsDelegate() { return nullptr; }
	virtual void cleanup() {}
	virtual InitParams GetInitParams() const { return {}; }
};
