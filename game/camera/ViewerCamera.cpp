#include "ViewerCamera.h"
#include "Engine.h"
#include "system/Input.h"
#include "scene/Scene.h"
#include "objects/Camera.h"

using namespace System;

ViewerCamera::ViewerCamera() 
{
    _angleX = -(float)M_PI / 8;
    _angleY = (float)M_PI;
}

void ViewerCamera::Update(float dt) {
  auto* scene_camera = Engine::Get()->GetScene()->GetCamera();

    auto input = Engine::Get()->GetInput();
    vec3 posDelta = vec3(0, 0, 0);
	if (input->keyDown(Key::E)) {
		posDelta += scene_camera->Transform().Up();
	}

	if (input->keyDown(Key::Q)) {
		posDelta += scene_camera->Transform().Down();
	}

    if (input->keyDown(Key::A)) {
      posDelta += scene_camera->Transform().Left();
    }

    if (input->keyDown(Key::D)) {
      posDelta += scene_camera->Transform().Right();
    }

    if (input->keyDown(Key::W)) {
      posDelta += scene_camera->Transform().Forward();
    }

    if (input->keyDown(Key::S)) {
      posDelta += scene_camera->Transform().Backward();
    }

    if (input->keyDown(Key::MouseRight)) {
		//scene_camera->transform()->rotate(vec3(1, 0, 0), -input->mouseDelta().y * 0.008f);
		//scene_camera->transform()->rotate(vec3(0, 1, 0), -input->mouseDelta().x * 0.008f);
		_angleX -= input->mouseDelta().y * 0.008f;
		_angleY -= input->mouseDelta().x * 0.008f;
    }

	scene_camera->Transform().Translate(posDelta * dt * 20.0f);
	quat rotation(vec3(_angleX, _angleY, 0));
	scene_camera->Transform().rotation = rotation;

  //auto* scene_camera = Engine::Get()->GetScene()->GetCamera();
  //scene_camera->transform()->setMatrix(transform()->worldMatrix());
}
