#pragma once

namespace core
{

	class NonCopyable
	{
	public:
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	
	protected:
		NonCopyable() = default;
		virtual ~NonCopyable() = default;
	};

}