/*\

#include <ktx.h>
ktxTexture* texture;
KTX_error_code result;
ktx_size_t offset;
ktx_uint8_t* image;
ktx_uint32_t level, layer, faceSlice;
result = ktxTexture_CreateFromNamedFile("mytex3d.ktx",
KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
&texture);


*/

#include "lib/catch/catch.hpp"
#include <lib/ktx/ktx.h>
#include <vector>
#include <string>

TEST_CASE("KTX")
{
	ktxTexture* texture;
	KTX_error_code result;
	ktx_size_t offset;
	ktx_uint8_t* image;
	ktx_uint32_t level, layer, faceSlice;
	result = ktxTexture_CreateFromNamedFile("resources/lama.ktx",
	KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
	&texture);

	REQUIRE(result == KTX_error_code::KTX_SUCCESS);
	REQUIRE(texture->numLevels == 8);
}
