#include "lib/catch/catch.hpp"
#include "render/material/Material.h"

using namespace render;

TEST_CASE("ConstantBindingStorage")
{
	{
		ConstantBindingStorage storage;
		storage.AddFloatConstant("test1", 1.0f);
		storage.AddFloatConstant("test2", 2.0f);
		storage.AddFloatConstant("test3", 3.0f);
		REQUIRE(storage.GetConstantCount() == 3);
		REQUIRE(storage.GetFloatConstant("test1") == 1.0f);
		REQUIRE(storage.GetFloatConstant("test2") == 2.0f);
		REQUIRE(storage.GetFloatConstant("test3") == 3.0f);
		REQUIRE(storage.GetConstantByIndex(0).size == sizeof(float));
		REQUIRE(storage.GetConstantByIndex(1).size == sizeof(float));
		REQUIRE(storage.GetConstantByIndex(2).size == sizeof(float));
		storage.RemoveConstant("test2");
		REQUIRE(storage.GetFloatConstant("test2") == std::nullopt);
		REQUIRE(storage.GetFloatConstant("test3") == 3.0f);
		REQUIRE(storage.GetConstantCount() == 2);
	}

	{
		ConstantBindingStorage storage;
		storage.AddFloat4Constant("stub0", vec4(0.0f));
		storage.AddFloat4Constant("stub1", vec4(0.0f));
		storage.AddFloat4Constant("stub2", vec4(0.0f));
		storage.AddFloat4Constant("stub3", vec4(0.0f));
		storage.AddFloat4Constant("stub4", vec4(0.0f));
		storage.AddFloat4Constant("stub5", vec4(0.0f));
		storage.AddFloat4Constant("stub6", vec4(0.0f));
		storage.AddFloat4Constant("stub7", vec4(0.0f));
		storage.AddFloat4Constant("stub8", vec4(0.0f));
		storage.AddFloat4Constant("test_float4_-1.0", vec4(-1.0f));
		storage.AddFloatConstant("test1", 1.0f);
		storage.AddFloat4Constant("test_float4_2", vec4(2.0f));
		storage.AddFloatConstant("test2", 2.0f);
		storage.AddFloatConstant("test3", 3.0f);
		storage.AddFloat4Constant("test_float4_3", vec4(3.0f));
		REQUIRE(storage.GetFloat4Constant("test_float4_3") == vec4(3.0f));
		REQUIRE(storage.GetFloat4Constant("test_float4_-1.0") == vec4(-1.0f));
		REQUIRE(storage.GetFloatConstant("test2") == 2.0f);
		REQUIRE(storage.GetFloatConstant("test3") == 3.0f);
		storage.RemoveConstant("test1");
		storage.RemoveConstant("test_float4_-1.0");
		storage.RemoveConstant("test_float4_3");
		storage.RemoveConstant("stub0");
		REQUIRE(storage.GetFloatConstant("test_float4_-1.0") == std::nullopt);
		REQUIRE(storage.GetFloatConstant("test3") == 3.0f);
		REQUIRE(storage.GetFloat4Constant("test_float4_2") == vec4(2.0f));
		REQUIRE(storage.GetFloat4Constant("test_float4_3") == std::nullopt);
		storage.AddFloat4Constant("test_float4_3", vec4(33.0f));
		storage.AddFloatConstant("stub9", 0);
		storage.AddFloat4Constant("stub1", vec4(1.0f));
		REQUIRE(storage.GetFloat4Constant("test_float4_3") == vec4(33.0f));
		REQUIRE(storage.GetConstantByIndex(0).size == sizeof(vec4));
		REQUIRE(storage.GetConstantByIndex(storage.GetConstantCount() - 1).size == sizeof(float));
		REQUIRE(storage.GetFloat4Constant("stub1") == vec4(1.0f));
	}
}