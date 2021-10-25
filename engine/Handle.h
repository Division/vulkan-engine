#pragma once

#include <mutex>
#include <atomic>
#include <array>
#include <vector>
#include "system/BaseHandle.h"

namespace Common
{
	using Resource = System::Resource;
	
	using Releaser = System::ResourceReleaser<1u>;

	template<typename T>
	using Handle = System::BaseHandle<T, Releaser>;
}