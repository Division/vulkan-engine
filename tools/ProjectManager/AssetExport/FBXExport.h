#pragma once

#include "ExportBase.h"

namespace Asset::Export::FBX
{
	enum class ExportType
	{
		Mesh,
		Animation
	};

	enum class AnimationType
	{
		Normal,
		Additive
	};

	class FBXExport : public Base
	{
	public:
		FBXExport(ExportType export_type, AnimationType animation_type);

		Result Export() override;
	private:
		ExportType export_type;
		AnimationType animation_type;
	};
}