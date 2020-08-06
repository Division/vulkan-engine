#include "lib/catch/catch.hpp"
#include "memory/PoolAllocator.h"
#include <vector>
#include <string>

using namespace Memory;

struct TestStruct
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
};

TEST_CASE("Pool allocator")
{
	PoolAllocator<Tag::Render> pool_allocator(sizeof(TestStruct), 10);
	std::vector<TestStruct*> tests;
	for (uint32_t i = 0; i < 10; i++)
	{
		TestStruct* test = (TestStruct*)pool_allocator.Allocate();
		new (test) TestStruct{ i, i * 2, 0, 1 };
		tests.push_back(test);
	}
	REQUIRE(pool_allocator.GetFreeListStart() == nullptr);

	REQUIRE(pool_allocator.GetAllocatedCount() == 10);
	REQUIRE(pool_allocator.CanAllocate() == false);
	REQUIRE(pool_allocator.Allocate() == nullptr);

	for (int i = 0; i < tests.size(); i++)
	{
		if (i % 2 == 0)
			pool_allocator.Deallocate(tests[i]);
	}

	REQUIRE(pool_allocator.GetFreeListStart() != nullptr);

	REQUIRE(pool_allocator.CanAllocate());
	auto* item = pool_allocator.Allocate();
	
	for (int i = 0; i < tests.size(); i++)
	{
		if (i % 2 != 0)
			pool_allocator.Deallocate(tests[i]);
	}

	REQUIRE(pool_allocator.GetAllocatedCount() == 1);
	pool_allocator.Deallocate(item);
	REQUIRE(pool_allocator.GetAllocatedCount() == 0);

	tests.clear();
	for (uint32_t i = 0; i < 5; i++)
	{
		TestStruct* test = (TestStruct*)pool_allocator.Allocate();
		new (test) TestStruct{ i, i * 2, 0, 1 };
		tests.push_back(test);
	}

	for (uint32_t i = 0; i < tests.size(); i++)
		REQUIRE((tests[i]->a == i && tests[i]->b == i * 2));
}
