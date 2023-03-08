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
	p->addModel(modelSlipspander);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}

bool use_global_contrast[MODULES_LEN] = {
	true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true
};
float global_contrast = CONTRAST_MAX;
float module_contrast[MODULES_LEN] = {
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX,
	CONTRAST_MAX
};

json_t* settingsToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "global_contrast", json_real(global_contrast));
	json_t* use_global_contrastJ = json_array();
	for (int i = 0; i < MODULES_LEN; i++) {
		json_array_append_new(use_global_contrastJ, json_boolean(use_global_contrast[i]));
	}
	json_object_set_new(rootJ, "use_global_contrast", use_global_contrastJ);
	json_t* module_contrastJ = json_array();
	for (int i = 0; i < MODULES_LEN; i++) {
		json_array_append_new(module_contrastJ, json_real(module_contrast[i]));
	}
	json_object_set_new(rootJ, "module_contrast", module_contrastJ);
	return rootJ;
}

void settingsFromJson(json_t* rootJ) {
	json_t* global_contrastJ = json_object_get(rootJ, "global_contrast");
	if (global_contrastJ) {
		global_contrast = json_real_value(global_contrastJ);
	}
	else {
		global_contrast = CONTRAST_MAX;
	}
	json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
	if (use_global_contrastJ) {
		for (int i = 0; i < MODULES_LEN; i++) {
			json_t* use_global_contrastJ_i = json_array_get(use_global_contrastJ, i);
			if (use_global_contrastJ_i)
				use_global_contrast[i] = json_boolean_value(use_global_contrastJ_i);
		}
	}
	else {
		for (int i = 0; i < MODULES_LEN; i++) {
			use_global_contrast[i] = true;
		}
	}
	json_t* module_contrastJ = json_object_get(rootJ, "module_contrast");
	if (module_contrastJ) {
		for (int i = 0; i < MODULES_LEN; i++) {
			json_t* module_contrastJ_i = json_array_get(module_contrastJ, i);
			if (module_contrastJ_i)
				module_contrast[i] = json_real_value(module_contrastJ_i);
		}
	}
	else {
		for (int i = 0; i < MODULES_LEN; i++) {
			module_contrast[i] = CONTRAST_MAX;
		}
	}
}
