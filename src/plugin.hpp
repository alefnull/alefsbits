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
    MODULES_LEN
};

extern bool use_global_contrast[MODULES_LEN];
extern float module_contrast[MODULES_LEN];

// global contrast
extern float global_contrast;

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
