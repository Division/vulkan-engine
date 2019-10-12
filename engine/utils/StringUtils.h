#pragma once
#include <string>

namespace utils
{
	
	void ReplaceAll(std::wstring& src, const std::wstring& search, const std::wstring& replacement)
	{
		std::wstring::size_type position = 0;
		
		while ((position = src.find(search, position)) != std::wstring::npos)
		{
			src.replace(position, search.length(), replacement);
			position += replacement.length();
		}
	}

}