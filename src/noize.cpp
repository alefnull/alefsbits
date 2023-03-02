#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"


struct Noize : ThemeableModule {
	enum ParamId {
		DURATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DURATION_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		NOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Noize() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DURATION_PARAM, 0.0f, 0.001f, 0.0f, "duration");
		configInput(DURATION_INPUT, "duration cv");
		configOutput(NOISE_OUTPUT, "noize");
	}

	float last_value = 0.0f;
	float time = 0.0f;

	void process(const ProcessArgs& args) override {
		float duration = params[DURATION_PARAM].getValue();
		if (inputs[DURATION_INPUT].isConnected()) {
			float cv = rescale(inputs[DURATION_INPUT].getVoltage(), 0.0f, 10.0f, 0.0f, 0.001f);
			duration = clamp(duration + cv, 0.0f, 0.001f);
		}
		if (time > duration) {
			last_value = random::uniform() * 2.0f - 1.0f;
			time = 0;
		}
		time += args.sampleTime;
		outputs[NOISE_OUTPUT].setVoltage(last_value * 5.0f);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "contrast", json_real(contrast));
		json_object_set_new(rootJ, "use_global_contrast", json_boolean(use_global_contrast));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* contrastJ = json_object_get(rootJ, "contrast");
		if (contrastJ) {
			contrast = json_number_value(contrastJ);
		}
		json_t* use_global_contrastJ = json_object_get(rootJ, "use_global_contrast");
		if (use_global_contrastJ) {
			use_global_contrast = json_boolean_value(use_global_contrastJ);
		}
	}
};


struct NoizeWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	NoizeWidget(Noize* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/noize.svg"));
		setPanel(svgPanel);
        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 26.056)), module, Noize::DURATION_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(10.16, 42.056)), module, Noize::DURATION_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(10.16, 109.258)), module, Noize::NOISE_OUTPUT));
	}

	void step() override {
		Noize* noizeModule = dynamic_cast<Noize*>(this->module);
		if (!noizeModule) return;
		if (noizeModule->contrast != noizeModule->global_contrast) {
			noizeModule->use_global_contrast = false;
		}
		if (noizeModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = noizeModule->contrast;
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
        Noize* module = dynamic_cast<Noize*>(this->module);
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
	}
};


Model* modelNoize = createModel<Noize, NoizeWidget>("noize");