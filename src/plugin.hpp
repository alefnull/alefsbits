#pragma once
#include <rack.hpp>

#define CONTRAST_MIN 0.1f
#define CONTRAST_MAX 0.9f

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelSimplexandhold;
extern Model* modelBlank6hp;
extern Model* modelPolyrand;
extern Model* modelNoize;
extern Model* modelSteps;
extern Model* modelFibb;
extern Model* modelOctsclr;
extern Model* modelShift;
extern Model* modelMlt;
extern Model* modelMath;
extern Model* modelLogic;
extern Model* modelProbablynot;
extern Model* modelPolyplay;
extern Model* modelLights;
extern Model* modelSlips;
extern Model* modelTurnt;
extern Model* modelSlipspander;
extern Model* modelNos;

enum ModuleNames {
    SIMPLEXANDHOLD,
    BLANK6HP,
    POLYRAND,
    NOIZE,
    STEPS,
    FIBB,
    OCTSCLR,
    SHIFT,
    MLT,
    MATH,
    LOGIC,
    PROBABLYNOT,
    POLYPLAY,
    LIGHTS,
    SLIPS,
    TURNT,
    NOS,
    MODULES_LEN
};

struct SpanderMessage {
    std::vector<int> custom_scale;
};

extern bool use_global_contrast[MODULES_LEN];
extern float module_contrast[MODULES_LEN];

extern float global_contrast;

extern void settings_save();
extern void settings_load();
extern json_t* settingsToJson();
extern void settingsFromJson(json_t* rootJ);

struct ContrastQuantity : Quantity {
    float* contrast;

    ContrastQuantity(float* contrast) {
        this->contrast = contrast;
    }

    void setValue(float value) override {
        *contrast = clamp(value, CONTRAST_MIN, CONTRAST_MAX);
    }

    float getValue() override {
        return *contrast;
    }

    float getDefaultValue() override {
        return CONTRAST_MAX;
    }

    float getMinValue() override {
        return CONTRAST_MIN;
    }

    float getMaxValue() override {
        return CONTRAST_MAX;
    }

    float getDisplayValue() override {
        return getValue();
    }

    std::string getUnit() override {
        return "";
    }
};

struct ContrastSlider : ui::Slider {
    ContrastSlider(float* contrast) {
        quantity = new ContrastQuantity(contrast);
    }
    ~ContrastSlider() {
        delete quantity;
    }
};

struct GlobalOption : ui::MenuItem {
    bool* global;
    GlobalOption(bool* global) {
        this->global = global;
        this->text = "use global contrast";
        this->rightText = CHECKMARK(*global);
    }
    void onAction(const ActionEvent& e) override {
        *global = !(*global);
        e.unconsume();
    }
    void step() override {
        rightText = CHECKMARK(*global);
        MenuItem::step();
    }
};

struct BitKnob : RoundBlackKnob {
	BitKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/bitknob_fg.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/bitknob_bg.svg")));
	}
};

struct LargeBitKnob : RoundLargeBlackKnob {
    LargeBitKnob() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/largebitknob_fg.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/largebitknob_bg.svg")));
    }
};

struct SmallBitKnob : RoundSmallBlackKnob {
    SmallBitKnob() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/smallbitknob_fg.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/smallbitknob_bg.svg")));
    }
};

struct BitPort : SvgPort {
    BitPort() {
        setSvg(APP->window->loadSvg(rack::asset::plugin(pluginInstance, "res/components/bitport.svg")));
        this->shadow->opacity = 0.f;
    }
};
