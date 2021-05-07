#pragma once

#include <string>
#include "system/FileSystem.h"
#include <unordered_set>
#include <filesystem>
#include "utils/StringUtils.h"

namespace Asset
{

	namespace Types
	{
		class AssetEntry;
	}

	template <typename T>
	using UnorderedMapWChar = std::unordered_map<const wchar_t*, T, utils::HasherWChar, utils::HasherWChar>;

	typedef uint64_t ID;

	struct OutputEntry
	{
		OutputEntry(uint64_t hash, uint64_t timestamp, std::wstring filename)
			: hash(hash)
			, timestamp(timestamp)
			, filename(std::move(filename))
			, filename_str(utils::WStringToString(this->filename))
		{}

		uint64_t hash = 0;
		uint64_t timestamp = 0;
		std::wstring filename;
		std::string filename_str;
	};

	struct SrcEntry
	{
		uint64_t hash = 0;
		uint64_t timestamp = 0;
		std::wstring filename;
		UnorderedMapWChar<std::unique_ptr<OutputEntry>> outputs;
	};

	class FolderMetadata;

	class FolderCache
	{
	public:

		FolderCache(std::filesystem::path path, FolderMetadata* folder_metadata, const wchar_t* asset_root, const wchar_t* cache_root);
		bool Save();

		SrcEntry* GetInputFile(const wchar_t* input_filename);
		void RemoveInputFile(const wchar_t* input_filename);
		SrcEntry* AddInputFile(const wchar_t* input_filename, uint64_t input_hash, uint64_t timestamp);
		void AddOutputFile(const wchar_t* input_filename, std::wstring output_filename, uint64_t output_hash, uint64_t timestamp);
		bool HasModifications(const wchar_t* input_filename);

	private:
		UnorderedMapWChar<std::unique_ptr<SrcEntry>> input_entries;
		std::wstring path;
		const wchar_t* asset_root;
		const wchar_t* cache_root;
		const FolderMetadata* folder_metadata;
	};

	struct FolderMetadata
	{
		std::wstring GetAssetPath(const std::wstring& filename) const;
		bool NeedsReexport() const;

		std::unordered_map<std::wstring, std::unique_ptr<Types::AssetEntry>> entries;
		std::unique_ptr<FolderCache> folder_cache;
		std::wstring path; // relative to project
		uint64_t hash;
	};

	class Cache
	{
	public:
		typedef std::function<std::unique_ptr<Types::AssetEntry>()> AssetEntryCreateFunction;
		typedef std::function<bool(const std::wstring&)> AssetEntryNameTestFunction;

		enum class ChangeType
		{
			None,
			Delete,
			Modify
		};

		struct OutputModification
		{
			ID src_asset;
			std::vector<std::pair<std::wstring, ChangeType>> changes;
		};

		struct AssetModifications
		{
			std::vector<std::wstring> added_inputs;
			std::vector<ID> deleted_inputs;
			std::vector<ID> modified_inputs;
			std::vector<OutputModification> modified_outputs;
			std::wstring folder;
		};

		Cache();
		~Cache();

		void SetProjectDirectory(const std::wstring& project_dir);
		const std::wstring& GetProjectDirectory() { return project_directory; }
		std::wstring GetMetadataPath(const std::filesystem::path& folder);
		std::wstring GetFolderCachePath(const std::filesystem::path& folder);

		const FileSystem::FileTree* GetFileTree() const { return &file_tree; }

		void ExportPending();

		bool Reload();

		class TaskManager;

	private:
		std::unique_ptr<Types::AssetEntry> CreateNewEntry(FolderMetadata* metadata, const std::wstring& path);
		bool ParseMetadata(FolderMetadata& out, std::vector<uint8_t>& data, const std::filesystem::path& folder);
		bool WriteMetadata(const FolderMetadata& metadata, const std::filesystem::path& folder);

		bool LoadFolderMetadata(std::wstring folder, FolderMetadata& out_result);
		AssetEntryCreateFunction GetAssetCreateFunction(const std::wstring filename);
		void ReexportFolder(FolderMetadata& metadata, std::vector<Types::AssetEntry*>& failed_assets);
		bool ExportAsset(Types::AssetEntry& asset);

	private:
		std::unordered_map<ID, Types::AssetEntry*> entries; // complete map of assets
		std::unordered_map<std::wstring, FolderMetadata> asset_folder_entries; // list of assets for every tracked folder

		std::vector<std::pair<AssetEntryNameTestFunction, AssetEntryCreateFunction>> create_functions;

		FileSystem::FileTree file_tree;
		std::wstring project_directory;
		std::wstring cache_directory;

		std::unique_ptr<TaskManager> task_manager;
	};

}