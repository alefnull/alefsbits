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

	dsp::SchmittTrigger trigger[MAX_POLY];
	float last_value[MAX_POLY] = { 0.f };
	int current_channel[MAX_POLY] = { 0 };

	Polyrand() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(TRIGGER_INPUT, "");
		configInput(POLY_INPUT, "");
		configOutput(RANDOM_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int trig_channels = inputs[TRIGGER_INPUT].isConnected() ? inputs[TRIGGER_INPUT].getChannels() : 1;
		if (trig_channels > MAX_POLY) {
			trig_channels = MAX_POLY;
		}
		outputs[RANDOM_OUTPUT].setChannels(trig_channels);

		if (inputs[TRIGGER_INPUT].isConnected() && 
				inputs[POLY_INPUT].isConnected() && 
				outputs[RANDOM_OUTPUT].isConnected()) {

			int poly_channels = inputs[POLY_INPUT].getChannels();
			if (poly_channels > MAX_POLY) {
				poly_channels = MAX_POLY;
			}
			for (int i = 0; i < trig_channels; i++) {
				if (trigger[i].process(inputs[TRIGGER_INPUT].getVoltage(i))) {
					int chan = random::u32() % poly_channels;
					current_channel[i] = chan;
				}
				last_value[i] = inputs[POLY_INPUT].getVoltage(current_channel[i]);
			}

			for (int i = 0; i < trig_channels; i++) {
				outputs[RANDOM_OUTPUT].setVoltage(last_value[i], i);
			}
		}
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

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 105.127)), module, Polyrand::RANDOM_OUTPUT));
	}
};


Model* modelPolyrand = createModel<Polyrand, PolyrandWidget>("polyrand");