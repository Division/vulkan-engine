//
// Created by Sidorenko Nikita on 3/25/18.
//

#include "Texture.h"
#include "system/Logging.h"
#include "Engine.h"

void Texture::_uploadData() {

}

void Texture::_release() {

}

Texture::~Texture() {
	_release();
}
