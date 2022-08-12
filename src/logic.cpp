#include "plugin.hpp"

#define MAX_POLY 16

struct Logic : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		A_INPUT,
		B_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AND_OUTPUT,
		OR_OUTPUT,
		XOR_OUTPUT,
		NAND_OUTPUT,
		NOR_OUTPUT,
		XNOR_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Logic() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(A_INPUT, "");
		configInput(B_INPUT, "");
		configOutput(AND_OUTPUT, "");
		configOutput(OR_OUTPUT, "");
		configOutput(XOR_OUTPUT, "");
		configOutput(NAND_OUTPUT, "");
		configOutput(NOR_OUTPUT, "");
		configOutput(XNOR_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		int a_channels = inputs[A_INPUT].getChannels();
		int b_channels = inputs[B_INPUT].getChannels();
		int channels = std::max(a_channels, b_channels);
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			outputs[i].setChannels(channels);
		}
		for (int c = 0; c < channels; c++) {
			bool a = inputs[A_INPUT].getVoltage(c) > 0.0;
			bool b = inputs[B_INPUT].getVoltage(c) > 0.0;
			outputs[AND_OUTPUT].setVoltage(a && b ? 10.0 : 0.0, c);
			outputs[OR_OUTPUT].setVoltage(a || b ? 10.0 : 0.0, c);
			outputs[XOR_OUTPUT].setVoltage(a ^ b ? 10.0 : 0.0, c);
			outputs[NAND_OUTPUT].setVoltage(!(a && b) ? 10.0 : 0.0, c);
			outputs[NOR_OUTPUT].setVoltage(!(a || b) ? 10.0 : 0.0, c);
			outputs[XNOR_OUTPUT].setVoltage(!(a ^ b) ? 10.0 : 0.0, c);
		}
	}
};


struct LogicWidget : ModuleWidget {
	LogicWidget(Logic* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/logic.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.599, 24.981)), module, Logic::A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.599, 36.724)), module, Logic::B_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 51.547)), module, Logic::AND_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 62.079)), module, Logic::OR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 73.563)), module, Logic::XOR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 84.639)), module, Logic::NAND_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 96.023)), module, Logic::NOR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.285, 106.963)), module, Logic::XNOR_OUTPUT));
	}
};


Model* modelLogic = createModel<Logic, LogicWidget>("logic");