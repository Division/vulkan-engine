#pragma once

#include "CommonIncludes.h"


class ViewerCamera {
public:
	ViewerCamera();
	void Update(float dt);
private:
	float _angleX = 0;//-(float)M_PI / 8.0f;
	float _angleY = 0;
};
