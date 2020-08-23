#pragma once

#include <fbxsdk.h>
#include <functional>

namespace Exporter
{
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);
	void PrintAttribute(FbxNodeAttribute* pAttribute);
	void PrintNode(FbxNode* pNode);
	void PrintAttribute(FbxNodeAttribute* pAttribute);

	void IterateNodesDepthFirst(FbxNode* node, std::function<bool(FbxNode*, uint32_t depth)> callback, uint32_t depth = 0);
}