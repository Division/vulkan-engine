#include "StringUtils.h"

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

	std::wstring StringToWString(const std::string& str)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.from_bytes(str);
	}

	std::string WStringToString(const std::wstring& wstr)
	{
		using convert_typeX = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_typeX, wchar_t> converterX;

		return converterX.to_bytes(wstr);
	}
}