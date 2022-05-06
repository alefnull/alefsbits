#include "plugin.hpp"


struct Steps : Module {
	enum ParamId {
		STEPS_PARAM,
		STEP1_PARAM,
		STEP2_PARAM,
		STEP3_PARAM,
		STEP4_PARAM,
		STEP5_PARAM,
		STEP6_PARAM,
		STEP7_PARAM,
		STEP8_PARAM,
		RAND_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		RAND_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		EOC_OUTPUT,
		CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STEP1_LIGHT,
		STEP2_LIGHT,
		STEP3_LIGHT,
		STEP4_LIGHT,
		STEP5_LIGHT,
		STEP6_LIGHT,
		STEP7_LIGHT,
		STEP8_LIGHT,
		LIGHTS_LEN
	};

	dsp::PulseGenerator eoc_pulse;
	dsp::PulseGenerator rand_pulse;
	dsp::SchmittTrigger clock_trigger;
	dsp::SchmittTrigger rand_trigger;
	dsp::SchmittTrigger prand_trigger;
	dsp::SchmittTrigger reset_trigger;
	int step = 0;
	int steps = 8;
	float range = 1.0;

	Steps() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(STEPS_PARAM, 1.f, 8.f, 8.f, "steps");
		getParamQuantity(STEPS_PARAM)->snapEnabled = true;
		configParam(STEP1_PARAM, -1.f, 1.f, 0.f, "step 1 cv");
		configParam(STEP2_PARAM, -1.f, 1.f, 0.f, "step 2 cv");
		configParam(STEP3_PARAM, -1.f, 1.f, 0.f, "step 3 cv");
		configParam(STEP4_PARAM, -1.f, 1.f, 0.f, "step 4 cv");
		configParam(STEP5_PARAM, -1.f, 1.f, 0.f, "step 5 cv");
		configParam(STEP6_PARAM, -1.f, 1.f, 0.f, "step 6 cv");
		configParam(STEP7_PARAM, -1.f, 1.f, 0.f, "step 7 cv");
		configParam(STEP8_PARAM, -1.f, 1.f, 0.f, "step 8 cv");
		configParam(RAND_PARAM, 0.f, 10.f, 0.f, "randomize steps");
		configInput(CLOCK_INPUT, "clock");
		configInput(RESET_INPUT, "reset");
		configInput(RAND_INPUT, "random trigger");
		configOutput(EOC_OUTPUT, "end of cycle");
		configOutput(CV_OUTPUT, "cv");
		configLight(STEP1_LIGHT, "step 1");
		configLight(STEP2_LIGHT, "step 2");
		configLight(STEP3_LIGHT, "step 3");
		configLight(STEP4_LIGHT, "step 4");
		configLight(STEP5_LIGHT, "step 5");
		configLight(STEP6_LIGHT, "step 6");
		configLight(STEP7_LIGHT, "step 7");
		configLight(STEP8_LIGHT, "step 8");
	}

	void process(const ProcessArgs& args) override {
		if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
			step = 0;
		}
		int steps = params[STEPS_PARAM].getValue();
		advance_lights(step);
		
		if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			step++;
			if (step > steps) {
				step = 1;
				eoc_pulse.trigger(1e-3);
			}
		}
		if (prand_trigger.process(params[RAND_PARAM].getValue())) {
			rand_pulse.trigger(1e-3);
		}
		if (rand_trigger.process(inputs[RAND_INPUT].getVoltage()) || rand_pulse.process(args.sampleTime)) {
			randomize_steps();	
		}
		outputs[EOC_OUTPUT].setVoltage(eoc_pulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[CV_OUTPUT].setVoltage(params[step].getValue() * range);
	}

	void randomize_steps() {
		for (int i = 1; i <= steps; i++) {
			params[STEP1_PARAM + i - 1].setValue(random::uniform() * 2.f - 1.f);
		}
	}

	void advance_lights(int step) {
		for (int i = 1; i <= steps; i++) {
			lights[STEP1_LIGHT + i - 1].setBrightness(i == step ? 1.f : 0.f);
		}
	}
};


struct StepsWidget : ModuleWidget {
	StepsWidget(Steps* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/steps.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(8.083, 38.726)), module, Steps::STEPS_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 23.545)), module, Steps::STEP1_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 35.069)), module, Steps::STEP2_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 46.593)), module, Steps::STEP3_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 58.117)), module, Steps::STEP4_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 69.641)), module, Steps::STEP5_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 81.165)), module, Steps::STEP6_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 92.689)), module, Steps::STEP7_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 104.213)), module, Steps::STEP8_PARAM));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 23.545)), module, Steps::STEP1_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 35.069)), module, Steps::STEP2_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 46.593)), module, Steps::STEP3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 58.117)), module, Steps::STEP4_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 69.641)), module, Steps::STEP5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 81.165)), module, Steps::STEP6_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 92.689)), module, Steps::STEP7_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.5, 104.213)), module, Steps::STEP8_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 23.545)), module, Steps::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 73.069)), module, Steps::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 53.906)), module, Steps::RAND_INPUT));

		addParam(createParamCentered<TL1105>(mm2px(Vec(8.336, 65.500)), module, Steps::RAND_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.336, 89.08)), module, Steps::EOC_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(8.336, 102.875)), module, Steps::CV_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Steps* module = dynamic_cast<Steps*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		// add a submenu to choose the range, between
		// +/- 1V, +/- 3V, +/- 5V, +/- 10V
		menu->addChild(createSubmenuItem("Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("-/+ 1v", CHECKMARK(module->range == 1), [module]() { module->range = 1; }));
			rangeMenu->addChild(createMenuItem("-/+ 3v", CHECKMARK(module->range == 3), [module]() { module->range = 3; }));
			rangeMenu->addChild(createMenuItem("-/+ 5v", CHECKMARK(module->range == 5), [module]() { module->range = 5; }));
			rangeMenu->addChild(createMenuItem("-/+ 10v", CHECKMARK(module->range == 10), [module]() { module->range = 10; }));
			menu->addChild(rangeMenu);
		}));
	}
};


Model* modelSteps = createModel<Steps, StepsWidget>("steps");