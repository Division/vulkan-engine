#include <vector>
#include "Allocator.h"

namespace core { namespace Memory {

	template <typename T, Tag tag>
	using Vector = std::vector<T, TaggedAllocator<T, tag>>;

}}