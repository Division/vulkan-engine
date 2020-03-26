#include "ViewerCamera.h"
#include "Engine.h"
#include "system/Input.h"
#include "scene/Scene.h"

using namespace core;
using namespace core::system;

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
		posDelta += scene_camera->transform()->up();
	}

	if (input->keyDown(Key::Q)) {
		posDelta += scene_camera->transform()->down();
	}

    if (input->keyDown(Key::A)) {
      posDelta += scene_camera->transform()->left();
    }

    if (input->keyDown(Key::D)) {
      posDelta += scene_camera->transform()->right();
    }

    if (input->keyDown(Key::W)) {
      posDelta += scene_camera->transform()->forward();
    }

    if (input->keyDown(Key::S)) {
      posDelta += scene_camera->transform()->backward();
    }

    if (input->keyDown(Key::MouseLeft)) {
		//scene_camera->transform()->rotate(vec3(1, 0, 0), -input->mouseDelta().y * 0.008f);
		//scene_camera->transform()->rotate(vec3(0, 1, 0), -input->mouseDelta().x * 0.008f);
		_angleX -= input->mouseDelta().y * 0.008f;
		_angleY -= input->mouseDelta().x * 0.008f;
    }

	scene_camera->transform()->translate(posDelta * dt * 20.0f);
	quat rotation(vec3(_angleX, _angleY, 0));
	scene_camera->transform()->rotation(rotation);

  //auto* scene_camera = Engine::Get()->GetScene()->GetCamera();
  //scene_camera->transform()->setMatrix(transform()->worldMatrix());
}
