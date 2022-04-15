#include "plugin.hpp"

#define MAX_POLY 16

struct Polyrand : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		POLY_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		RANDOM_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::SchmittTrigger trigger;
	float last_value = 0.0f;

	Polyrand() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(TRIGGER_INPUT, "");
		configInput(POLY_INPUT, "");
		configOutput(RANDOM_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[POLY_INPUT].getChannels();
		if (channels > MAX_POLY) {
			channels = MAX_POLY;
		}
		if (inputs[TRIGGER_INPUT].isConnected()) {
			if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
				int chan = random::u32() % channels;
				last_value = inputs[POLY_INPUT].getVoltage(chan);	
			}
		}
		outputs[RANDOM_OUTPUT].setVoltage(last_value);
	}
};


struct PolyrandWidget : ModuleWidget {
	PolyrandWidget(Polyrand* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/polyrand.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 30.154)), module, Polyrand::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 46.058)), module, Polyrand::POLY_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 112.356)), module, Polyrand::RANDOM_OUTPUT));
	}
};


Model* modelPolyrand = createModel<Polyrand, PolyrandWidget>("polyrand");