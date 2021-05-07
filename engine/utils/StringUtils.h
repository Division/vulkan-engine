#pragma once
#include <string>
#include <codecvt>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "Math.h"

namespace utils
{
	struct HasherWChar
	{
		size_t operator()(const wchar_t* key) const { return (size_t)FastHash64((void*)key, wcslen(key) * sizeof(wchar_t)); }
		bool operator()(const wchar_t* a, const wchar_t* b) const { return wcscmp(a, b) == 0; }
	};

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

	template<typename T>
	constexpr size_t GetStringCharSize(const std::basic_string<T>& str) { return sizeof(std::basic_string<T>::traits_type::char_type); };

	template<typename T>
	size_t GetStringByteSize(const std::basic_string<T>& str) { return str.length() * GetStringCharSize(str); };

	std::wstring StringToWString(const std::string& str);

	std::string WStringToString(const std::wstring& wstr);

	// returns if string starts with value substring
	template <typename T>
	bool BeginsWith(const T& string, const T& value)
	{
		return string.find(value) == 0;
	}


	// returns if string ends with value substring
	template <typename T>
	bool EndsWith(const T& string, const T& value)
	{
		if (string.length() < value.length()) return false;

		const size_t offset = string.length() - value.length();

		for (int i = 0; i < value.length(); i++)
			if (string[offset + i] != value[i])
				return false;

		return true;
	}

	inline uint64_t ReadHexString(const std::string& str)
	{
		char* end;
		unsigned long long result;
		errno = 0;
		result = strtoull(str.c_str(), &end, 16);
		if (result == 0 && end == str) {
			/* str was not a number */
			return -1;
		}
		else if (result == ULLONG_MAX && errno) {
			/* the value of str does not fit in unsigned long long */
			return -1;
		}
		else if (*end) {
			/* str began with a number but has junk left over at the end */
			return -1;
		}

		return result;
	}

	inline std::string WriteHexString(uint64_t value)
	{
		std::stringstream str;
		str << std::setw(16) << std::setfill('0') << std::hex << value;
		return str.str();
	}
}