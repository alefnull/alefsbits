#include "plugin.hpp"

#define MAX_POLY 16


struct Octsclr : Module {
	enum ParamId {
		SCALER_PARAM,
		OFFSET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SCALED_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Octsclr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SCALER_PARAM, 0, 20, 10, "scaler");
		getParamQuantity(SCALER_PARAM)->snapEnabled = true;
		configParam(OFFSET_PARAM, -3, 3, 0, "offset");
		getParamQuantity(OFFSET_PARAM)->snapEnabled = true;
		configInput(SOURCE_INPUT, "source");
		configOutput(SCALED_OUTPUT, "scaled");
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[SOURCE_INPUT].getChannels();
		if (channels > MAX_POLY) {
			channels = MAX_POLY;
		}
		outputs[SCALED_OUTPUT].setChannels(channels);
		float scaler = (float)params[SCALER_PARAM].getValue() / 10.0f;
		float offset = (float)params[OFFSET_PARAM].getValue();
		for (int i = 0; i < channels; i++) {
			float source = inputs[SOURCE_INPUT].getPolyVoltage(i);
			outputs[SCALED_OUTPUT].setVoltage(clamp(source * scaler + offset, -10.0f, 10.0f), i);
		}
	}
};


struct OctsclrWidget : ModuleWidget {
	OctsclrWidget(Octsclr* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/octsclr.svg")));

		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 28.443)), module, Octsclr::SCALER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 48.843)), module, Octsclr::OFFSET_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 91.678)), module, Octsclr::SOURCE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.62, 106.319)), module, Octsclr::SCALED_OUTPUT));
	}
};


Model* modelOctsclr = createModel<Octsclr, OctsclrWidget>("octsclr");