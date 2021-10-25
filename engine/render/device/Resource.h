#pragma once

#include <mutex>
#include <atomic>
#include <array>
#include <vector>
#include "system/BaseHandle.h"

namespace Device
{
	using Resource = System::Resource;
	
	using Releaser = System::ResourceReleaser<3u>;

	template<typename T>
	using Handle = System::BaseHandle<T, Releaser>;
}