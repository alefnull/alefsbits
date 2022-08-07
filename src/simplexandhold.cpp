#include "plugin.hpp"
#include "inc/SimplexNoise.cpp"

#define MAX_POLY 16

struct Simplexandhold : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SAMPLE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	SimplexNoise noise;
	dsp::SchmittTrigger trigger[MAX_POLY];
	bool unipolar = false;
	float last_sample[MAX_POLY] = {0.0};
	float x[MAX_POLY] = {0.0};
	float range = 1.0;

	Simplexandhold() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(TRIGGER_INPUT, "trigger");
		configOutput(SAMPLE_OUTPUT, "sample");
		noise.init();
	}

	void process(const ProcessArgs& args) override {
		int chans = std::max(1, inputs[TRIGGER_INPUT].getChannels());
		outputs[SAMPLE_OUTPUT].setChannels(chans);
		for (int c = 0; c < chans; c++) {
			if (trigger[c].process(inputs[TRIGGER_INPUT].getVoltage(c))) {
				last_sample[c] = noise.noise(x[c], 0.0) * range;
			}
			x[c] += 0.001;
			if (unipolar) {
				outputs[SAMPLE_OUTPUT].setVoltage((last_sample[c] + range) / 2.0, c);
			}
			else {
				outputs[SAMPLE_OUTPUT].setVoltage(last_sample[c], c);
			}
		}
	}
};


struct SimplexandholdWidget : ModuleWidget {
	SimplexandholdWidget(Simplexandhold* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/simplexandhold.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 35.937)), module, Simplexandhold::TRIGGER_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 100.446)), module, Simplexandhold::SAMPLE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Simplexandhold* module = dynamic_cast<Simplexandhold*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		// add a submenu to choose the range, between
		// +/- 1V, +/- 3V, +/- 5V, +/- 10V
		menu->addChild(createSubmenuItem("Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("-/+ 1v", CHECKMARK(module->range == 1), [module]() { module->range = 1; }));
			rangeMenu->addChild(createMenuItem("-/+ 3v", CHECKMARK(module->range == 3), [module]() { module->range = 3; }));
			rangeMenu->addChild(createMenuItem("-/+ 5v", CHECKMARK(module->range == 5), [module]() { module->range = 5; }));
			rangeMenu->addChild(createMenuItem("-/+ 10v", CHECKMARK(module->range == 10), [module]() { module->range = 10; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar), [module]() { module->unipolar = !module->unipolar; }));
	}
};

Model* modelSimplexandhold = createModel<Simplexandhold, SimplexandholdWidget>("simplexandhold");