#pragma once
#include <rack.hpp>
using namespace rack;

struct Inverter : TransparentWidget
{
    bool invert = false;
    Inverter() {}
    void draw(const DrawArgs &args) override
    {
        if (invert)
        {
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, SCHEME_WHITE);
            nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            nvgGlobalCompositeBlendFuncSeparate(args.vg, NVG_ONE_MINUS_DST_COLOR,
                                                NVG_ZERO, NVG_ONE_MINUS_DST_COLOR,
                                                NVG_ONE);
            nvgFill(args.vg);
            nvgClosePath(args.vg);
        }
    }
};
