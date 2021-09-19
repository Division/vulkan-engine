#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "AssetExport/TextureExport.h"
#include "AssetExport/FBXExport.h"
#include "AssetExport/CopyExport.h"

namespace Asset
{
	class FolderMetadata;
	struct SrcEntry;

	namespace Export
	{
		class Base;
	}
}

namespace Asset::Types
{
	typedef rapidjson::GenericDocument<rapidjson::UTF16LE<>> WDocument;
	typedef rapidjson::GenericValue<rapidjson::UTF16LE<>> WValue;

	class AssetEntry;

	uint64_t GenerateAssetID(const AssetEntry& entry);

	class SerializableObject
	{
	public:
		SerializableObject() = default;
		virtual ~SerializableObject() = default;

		virtual void Deserialize(WValue& json) = 0;
		virtual WValue Serialize(WDocument& document) = 0;
	};

	class AssetEntry: public SerializableObject
	{
	public:
		WValue Serialize(WDocument& document) override;
		void Deserialize(WValue& json) override;

		size_t GetTypeID() const { assert(type_id != 0); return type_id; }
		uint64_t GetID() const { return id; }
		virtual bool ShouldSerialize() const { return true; }
		void SetID(uint64_t value) { id = value; }
		virtual void Initialize(FolderMetadata* metadata, const std::wstring& relative_path);
		bool IsMissing() const { return is_missing; }
		void SetMissing(bool value) { is_missing = value; }
		void SetHidden(bool hidden) { is_hidden = hidden; }
		bool GetHidden() const { return is_hidden; }
		void SetNeedsReexport(bool value) { needs_reexport = value; }
		bool GetNeedsReexport() const { return needs_reexport; }
		void SetCacheEntry(SrcEntry* entry) { cache_entry = entry; }
		SrcEntry* GetCacheEntry() const { return cache_entry; }

		virtual const wchar_t* GetType() const = 0;
		virtual std::unique_ptr<Export::Base> GetExporter() = 0;

		const std::wstring& GetFilename() const { return filename; }
		const std::string& GetStrFilename() const { return str_filename; }
		FolderMetadata* GetFolderMetadata() { return metadata; }
		const FolderMetadata* GetFolderMetadata() const { return metadata; }

	protected:
		FolderMetadata* metadata = nullptr;
		SrcEntry* cache_entry = nullptr;
		size_t type_id = 0;
		uint64_t id = 0;
		std::wstring relative_path;
		std::wstring filename;
		std::string str_filename;
		bool is_missing = false;
		bool is_hidden = false;
		bool needs_reexport = true;
	};

	class Texture : public AssetEntry
	{
	public:
		void Deserialize(WValue& json) override;
		WValue Serialize(WDocument& document) override;
		const wchar_t* GetType() const override { return L"texture"; };
		std::unique_ptr<Export::Base> GetExporter() override;

	private:
		Asset::Export::Texture::CompressionType compression_type = Asset::Export::Texture::CompressionType::BC7;
	};

	class FBX : public AssetEntry
	{
	public:
		void Initialize(FolderMetadata* metadata, const std::wstring& relative_path) override;
		void Deserialize(WValue& json) override;
		WValue Serialize(WDocument& document) override;
		const wchar_t* GetType() const override { return L"fbx"; };
		std::unique_ptr<Export::Base> GetExporter() override;

	private:
		Asset::Export::FBX::ExportType export_type = Asset::Export::FBX::ExportType::Mesh;
		Asset::Export::FBX::AnimationType animation_type = Asset::Export::FBX::AnimationType::Normal;
	};

	class PlainCopy : public AssetEntry
	{
	public:
		const wchar_t* GetType() const override { return L"copy"; };
		std::unique_ptr<Export::Base> GetExporter() override;
	};

	class IgnoredEntry : public AssetEntry
	{
	public:
		bool ShouldSerialize() const override { return false; }
		void Deserialize(WValue& json) override {};
		WValue Serialize(WDocument& document) override { return {}; };
		const wchar_t* GetType() const override { return nullptr; };
		std::unique_ptr<Export::Base> GetExporter() override { return nullptr; };
	};

	template<typename T> 
	constexpr size_t GetTypeID() { return typeof(T).hash_code(); }
}