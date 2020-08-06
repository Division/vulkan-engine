#include "lib/catch/catch.hpp"
#include "utils/DataStructures.h"
#include <iostream>

using namespace utils;

TEST_CASE("Small Vector")
{
	SmallVector<int, 3> small_vec;
	for (int i = 0; i < small_vec.capacity(); i++)
	{
		small_vec.push_back(i);
	}

	REQUIRE(small_vec.size() == small_vec.capacity());

	int index = 0;
	for (auto val : small_vec)
	{
		REQUIRE(val == index++);
	}
	REQUIRE(index == small_vec.size());

	struct Test1
	{
		int v1 = 1;
		int v2 = 2;
	};

	SmallVector<Test1, 1> test1;
	test1.resize(1);
	REQUIRE(((test1[0].v1 == 1) && (test1[0].v2 == 2)));
	test1.push_back({ 3, 4 });
	REQUIRE(((test1[0].v1 == 1) && (test1[0].v2 == 2)));
	REQUIRE(((test1[1].v1 == 3) && (test1[1].v2 == 4)));
	REQUIRE(test1.capacity() == 2);
	test1.push_back({5, 6});
	REQUIRE(((test1[1].v1 == 3) && (test1[1].v2 == 4)));
	REQUIRE(((test1[2].v1 == 5) && (test1[2].v2 == 6)));
	REQUIRE(test1.capacity() == 4);

	int counter = 0;
	
	struct TestInitDestroy
	{
		int* counter = 0;
		TestInitDestroy(int* counter)
			: counter(counter)
		{
			(*counter)++;
		}
		
		TestInitDestroy(const TestInitDestroy& other)
		{
			counter = other.counter;
			(*counter)++;
		}

		TestInitDestroy() = default;

		~TestInitDestroy()
		{
			if (counter)
				(*counter)--;
		}
	};

	{
		SmallVector<TestInitDestroy, 2> test2;
		test2.push_back(TestInitDestroy(&counter));
		test2.push_back(TestInitDestroy(&counter));
		REQUIRE(counter == 2);
	}
	REQUIRE(counter == 0);

	{
		SmallVector<TestInitDestroy, 2> test2;
		test2.push_back(TestInitDestroy(&counter));
		test2.push_back(TestInitDestroy(&counter));
		test2.push_back(TestInitDestroy(&counter));
		REQUIRE(counter == 3);
	}
	REQUIRE(counter == 0);

	{
		SmallVector<TestInitDestroy, 2> test2;
		for (int i = 0; i < 10; i++)
			test2.push_back(TestInitDestroy(&counter));
		REQUIRE(counter == 10);

		while (test2.size())
			test2.pop_back();
		REQUIRE(counter == 0);
		REQUIRE(test2.capacity() == 16);
	}

	REQUIRE(counter == 0);

	{
		SmallVector<TestInitDestroy, 2> test2;
		test2.push_back(TestInitDestroy(&counter));
		test2.push_back(TestInitDestroy(&counter));
		test2.push_back(TestInitDestroy(&counter));
		test2.clear();
		REQUIRE(counter == 0);
	}
	REQUIRE(counter == 0);

	struct TestStruct
	{
		std::wstring data1;
		std::string data2;
		int num;
	};

	{
		SmallVector<TestStruct, 2> test_vec;
		//std::vector<TestStruct> test_vec;

		auto print_vec = [&test_vec]() {
			for (auto& v : test_vec)
				std::wcout << v.data1 << ", ";
			std::cout << std::endl;
		};

		test_vec.push_back({ std::wstring{L"data1_3"}, std::string{"data2_3"}, 123 });
		test_vec.push_back({ std::wstring{L"data1"}, std::string{"data2"}, 123 });
		test_vec.push_back({ std::wstring{L"data1_2"}, std::string{"data2_2"}, 123 });
		REQUIRE(test_vec[0].data1 == L"data1_3");
		REQUIRE(test_vec[1].data1 == L"data1");
		REQUIRE(test_vec[0].data2 == "data2_3");
		REQUIRE(test_vec[1].data2 == "data2");

		print_vec();
		std::sort(test_vec.begin(), test_vec.end(), [](TestStruct& a, TestStruct& b) { return a.data1 < b.data1; });
		print_vec();

		REQUIRE(test_vec[0].data1 == L"data1");
		REQUIRE(test_vec[1].data1 == L"data1_2");
		REQUIRE(test_vec[2].data1 == L"data1_3");

	}

}
