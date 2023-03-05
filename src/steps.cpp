#include "steps.hpp"
#include "inc/cvRange.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"


void Steps::process(const ProcessArgs& args) {

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

	if (inputs[CLOCK_INPUT].isConnected()) {
		float cv_out = step == 0 ? params[STEP1_PARAM].getValue() : params[STEP1_PARAM + step - 1].getValue();
		cv_out = cv_range.map(cv_out);
		outputs[CV_OUTPUT].setVoltage(cv_out);
		outputs[EOC_OUTPUT].setVoltage(eoc_pulse.process(args.sampleTime) ? 10.f : 0.f);
	} else {
		outputs[CV_OUTPUT].setVoltage(0.f);
		outputs[EOC_OUTPUT].setVoltage(0.f);
	}
}

void Steps::onReset() {
	step = 0;
	reset_queued = false;
	advance_lights(1);
}

void Steps::onRandomize() {
	params[STEPS_PARAM].setValue(random::u32() % 8 + 1);
	randomize_steps();
}

json_t* Steps::dataToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "steps", json_integer(steps));
	json_object_set_new(rootJ, "cv range", cv_range.dataToJson());
	return rootJ;
}

void Steps::dataFromJson(json_t* rootJ) {
	json_t* stepsJ = json_object_get(rootJ, "steps");
	if (stepsJ) {
		steps = json_integer_value(stepsJ);
	}
	json_t* cv_rangeJ = json_object_get(rootJ, "cv range");
	if (cv_rangeJ) {
		cv_range.dataFromJson(cv_rangeJ);
	}
}

void Steps::randomize_steps() {
	for (int i = 1; i <= PARAMS_LEN - 2; i++) {
		params[STEP1_PARAM + i - 1].setValue(random::uniform() * 2.f - 1.f);
	}
}

void Steps::advance_lights(int step) {
	for (int i = 1; i <= steps; i++) {
		lights[STEP1_LIGHT + i - 1].setBrightness(i == step ? 1.f : 0.f);
	}
}

void Steps::advance_gate_outputs(int step) {
	if (latch) {
		for (int i = 1; i <= steps; i++) {
			outputs[STEP1_OUTPUT + i - 1].setVoltage(i == step ? 10.f : 0.f);
		}
	}
	else {
		for (int i = 1; i <= steps; i++) {
			if (i == step) {
				step_pulse[i - 1].trigger(1e-3);
			}
		}
		outputs[STEP1_OUTPUT + step - 1].setVoltage(step_pulse[step].process(1e-3) ? 10.f : 0.f);
	}
}


struct StepsWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	StepsWidget(Steps* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/steps.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

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

		addInput(createInputCentered<BitPort>(mm2px(Vec(8.336, 19.545)), module, Steps::CLOCK_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(8.336, 73.069)), module, Steps::RESET_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(8.336, 50.406)), module, Steps::RAND_INPUT));

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
	
	void step() override {
		Steps* stepsModule = dynamic_cast<Steps*>(this->module);
		if (!stepsModule) return;
		if (use_global_contrast[STEPS]) {
			module_contrast[STEPS] = global_contrast;
		}
		if (module_contrast[STEPS] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[STEPS];
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
		Steps* steps_module = dynamic_cast<Steps*>(this->module);
		assert(steps_module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[STEPS]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[STEPS]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [steps_module]() {
					global_contrast = module_contrast[STEPS];
                }));
            menu->addChild(contrastMenu);
        }));

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("latch", CHECKMARK(steps_module->latch), [steps_module]() { steps_module->latch = !steps_module->latch; }));
		steps_module->cv_range.addMenu(steps_module, menu);
	}
};


Model* modelSteps = createModel<Steps, StepsWidget>("steps");
