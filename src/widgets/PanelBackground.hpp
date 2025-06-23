#pragma once

#include <rack.hpp>

#define CONTRAST_MIN 0.1f
#define CONTRAST_MAX 0.9f

using namespace rack;

struct PanelBackground : TransparentWidget
{
  float contrast = CONTRAST_MAX;
  bool inverted = false;
  NVGcolor color = nvgRGB(0xff * contrast, 0xff * contrast, 0xff * contrast);

  PanelBackground()
  {
    if (this->parent)
    {
      this->box.size.x = this->parent->box.size.x - 2;
      this->box.size.y = this->parent->box.size.y - 2;
      this->box.pos.x = 1;
      this->box.pos.y = 1;
    }
  }

  void invert(bool invert);
  void drawPanel(const DrawArgs &args);
  void draw(const DrawArgs &args) override;
};
