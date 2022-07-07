#include "steps.hpp"


void Steps::process(const ProcessArgs& args) {
	// Stepspander* right_module = static_cast<Stepspander*>(rightExpander.module);
	// if (right_module) {
	// 	if (!expanded) {
	// 		expanded = true;
	// 		getParamQuantity(STEPS_PARAM)->maxValue = 16.f;
	// 	}
	// }
	// else {
	// 	if (params[STEPS_PARAM].getValue() > 8.f) {
	// 		params[STEPS_PARAM].setValue(8.f);
	// 	}
	// 	if (expanded) {
	// 		expanded = false;
	// 		getParamQuantity(STEPS_PARAM)->maxValue = 8.f;
	// 	}
	// }

	steps = params[STEPS_PARAM].getValue();

	if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
		if (inputs[CLOCK_INPUT].isConnected()) {
			reset_queued = true;
		} else {
			step = 0;
			advance_lights(1);
		}	
	}
	
	if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
		if (reset_queued) {
			step = 0;
			reset_queued = false;
		}
		step++;
		if (step > steps) {
			step = 1;
			eoc_pulse.trigger(1e-3);
		}
		advance_lights(step);
	}
	if (prand_trigger.process(params[RAND_PARAM].getValue())) {
		rand_pulse.trigger(1e-3);
	}
	if (rand_trigger.process(inputs[RAND_INPUT].getVoltage()) || rand_pulse.process(args.sampleTime)) {
		randomize_steps();
	}
	advance_gate_outputs(step);
	// if (step <= 8) {
		outputs[CV_OUTPUT].setVoltage(params[STEP1_PARAM + step - 1].getValue());
	// }
	// else {
	// 	outputs[CV_OUTPUT].setVoltage(right_module->params[STEP1_PARAM + step - 8].getValue());
	// }
	outputs[EOC_OUTPUT].setVoltage(eoc_pulse.process(args.sampleTime) ? 10.f : 0.f);
}

void Steps::randomize_steps() {
	for (int i = 1; i <= steps; i++) {
		params[STEP1_PARAM + i - 1].setValue(random::uniform() * 2.f - 1.f);
		// if (right_module) {
		// 	right_module->params[STEP1_PARAM + i - 1].setValue(random::uniform() * 2.f - 1.f);
		// }
	}
}

void Steps::advance_lights(int step) {
	for (int i = 1; i <= steps; i++) {
		lights[STEP1_LIGHT + i - 1].setBrightness(i == step ? 1.f : 0.f);
		// if (right_module) {
		// 	right_module->lights[STEP1_LIGHT + i - 1].setBrightness(i + 8 == step ? 1.f : 0.f);
		// }
	}
}

void Steps::advance_gate_outputs(int step) {
	if (latch) {
		for (int i = 1; i <= steps; i++) {
			outputs[STEP1_OUTPUT + i - 1].setVoltage(i == step ? 10.f : 0.f);
			// if (right_module) {
			// 	right_module->outputs[STEP1_OUTPUT + i - 8 - 1].setVoltage(i + 8 == step ? 10.f : 0.f);
			// }
		}
	}
	else {
		for (int i = 1; i <= steps; i++) {
			if (i == step) {
				// if (right_module) {
				// 	right_module->step_pulse[i - 1].trigger(1e-3);
				// }
				// else {
					step_pulse[i - 1].trigger(1e-3);
				// }
			}
		}
		outputs[STEP1_OUTPUT + step - 1].setVoltage(step_pulse[step].process(1e-3) ? 10.f : 0.f);
		// if (right_module) {
		// 	right_module->outputs[STEP1_OUTPUT + step - 1].setVoltage(step_pulse[step].process(1e-3) ? 10.f : 0.f);
		// }
	}
}


struct StepsWidget : ModuleWidget {
	StepsWidget(Steps* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/steps2.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(8.083, 35.226)), module, Steps::STEPS_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 23.545)), module, Steps::STEP1_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 35.069)), module, Steps::STEP2_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 46.593)), module, Steps::STEP3_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 58.117)), module, Steps::STEP4_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 69.641)), module, Steps::STEP5_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 81.165)), module, Steps::STEP6_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 92.689)), module, Steps::STEP7_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.099, 104.213)), module, Steps::STEP8_PARAM));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 23.545)), module, Steps::STEP1_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 35.069)), module, Steps::STEP2_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 46.593)), module, Steps::STEP3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 58.117)), module, Steps::STEP4_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 69.641)), module, Steps::STEP5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 81.165)), module, Steps::STEP6_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 92.689)), module, Steps::STEP7_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(28, 104.213)), module, Steps::STEP8_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 19.545)), module, Steps::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 73.069)), module, Steps::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.336, 50.406)), module, Steps::RAND_INPUT));

		addParam(createParamCentered<TL1105>(mm2px(Vec(8.336, 64.000)), module, Steps::RAND_PARAM));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 23.545)), module, Steps::STEP1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 35.069)), module, Steps::STEP2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 46.593)), module, Steps::STEP3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 58.117)), module, Steps::STEP4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 69.641)), module, Steps::STEP5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 81.165)), module, Steps::STEP6_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 92.689)), module, Steps::STEP7_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37, 104.213)), module, Steps::STEP8_OUTPUT));

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
		menu->addChild(createMenuItem("Latch", CHECKMARK(module->latch), [module]() { module->latch = !module->latch; }));
	}
};


Model* modelSteps = createModel<Steps, StepsWidget>("steps");
