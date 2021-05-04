#pragma once

#include <string>
#include <deque>
#include <functional>
#include <filesystem>

namespace FileSystem
{
	typedef std::function<void(const std::wstring& path, bool is_directory)> DirectoryIteratorCallback;

	bool IterateFilesInDirectory(const std::wstring& path, bool recursive, bool include_directories, DirectoryIteratorCallback callback);

	std::wstring FormatPath(const std::filesystem::path& path);

	uint64_t GetFileHash(const std::filesystem::path& path);

	uint64_t GetFileTimestamp(const std::filesystem::path& path);

	bool CreateDirectories(const std::filesystem::path& path);
	
	bool DeleteFile(const std::filesystem::path& path);

	class FileTree
	{
	public:
		struct Node
		{
			std::wstring path;
			std::string path_char;
			std::wstring full_path;
		};

		struct DirectoryNode : public Node
		{
			std::list<DirectoryNode> directories;
			std::list<Node> files;
			void* user_data = nullptr;

			void Clear()
			{
				directories.clear();
				files.clear();
			}

			// No path, just filename
			bool HasFileName(const std::wstring& filename)
			{
				const std::wstring fs_filename = std::filesystem::path(filename).filename();

				for (auto& node : files)
					if (node.path == fs_filename)
						return true;

				return false;
			}
		};

		FileTree();
		FileTree(const std::wstring& root_path, const std::wstring& base_path);
		void SetPath(const std::wstring& path, const std::wstring& base_path = L"");
		const DirectoryNode* GetRootNode() const { return &root_node; }
		void UpdateTree();
		void ForEachDirectory(std::function<void(DirectoryNode* node)>, DirectoryNode* node = nullptr);

	private:
		std::wstring base_path;
		std::wstring root_path;
		DirectoryNode root_node;
	};
}