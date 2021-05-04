#include <string>
#include <filesystem>
#include <vector>

namespace Asset::Export
{
	struct Result
	{
		struct ExportedAsset
		{
			std::wstring path;
		};

		bool success = false;
		std::vector<ExportedAsset> exported_assets;
	};

	class Base
	{
	public:
		Base() = default;
		virtual ~Base() = default;
		
		virtual void Initialize(std::filesystem::path asset_relative_path, std::filesystem::path asset_base_path, std::filesystem::path cache_path)
		{
			this->asset_relative_path = std::move(asset_relative_path);
			this->asset_base_path = std::move(asset_base_path);
			this->cache_path = std::move(cache_path);
			asset_cache_directory = (this->cache_path / this->asset_relative_path).parent_path();
		}

		virtual Result Export() = 0;

	protected:
		std::filesystem::path asset_relative_path;
		std::filesystem::path asset_base_path;
		std::filesystem::path cache_path;
		std::filesystem::path asset_cache_directory;
	};

}