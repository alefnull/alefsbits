#include "plugin.hpp"


struct Simplexvco : Module {
	enum ParamId {
		FREQUENCY_PARAM,
		X_PARAM,
		Y_PARAM,
		SCALE_PARAM,
		DETAIL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FREQUENCY_INPUT,
		X_INPUT,
		Y_INPUT,
		SCALE_INPUT,
		DETAIL_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Simplexvco() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQUENCY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X_PARAM, 0.f, 1.f, 0.f, "");
		configParam(Y_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SCALE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DETAIL_PARAM, 0.f, 1.f, 0.f, "");
		configInput(FREQUENCY_INPUT, "");
		configInput(X_INPUT, "");
		configInput(Y_INPUT, "");
		configInput(SCALE_INPUT, "");
		configInput(DETAIL_INPUT, "");
		configOutput(OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct SimplexvcoWidget : ModuleWidget {
	SimplexvcoWidget(Simplexvco* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/simplexvco.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.498, 33.666)), module, Simplexvco::FREQUENCY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.498, 50.808)), module, Simplexvco::X_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.498, 64.027)), module, Simplexvco::Y_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.498, 77.93)), module, Simplexvco::SCALE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.498, 92.8)), module, Simplexvco::DETAIL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 33.666)), module, Simplexvco::FREQUENCY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 50.808)), module, Simplexvco::X_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 64.027)), module, Simplexvco::Y_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 77.93)), module, Simplexvco::SCALE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.792, 92.8)), module, Simplexvco::DETAIL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16, 111.388)), module, Simplexvco::OUT_OUTPUT));
	}
};


Model* modelSimplexvco = createModel<Simplexvco, SimplexvcoWidget>("simplexvco");