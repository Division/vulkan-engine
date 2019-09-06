//
// Created by Sidorenko Nikita on 11/24/18.
//

#ifndef CPPWRAPPER_PERFORMANCE_H
#define CPPWRAPPER_PERFORMANCE_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "system/Logging.h"
#include "Engine.h"
#include "render/renderer/SceneRenderer.h"
#include "tbb/tick_count.h"

namespace engine {

class Performance {
public:
  enum class Entry : int {
    Frame, // Complete render / update duration
    DepthPrePass,
    MainPass,
    SwapBuffers,
    UpdateTransform,
	UpdateVisibility,
    LightGrid
  };

  static void initialize () {
	_initialTime = tbb::tick_count::now();
    registerEntry("Frame", (int)Entry::Frame);
    registerEntry("DepthPrePass", (int)Entry::DepthPrePass);
    registerEntry("SwapBuffers", (int)Entry::SwapBuffers);
    registerEntry("MainPass", (int)Entry::MainPass);
	registerEntry("UpdateTransform", (int)Entry::UpdateTransform);
	registerEntry("UpdateVisibility", (int)Entry::UpdateVisibility);
//    registerEntry("Update", (int)Entry::Update);
    registerEntry("LightGrid", (int)Entry::LightGrid);
  }

  static void registerEntry(std::string name, int entry) {
    _entries[entry].name = name;
  }

  static void startTimer(Entry entry) {
    if (_lastUpdateTime == 0.0) {
      _lastUpdateTime = getMilliseconds() / 1000; // seconds
    }


    auto &data = _entries[(int)entry];
    data.lastTime = getMilliseconds();
  }

  static void stopTimer(Entry entry) {
    double time = getMilliseconds();

    auto &data = _entries[(int)entry];
    data.timeIntervals.push_back(time - data.lastTime);

    double timeSeconds = time / 1000;

    if (timeSeconds - _lastUpdateTime > 1) {
      _lastUpdateTime = timeSeconds; // seconds
      _calculateAverages();
    }
  }

  static void printAverages () {
    if (_averagePrinted) {
      return;
    }

    _averagePrinted = true;

    ENGLog("======== Performance ========");
    ENGLog("  [FPS] : %i", (int)_fps);
    for (auto &entry : _entries) {
      ENGLog("  [%s] : %f", entry.second.name.c_str(), entry.second.average);
    }

  //  ENGLog("Draw calls: %i", Engine::Get()->sceneRenderer()->ropCount());
  }

  static double getMilliseconds() {
	 // return Engine::Get()->time() * 1000.0;
    auto t = tbb::tick_count::now();
	return (t - _initialTime).seconds() * 1000.0;
  }

private:
  static double _fps;
  static double _lastUpdateTime;
  static bool _averagePrinted;
  static tbb::tick_count _initialTime;

  static void _calculateAverages() {
    _averagePrinted = false;

    for (auto &entry : _entries) {
      auto &data = entry.second;
      if (data.timeIntervals.empty()) {
        data.average = 0;
        continue;
      }

      double summMS = 0;
      for (double ms : data.timeIntervals) {
        summMS += ms;
      }

      data.average = summMS / data.timeIntervals.size();
      data.timeIntervals.clear();
    }

    _fps = 1000.0 / _entries[(int)Entry::Frame].average;
  }

  struct EntryData {
    std::string name;
    double average = 0;
    double lastTime;
    std::vector<double> timeIntervals;
  };

  static std::map<int, EntryData> _entries;
};

}

#endif //CPPWRAPPER_PERFORMANCE_H
