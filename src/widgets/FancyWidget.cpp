#include "FancyWidget.hpp"

void FancyWidget::withPath(const DrawArgs& args, std::function<void()> f) {
    nvgBeginPath(args.vg);
    f();
    nvgClosePath(args.vg);
}

void FancyWidget::withStroke(const DrawArgs& args, float size, NVGcolor color,
                             std::function<void()> f) {
    nvgStrokeWidth(args.vg, size);
    nvgStrokeColor(args.vg, color);
    f();
    nvgStroke(args.vg);
}

void FancyWidget::withFill(const DrawArgs& args, NVGcolor color,
                           std::function<void()> f) {
    nvgFillColor(args.vg, color);
    f();
    nvgFill(args.vg);
}