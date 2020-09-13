#pragma once

#include <fbxsdk.h>
#include <functional>
#include <filesystem>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

namespace Exporter
{
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);
	void PrintAttribute(FbxNodeAttribute* pAttribute);
	void PrintNode(FbxNode* pNode);
	void PrintAttribute(FbxNodeAttribute* pAttribute);

	void IterateNodesDepthFirst(FbxNode* node, std::function<bool(FbxNode*, uint32_t depth)> callback, uint32_t depth = 0);
	bool LoadJSONFile(const std::filesystem::path& json_path, rapidjson::Document& out_json);
}