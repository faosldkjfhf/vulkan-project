#pragma once

#include "pch.h"
#include <deque>

namespace bisky {
namespace core {

struct DeletionQueue {
  std::deque<std::function<void()>> deletors;

  void push_function(std::function<void()> &&function) { deletors.push_back(function); }

  void flush() {
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
      (*it)();
    }

    deletors.clear();
  }
};

} // namespace core
} // namespace bisky
