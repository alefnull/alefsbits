#pragma once

#include <rack.hpp>

using namespace rack;

struct FancyWidget : OpaqueWidget
{
  void withPath(const DrawArgs &args, std::function<void()> f);
  void withStroke(const DrawArgs &args, float size, NVGcolor color,
                  std::function<void()> f);
  void withFill(const DrawArgs &args, NVGcolor color,
                std::function<void()> f);
};