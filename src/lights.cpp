#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"


struct Lights : ThemeableModule {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(GATE_INPUT, 8),
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(GATE_LIGHT, 8),
		LIGHTS_LEN
	};

	Lights() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	}

	bool latch = false;
	bool gate[8] = { false };
	dsp::SchmittTrigger gate_trigger[8];

	void process(const ProcessArgs& args) override {
		if (latch) {
			for (int i = 0; i < 8; i++) {
				if (gate_trigger[i].process(inputs[GATE_INPUT + i].getVoltage())) {
					gate[i] = !gate[i];
				}
				lights[GATE_LIGHT + i].setBrightness(gate[i] ? 1.f : 0.f);
			}
		}
		else {
			for (int i = 0; i < 8; i++) {
				lights[GATE_LIGHT + i].setBrightness(inputs[GATE_INPUT + i].getVoltage() > 5.0f ? 1.0f : 0.0f);
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "contrast", json_real(contrast));
		json_object_set_new(rootJ, "use_global_contrast", json_boolean(use_global_contrast));
		json_object_set_new(rootJ, "latch", json_boolean(latch));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* contrastJ = json_object_get(rootJ, "contrast");
		if (contrastJ) {
			contrast = json_real_value(contrastJ);
		}
		json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
		if (use_global_contrastJ) {
			use_global_contrast = json_boolean_value(use_global_contrastJ);
		}
		json_t* latchJ = json_object_get(rootJ, "latch");
		if (latchJ) {
			latch = json_boolean_value(latchJ);
		}
	}
};


struct LightsWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	LightsWidget(Lights* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/lights.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		float x = RACK_GRID_WIDTH;
		float y = RACK_GRID_WIDTH;
		// float x_start = x;
		float y_start = y * 6;
		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		addInput(createInputCentered<BitPort>(Vec(x, y_start), module, Lights::GATE_INPUT + 0));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y_start), module, Lights::GATE_LIGHT + 0));

		y = y_start + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 1));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 1));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 2));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 2));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 3));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 3));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 4));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 4));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 5));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 5));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 6));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 6));

		y = y + dy;
		addInput(createInputCentered<BitPort>(Vec(x, y), module, Lights::GATE_INPUT + 7));
		addChild(createLightCentered<LargeLight<RedLight>>(Vec(x + dx, y), module, Lights::GATE_LIGHT + 7));
	}

	void step() override {
		Lights* lightsModule = dynamic_cast<Lights*>(this->module);
		if (!lightsModule) return;
		if (lightsModule->contrast != lightsModule->global_contrast) {
			lightsModule->use_global_contrast = false;
		}
		if (lightsModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = lightsModule->contrast;
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
        Lights* module = dynamic_cast<Lights*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        menu->addChild(createSubmenuItem("contrast", "", [=](Menu* menu) {
            Menu* contrastMenu = new Menu();
            ContrastSlider *contrastSlider = new ContrastSlider(&(module->contrast));
            contrastSlider->box.size.x = 200.f;
            contrastMenu->addChild(createMenuItem("use global contrast",
                CHECKMARK(module->use_global_contrast),
                [module]() { 
                    module->use_global_contrast = !module->use_global_contrast;
                    if (module->use_global_contrast) {
                        module->load_global_contrast();
                        module->contrast = module->global_contrast;
                    }
                }));
            contrastMenu->addChild(new MenuSeparator());
            contrastMenu->addChild(contrastSlider);
            contrastMenu->addChild(createMenuItem("set global contrast", "",
                [module]() {
                    module->save_global_contrast(module->contrast);
                }));
            menu->addChild(contrastMenu);
        }));
		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Latch", CHECKMARK(module->latch), [module]() {
			module->latch = !module->latch;
		}));
	}
};

Model* modelLights = createModel<Lights, LightsWidget>("lights");