#pragma once

#include "rapidjson/document.h"
#include "Math.h"

namespace utils::JSON
{

	inline vec3 ReadVector3(const rapidjson::Value& data)
	{
		if (!data.IsArray())
			throw std::runtime_error("vector must be an array");

		vec3 result;
		int i = 0;
		for (auto iter = data.Begin(); iter != data.End(); iter++)
		{
			if (i == 3)
				throw std::runtime_error("vector array must be of length 3");

			result[i++] = iter->GetFloat();
		}

		if (i != 3)
			throw std::runtime_error("vector array must be of length 3");

		return result;
	}

	inline vec4 ReadVector4(const rapidjson::Value& data)
	{
		if (!data.IsArray())
			throw std::runtime_error("vector must be an array");

		vec4 result;
		int i = 0;
		for (auto iter = data.Begin(); iter != data.End(); iter++)
		{
			if (i == 4)
				throw std::runtime_error("vector array must be of length 4");

			result[i++] = iter->GetFloat();
		}

		if (i != 4)
			throw std::runtime_error("vector array must be of length 4");

		return result;
	}

}