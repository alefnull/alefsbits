#pragma once
#include <rack.hpp>


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
