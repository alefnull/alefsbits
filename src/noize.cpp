#include "plugin.hpp"


struct Noize : Module {
	enum ParamId {
		DURATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUTS_LEN
	};
	enum OutputId {
		NOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Noize() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DURATION_PARAM, 0.0f, 0.002f, 0.0f, "duration");
		configOutput(NOISE_OUTPUT, "noize");
	}

	float last_value = 0.0f;
	float time = 0.0f;

	void process(const ProcessArgs& args) override {
		float duration = params[DURATION_PARAM].getValue();
		time += args.sampleTime;
		if (time > duration) {
			last_value = random::uniform() * 2.0f - 1.0f;
			time = 0.0f;
		}
		outputs[NOISE_OUTPUT].setVoltage(last_value * 5.0f);
	}
};


struct NoizeWidget : ModuleWidget {
	NoizeWidget(Noize* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/noize.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 27.056)), module, Noize::DURATION_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 109.258)), module, Noize::NOISE_OUTPUT));
	}
};


Model* modelNoize = createModel<Noize, NoizeWidget>("noize");