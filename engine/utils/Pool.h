#pragma once

#include "CommonIncludes.h"

namespace core { namespace utils {

	template<typename T>
	class Pool
	{
	public:
		std::unique_ptr<T> Obtain()
		{
			if (available_items.size())
			{
				auto result = std::move(available_items.back());
				available_items.pop_back();
				return result;
			}
			else
				return std::make_unique<T>();
		}

		void Release(std::unique_ptr<T> item)
		{
			available_items.push_back(std::move(item));
		}

	private:
		std::vector<std::unique_ptr<T>> available_items;
	};

} }