//
// Created by Sidorenko Nikita on 2018-12-15.
//

#ifndef CPPWRAPPER_ANIMATIONCONTROLLER_H
#define CPPWRAPPER_ANIMATIONCONTROLLER_H

#include "resources/ModelBundle.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class GameObject;

class AnimationPlayback {
public:
  AnimationPlayback(const AnimationSequence &sequence, AnimationDataPtr animationData);

  void update(float dt);
  void play(bool loop, bool resetTime = true);
  void stop();

  int frameA() const { return _currentFrameA; }
  int frameB() const { return _currentFrameB; }
  float delta() const { return _currentDelta; }
  float weight() const { return _weight; }
  void weight(float weight) { _weight = weight; }
  bool isPlaying() const { return _isPlaying; }

private:
  AnimationSequence _sequence;
  AnimationDataPtr _animationData;

  int _currentFrameA = 0;
  int _currentFrameB = 0;
  float _currentDelta = 0; // [0..1] interpolation value between frames A and B
  float _currentFrame = 0; // float current frame value

  float _duration; // seconds
  float _weight = 0;
  float _animationTime = 0;
  float _timePerFrame = 0;
  bool _isLooping = false;
  bool _isPlaying = false;
};

typedef std::shared_ptr<AnimationPlayback> AnimationPlaybackPtr;

class AnimationController {
public:
  template <typename T> friend std::shared_ptr<T> CreateGameObject();

  AnimationController() = default;
  AnimationController(const AnimationController &a) = delete;

  void animationData(AnimationDataPtr value);
  const AnimationDataPtr animationData() const { return _animationData; }
  bool hasAnimation() const { return (bool)_animationData; }
  bool autoUpdate() const { return _autoUpdate; }
  void autoUpdate(bool value) { _autoUpdate = value; }

  std::shared_ptr<GameObject> gameObject() { return _gameObject.lock(); }
  void addChildController(std::shared_ptr<AnimationController> controller) { _childControllers.push_back(controller); }

  AnimationPlaybackPtr getPlayback(const std::string &name) { return _playbacks.at(name); }

  void play(std::string animationName = "default", bool loop = false);
  void update(float dt);

  void applyWeightedPlaybacks(std::vector<AnimationPlaybackPtr> &playbacks, float totalWeight);

private:
  AnimationDataPtr _animationData;
  std::weak_ptr<GameObject> _gameObject;
  bool _autoUpdate = true;

  // list of controllers that have the same animations set
  // They are controlled manually from a single place (this, parent one)
  std::vector<std::shared_ptr<AnimationController>> _childControllers;
  std::unordered_map<std::string, AnimationPlaybackPtr> _playbacks;
  std::vector<AnimationPlaybackPtr> _activePlaybacks; // populated every update
  std::unordered_map<std::string, AnimationSequence> _sequences;
};

typedef std::shared_ptr<AnimationController> AnimationControllerPtr;

#endif //CPPWRAPPER_ANIMATIONCONTROLLER_H
