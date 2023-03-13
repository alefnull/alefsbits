#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"


struct Fibb : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		FIBB2_OUTPUT,
		FIBB3_OUTPUT,
		FIBB5_OUTPUT,
		FIBB8_OUTPUT,
		FIBB13_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FIBB2_LIGHT,
		FIBB3_LIGHT,
		FIBB5_LIGHT,
		FIBB8_LIGHT,
		FIBB13_LIGHT,
		LIGHTS_LEN
	};

	dsp::SchmittTrigger clock_trigger;
	dsp::SchmittTrigger reset_trigger;
	dsp::ClockDivider div_2;
	dsp::ClockDivider div_3;
	dsp::ClockDivider div_5;
	dsp::ClockDivider div_8;
	dsp::ClockDivider div_13;
	bool clock_high = false;
	bool out_2 = false;
	bool out_3 = false;
	bool out_5 = false;
	bool out_8 = false;
	bool out_13 = false;

	Fibb() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(CLOCK_INPUT, "clock");
		configInput(RESET_INPUT, "reset");
		configOutput(FIBB2_OUTPUT, "clock / 2");
		configOutput(FIBB3_OUTPUT, "clock / 3");
		configOutput(FIBB5_OUTPUT, "clock / 5");
		configOutput(FIBB8_OUTPUT, "clock / 8");
		configOutput(FIBB13_OUTPUT, "clock / 13");
		div_2.setDivision(2);
		div_3.setDivision(3);
		div_5.setDivision(5);
		div_8.setDivision(8);
		div_13.setDivision(13);
        if (use_global_contrast[FIBB]) {
            module_contrast[FIBB] = global_contrast;
        }
	}

	void process(const ProcessArgs& args) override {
		if (reset_trigger.process(inputs[RESET_INPUT].getVoltage())) {
			reset();
		}
		clock_high = inputs[CLOCK_INPUT].getVoltage() > 0.0;
		if (!clock_high) {
			out_2 = false;
			out_3 = false;
			out_5 = false;
			out_8 = false;
			out_13 = false;
		}
		if (clock_trigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (div_2.process()) {
				out_2 = true;
			}
			if (div_3.process()) {
				out_3 = true;
			}
			if (div_5.process()) {
				out_5 = true;
			}
			if (div_8.process()) {
				out_8 = true;
			}
			if (div_13.process()) {
				out_13 = true;
			}
		}
		lights[FIBB2_LIGHT].setBrightness(out_2 && clock_high ? 1.0 : 0.0);
		lights[FIBB3_LIGHT].setBrightness(out_3 && clock_high ? 1.0 : 0.0);
		lights[FIBB5_LIGHT].setBrightness(out_5 && clock_high ? 1.0 : 0.0);
		lights[FIBB8_LIGHT].setBrightness(out_8 && clock_high ? 1.0 : 0.0);
		lights[FIBB13_LIGHT].setBrightness(out_13 && clock_high ? 1.0 : 0.0);
		outputs[FIBB2_OUTPUT].setVoltage(out_2 && clock_high ? 10.0 : 0.0);
		outputs[FIBB3_OUTPUT].setVoltage(out_3 && clock_high ? 10.0 : 0.0);
		outputs[FIBB5_OUTPUT].setVoltage(out_5 && clock_high ? 10.0 : 0.0);
		outputs[FIBB8_OUTPUT].setVoltage(out_8 && clock_high ? 10.0 : 0.0);
		outputs[FIBB13_OUTPUT].setVoltage(out_13 && clock_high ? 10.0 : 0.0);
	}

	void reset() {
		clock_trigger.reset();
		div_2.reset();
		div_3.reset();
		div_5.reset();
		div_8.reset();
		div_13.reset();
		clock_high = false;
		out_2 = false;
		out_3 = false;
		out_5 = false;
		out_8 = false;
		out_13 = false;
	}
};


struct FibbWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	FibbWidget(Fibb* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/fibb.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addInput(createInputCentered<BitPort>(mm2px(Vec(5.16, 22.719)), module, Fibb::CLOCK_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(15.16, 22.719)), module, Fibb::RESET_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 37.177)), module, Fibb::FIBB2_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 53.028)), module, Fibb::FIBB3_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 68.88)), module, Fibb::FIBB5_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 84.732)), module, Fibb::FIBB8_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(9.16, 100.583)), module, Fibb::FIBB13_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 37.177)), module, Fibb::FIBB2_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 53.028)), module, Fibb::FIBB3_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 68.88)), module, Fibb::FIBB5_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 84.732)), module, Fibb::FIBB8_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.16, 100.583)), module, Fibb::FIBB13_LIGHT));
	}

	void step() override {
		Fibb* fibbModule = dynamic_cast<Fibb*>(this->module);
		if (!fibbModule) return;
		if (use_global_contrast[FIBB]) {
			module_contrast[FIBB] = global_contrast;
		}
		if (module_contrast[FIBB] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[FIBB];
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
        Fibb* module = dynamic_cast<Fibb*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[FIBB]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[FIBB]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[FIBB];
					use_global_contrast[FIBB] = true;
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelFibb = createModel<Fibb, FibbWidget>("fibb");