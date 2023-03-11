#pragma once

#include <rack.hpp>

using namespace rack;

#include "../inc/ResizableRingBuffer.hpp"
#include "FancyWidget.hpp"
#include "ScopeData.hpp"

struct Scope : FancyWidget {
    ScopeData* data;

    Scope(ScopeData* data);

    void onButton(const event::Button& e) override;

    void onHover(const event::Hover& e) override;
    void onHoverScroll(const event::HoverScroll& e) override;

    std::pair<float, float> rangeForMode();

    float calculateX(int i);
    float calculateY(float min, float max, float value);

    void drawCurve(const DrawArgs& args,
                   std::function<float(float, float)> clamp, Vec gradientPoint);

    void drawMinCurve(const DrawArgs& args);
    void drawMaxCurve(const DrawArgs& args);
    void drawWave(const DrawArgs& args);
    void drawTriggers(const DrawArgs& args);
    void drawGridline(const DrawArgs& args, float percent);
    void drawGridlines(const DrawArgs& args);
    void drawBackground(const DrawArgs& args);
    void draw(const DrawArgs& args) override;
};