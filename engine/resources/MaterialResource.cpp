#pragma once

#include "MaterialResource.h"
#include "loader/FileLoader.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "utils/StringUtils.h"
#include "utils/JsonUtils.h"
#include "system/Logging.h"
#include "render/renderer/RenderModeUtils.h"

namespace Resources
{

	MaterialResource::MaterialResource(const std::wstring& filename)
	{
		auto data = loader::LoadFile(filename);

		if (!data.size())
		{
			throw Exception(filename) << "Error reading material data";
		}

		rapidjson::Document json;
		json.Parse((char*)data.data(), data.size());
		if (json.HasParseError() || !json.IsObject())
		{
			throw Exception(filename) << "Error parsing material";
		}

		auto new_material = Material::Create();

		for (auto iter = json.MemberBegin(); iter != json.MemberEnd(); iter++)
		{
			if (iter->value.IsNull())
				continue;

			std::string name = iter->name.GetString();

			if (name == "shader")
			{
				new_material->SetShaderPath(utils::StringToWString(iter->value.GetString()));
			}
			else if (name == "texture0")
			{
				new_material->SetTexture0Resource(TextureResource::Handle(utils::StringToWString(iter->value.GetString())));
			}
			else if (name == "normal_map")
			{
				new_material->SetNormalMapResource(TextureResource::Linear(utils::StringToWString(iter->value.GetString())));
			}
			else if (name == "lighting_enabled")
			{
				new_material->LightingEnabled(iter->value.GetBool());
			}
			else if (name == "vertex_color_enabled")
			{
				new_material->VertexColorEnabled(iter->value.GetBool());
			}
			else if (name == "roughness")
			{
				new_material->SetRoughness(iter->value.GetFloat());
			}
			else if (name == "metallness")
			{
				new_material->SetMetalness(iter->value.GetFloat());
			}
			else if (name == "color")
			{
				new_material->SetColor(utils::JSON::ReadVector4(iter->value));
			}
			else if (name == "queue")
			{
				std::string queue = iter->value.GetString();
				utils::Lowercase(queue);
				new_material->SetRenderQueue(render::RENDER_QUEUE_NAME_MAP.at(queue));
			}
			else
			{
				ENGLog("Invalid json key: %s in file %s", name.c_str(), utils::WStringToString(filename).c_str());
				throw Exception(filename) << "Error parsing material";
			}

		}

		material = new_material;
	}

	MaterialResource::~MaterialResource()
	{

	}

}