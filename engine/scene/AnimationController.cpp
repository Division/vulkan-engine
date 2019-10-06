//
// Created by Sidorenko Nikita on 2018-12-15.
//

#include "AnimationController.h"
#include "GameObject.h"
#include <cmath>
#include <algorithm>

const float MIN_UPDATE_WEIGHT = 0.001f;

AnimationPlayback::AnimationPlayback(const AnimationSequence &sequence, AnimationDataPtr animationData)
  : _sequence(sequence), _animationData(animationData) {
  _timePerFrame = 1.0f / _animationData->fps;
}

void AnimationPlayback::update(float dt) {
  _animationTime += dt;

  if (!_isLooping && _animationTime >= _duration) {
    _isPlaying = false;
    _animationTime = _duration;
  }

  _currentFrame = _animationTime / _timePerFrame;
  int currentFrameInt = (int)lround(floorf(_currentFrame)) % _sequence.count;
  _currentFrameA = (_sequence.startFrame + currentFrameInt) % _animationData->frameCount;
  _currentFrameB = (_sequence.startFrame + currentFrameInt) % _animationData->frameCount;
  _currentDelta = _currentFrame - floorf(_currentFrame);
}

void AnimationPlayback::play(bool loop, bool resetTime) {
  if (!this->_animationData) {
    ENGLog("Attempt to play with null AnimationData");
    return;
  }

  _weight = 1;
  _isPlaying = true;
  _isLooping = loop;
  _animationTime = 0;
}

void AnimationPlayback::stop() {
  _isPlaying = false;
  _isLooping = false;
  _weight = 0;
}


void AnimationController::update(float dt) {
  _activePlaybacks.clear();
  float totalWeight = 0;

  for (auto &iterator : _playbacks) {
    auto &p = iterator.second;

    if (p->isPlaying()) {
      p->update(dt);
    }

    if (p->weight() > MIN_UPDATE_WEIGHT) {
      totalWeight += p->weight();
      _activePlaybacks.push_back(p);
    }
  }

  applyWeightedPlaybacks(_activePlaybacks, totalWeight);

  for (auto &child : _childControllers) {
    if (child->animationData()) {
      child->applyWeightedPlaybacks(_activePlaybacks, totalWeight);
    }
  }
}

void AnimationController::play(std::string animationName, bool loop) {
  if (!this->_animationData) {
    ENGLog("Attempt to play with null AnimationData");
    return;
  }

  for (auto &iterator : _playbacks) {
    iterator.second->stop();
  }
  _playbacks.at(animationName)->play(loop, true);
  _playbacks.at(animationName)->weight(1);
}

void AnimationController::animationData(AnimationDataPtr value) {
  _animationData = value;
  _sequences.clear();

  if (!value->sequences.empty()) {
    _sequences["default"] = value->sequences[0];

    for (auto &sequence : value->sequences) {
      _sequences[sequence.name] = sequence;
      _playbacks[sequence.name] = std::make_shared<AnimationPlayback>(sequence, value);
    }
  } else {
    _sequences["default"] = { "default", 0, _animationData->frameCount };
  }

  _playbacks["default"] = std::make_shared<AnimationPlayback>(_sequences["default"], value);
}

void AnimationController::applyWeightedPlaybacks(std::vector<AnimationPlaybackPtr> &playbacks, float totalWeight) {
  if (totalWeight < 0.001) {
    return;
  }

  mat4 m(0);
  vec3 scale;
  vec3 position;
  quat rotation(0, 0, 0, 0);
  for (auto &p : playbacks) {
    auto weight = p->weight();
    weight /= totalWeight;

    auto frameA = p->frameA();
    auto frameB = p->frameB();
    auto delta = p->delta();

    if (_animationData->isMatrix) {
      m += _animationData->getMatrix(frameA, frameB, delta) * weight;
    } else {
      scale += _animationData->getScale(frameA, frameB, delta) * weight;
      position += _animationData->getPosition(frameA, frameB, delta) * weight;
      rotation += _animationData->getRotation(frameA, frameB, delta) * weight;
    }
  }

  auto obj = gameObject();
  if (_animationData->isMatrix) {
    obj->transform()->setMatrix(m);
  } else {
    obj->transform()->position(position);
    obj->transform()->scale(scale);
    obj->transform()->rotation(rotation);
  }
}
