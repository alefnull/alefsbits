#include "plugin.hpp"


struct Shift : Module {
	enum ParamId {
		REGISTER_1_PARAM,
		REGISTER_2_PARAM,
		REGISTER_3_PARAM,
		REGISTER_4_PARAM,
		REGISTER_5_PARAM,
		REGISTER_6_PARAM,
		REGISTER_7_PARAM,
		REGISTER_8_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		TRIGGER_INPUT,
		REGISTER_1_INPUT,
		REGISTER_2_INPUT,
		REGISTER_3_INPUT,
		REGISTER_4_INPUT,
		REGISTER_5_INPUT,
		REGISTER_6_INPUT,
		REGISTER_7_INPUT,
		REGISTER_8_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		REGISTER_1_OUTPUT,
		REGISTER_2_OUTPUT,
		REGISTER_3_OUTPUT,
		REGISTER_4_OUTPUT,
		REGISTER_5_OUTPUT,
		REGISTER_6_OUTPUT,
		REGISTER_7_OUTPUT,
		REGISTER_8_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Shift() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(REGISTER_1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_3_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_4_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_5_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_6_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_7_PARAM, 0.f, 1.f, 0.f, "");
		configParam(REGISTER_8_PARAM, 0.f, 1.f, 0.f, "");
		configInput(SIGNAL_INPUT, "");
		configInput(TRIGGER_INPUT, "");
		configInput(REGISTER_1_INPUT, "");
		configInput(REGISTER_2_INPUT, "");
		configInput(REGISTER_3_INPUT, "");
		configInput(REGISTER_4_INPUT, "");
		configInput(REGISTER_5_INPUT, "");
		configInput(REGISTER_6_INPUT, "");
		configInput(REGISTER_7_INPUT, "");
		configInput(REGISTER_8_INPUT, "");
		configOutput(REGISTER_1_OUTPUT, "");
		configOutput(REGISTER_2_OUTPUT, "");
		configOutput(REGISTER_3_OUTPUT, "");
		configOutput(REGISTER_4_OUTPUT, "");
		configOutput(REGISTER_5_OUTPUT, "");
		configOutput(REGISTER_6_OUTPUT, "");
		configOutput(REGISTER_7_OUTPUT, "");
		configOutput(REGISTER_8_OUTPUT, "");
	}

    dsp::SchmittTrigger trigger;
	float last_sample[8] = {0.f};
	bool unipolar = false;
	float range = 1.f;

	void process(const ProcessArgs& args) override {
		float signal = 0.f;
		if (inputs[SIGNAL_INPUT].isConnected()) {
			signal = inputs[SIGNAL_INPUT].getVoltage();
		}
		else {
			if (unipolar) {
				signal = random::uniform() * range;
			}
			else {
				signal = random::uniform() * range - range / 2.f;
			}
		}
		float out = 0.f;
		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			for (int i = OUTPUTS_LEN - 1; i >= 0; i--) {
				float chance = params[REGISTER_1_PARAM + i].getValue();
				if (inputs[REGISTER_1_INPUT + i].isConnected()) {
					chance = clamp(chance * inputs[REGISTER_1_INPUT + i].getVoltage(), 0.f, 1.f);
				}
				if (i == 0) {
					out = signal;
				} else {
					out = last_sample[i - 1];
				}
				if (random::uniform() < chance) {
					last_sample[i] = out;
				}
				outputs[REGISTER_1_OUTPUT + i].setVoltage(last_sample[i]);
			}
		}
	}
};


struct ShiftWidget : ModuleWidget {
	ShiftWidget(Shift* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/shift.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 41.655)), module, Shift::REGISTER_1_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 50.764)), module, Shift::REGISTER_2_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 59.874)), module, Shift::REGISTER_3_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 68.983)), module, Shift::REGISTER_4_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 78.092)), module, Shift::REGISTER_5_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 87.202)), module, Shift::REGISTER_6_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 96.311)), module, Shift::REGISTER_7_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(14.588, 105.42)), module, Shift::REGISTER_8_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.871, 21.057)), module, Shift::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(22.689, 21.057)), module, Shift::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 41.655)), module, Shift::REGISTER_1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 50.764)), module, Shift::REGISTER_2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 59.874)), module, Shift::REGISTER_3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 68.983)), module, Shift::REGISTER_4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 78.092)), module, Shift::REGISTER_5_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 87.202)), module, Shift::REGISTER_6_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 96.311)), module, Shift::REGISTER_7_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.54, 105.42)), module, Shift::REGISTER_8_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 41.655)), module, Shift::REGISTER_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 50.764)), module, Shift::REGISTER_2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 59.874)), module, Shift::REGISTER_3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 68.983)), module, Shift::REGISTER_4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 78.092)), module, Shift::REGISTER_5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 87.202)), module, Shift::REGISTER_6_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 96.311)), module, Shift::REGISTER_7_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.908, 105.42)), module, Shift::REGISTER_8_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Shift* module = dynamic_cast<Shift*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		// add a submenu to choose the range, between
		// +/- 1V, +/- 3V, +/- 5V, +/- 10V
		menu->addChild(createSubmenuItem("Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("-/+ 1v", CHECKMARK(module->range == 1), [module]() { module->range = 1; }));
			rangeMenu->addChild(createMenuItem("-/+ 2v", CHECKMARK(module->range == 2), [module]() { module->range = 2; }));
			rangeMenu->addChild(createMenuItem("-/+ 3v", CHECKMARK(module->range == 3), [module]() { module->range = 3; }));
			rangeMenu->addChild(createMenuItem("-/+ 5v", CHECKMARK(module->range == 5), [module]() { module->range = 5; }));
			rangeMenu->addChild(createMenuItem("-/+ 10v", CHECKMARK(module->range == 10), [module]() { module->range = 10; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar), [module]() { module->unipolar = !module->unipolar; }));
	}
};


Model* modelShift = createModel<Shift, ShiftWidget>("shift");