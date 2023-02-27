#include "plugin.hpp"

#define MAX_POLY 16

struct Math : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ADD_OUTPUT,
		SUB_OUTPUT,
		MULT_OUTPUT,
		DIV_OUTPUT,
		MOD_OUTPUT,
		AVG_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Math() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(A_INPUT, "");
		configInput(B_INPUT, "");
		configOutput(ADD_OUTPUT, "");
		configOutput(SUB_OUTPUT, "");
		configOutput(MULT_OUTPUT, "");
		configOutput(DIV_OUTPUT, "");
		configOutput(MOD_OUTPUT, "");
		configOutput(AVG_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int a_channels = inputs[A_INPUT].getChannels();
		int b_channels = inputs[B_INPUT].getChannels();
		int channels = std::max(a_channels, b_channels);
		for (int o = 0; o < OUTPUTS_LEN; o++) {
			outputs[o].setChannels(channels);
		}
		for (int c = 0; c < channels; c++) {
			float a = inputs[A_INPUT].getVoltage(c);
			float b = inputs[B_INPUT].getVoltage(c);
			outputs[ADD_OUTPUT].setVoltage(clamp(a + b, -10.0f, 10.0f), c);
			outputs[SUB_OUTPUT].setVoltage(clamp(a - b, -10.0f, 10.0f), c);
			outputs[MULT_OUTPUT].setVoltage(clamp(a * b, -10.0f, 10.0f), c);
			outputs[DIV_OUTPUT].setVoltage(clamp(a / b, -10.0f, 10.0f), c);
			outputs[MOD_OUTPUT].setVoltage(clamp(fmod(a, b), -10.0f, 10.0f), c);
			outputs[AVG_OUTPUT].setVoltage(clamp((a + b) / 2.0f, -10.0f, 10.0f), c);
		}
	}
};


struct MathWidget : ModuleWidget {
	MathWidget(Math* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/math.svg")));

		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.599, 24.981)), module, Math::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.599, 36.724)), module, Math::B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 51.547)), module, Math::ADD_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 62.079)), module, Math::SUB_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 73.563)), module, Math::MULT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 84.639)), module, Math::DIV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 96.023)), module, Math::MOD_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 106.963)), module, Math::AVG_OUTPUT));
	}
};


Model* modelMath = createModel<Math, MathWidget>("math");