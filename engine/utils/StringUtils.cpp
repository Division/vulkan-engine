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

	std::wstring GetStringBeforeLast(const std::wstring& str, const wchar_t character)
	{
		const auto pos = str.find_last_of(character);
		if (pos == std::wstring::npos)
			return str;
		else
			return str.substr(0, pos);
	}

	std::wstring GetStringAfterLast(const std::wstring& str, const wchar_t character)
	{
		const auto pos = str.find_last_of(character);
		if (pos == std::wstring::npos)
			return str;
		else
			return str.substr(pos + 1, -1);
	}
}