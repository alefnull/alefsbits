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
		configParam(REGISTER_1_PARAM, 0.f, 1.f, 1.f, "register 1");
		configParam(REGISTER_2_PARAM, 0.f, 1.f, 1.f, "register 2");
		configParam(REGISTER_3_PARAM, 0.f, 1.f, 1.f, "register 3");
		configParam(REGISTER_4_PARAM, 0.f, 1.f, 1.f, "register 4");
		configParam(REGISTER_5_PARAM, 0.f, 1.f, 1.f, "register 5");
		configParam(REGISTER_6_PARAM, 0.f, 1.f, 1.f, "register 6");
		configParam(REGISTER_7_PARAM, 0.f, 1.f, 1.f, "register 7");
		configParam(REGISTER_8_PARAM, 0.f, 1.f, 1.f, "register 8");
		configInput(SIGNAL_INPUT, "signal");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(REGISTER_1_INPUT, "register 1");
		configInput(REGISTER_2_INPUT, "register 2");
		configInput(REGISTER_3_INPUT, "register 3");
		configInput(REGISTER_4_INPUT, "register 4");
		configInput(REGISTER_5_INPUT, "register 5");
		configInput(REGISTER_6_INPUT, "register 6");
		configInput(REGISTER_7_INPUT, "register 7");
		configInput(REGISTER_8_INPUT, "register 8");
		configOutput(REGISTER_1_OUTPUT, "register 1");
		configOutput(REGISTER_2_OUTPUT, "register 2");
		configOutput(REGISTER_3_OUTPUT, "register 3");
		configOutput(REGISTER_4_OUTPUT, "register 4");
		configOutput(REGISTER_5_OUTPUT, "register 5");
		configOutput(REGISTER_6_OUTPUT, "register 6");
		configOutput(REGISTER_7_OUTPUT, "register 7");
		configOutput(REGISTER_8_OUTPUT, "register 8");
	}

    dsp::SchmittTrigger trigger;
	float last_sample[8] = {0.f};
	bool unipolar = false;
	bool scrambled = false;
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
			bool used[8] = {false};
			for (int i = OUTPUTS_LEN - 1; i >= 0; i--) {
				float chance = params[REGISTER_1_PARAM + i].getValue();
				if (inputs[REGISTER_1_INPUT + i].isConnected()) {
					chance = clamp(chance * inputs[REGISTER_1_INPUT + i].getVoltage() / 10.f, 0.f, 1.f);
				}
				if (i == 0) {
					out = signal;
				} else {
					if (!scrambled) {
						out = last_sample[i - 1];
					} else {
						int r = random::uniform() * OUTPUTS_LEN;
						while (used[r]) {
							r = random::uniform() * OUTPUTS_LEN;
						}
						used[r] = true;
						out = last_sample[r];
					}
				}
				if (random::uniform() < chance) {
					last_sample[i] = out;
				}
				outputs[REGISTER_1_OUTPUT + i].setVoltage(last_sample[i]);
			}
		}
	}

	void onReset() override {
		for (int i = 0; i < 8; i++) {
			last_sample[i] = 0.f;
		}
	}

	void onRandomize() override {
		for (int i = 0; i < 8; i++) {
			params[REGISTER_1_PARAM + i].setValue(random::uniform());
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "unipolar", json_boolean(unipolar));
		json_object_set_new(rootJ, "scrambled", json_boolean(scrambled));
		json_object_set_new(rootJ, "range", json_real(range));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* unipolarJ = json_object_get(rootJ, "unipolar");
		if (unipolarJ) {
			unipolar = json_boolean_value(unipolarJ);
		}
		json_t* scrambledJ = json_object_get(rootJ, "scrambled");
		if (scrambledJ) {
			scrambled = json_boolean_value(scrambledJ);
		}
		json_t* rangeJ = json_object_get(rootJ, "range");
		if (rangeJ) {
			range = json_real_value(rangeJ);
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
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Scrambled Eggs!", CHECKMARK(module->scrambled), [module]() { module->scrambled = !module->scrambled; }));
	}
};


Model* modelShift = createModel<Shift, ShiftWidget>("shift");