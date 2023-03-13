#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"

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
		AND_LIGHT,
		OR_LIGHT,
		XOR_LIGHT,
		NAND_LIGHT,
		NOR_LIGHT,
		XNOR_LIGHT,
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
		configLight(AND_LIGHT, "");
		configLight(OR_LIGHT, "");
		configLight(XOR_LIGHT, "");
		configLight(NAND_LIGHT, "");
		configLight(NOR_LIGHT, "");
		configLight(XNOR_LIGHT, "");
		if (use_global_contrast[LOGIC]) {
			module_contrast[LOGIC] = global_contrast;
		}
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
			bool _and = a && b;
			bool _or = a || b;
			bool _xor = a ^ b;
			bool _nand = !_and;
			bool _nor = !_or;
			bool _xnor = !_xor;
			outputs[AND_OUTPUT].setVoltage(_and ? 10.0 : 0.0, c);
			lights[AND_LIGHT].setBrightness(_and ? 1.0 : 0.0);
			outputs[OR_OUTPUT].setVoltage(_or ? 10.0 : 0.0, c);
			lights[OR_LIGHT].setBrightness(_or ? 1.0 : 0.0);
			outputs[XOR_OUTPUT].setVoltage(_xor ? 10.0 : 0.0, c);
			lights[XOR_LIGHT].setBrightness(_xor ? 1.0 : 0.0);
			outputs[NAND_OUTPUT].setVoltage(_nand ? 10.0 : 0.0, c);
			lights[NAND_LIGHT].setBrightness(_nand ? 1.0 : 0.0);
			outputs[NOR_OUTPUT].setVoltage(_nor ? 10.0 : 0.0, c);
			lights[NOR_LIGHT].setBrightness(_nor ? 1.0 : 0.0);
			outputs[XNOR_OUTPUT].setVoltage(_xnor ? 10.0 : 0.0, c);
			lights[XNOR_LIGHT].setBrightness(_xnor ? 1.0 : 0.0);
		}
	}
};


struct LogicWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	LogicWidget(Logic* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/logic.svg"));
		setPanel(svgPanel);
		
        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addInput(createInputCentered<BitPort>(mm2px(Vec(10.599, 24.981)), module, Logic::A_INPUT));
		addInput(createInputCentered<BitPort>(mm2px(Vec(10.599, 36.724)), module, Logic::B_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 51.547)), module, Logic::AND_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 62.079)), module, Logic::OR_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 73.563)), module, Logic::XOR_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 84.639)), module, Logic::NAND_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 96.023)), module, Logic::NOR_OUTPUT));
		addOutput(createOutputCentered<BitPort>(mm2px(Vec(8.285, 106.963)), module, Logic::XNOR_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 47.547)), module, Logic::AND_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 58.079)), module, Logic::OR_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 69.563)), module, Logic::XOR_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 80.639)), module, Logic::NAND_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 92.023)), module, Logic::NOR_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.285, 102.963)), module, Logic::XNOR_LIGHT));
	}

	void step() override {
		Logic* logicModule = dynamic_cast<Logic*>(this->module);
		if (!logicModule) return;
		if (use_global_contrast[LOGIC]) {
			module_contrast[LOGIC] = global_contrast;
		}
		if (module_contrast[LOGIC] != panelBackground->contrast) {
			panelBackground->contrast = module_contrast[LOGIC];
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
        Logic* module = dynamic_cast<Logic*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module_contrast[LOGIC]));
            contrastSlider->box.size.x = 200.f;
            GlobalOption *globalOption = new GlobalOption(&(use_global_contrast[LOGIC]));
			contrastMenu->addChild(globalOption);
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
					global_contrast = module_contrast[LOGIC];
					use_global_contrast[LOGIC] = true;
                }));
            menu->addChild(contrastMenu);
        }));
	}
};


Model* modelLogic = createModel<Logic, LogicWidget>("logic");