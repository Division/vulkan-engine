#pragma once
#include <string>

namespace utils
{
	
	void ReplaceAll(std::wstring& src, const std::wstring& search, const std::wstring& replacement);

	template <typename T>
	void Lowercase(std::basic_string<T>& src)
	{
		std::transform(src.begin(), src.end(), src.begin(),
			[](auto c){ return std::tolower(c); });
	}

	template <typename T>
	void Uppercase(std::basic_string<T>& src)
	{
		std::transform(src.begin(), src.end(), src.begin(),
			[](auto c){ return std::toupper(c); });
	}

}