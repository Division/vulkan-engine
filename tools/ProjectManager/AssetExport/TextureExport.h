#pragma once

#include "ExportBase.h"

namespace Asset::Export::Texture
{
	enum class CompressionType
	{
		Copy,
		Uncompressed,
		BC1,
		BC3,
		BC7
	};

	class TextureExport : public Base
	{
	public:
		Result Export() override;
	};
}