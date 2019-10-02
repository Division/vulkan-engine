#pragma once

#include "CommonIncludes.h"
#include "objects/Camera.h"

class PlayerController;

class FollowCamera : public Camera {
public:
	void start() override;
	void update(float dt) override;
	void setPlayer(std::shared_ptr<PlayerController> player);
	void setFreeCamera(bool isFree);
private:
	float _angleX = 0;//-(float)M_PI / 8.0f;
	float _angleY = 0;
	bool _isFreeCamera = false;
	std::weak_ptr<PlayerController> _player;
	std::weak_ptr<GameObject> _container;
};
