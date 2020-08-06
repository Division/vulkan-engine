#include "ResourceCache.h"
#include <vector>

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

	ResourceBase* Cache::GetResource(const std::wstring& filename)
	{
		auto it = resources.find(filename);
		if (it == resources.end())
			return nullptr;
		else
			return it->second.get();
	}

	void Cache::SetResource(const std::wstring& filename, std::unique_ptr<ResourceBase> resource)
	{
		auto it = resources.find(filename);
		if (it != resources.end())
			throw std::runtime_error("Resource already in cache");

		resources.insert(std::make_pair(filename, std::move(resource)));
	}

	void Cache::GCCollect()
	{
		std::vector<const std::wstring*> filenames_to_free;
		for (auto& pair : resources)
		{
			if (pair.second->GetRefCount() == 0)
				filenames_to_free.push_back(&pair.first);
		}

		for (auto* filename : filenames_to_free)
			resources.erase(*filename);
	}
}