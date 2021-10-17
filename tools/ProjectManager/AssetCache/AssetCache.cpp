#include "AssetCache.h"
#include <filesystem>
#include <iomanip>
#include "AssetTypes.h"
#include "loader/FileLoader.h"
#include "rapidjson/prettywriter.h"
#include "utils/StringUtils.h"
#include "System/Dialogs.h"

namespace fs = std::filesystem;
using namespace rapidjson;

namespace Asset
{
	constexpr auto ASSET_METADATA_NAME = L".asset_metadata.json";
	constexpr auto ASSET_CACHE_NAME = L".asset_cache.txt";
	constexpr auto ASSET_CACHE_FOLDER_SUFFIX = L".cache";

	FolderCache::FolderCache(fs::path path, FolderMetadata* folder_metadata, const wchar_t* asset_root, const wchar_t* cache_root)
		: path(std::move(path))
		, folder_metadata(folder_metadata)
		, asset_root(asset_root)
		, cache_root(cache_root)
	{
		const fs::path cache_file_path = path / ASSET_CACHE_NAME;
		std::wifstream stream(cache_file_path);
		if (!stream.is_open())
			return;

		try
		{
			std::wstring token;
			stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

			auto parse_file = [&stream, &token](std::wstring& filename, uint64_t& hash, uint64_t& time)
			{
				filename = L"";
				hash = 0;
				time = 0;

				stream >> std::quoted(filename);
				if (filename.empty())
					throw std::runtime_error("Parsing error: filename not found");

				stream >> token;
				if (token.empty())
					throw std::runtime_error("Parsing error: hash not found");

				hash = utils::ReadHexString(utils::WStringToString(token));

				stream >> time;
				stream >> std::ws;

				if (/*hash == 0 || */time == 0) // 0 hash is valid for empty files
				{
					throw std::runtime_error("Parsing error: hash and time can't be zero");
				}
			};

			std::wstring filename;
			uint64_t hash, timestamp;

			SrcEntry* current_input = nullptr;
			while (true)
			{
				stream >> token;

				if (token == L"in:")
				{
					parse_file(filename, hash, timestamp);
					current_input = AddInputFile(filename.c_str(), hash, timestamp);
				}
				else if (token == L"out:")
				{
					if (!current_input)
						throw std::runtime_error("Parsing error: missing in:");

					stream >> std::ws;
					while (!stream.eof() && stream.peek() == L'"')
					{
						parse_file(filename, hash, timestamp);
						AddOutputFile(current_input->filename.c_str(), std::move(filename), hash, timestamp);
					}

					current_input = nullptr;
				}
				else
					throw std::runtime_error("Parsing error: in: or out: expected");

				if (!stream.eof())
					stream >> std::ws;

				if (stream.eof())
					break;
			}

		}
		catch (...)
		{
			// Failed parsing
			input_entries.clear();
			// TODO: log
		}
	}

	bool FolderCache::Save()
	{

		const auto cache_file_path = fs::path(path) / ASSET_CACHE_NAME;
		if (input_entries.empty())
		{
			FileSystem::DeleteFile(cache_file_path);
			return false;
		}

		FileSystem::CreateDirectories(fs::path(path));
		std::wofstream stream(cache_file_path);

		try
		{
			stream.exceptions(std::wofstream::badbit | std::wofstream::failbit);
			for (auto& input_it : input_entries)
			{
				auto* input = input_it.second.get();

				stream << L"in:\n";
				stream << L"\t\"" << input->filename << L"\" " << utils::StringToWString(utils::WriteHexString(input->hash)) << L" " << input->timestamp << L"\n";
				if (input->outputs.empty())
					continue;

				stream << L"out:\n";
				if (input->outputs.size())
				{
					for (auto& out_it : input->outputs)
					{
						auto* output = out_it.second.get();
						stream << L"\t\"" << output->filename << L"\" " << utils::StringToWString(utils::WriteHexString(output->hash)) << L" " << output->timestamp << L"\n";
					}
				}
			}
		}
		catch (...)
		{
			FileSystem::DeleteFile(cache_file_path);
			return false;
		}

		return true;
	}

	SrcEntry* FolderCache::GetInputFile(const wchar_t* input_filename)
	{
		auto it = input_entries.find(input_filename);
		if (it != input_entries.end())
			return it->second.get();
		else
			return nullptr;
	}

	void FolderCache::RemoveInputFile(const wchar_t* input_filename)
	{
		auto it = input_entries.find(input_filename);
		assert(it != input_entries.end());
		if (it != input_entries.end())
			input_entries.erase(it);
	}

	SrcEntry* FolderCache::AddInputFile(const wchar_t* input_filename, uint64_t input_hash, uint64_t timestamp)
	{
		auto it = input_entries.find(input_filename);
		assert(it == input_entries.end());
		assert(input_hash != 0 && timestamp != 0);

		auto entry = std::make_unique<SrcEntry>();
		entry->filename = input_filename;
		entry->hash = input_hash;
		entry->timestamp = timestamp;
		auto result = input_entries.insert({ entry->filename.c_str(), std::move(entry) });
		return result.first->second.get();
	}

	void FolderCache::AddOutputFile(const wchar_t* input_filename, std::wstring output_filename, uint64_t output_hash, uint64_t timestamp)
	{
		auto it = input_entries.find(input_filename);
		assert(it != input_entries.end());
		assert(output_hash != 0 && timestamp != 0);

		if (output_hash == 0 || timestamp == 0)
			return;

		auto entry = std::make_unique<OutputEntry>(output_hash, timestamp, std::move(output_filename));
		it->second->outputs.insert({ entry->filename.c_str(), std::move(entry) });
	}

	bool FolderCache::HasModifications(const wchar_t* input_filename)
	{
		auto it = input_entries.find(input_filename);
		if (it == input_entries.end())
			return true;

		SrcEntry* entry = it->second.get();
		assert(entry);

		const auto full_src_path = fs::path(asset_root) / entry->filename;

		//if (entry->timestamp != FileSystem::GetFileTimestamp(full_src_path))
			if (entry->hash != FileSystem::GetFileHash(full_src_path))
				return true;

		for (auto& output : entry->outputs)
		{
			OutputEntry* output_entry = output.second.get();
			const auto full_output_path = fs::path(cache_root) / output_entry->filename;
			//if (output_entry->timestamp != FileSystem::GetFileTimestamp(full_output_path))
				if (output_entry->hash != FileSystem::GetFileHash(full_output_path))
					return true;
		}

		return false;
	}

	std::wstring FolderMetadata::GetAssetPath(const std::wstring& filename) const
	{
		return FileSystem::FormatPath(path + L"/" + filename);
	}

	bool FolderMetadata::NeedsReexport() const
	{
		for (auto& entry : entries)
		{
			if (!entry.second->ShouldSerialize())
				continue;

			if (folder_cache->HasModifications(GetAssetPath(entry.second->GetFilename()).c_str()))
				return true;
		}

		return false;
	}

	class Cache::TaskManager
	{
	public:
		class Task
		{
		public:
			virtual ~Task() {}
			virtual void Execute() {}
		};

		class RefreshDirectoryTask : public Task
		{
		public:
			RefreshDirectoryTask(std::wstring directory) : directory(directory) {}

			void Execute() override
			{
				fs::path metadata_path = directory / ASSET_METADATA_NAME;

				bool should_refresh = false;

				if (!fs::exists(metadata_path))
				{
					should_refresh = true;
				}
				else
				{
					// 
				}
			}

		private:
			fs::path directory;
		};

		void Wait()
		{

		}

	private:
		std::deque<Task> task_queue;
	};

	Cache::Cache()
	{
		task_manager = std::make_unique<TaskManager>();
		create_functions.push_back({ [](const std::wstring& filename) 
		{
			return filename == L"texture" || 
				utils::EndsWith(filename, std::wstring(L".png")) || 
				utils::EndsWith(filename, std::wstring(L".jpg"));
		}, [] { return std::make_unique<Asset::Types::Texture>(); } });

		create_functions.push_back({ [](const std::wstring& filename)
		{
			return filename == L"fbx" || utils::EndsWith(filename, std::wstring(L".fbx"));
		}, [] { return std::make_unique<Asset::Types::FBX>(); } });

		create_functions.push_back({ [](const std::wstring& filename)
		{
			return filename == L"copy" 
				|| utils::EndsWith(filename, std::wstring(L".dds")) 
				|| utils::EndsWith(filename, std::wstring(L".ktx"))
				|| utils::EndsWith(filename, std::wstring(L".mat"))
				|| utils::EndsWith(filename, std::wstring(L".hlsl"))
				|| utils::EndsWith(filename, std::wstring(L".entity"));
		}, [] { return std::make_unique<Asset::Types::PlainCopy>(); } });

		// Should go last
		create_functions.push_back({ [](const std::wstring& filename) { return true; }, [] { return std::make_unique<Asset::Types::IgnoredEntry>(); } });
	}

	Cache::~Cache() = default;

	Cache::AssetEntryCreateFunction Cache::GetAssetCreateFunction(const std::wstring filename)
	{
		for (auto& pair : create_functions)
			if (pair.first(filename))
				return pair.second;

		return nullptr;
	}

	void Cache::SetupSymlinks()
	{
		FileSystem::CreateDirectorySymlink(L"./assets", cache_directory);
	}

	void Cache::SetProjectDirectory(const std::wstring& dir)
	{
		try {
			fs::path absolute = fs::absolute(fs::path(dir));
			if (!fs::is_directory(absolute))
				absolute = absolute.parent_path();

			project_directory = absolute;
			cache_directory = project_directory + ASSET_CACHE_FOLDER_SUFFIX;

			file_tree.SetPath(project_directory, project_directory);
			SetupSymlinks();
		}
		catch (fs::filesystem_error)
		{
			return;
		}

		Reload();
	}

	std::wstring Cache::GetMetadataPath(const fs::path& folder)
	{
		return (fs::path(project_directory) / folder / ASSET_METADATA_NAME).lexically_normal();
	}

	std::wstring Cache::GetFolderCachePath(const std::filesystem::path& folder)
	{
		return (fs::path(cache_directory) / folder / ASSET_CACHE_NAME).lexically_normal();
	}

	bool Cache::ParseMetadata(FolderMetadata& out, std::vector<uint8_t>& data, const std::filesystem::path& folder)
	{
		std::wstringstream sstream;

		sstream.write((wchar_t*)data.data(), data.size() / sizeof(wchar_t));

		Asset::Types::WDocument json;
		json.Parse(sstream.str().c_str());
		if (json.HasParseError() || !json.IsArray())
		{
			return false;
		}

		auto arr = json.GetArray();

		for (auto iter = arr.Begin(); iter != arr.End(); iter++)
		{
			if (!iter->IsObject())
				return false;

			auto& entity_data = *iter;

			if (!entity_data.IsObject())
				return false;

			auto obj = entity_data.GetObject();
			if (!obj.HasMember(L"type") || !obj[L"type"].IsString())
				return false;

			auto create_function = GetAssetCreateFunction(obj[L"type"].GetString());
			if (!create_function)
				return false;

			auto filename = obj[L"filename"].GetString();

			auto entry = create_function();

			const auto relative_filename = out.GetAssetPath(filename);

			entry->Initialize(&out, relative_filename);
			entry->Deserialize(entity_data);
			if (out.entries.find(relative_filename) != out.entries.end())
				return false;

			out.entries.insert({ relative_filename, std::move(entry) });
		}

		return true;
	}

	bool Cache::WriteMetadata(const FolderMetadata& metadata, const std::filesystem::path& folder)
	{
		std::vector<Types::AssetEntry*> sorted_entries;
		for (auto& pair : metadata.entries)
			sorted_entries.push_back(pair.second.get());

		std::sort(sorted_entries.begin(), sorted_entries.end(), [](Types::AssetEntry* a, Types::AssetEntry* b) { return a->GetFilename() < b->GetFilename(); });

		Asset::Types::WDocument document;
		auto& allocator = document.GetAllocator();
		document.SetArray();
		auto& array = document.GetArray();

		for (auto* item : sorted_entries)
		{
			if (!item->ShouldSerialize())
				continue;

			auto val = item->Serialize(document);
			array.PushBack(val, allocator);
		}

		GenericStringBuffer<Asset::Types::WDocument::ValueType::EncodingType> buffer;
		PrettyWriter<decltype(buffer), Asset::Types::WDocument::ValueType::EncodingType, Asset::Types::WDocument::ValueType::EncodingType> writer(buffer);
		document.Accept(writer);

		std::wstring str(buffer.GetString());

		try
		{
			std::ofstream output_stream(GetMetadataPath(folder), std::ofstream::binary);
			output_stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);
			output_stream.write((char*)buffer.GetString(), buffer.GetSize());
		}
		catch (std::ofstream::failure e)
		{
			return false;
		}

		return true;
	}

	bool Cache::LoadFolderMetadata(std::wstring folder, FolderMetadata& out_result)
	{
		auto path = GetMetadataPath(folder);
		auto data = loader::LoadFile(path);

		out_result.path = FileSystem::FormatPath(folder);
		out_result.folder_cache = std::make_unique<FolderCache>(fs::path(cache_directory) / folder, &out_result, project_directory.c_str(), cache_directory.c_str());

		if (!data.size())
			return true;

		if (!ParseMetadata(out_result, data, folder))
		{
			out_result.entries.clear();
			out_result.hash = 0;
			return false;
		}

		for (auto& entry : out_result.entries)
		{
			auto* asset = entry.second.get();
			auto* cache_src_file = out_result.folder_cache->GetInputFile(out_result.GetAssetPath(asset->GetFilename()).c_str());
			asset->SetCacheEntry(cache_src_file);
			if (cache_src_file)
				asset->SetNeedsReexport(out_result.folder_cache->HasModifications(cache_src_file->filename.c_str()));
			else
				asset->SetNeedsReexport(true);
		}

		return true;
	}

	std::unique_ptr<Types::AssetEntry> Cache::CreateNewEntry(FolderMetadata* metadata, const std::wstring& path)
	{
		std::unique_ptr<Types::AssetEntry> result = nullptr;

		auto create_function = GetAssetCreateFunction(path);
		if (create_function)
		{
			result = create_function();
			result->Initialize(metadata, path);
			result->SetID(Asset::Types::GenerateAssetID(*result));
			if (utils::EndsWith(path, std::wstring(ASSET_METADATA_NAME)))
				result->SetHidden(true);
		}

		return result;
	}

	bool Cache::Reload()
	{
		task_manager->Wait();
		file_tree.UpdateTree();

		std::vector<FileSystem::FileTree::DirectoryNode*> all_dirs;
		file_tree.ForEachDirectory([&](FileSystem::FileTree::DirectoryNode* node) {
			all_dirs.push_back(node);
		});

		asset_folder_entries.clear();
		std::vector<Types::AssetEntry*> missing_assets;
		std::vector<Types::AssetEntry*> added_assets;
		std::vector<Types::AssetEntry*> reexport_assets;
		std::vector<FolderMetadata*> failed_metadata;

		for (auto* dir : all_dirs)
		{
			// Loading metadata
			const fs::path fs_folder = fs::path(dir->full_path).lexically_relative(project_directory);
			auto it = asset_folder_entries.insert({ FileSystem::FormatPath(fs_folder), FolderMetadata() });
			if (!LoadFolderMetadata(fs_folder, it.first->second))
				failed_metadata.push_back(&it.first->second);
			auto& metadata = it.first->second;
			dir->user_data = &metadata;
			assert(metadata.path == it.first->first);

			// Getting missing files that used to exist in the metadata
			for (auto& entry : metadata.entries)
			{
				const bool exists = dir->HasFileName(entry.second->GetFilename());
				entry.second->SetMissing(!exists);
				if (!exists)
					missing_assets.push_back(entry.second.get());
			}

			// Getting new files that don't exist in the metadata
			for (auto& file : dir->files)
			{
				const auto file_path = FileSystem::FormatPath(metadata.path + L"/" + file.path);
				if (metadata.entries.find(file_path) == metadata.entries.end())
				{
					auto entry = CreateNewEntry(&metadata, file_path);
					if (entry)
					{
						if (entry->ShouldSerialize())
							added_assets.push_back(entry.get());

						metadata.entries[file_path] = std::move(entry);
					}
				}
			}

		}

		if (failed_metadata.size())
		{
			std::stringstream stream;
			stream << "Failed reading metadata for folders:\n";
			for (auto metadata : failed_metadata)
				stream << "    " << utils::WStringToString(metadata->path) << "\n";
			System::ShowMessageBox("Metadata error", stream.str().c_str());
		}

		// Re-saving metadata with added assets
		std::unordered_set<FolderMetadata*> metadata_to_resave;
		for (auto* asset : added_assets)
			metadata_to_resave.insert(asset->GetFolderMetadata());

		for (auto* metadata : metadata_to_resave)
		{
			WriteMetadata(*metadata, metadata->path);
		}

		return true;
	}

	void Cache::ExportPending()
	{
		std::unordered_set<FolderMetadata*> folders_to_reexport;
		for (auto& entry : asset_folder_entries)
		{
			// Just all folders for now
			//if (entry.second.NeedsReexport())
			//{
				folders_to_reexport.insert(&entry.second);
				//continue;
			//}
		}

		std::vector<Types::AssetEntry*> failed_assets;

		for (auto* metadata : folders_to_reexport)
			ReexportFolder(*metadata, failed_assets);

		if (failed_assets.size())
		{
			std::stringstream stream;
			stream << "Failed to export the following assets:\n";
			for (auto* asset : failed_assets)
			{
				stream << "    " << utils::WStringToString(asset->GetFolderMetadata()->path) << "/" << asset->GetStrFilename() << "\n";
			}

			System::ShowMessageBox("Export error", stream.str().c_str());
		}
	}

	void Cache::ReexportFolder(FolderMetadata& metadata, std::vector<Types::AssetEntry*>& failed_assets)
	{
		auto* folder_cache = metadata.folder_cache.get();

		for (auto& entry : metadata.entries)
		{
			Types::AssetEntry* asset = entry.second.get();
			if (folder_cache->HasModifications(metadata.GetAssetPath(asset->GetFilename()).c_str()))
			{
				const bool success = ExportAsset(*asset);
				if (!success)
					failed_assets.push_back(asset);
			}
		}
	}

	bool Cache::ExportAsset(Types::AssetEntry& asset)
	{
		auto exporter = asset.GetExporter();
		if (!exporter)
			return true;

		if (asset.IsMissing())
			return true;

		auto* folder_metadata = asset.GetFolderMetadata();
		auto* folder_cache = folder_metadata->folder_cache.get();
		
		const auto asset_path = folder_metadata->GetAssetPath(asset.GetFilename());

		if (folder_cache->GetInputFile(asset_path.c_str()))
		{
			folder_cache->RemoveInputFile(asset_path.c_str());
			folder_cache->Save();
		}

		if (!FileSystem::CreateDirectories(cache_directory / fs::path(asset_path).parent_path()))
			return false;

		exporter->Initialize(asset_path, project_directory, cache_directory);
		auto result = exporter->Export();

		if (result.success)
		{
			const auto src_path = fs::path(project_directory) / asset_path;

			folder_cache->AddInputFile(asset_path.c_str(), FileSystem::GetFileHash(src_path), FileSystem::GetFileTimestamp(src_path));
			for (auto& exported_asset : result.exported_assets)
			{
				auto output_path = FileSystem::FormatPath(fs::path(cache_directory) / exported_asset.path);
				folder_cache->AddOutputFile(asset_path.c_str(), exported_asset.path, FileSystem::GetFileHash(output_path), FileSystem::GetFileTimestamp(output_path));
			}
			folder_cache->Save();
		}

		return result.success;
	}
}