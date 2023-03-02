#include "plugin.hpp"
#include "widgets/PanelBackground.hpp"
#include "widgets/InverterWidget.hpp"
#include "widgets/BitPort.hpp"

#define MAX_POLY 16


struct Octsclr : ThemeableModule {
	enum ParamId {
		SCALER_PARAM,
		OFFSET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SOURCE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SCALED_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Octsclr() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(SCALER_PARAM, 0, 20, 10, "scaler");
		getParamQuantity(SCALER_PARAM)->snapEnabled = true;
		configParam(OFFSET_PARAM, -3, 3, 0, "offset");
		getParamQuantity(OFFSET_PARAM)->snapEnabled = true;
		configInput(SOURCE_INPUT, "source");
		configOutput(SCALED_OUTPUT, "scaled");
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[SOURCE_INPUT].getChannels();
		if (channels > MAX_POLY) {
			channels = MAX_POLY;
		}
		outputs[SCALED_OUTPUT].setChannels(channels);
		float scaler = (float)params[SCALER_PARAM].getValue() / 10.0f;
		float offset = (float)params[OFFSET_PARAM].getValue();
		for (int i = 0; i < channels; i++) {
			float source = inputs[SOURCE_INPUT].getPolyVoltage(i);
			outputs[SCALED_OUTPUT].setVoltage(clamp(source * scaler + offset, -10.0f, 10.0f), i);
		}
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
			use_global_contrast = json_is_true(use_global_contrastJ);
		}
	}
};


struct OctsclrWidget : ModuleWidget {
    PanelBackground *panelBackground = new PanelBackground();
    SvgPanel *svgPanel;
    Inverter *inverter = new Inverter();
	OctsclrWidget(Octsclr* module) {
		setModule(module);
		svgPanel = createPanel(asset::plugin(pluginInstance, "res/octsclr.svg"));
		setPanel(svgPanel);

        panelBackground->box.size = svgPanel->box.size;
        svgPanel->fb->addChildBottom(panelBackground);
        inverter->box.pos = Vec(0.f, 0.f);
        inverter->box.size = Vec(box.size.x, box.size.y);
        addChild(inverter);

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 28.443)), module, Octsclr::SCALER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 48.843)), module, Octsclr::OFFSET_PARAM));

		addInput(createInputCentered<BitPort>(mm2px(Vec(7.62, 91.678)), module, Octsclr::SOURCE_INPUT));

		addOutput(createOutputCentered<BitPort>(mm2px(Vec(7.62, 106.319)), module, Octsclr::SCALED_OUTPUT));
	}

	void step() override {
		Octsclr* octModule = dynamic_cast<Octsclr*>(this->module);
		if (!octModule) return;
		if (octModule->contrast != octModule->global_contrast) {
			octModule->use_global_contrast = false;
		}
		if (octModule->contrast != panelBackground->contrast) {
			panelBackground->contrast = octModule->contrast;
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
        Octsclr* module = dynamic_cast<Octsclr*>(this->module);
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


Model* modelOctsclr = createModel<Octsclr, OctsclrWidget>("octsclr");