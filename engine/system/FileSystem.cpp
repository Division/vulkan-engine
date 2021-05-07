#include "FileSystem.h"
#include "utils/StringUtils.h"
#include "loader/FileLoader.h"
#include <chrono>
#include <optick/src/optick.h>

#if defined ( _WIN32 )
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace FileSystem
{

	std::wstring FormatPath(const fs::path& path)
	{
		std::wstring result = path.lexically_normal().wstring();
		utils::ReplaceAll(result, L"\\", L"/");
		return result;
	}

	std::wstring GetRelativePath(const std::filesystem::path& full_path, const std::filesystem::path& base_path)
	{
		return FormatPath(full_path.lexically_relative(base_path));
	}

	uint64_t GetFileHash(const std::filesystem::path& path)
	{
		OPTICK_EVENT();
		auto data = loader::LoadFile(path);
		if (!data.size())
			return 0;

		return FastHash64(data.data(), data.size());
	}

	uint64_t GetFileTimestamp(const std::filesystem::path& path)
	{
#if defined ( _WIN32 )
		{
			struct _stat64 fileInfo;
			if (_wstati64(path.wstring().c_str(), &fileInfo) != 0)
				return 0;
			return fileInfo.st_mtime;
		}
#else
		{
			std::error_code error;
			auto fsTime = std::filesystem::last_write_time(path, error);
			if (error)
				return 0;
			return decltype (fsTime)::clock::to_time_t(fsTime);
		}
#endif
	}

	bool CreateDirectories(const fs::path& path)
	{
		std::error_code error;
		std::filesystem::create_directories(path, error);
		return !error;
	}

	bool DeleteFile(const std::filesystem::path& path)
	{
		std::error_code error;
		if (fs::is_directory(path, error) || error)
			return false;

		fs::remove(path, error);
		return !error;
	}

	bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst)
	{
		std::error_code error;
		if (fs::is_directory(src, error) || error)
			return false;

		fs::copy(src, dst, error);
		return !error;
	}

	bool CreateDirectorySymlink(const std::filesystem::path& symlink_path, const std::filesystem::path& target_path)
	{
		std::error_code error;
		if (!fs::is_directory(target_path, error) || error)
			return false;

		if (fs::is_symlink(symlink_path, error) && !error)
			fs::remove(symlink_path);

		fs::create_directory_symlink(target_path, symlink_path, error);
		return !error;
	}

	bool IterateFilesInDirectory(const std::wstring& path, bool recursive, bool include_directories, DirectoryIteratorCallback callback)
	{
		try
		{
			fs::path root_path = fs::absolute(fs::path(path));

			for (auto it : fs::directory_iterator(root_path))
			{
				callback(it.path(), it.is_directory());
				if (it.is_directory() && recursive)
				{
					if (!IterateFilesInDirectory(it.path(), recursive, include_directories, callback))
						return false;
				}
			}

		}
		catch (fs::filesystem_error e)
		{
			return false;
		}

		return true;
	}

	FileTree::FileTree() = default;

	FileTree::FileTree(const std::wstring& root_path, const std::wstring& base_path)
	{
		SetPath(root_path, base_path);
	}

	void FileTree::SetPath(const std::wstring& path, const std::wstring& base_path)
	{
		this->base_path = base_path;
		this->root_path = path;
		UpdateTree();
	}

	void FileTree::UpdateTree()
	{
		root_node.Clear();

		bool success = true;

		root_node.path = root_path;
		root_node.full_path = root_path;

		std::deque<FileTree::DirectoryNode*> stack;
		auto asd = &root_node;
		stack.push_back(asd);
		
		const auto get_relative_path = [&](const fs::path& base, const fs::path path) -> fs::path
		{
			if (base.empty())
				return path;

			try {
				return fs::relative(path, base);
			}
			catch (fs::filesystem_error)
			{
				return path;
			}
		};

		while (stack.size())
		{
			DirectoryNode* node = stack.back();
			stack.pop_back();

			if (!IterateFilesInDirectory(node->full_path, false, true, [&](const std::wstring& path, bool is_directory) {

				if (is_directory)
				{
					auto& directory_node = node->directories.emplace_back();
					directory_node.full_path = path;
					directory_node.path = get_relative_path(fs::path(path).parent_path(), path);
					directory_node.path_char = utils::WStringToString(directory_node.path);

					stack.push_back(&directory_node);
				}
				else
				{
					auto& file_node = node->files.emplace_back();
					file_node.full_path = path;
					file_node.path = get_relative_path(fs::path(path).parent_path(), path);
					file_node.path_char = utils::WStringToString(file_node.path);
				}
			}))
			{
				success = false;
				break;
			}
		}
		
		if (!success)
			root_node.Clear();
	}

	void FileTree::ForEachDirectory(std::function<void(DirectoryNode* node)> callback, DirectoryNode* node)
	{
		if (!node) node = &root_node;

		callback(node);

		for (auto& sub_node : node->directories)
			ForEachDirectory(callback, &sub_node);
	}
}