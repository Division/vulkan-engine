#pragma once

#include "ExportBase.h"

namespace Asset::Export::FBX
{
	enum class ExportType
	{
		Mesh,
		Animation
	};

	class FBXExport : public Base
	{
	public:
		FBXExport(ExportType export_type);

		Result Export() override;
	private:
		ExportType export_type;
	};
}