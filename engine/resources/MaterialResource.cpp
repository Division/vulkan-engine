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
			else if (name == "alpha_cutoff")
			{
				new_material->SetAlphaCutoff(iter->value.GetBool());
			}
			else if (name == "extra_textures")
			{
				if (!iter->value.IsArray())
				{
					ENGLog("extra_textures must be an array. File: %s", utils::WStringToString(filename).c_str());
					throw Exception(filename) << "extra_textures must be an array";
				}

				auto arr = iter->value.GetArray();
 				for (auto tex_iter = arr.Begin(); tex_iter != arr.End(); tex_iter++)
				{
					if (!tex_iter->IsObject())
					{
						ENGLog("extra_textures elements must be objects"); 
						throw Exception(filename) << "extra_textures elements must be objects";
					}

					auto tex_data = tex_iter->GetObjectA();
					if (!tex_data.HasMember("path") || !tex_data["path"].IsString())
						throw Exception(filename) << "path must be specified as string";
					if (!tex_data.HasMember("name") || !tex_data["name"].IsString())
						throw Exception(filename) << "name must be specified as string";
					if (tex_data.HasMember("srgb") && !tex_data["srgb"].IsBool())
						throw Exception(filename) << "srgb must be bool";

					const bool is_srgb = tex_data.HasMember("srgb") ? tex_data["srgb"].GetBool() : true;
					const auto path = utils::StringToWString(tex_data["path"].GetString());
					TextureResource::Handle handle = is_srgb ? TextureResource::SRGB(path) : TextureResource::Linear(path);
					new_material->AddExtraTexture(handle, tex_data["name"].GetString());
				}
			}
			else if (name == "constants")
			{
				if (!iter->value.IsArray())
				{
					ENGLog("constants must be an array. File: %s", utils::WStringToString(filename).c_str());
					throw Exception(filename) << "constants must be an array";
				}

				auto arr = iter->value.GetArray();
				for (auto tex_iter = arr.Begin(); tex_iter != arr.End(); tex_iter++)
				{
					if (!tex_iter->IsObject())
					{
						ENGLog("constants elements must be objects");
						throw Exception(filename) << "constants elements must be objects";
					}

					auto constant_data = tex_iter->GetObjectA();

					if (!constant_data.HasMember("name") || !constant_data["name"].IsString())
						throw Exception(filename) << "name must be specified as string";
					if (!constant_data.HasMember("value") || (!(constant_data["value"].IsArray() || constant_data["value"].IsNumber())))
						throw Exception(filename) << "value must be specified as array or number";
					
					if (constant_data["value"].IsNumber())
					{
						auto value = constant_data["value"].GetFloat();
						new_material->AddFloatConstant(constant_data["name"].GetString(), value);
					}
					else
					{
						auto array_value = constant_data["value"].GetArray();

						if (array_value.Size() == 2)
						{
							auto value = utils::JSON::ReadVector2(array_value);
							new_material->AddFloat2Constant(constant_data["name"].GetString(), value);
						}
						else if (array_value.Size() == 3)
						{
							auto value = utils::JSON::ReadVector3(array_value);
							new_material->AddFloat3Constant(constant_data["name"].GetString(), value);
						}
						else if (array_value.Size() == 4)
						{
							auto value = utils::JSON::ReadVector4(array_value);
							new_material->AddFloat4Constant(constant_data["name"].GetString(), value);
						}
						else
						{
							throw Exception(filename) << "Invalid value array size";
						}
					}
				}
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