#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "inc/cvRange.hpp"


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
		if (use_global_contrast[SHIFT]) {
			module_contrast[SHIFT] = global_contrast;
		}
	}

    dsp::SchmittTrigger trigger;
	float last_sample[8] = {0.f};
	bool unipolar = false;
	bool scrambled = false;
	CVRange cv_range;

	void process(const ProcessArgs& args) override {
		float signal = 0.f;
		if (inputs[SIGNAL_INPUT].isConnected()) {
			signal = inputs[SIGNAL_INPUT].getVoltage();
		}
		else {
			signal = cv_range.map(random::uniform());
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
				outputs[REGISTER_1_OUTPUT + i].setVoltage(out);
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
		json_object_set_new(rootJ, "cv_range", cv_range.dataToJson());
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
		json_t* cv_rangeJ = json_object_get(rootJ, "cv_range");
		if (cv_rangeJ) {
			cv_range.dataFromJson(cv_rangeJ);
		}
	}
};


struct ShiftWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	ShiftWidget(Shift* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/shift.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 41.655)), module, Shift::REGISTER_1_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 50.764)), module, Shift::REGISTER_2_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 59.874)), module, Shift::REGISTER_3_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 68.983)), module, Shift::REGISTER_4_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 78.092)), module, Shift::REGISTER_5_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 87.202)), module, Shift::REGISTER_6_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 96.311)), module, Shift::REGISTER_7_PARAM));
		addParam(createParamCentered<SmallBitKnob>(mm2px(Vec(14.588, 105.42)), module, Shift::REGISTER_8_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(12.871, 21.057)), module, Shift::SIGNAL_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(22.689, 21.057)), module, Shift::TRIGGER_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 41.655)), module, Shift::REGISTER_1_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 50.764)), module, Shift::REGISTER_2_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 59.874)), module, Shift::REGISTER_3_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 68.983)), module, Shift::REGISTER_4_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 78.092)), module, Shift::REGISTER_5_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 87.202)), module, Shift::REGISTER_6_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 96.311)), module, Shift::REGISTER_7_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(5.54, 105.42)), module, Shift::REGISTER_8_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 41.655)), module, Shift::REGISTER_1_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 50.764)), module, Shift::REGISTER_2_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 59.874)), module, Shift::REGISTER_3_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 68.983)), module, Shift::REGISTER_4_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 78.092)), module, Shift::REGISTER_5_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 87.202)), module, Shift::REGISTER_6_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 96.311)), module, Shift::REGISTER_7_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(26.908, 105.42)), module, Shift::REGISTER_8_OUTPUT));
	}

	void step() override {
		Shift* shiftModule = dynamic_cast<Shift*>(this->module);
		if (!shiftModule) return;
		if (use_global_contrast[SHIFT]) {
			module_contrast[SHIFT] = global_contrast;
		}
		if (module_contrast[SHIFT] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[SHIFT];
			if (panelBackground->contrast < 0.4f) {
				panelBackground->invert(true);
				inverter->invert = true;
			}
			else {
				panelBackground->invert(false);
				inverter->invert = false;
			}
			svgPanel->fb->dirty = true;
		}
		ModuleWidget::step();
	}
	void appendContextMenu(Menu* menu) override {
		Shift* module = dynamic_cast<Shift*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[SHIFT]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[SHIFT]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[SHIFT];
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());
		module->cv_range.addMenu(module, menu);
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Scrambled Eggs!", CHECKMARK(module->scrambled), [module]() { module->scrambled = !module->scrambled; }));
	}
};


Model* modelShift = createModel<Shift, ShiftWidget>("shift");