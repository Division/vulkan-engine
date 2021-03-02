#include "ResourceCache.h"
#include <vector>
#include "utils/StringUtils.h"
#include "Handle.h"

namespace Resources
{
	namespace
	{
		std::unique_ptr<Cache> cache = std::make_unique<Cache>();
	}

	Cache& Cache::Get()
	{
		return *cache;
	}

	void Cache::Destroy()
	{
		bool has_leaks = false;

		// Destroying resources in multiple iterations since resource may retain references to other resources
		while (true)
		{
			const auto released_resource_count = GCCollect();
			const auto released_common_count = Common::GetReleaser().Clear(); // Need to clear it since Resources from cache could contain Common::Handle
			if (released_resource_count == 0 && released_common_count == 0)
				break;
		}

		for (auto& pair : resources)
		{
			if (pair.second->GetRefCount() != 0)
			{
				std::wcout << L"Resource leaked: " << pair.first << std::endl;
				has_leaks = true;
			}
		}

		cache = nullptr;

		if (has_leaks)
			throw std::runtime_error("Resource leaks detected");
	}

	ResourceBase* Cache::GetResource(uint32_t key)
	{
		auto it = resources.find(key);
		if (it == resources.end())
			return nullptr;
		else
			return it->second.get();
	}

	void Cache::SetResource(uint32_t key, std::unique_ptr<ResourceBase> resource)
	{
		auto it = resources.find(key);
		if (it != resources.end())
			throw std::runtime_error("Resource already in cache");

		resources.insert(std::make_pair(key, std::move(resource)));
	}

	size_t Cache::GCCollect()
	{
		size_t destroyed = 0;

		std::vector<uint32_t> filenames_to_free;
		for (auto& pair : resources)
		{
			if (pair.second->GetRefCount() == 0)
			{
				filenames_to_free.push_back(pair.first);
				destroyed += 1;
			}
		}

		for (auto filename : filenames_to_free)
			resources.erase(filename);

		return destroyed;
	}

	Exception::Exception(const std::wstring& filename)
	{
		message_stream << "Resource exception [" << utils::WStringToString(filename) << "]: ";
	}

	Exception::Exception(const Exception& other)
	{
		message_stream << other.message_stream.str();
	}

}