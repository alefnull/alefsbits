#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelSimplexandhold);
	p->addModel(modelBlank6hp);
	p->addModel(modelPolyrand);
	p->addModel(modelNoize);
	p->addModel(modelSteps);
	p->addModel(modelFibb);
	p->addModel(modelOctsclr);
	p->addModel(modelShift);
	p->addModel(modelMlt);
	p->addModel(modelMath);
	p->addModel(modelLogic);
	p->addModel(modelProbablynot);
	p->addModel(modelPolyplay);
	p->addModel(modelLights);
	p->addModel(modelSlips);
	p->addModel(modelTurnt);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
