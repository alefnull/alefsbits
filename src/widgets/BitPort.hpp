#pragma once
#include <rack.hpp>
#include "plugin.hpp"


struct BitPort : rack::app::SvgPort {
    BitPort() {
        setSvg(APP->window->loadSvg(rack::asset::plugin(pluginInstance, "res/components/bitport.svg")));
        this->shadow->opacity = 0.f;
    }
};
