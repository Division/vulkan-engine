#pragma once

#include "ExportBase.h"

namespace Asset::Export::Copy
{
	class CopyExport : public Base
	{
	public:
		Result Export() override;
	};
}