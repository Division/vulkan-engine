#pragma once

#include "CommonIncludes.h"

namespace render
{
	struct DebugSettings
	{
		bool draw_clusters = false;
		mat4 cluster_matrix;
		bool draw_bounding_boxes = false;
		bool draw_skeletons = false;
		bool draw_lights = false;
	};
}