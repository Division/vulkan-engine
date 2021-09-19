#include "AssetTypes.h"
#include "utils/StringUtils.h"
#include "system/FileSystem.h"
#include <filesystem>
#include "magic_enum/magic_enum.hpp"
#include "utils/Math.h"
#include <chrono>
#include "AssetCache/AssetCache.h"

using namespace rapidjson;
namespace fs = std::filesystem;

namespace Asset
{
	typedef uint64_t ID;
}

namespace Asset::Types
{
	uint64_t GenerateAssetID(const AssetEntry& entry)
	{
		// TODO: make sure id doesn't exist in the cache
		const auto name_hash = FastHash64(entry.GetFilename().data(), utils::GetStringByteSize(entry.GetFilename()));
		const auto folder_hash = FastHash64(entry.GetFolderMetadata()->path.data(), utils::GetStringByteSize(entry.GetFolderMetadata()->path));
		const uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		const uint64_t data[] = { name_hash, folder_hash, ms };
		return FastHash64(data, sizeof(data));
	}

	template<typename T, typename Allocator>
	void WriteEnum(const wchar_t* name, WValue& obj, T value, Allocator& allocator)
	{
		obj.AddMember(StringRef(name), WValue().SetString(utils::StringToWString(std::string(magic_enum::enum_name(value))).c_str(), allocator), allocator);
	}

	template<typename T>
	bool ReadEnum(const wchar_t* name, WValue& obj, T default_value, T& out_result)
	{
		if (!obj.HasMember(name))
		{
			out_result = default_value;
			return false;
		}

		const std::string str_value = utils::WStringToString(obj[name].GetString());
		const auto parsed_value = magic_enum::enum_cast<T>(str_value);
		if (!parsed_value.has_value())
		{
			out_result = default_value;
			return false;
		}

		out_result = parsed_value.value();
		return true;
	}

	void AssetEntry::Initialize(FolderMetadata* metadata, const std::wstring& in_relative_path)
	{
		this->metadata = metadata;
		type_id = typeid(*this).hash_code();
		const fs::path path = fs::path(in_relative_path).lexically_normal();
		relative_path = FileSystem::FormatPath(path.parent_path());
		filename = path.filename();
		str_filename = utils::WStringToString(filename);
		cache_entry = metadata->folder_cache->GetInputFile(filename.c_str());
	}

	WValue AssetEntry::Serialize(WDocument& document)
	{
		auto result = WValue(kObjectType);

		auto& allocator = document.GetAllocator();
		result.AddMember(L"type", WValue().SetString(StringRef(GetType())), allocator);
		result.AddMember(L"filename", WValue().SetString(StringRef(filename.data())), allocator);
		assert(id != 0);
		result.AddMember(L"uid", WValue().SetString(utils::StringToWString(utils::WriteHexString(id)).c_str(), allocator), allocator);

		return result;
	}

	void AssetEntry::Deserialize(WValue& json)
	{
		if (!json.HasMember(L"type") || !json.HasMember(L"filename") || !json.HasMember(L"uid"))
			throw std::runtime_error("invalid asset entry json");

		filename = json[L"filename"].GetString();
		id = utils::ReadHexString(utils::WStringToString(json[L"uid"].GetString()));
	}

	/////////////////////////////////////////////////////////////

	void Texture::Deserialize(WValue& json)
	{
		AssetEntry::Deserialize(json);

		if (!json.HasMember(L"compression_type"))
			throw std::runtime_error("compression_type missing in json");

		const bool compression_type_success = ReadEnum(L"compression_type", json, Asset::Export::Texture::CompressionType::BC7, compression_type);
		if (!compression_type_success)
			throw std::runtime_error("Failed reading compression_type in json");
	}

	WValue Texture::Serialize(WDocument& document)
	{
		auto result = AssetEntry::Serialize(document);

		WriteEnum(L"compression_type", result, compression_type, document.GetAllocator());

		return result;
	}

	std::unique_ptr<Export::Base> Texture::GetExporter()
	{
		return std::make_unique<Export::Texture::TextureExport>();
	}

	/////////////////////////////////////////////////////////////

	void FBX::Initialize(FolderMetadata* metadata, const std::wstring& relative_path)
	{
		AssetEntry::Initialize(metadata, relative_path);
		if (relative_path.find(L"@") != std::wstring::npos)
			export_type = Export::FBX::ExportType::Animation;
		
		auto lowercase_path = relative_path;
		utils::Lowercase(lowercase_path);
		if (lowercase_path.find(L"additive") != std::wstring::npos)
			animation_type = Export::FBX::AnimationType::Additive;
	}

	void FBX::Deserialize(WValue& json)
	{
		AssetEntry::Deserialize(json);

		ReadEnum(L"export_type", json, Asset::Export::FBX::ExportType::Mesh, export_type);
		ReadEnum(L"animation_type", json, Asset::Export::FBX::AnimationType::Normal, animation_type);
	}

	WValue FBX::Serialize(WDocument& document)
	{
		auto result = AssetEntry::Serialize(document);

		WriteEnum(L"export_type", result, export_type, document.GetAllocator());
		WriteEnum(L"animation_type", result, animation_type, document.GetAllocator());

		return result;
	}

	std::unique_ptr<Export::Base> FBX::GetExporter()
	{
		return std::make_unique<Export::FBX::FBXExport>(export_type, animation_type);
	}

	/////////////////////////////////////////////////////////////

	std::unique_ptr<Export::Base> PlainCopy::GetExporter()
	{
		return std::make_unique<Export::Copy::CopyExport>();
	}
}