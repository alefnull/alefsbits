#include "PanelBackground.hpp"


void PanelBackground::invert(bool invert) {
    inverted = invert;
    if (!inverted) {
        color = nvgRGB(0xff * contrast, 0xff * contrast, 0xff * contrast);
    } else {
        color = nvgRGB(0xff * (1 - contrast), 0xff * (1 - contrast), 0xff * (1 - contrast));
    }
}

void PanelBackground::setColor(NVGcolor color) {
    this->color = color;
}

void PanelBackground::drawPanel(const DrawArgs &args) {
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgClosePath(args.vg);
    // color = nvgRGB(0xff * contrast, 0xff * contrast, 0xff * contrast);
    nvgFillColor(args.vg, color);
    nvgFill(args.vg);
}

void PanelBackground::draw(const DrawArgs &args) {
    drawPanel(args);
}
